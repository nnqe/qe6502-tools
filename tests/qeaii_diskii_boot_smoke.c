#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qeaii.h>
#include "dsk2nib.h"

static const uint8_t s_rom[0x10000] =
#include "apple_ii_plus_disk_ii_card_5.hex"
;

static const uint8_t s_font[2048] =
#include "apple_ii_plus_video_rom.hex"
;

static uint32_t run_cycles(qeaii_t* pc, uint32_t cycles)
{
    uint32_t done = 0;
    while (done < cycles) {
        uint32_t chunk = qeaii_run(pc, cycles - done);
        if (chunk == 0) {
            return done;
        }
        done += chunk;
    }
    return done;
}

int main(void)
{
    uint8_t* dsk = (uint8_t*)calloc(1, qeaii_dsk_image_size);
    if (dsk == NULL) {
        return 1;
    }

    enum { sector_count = 16 };
    dsk[0] = sector_count;

    static const uint8_t boot1_code[] = {
        0xa9, 0x42,             /* LDA #$42 */
        0x8d, 0x00, 0x04,       /* STA $0400 */
        0x4c, 0x06, 0x08        /* JMP $0806 */
    };
    memcpy(&dsk[1], boot1_code, sizeof(boot1_code));

    for (int sector = 1; sector < sector_count; ++sector) {
        uint8_t* data = dsk + (sector * 256);
        for (int i = 0; i < 256; ++i) {
            data[i] = (uint8_t)(sector * 17 + i);
        }
    }

    qeaii_bootstrap_t bootstrap;
    memset(&bootstrap, 0, sizeof(bootstrap));
    memcpy(bootstrap.mem, s_rom, sizeof(s_rom));
    memcpy(bootstrap.font_rom, s_font, sizeof(s_font));
    bootstrap.first_rom_address = 0xc000;
    bootstrap.mem[0xfffc] = 0x00;
    bootstrap.mem[0xfffd] = 0xc6;
    bootstrap.mount_disk0 = true;

    char error[256];
    if (!qeaii_dsk2nib(dsk,
                       qeaii_dsk_image_size,
                       bootstrap.disk0.data,
                       sizeof(bootstrap.disk0.data),
                       error,
                       sizeof(error))) {
        fprintf(stderr, "qeaii_dsk2nib failed: %s\n", error);
        free(dsk);
        return 2;
    }
    free(dsk);

    qeaii_t pc;
    memset(&pc, 0, sizeof(pc));
    if (!qeaii_power_on(&pc, &bootstrap)) {
        return 3;
    }

    uint32_t cycles = 0;
    while (cycles < 8000000u && pc.bus.memory.data[0x0400] != 0x42) {
        uint32_t ran = run_cycles(&pc, 1000);
        if (ran == 0) {
            return 4;
        }
        cycles += ran;
    }

    if (pc.bus.memory.data[0x0400] != 0x42) {
        return 5;
    }
    if (pc.bus.memory.data[0x0800] != sector_count) {
        return 6;
    }

    return 0;
}
