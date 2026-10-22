#!/usr/bin/env python3
"""Compare the Wii port's frame dumps with reference frames of the original N64 game (refemu/run-ref.sh, angrylion).

Both sides are normalised to the N64's native 320x240 with an area filter (the port dumps 640x528 in 4:3 mode, the
reference 640x480), so scaling/VI-filter differences mostly cancel and real rendering differences stand out.
Emulation timing differs between the two, so each port frame is matched against every reference frame within
+-window frames and the best match is used.

Usage: refcompare.py <port_dir (framedump_N.png)> <ref_dir (ref_N.png)> <out_dir> [--window 150] [--block 8]
Writes <out_dir>/report.txt (one line per port frame: frame, best ref frame, mean diff, worst block) and
<out_dir>/cmp_<N>.png (port | reference | diff heat map) for the worst frames.
"""
import argparse
import os
import re

from PIL import Image, ImageChops, ImageStat

W, H = 320, 240


def load_norm(path: str) -> Image.Image:
    return Image.open(path).convert("RGB").resize((W, H), Image.BOX)


def frames(d: str, prefix: str) -> dict:
    out = {}
    for f in os.listdir(d):
        m = re.match(prefix + r"(\d+)\.png$", f)
        if m:
            out[int(m.group(1))] = os.path.join(d, f)
    return out


def block_scores(diff: Image.Image, block: int) -> list:
    g = diff.convert("L")
    scores = []
    for y in range(0, H, block):
        for x in range(0, W, block):
            scores.append((ImageStat.Stat(g.crop((x, y, x + block, y + block))).mean[0], x, y))
    return scores


def heat(diff: Image.Image, block: int) -> Image.Image:
    out = Image.new("RGB", (W, H))
    for s, x, y in block_scores(diff, block):
        v = min(255, int(s * 4))
        out.paste((v, 0, 255 - v if v < 32 else 0), (x, y, x + block, y + block))
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("port_dir")
    ap.add_argument("ref_dir")
    ap.add_argument("out_dir")
    ap.add_argument("--window", type=int, default=150)
    ap.add_argument("--block", type=int, default=8)
    ap.add_argument("--worst", type=int, default=8, help="how many composite images to write")
    ap.add_argument("--port-log", help="port SD log (script: W n done f=X lines) for event alignment")
    ap.add_argument("--ref-log", help="reference run.log (same lines) for event alignment")
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    def wait_events(path):
        ev = []
        if path and os.path.exists(path):
            txt = open(path, "rb").read().replace(b"\0", b"").decode("latin-1")
            for m in re.finditer(r"script: W (\d+) done f=(\d+)", txt):
                ev.append(int(m.group(2)))
        return ev

    pev, rev = wait_events(a.port_log), wait_events(a.ref_log)
    nev = min(len(pev), len(rev))

    def expected_ref(pf):
        """Map a port frame to the reference frame at the same game moment (offset of the latest wait event)."""
        k = -1
        for i in range(nev):
            if pev[i] <= pf:
                k = i
        return pf if k < 0 else rev[k] + (pf - pev[k])

    port = frames(a.port_dir, "framedump_")
    ref = frames(a.ref_dir, "ref_")
    ref_cache = {}
    rows = []
    for pf in sorted(port):
        er = expected_ref(pf)
        cands = [rf for rf in ref if abs(rf - er) <= a.window]
        if not cands:
            continue
        p = load_norm(port[pf])
        best = None
        for rf in cands:
            if rf not in ref_cache:
                ref_cache[rf] = load_norm(ref[rf])
            d = ImageChops.difference(p, ref_cache[rf])
            m = sum(ImageStat.Stat(d).mean) / 3
            if best is None or m < best[0]:
                best = (m, rf, d)
        m, rf, d = best
        worst_block = max(block_scores(d, a.block))
        rows.append((pf, rf, m, worst_block, p, ref_cache[rf], d))
    with open(os.path.join(a.out_dir, "report.txt"), "w") as f:
        f.write("port_frame ref_frame mean_diff worst_block(mean,x,y)\n")
        for pf, rf, m, wb, *_ in rows:
            f.write(f"{pf} {rf} {m:.2f} {wb[0]:.1f},{wb[1]},{wb[2]}\n")
    for pf, rf, m, wb, p, r, d in sorted(rows, key=lambda r: -r[2])[: a.worst]:
        c = Image.new("RGB", (W * 3 + 8, H), (255, 255, 255))
        c.paste(p, (0, 0))
        c.paste(r, (W + 4, 0))
        c.paste(heat(d, a.block), (2 * W + 8, 0))
        c.resize((c.width * 2, c.height * 2), Image.NEAREST).save(os.path.join(a.out_dir, f"cmp_{pf}.png"))
    if rows:
        ms = sorted(r[2] for r in rows)
        print(f"{len(rows)} frames: median diff {ms[len(ms) // 2]:.2f}, worst {ms[-1]:.2f}")


if __name__ == "__main__":
    main()
