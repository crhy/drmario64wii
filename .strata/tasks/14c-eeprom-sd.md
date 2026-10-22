# Task 14c: EEPROM saves on the SD card
port/ultra/os_stub.c stubs osEepromProbe/osEepromRead/osEepromWrite/osEepromLongRead/osEepromLongWrite. Implement them
in a new port/ultra/os_eeprom.c (remove the stubs): a 4 Kbit EEPROM (EEPROM_TYPE_4K, 64 blocks of 8 bytes = 512 bytes;
check which type the game probes for in src/ and use that size) kept in RAM, loaded at first use from
sd:/drmario/eeprom.bin (missing file -> all 0xFF? check what the game expects for a blank cart; N64 EEPROMs ship as 0x00
or 0xFF - read the game's save-check code and pick the value that makes it initialise a fresh save), and written back to
the file (fopen "r+b"/"wb", fwrite, fclose) on every Write/LongWrite. Writes come from game threads: do the file I/O
inside the call (it is short) and log "eeprom write addr=%u len=%u" once per call with PORT_LOG_VERBOSE only, plus one
non-verbose "eeprom: loaded|created" line at first use.
GATE: make -f Makefile.wii >/dev/null 2>REGRESSION1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t14c >/dev/null && .strata/imgmax.py .strata/scratch/t14c --min-colors 128 && test "$(stat -c %s .strata/scratch/t14c/sd/drmario/eeprom.bin 2>/dev/null)" -ge 512 && grep -aqE "eeprom: (loaded|created)" .strata/scratch/t14c/sd/drmario/log.txt
