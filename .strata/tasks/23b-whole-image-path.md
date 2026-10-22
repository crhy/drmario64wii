# Task 23b: draw hinted images from ONE whole-image texture (step 2 of 2; needs 23a's gfx_st.img_hint)
23a made the game emit a PortImgHint (include/port_img_hint.h) before each big-image helper, and the renderer keeps the
current hint in gfx_st.img_hint. Now use it, in port/gfx/gfx_tex.c only:
1. In gfx_tex_bind (texrect path): let h = gfx_st.img_hint. If h != NULL and the current load's SETTIMG address
   (gfx_timg_addr()) lies inside [h->img, h->img + h->w * h->h * bytes_per_texel(h->siz)) (4b: w*h/2), use the WHOLE
   image: decode all h->w x h->h texels from h->img (fmt/siz from the hint, palette from h->tlut when tlut_mode ==
   G_TT_RGBA16; reuse the existing texconv decoders) into one GX texture, cached by key (img, tlut, fmt, siz, w, h,
   CRC32 of the image bytes + palette). POT-pad like today (bind_gx_w/bind_gx_h = padded size).
2. Texcoords in WHOLE-image space. For a LOADTILE (tiStretchTexTile) the rect's s,t are already ABSOLUTE texel coords:
   u = s / bind_gx_w, v = t / bind_gx_h - do NOT subtract the tile origin on this path. For a LOADBLOCK strip
   (CopyTexBlock*/StretchTexBlock*) the rect's t is relative to the strip: add row0 = (timg_addr - h->img) /
   (h->w * bytes_per_texel) (4b: h->w/2).
3. Everything else unchanged (non-hinted loads, the 18b alias path for bg_snap_buf takes priority over hints,
   COPY-cycle rules, edge rules from task 20).
Gate = no rendering regressions + far fewer textures. Note: emulation timing drifts a little run to run, so only the
title frame (180) is compared exactly; gameplay is checked for coverage/colour/seams. Claude compares frames by eye.
GATE: make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && DR_DUMPTEX=1 .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t23b >/dev/null; python3 -c "import os,re; from PIL import Image, ImageChops, ImageStat as S; d='.strata/scratch/t23b/'; t180=max(S.Stat(ImageChops.difference(Image.open(d+'framedump_180.png').convert('RGB'), Image.open('.strata/scratch/ref20/framedump_180.png').convert('RGB'))).mean); g=Image.open(d+'framedump_1500.png'); L=g.convert('L'); px=L.tobytes(); nb=sum(1 for p in px if p>16)/len(px); sat=S.Stat(g.convert('HSV').split()[1]).mean[0]; w,h=g.size; rows=sum(1 for y in range(h) if max(L.crop((0,y,w,y+1)).tobytes())<=16); td=d+'textures/'+os.listdir(d+'textures')[0]; hs=[tuple(map(int,re.match(r'tex1_(\d+)x(\d+)_',f).groups())) for f in os.listdir(td)]; strips=sum(1 for w2,h2 in hs if h2<=8); print('t180',t180,'nb',nb,'sat',sat,'rows',rows,'textures',len(hs),'strips',strips); assert t180<4 and nb>0.6 and sat>60 and rows==0 and strips<=10 and len(hs)<300"
