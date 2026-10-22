#!/usr/bin/env python3
"""Build a Dolphin HD texture pack for the Wii port from dumped textures.

Input:  a folder of Dolphin texture dumps (tex1_WxH_hash_fmt.png, from DR_DUMPTEX=1 runs).
Output: <out>/RDMP01/<same file names>.png at SCALE x resolution (Dolphin: Load/Textures/RDMP01/).

Per texture: transparent texels get the colour of their nearest opaque neighbours (so the upscaler does not pull
black/garbage into edges), colour and alpha are upscaled separately with Real-ESRGAN (anime model: clean line art),
then recombined. Textures that are already done are skipped, so the pack can be grown run by run.

Usage: hd_pack.py <dump_dir> <out_dir> [--scale 4] [--esrgan path/to/realesrgan-ncnn-vulkan] [--gpu 0] [--tile 32]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageFilter

GAME_ID = "RDMP01"


def bleed_colour(im: Image.Image, rounds: int = 16) -> Image.Image:
    """Fill RGB under alpha==0 from opaque neighbours (repeated dilation), keep alpha."""
    rgba = im.convert("RGBA")
    a = rgba.getchannel("A")
    if a.getextrema()[0] == 255:
        return rgba
    rgb = rgba.convert("RGB")
    mask = a.point(lambda v: 255 if v > 0 else 0)
    filled = Image.new("RGB", rgb.size)
    filled.paste(rgb, mask=mask)
    known = mask
    for _ in range(rounds):
        grown = known.filter(ImageFilter.MaxFilter(3))
        if grown.getextrema()[0] == 255 and known.getextrema()[0] == 255:
            break
        # average of known neighbours: blur colour*mask / blur mask
        blurred = filled.filter(ImageFilter.BoxBlur(1))
        weight = known.filter(ImageFilter.BoxBlur(1))
        new = Image.new("RGB", rgb.size)
        bp, wp = blurred.load(), weight.load()
        fp, kp, np_ = filled.load(), known.load(), new.load()
        w, h = rgb.size
        for y in range(h):
            for x in range(w):
                if kp[x, y]:
                    np_[x, y] = fp[x, y]
                else:
                    wt = wp[x, y]
                    if wt:
                        r, g, b = bp[x, y]
                        np_[x, y] = (min(255, r * 255 // wt), min(255, g * 255 // wt), min(255, b * 255 // wt))
        filled = new
        known = grown
    out = filled.convert("RGBA")
    out.putalpha(a)
    return out


MIN_SIDE = 32   # the upscaler misbehaves on tiny images (e.g. 4-row strips): pad by edge replication, crop after


def upscale_file(esrgan: str, src: str, dst: str, gpu: int, tile: int) -> None:
    """One image per call: directory mode of realesrgan-ncnn-vulkan segfaults on this machine."""
    im = Image.open(src).convert("RGB")
    w, h = im.size
    pw, ph = max(w, MIN_SIDE), max(h, MIN_SIDE)
    if (pw, ph) != (w, h):
        padded = Image.new("RGB", (pw, ph))
        padded.paste(im, (0, 0))
        if pw > w:
            padded.paste(im.crop((w - 1, 0, w, h)).resize((pw - w, h)), (w, 0))
        if ph > h:
            padded.paste(padded.crop((0, h - 1, pw, h)).resize((pw, ph - h)), (0, h))
        tmp = dst + ".pad.png"
        padded.save(tmp)
        src_in = tmp
    else:
        src_in = src
    models = os.path.join(os.path.dirname(esrgan), "models")
    cmd = [esrgan, "-i", src_in, "-o", dst, "-n", "realesrgan-x4plus-anime", "-s", "4", "-g", str(gpu),
           "-t", str(tile), "-m", models]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if src_in != src:
        Image.open(dst).crop((0, 0, w * 4, h * 4)).save(dst)
        os.remove(src_in)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dump_dir")
    ap.add_argument("out_dir")
    ap.add_argument("--scale", type=int, default=4)
    ap.add_argument("--esrgan", default=".strata/upscale/resr/realesrgan-ncnn-vulkan")
    ap.add_argument("--gpu", type=int, default=0)
    ap.add_argument("--tile", type=int, default=32)
    args = ap.parse_args()

    pack = os.path.join(args.out_dir, GAME_ID)
    os.makedirs(pack, exist_ok=True)
    todo = [f for f in sorted(os.listdir(args.dump_dir))
            if f.startswith("tex1_") and f.endswith(".png") and not os.path.exists(os.path.join(pack, f))]
    print(f"{len(todo)} textures to upscale -> {pack}")
    if not todo:
        return

    work = tempfile.mkdtemp(prefix="hdpack-", dir=args.out_dir)   # on disk next to the pack, never /tmp (RAM)
    try:
        cin, ain, cout, aout = (os.path.join(work, d) for d in ("c_in", "a_in", "c_out", "a_out"))
        for d in (cin, ain, cout, aout):
            os.makedirs(d)
        has_alpha = set()
        for f in todo:
            im = Image.open(os.path.join(args.dump_dir, f)).convert("RGBA")
            bled = bleed_colour(im)
            bled.convert("RGB").save(os.path.join(cin, f))
            alpha = im.getchannel("A")
            if alpha.getextrema()[0] < 255:
                has_alpha.add(f)
                Image.merge("RGB", (alpha, alpha, alpha)).save(os.path.join(ain, f))
        for i, f in enumerate(todo, 1):
            upscale_file(args.esrgan, os.path.join(cin, f), os.path.join(cout, f), args.gpu, args.tile)
            if f in has_alpha:
                upscale_file(args.esrgan, os.path.join(ain, f), os.path.join(aout, f), args.gpu, args.tile)
            if i % 50 == 0:
                print(f"  {i}/{len(todo)}", flush=True)
        for f in todo:
            src = Image.open(os.path.join(args.dump_dir, f))
            w, h = src.size
            size = (w * args.scale, h * args.scale)
            col = Image.open(os.path.join(cout, f)).convert("RGB").resize(size, Image.LANCZOS)
            if f in has_alpha:
                a = Image.open(os.path.join(aout, f)).convert("L").resize(size, Image.LANCZOS)
                out = col.convert("RGBA")
                out.putalpha(a)
            else:
                out = col
            out.save(os.path.join(pack, f), optimize=True)
        print(f"done: {len(todo)} textures ({len(has_alpha)} with alpha)")
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
