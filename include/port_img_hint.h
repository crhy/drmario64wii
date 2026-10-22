#ifndef PORT_IMG_HINT_H
#define PORT_IMG_HINT_H
#ifdef TARGET_WII
typedef struct PortImgHint {
    u32 magic;
    const void *img;
    const void *tlut;
    u16 w, h;
    u8 fmt, siz, tlut_mode, pad;
} PortImgHint;
#define PORT_IMG_MAGIC 0x50494D47u  /* 'PIMG' */
void port_img_hint(Gfx **gfxP, const void *img, const void *tlut, int w, int h, int fmt, int siz, int tlut_mode);
#endif
#endif
