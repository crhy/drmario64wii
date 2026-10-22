#!/usr/bin/env python3
"""Copy a DOL for use as a disc's sys/main.dol, with the header's BSS size set to 0.

Retail apploaders (the disc template's apploader.img) refuse DOLs whose BSS reaches past ~0x80800000; ours ends near
0x80dd0000 because of the game's large buffers. libogc's crt0 clears __sbss_start..__bss_end itself before main(), so
the apploader's BSS clear is redundant and the field can be zeroed. Text/data sections are copied unchanged.

Usage: dol_for_disc.py <in.dol> <out.dol>
"""
import struct
import sys


def main() -> None:
    src, dst = sys.argv[1], sys.argv[2]
    data = bytearray(open(src, "rb").read())
    bss_addr, bss_size = struct.unpack(">2I", data[0xD8:0xE0])
    data[0xDC:0xE0] = struct.pack(">I", 0)
    open(dst, "wb").write(data)
    print(f"{dst}: BSS {bss_addr:#010x}+{bss_size:#x} left to crt0")


if __name__ == "__main__":
    main()
