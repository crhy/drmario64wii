#!/usr/bin/env python3
# Generate port/gen/n64_syms.ld: a GNU ld fragment that gives the Wii link the
# N64 build's segment-boundary symbols and segmented asset addresses, so game
# code compiled for Wii (SEGMENT_ROM_START/END, SEGMENT_VRAM_*, SEGMENT_BSS_*,
# and asset symbols) resolves to the same values as on N64.
#
# Reads build/us/drmario64.us.elf via mips-linux-gnu-nm (powerpc-eabi-nm cannot
# read a MIPS ELF). Writes NAME = 0xVALUE; lines, sorted by name, no duplicates.
#
# Included symbols:
#   (a) segment boundary symbols produced by include/segment_symbols.h macros:
#       <segment>_VRAM, <segment>_VRAM_END, <segment>_ROM_START,
#       <segment>_ROM_END, <segment>_BSS_START, <segment>_BSS_END
#       (plus the linker's own *_alloc_VRAM / *_noload_VRAM variants, which
#        share those suffixes and are harmless extra definitions).
#   (b) every defined symbol whose value is below 0x10000000 (segmented asset
#       addresses such as 0x05001234), keeping only nm types D d B b R r T t A
#       so absolute/size-only markers that are not real addresses are dropped.
#
# No third-party deps.

import os
import re
import subprocess
import sys

ELF = "build/us/drmario64.us.elf"
OUT = "port/gen/n64_syms.ld"

# nm types that denote a real address (data, bss, read-only, text, absolute).
KEEP_TYPES = set("DdBbRrTtA")

# Boundary suffixes the segment_symbols.h macros produce.
BOUNDARY = re.compile(r"_(VRAM|VRAM_END|ROM_START|ROM_END|BSS_START|BSS_END)$")


def main():
    if not os.path.exists(ELF):
        sys.exit("error: %s not found (build the N64 ROM first)" % ELF)

    proc = subprocess.run(
        ["mips-linux-gnu-nm", ELF],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if proc.returncode != 0:
        sys.exit("error: mips-linux-gnu-nm failed: %s" % proc.stderr.strip())

    syms = {}
    for line in proc.stdout.splitlines():
        parts = line.split(" ", 2)
        if len(parts) < 3:
            continue
        val_s, typ, name = parts
        try:
            val = int(val_s, 16)
        except ValueError:
            continue
        is_boundary = bool(BOUNDARY.search(name))
        is_low = val < 0x10000000 and typ in KEEP_TYPES
        if (is_boundary or is_low) and name not in syms:
            syms[name] = val

    names = sorted(syms)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        for name in names:
            f.write("%s = 0x%08X;\n" % (name, syms[name]))

    print("wrote %d symbols to %s" % (len(names), OUT))


if __name__ == "__main__":
    main()
