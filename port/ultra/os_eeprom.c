/* Dr. Mario 64 -> Wii native port: EEPROM saves on the SD card (task 14c).

 * The game (src/main_segment/record.c) probes for an EEPROM, reads the whole
 * thing into a 512-byte cache (CACHE_SIZE 0x200), and writes it back 8-byte
 * blocks at a time. There is no N64 EEPROM on the Wii, so we keep a 4 Kbit
 * EEPROM (EEPROM_TYPE_4K: EEPROM_MAXBLOCKS 64 * EEPROM_BLOCK_SIZE 8 = 512
 * bytes) in RAM and persist it to sd:/drmario/eeprom.bin.

 * The game's save-check (EepRom_ReadAll) compares the first bytes against the
 * ASCII header "DM64"; a blank EEPROM must NOT match it so the game
 * initialises a fresh save (EepRomErr_NotInit -> EepRom_InitFirst -> write).
 * A fresh N64 EEPROM ships as 0xFF, which never spells "DM64", so a missing
 * file is filled with 0xFF. */

#include <PR/ultratypes.h>
#include <PR/os_eeprom.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "port_log.h"

#define EEPROM_BYTES (EEPROM_MAXBLOCKS * EEPROM_BLOCK_SIZE) /* 512 */

static u8 eeprom_ram[EEPROM_BYTES];
static int eeprom_loaded = 0;

static void eeprom_ensure_loaded(void) {
    FILE *f;
    if (eeprom_loaded)
        return;
    eeprom_loaded = 1;
    f = fopen("sd:/drmario/eeprom.bin", "rb");
    if (f != NULL) {
        /* Pre-fill with the blank-cart value so a short file still inits. */
        memset(eeprom_ram, 0xFF, EEPROM_BYTES);
        if (fread(eeprom_ram, 1, EEPROM_BYTES, f) == EEPROM_BYTES) {
            PORT_LOG("eeprom: loaded");
        } else {
            PORT_LOG("eeprom: loaded");
        }
        fclose(f);
    } else {
        memset(eeprom_ram, 0xFF, EEPROM_BYTES);
        PORT_LOG("eeprom: created");
    }
}

static void eeprom_commit(void) {
    FILE *f = fopen("sd:/drmario/eeprom.bin", "wb");
    if (f != NULL) {
        fwrite(eeprom_ram, 1, EEPROM_BYTES, f);
        fflush(f);
        fsync(fileno(f));
        fclose(f);
    }
}

s32 osEepromProbe(OSMesgQueue *mq) {
    (void)mq;
    eeprom_ensure_loaded();
    return EEPROM_TYPE_4K;
}

s32 osEepromRead(OSMesgQueue *mq, u8 address, u8 *buffer) {
    (void)mq;
    eeprom_ensure_loaded();
    if (address < EEPROM_MAXBLOCKS && buffer != NULL)
        memcpy(buffer, eeprom_ram + (u32)address * EEPROM_BLOCK_SIZE, EEPROM_BLOCK_SIZE);
    return 0;
}

s32 osEepromWrite(OSMesgQueue *mq, u8 address, u8 *buffer) {
    (void)mq;
    eeprom_ensure_loaded();
    if (address < EEPROM_MAXBLOCKS && buffer != NULL)
        memcpy(eeprom_ram + (u32)address * EEPROM_BLOCK_SIZE, buffer, EEPROM_BLOCK_SIZE);
    eeprom_commit();
    return 0;
}

s32 osEepromLongRead(OSMesgQueue *mq, u8 address, u8 *buffer, int length) {
    (void)mq;
    eeprom_ensure_loaded();
    if (buffer != NULL && length > 0) {
        u32 off = (u32)address * EEPROM_BLOCK_SIZE;
        if (off < EEPROM_BYTES) {
            u32 n = (u32)length;
            if (off + n > EEPROM_BYTES)
                n = EEPROM_BYTES - off;
            memcpy(buffer, eeprom_ram + off, n);
        }
    }
    return 0;
}

s32 osEepromLongWrite(OSMesgQueue *mq, u8 address, u8 *buffer, int length) {
    (void)mq;
    eeprom_ensure_loaded();
    if (buffer != NULL && length > 0) {
        u32 off = (u32)address * EEPROM_BLOCK_SIZE;
        if (off < EEPROM_BYTES) {
            u32 n = (u32)length;
            if (off + n > EEPROM_BYTES)
                n = EEPROM_BYTES - off;
            memcpy(eeprom_ram + off, buffer, n);
        }
    }
    eeprom_commit();
#ifdef PORT_LOG_VERBOSE
    PORT_LOG("eeprom write addr=%u len=%u", (u32)address, (u32)length);
#endif
    return 0;
}
