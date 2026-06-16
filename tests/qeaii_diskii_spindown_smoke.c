#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <qeaii.h>

static void fill_disk(qeaii_diskette_t* disk)
{
    memset(disk, 0, sizeof(*disk));
    for (uint16_t i = 0; i < qeaii_disk_track_size; ++i) {
        disk->data[i] = (uint8_t)(0x80u | (i & 0x7fu));
    }
}

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
    qeaii_bootstrap_t bootstrap;
    memset(&bootstrap, 0, sizeof(bootstrap));
    bootstrap.first_rom_address = 0xc000;
    bootstrap.mount_disk0 = true;
    fill_disk(&bootstrap.disk0);

    static const uint8_t program[] = {
        0xad, 0xe9, 0xc0,       /* LDA $C0E9: motor on */
        0xad, 0xe8, 0xc0,       /* LDA $C0E8: motor off */
        0xea,                   /* NOP */
        0x4c, 0x06, 0x80        /* JMP $8006 */
    };
    memcpy(&bootstrap.mem[0x8000], program, sizeof(program));
    bootstrap.mem[0xfffc] = 0x00;
    bootstrap.mem[0xfffd] = 0x80;

    qeaii_t pc;
    memset(&pc, 0, sizeof(pc));
    if (!qeaii_power_on(&pc, &bootstrap)) {
        return 1;
    }

    (void)run_cycles(&pc, 64);
    if (pc.driveII.motor_on) {
        fprintf(stderr, "motor should be commanded off\n");
        return 2;
    }
    if (!pc.driveII.spinning) {
        fprintf(stderr, "drive should still coast after motor-off\n");
        return 3;
    }

    uint16_t pos_after_off = pc.driveII.drives[0].track_pos;
    (void)run_cycles(&pc, 512);
    uint16_t pos_after_coast = pc.driveII.drives[0].track_pos;
    if (pos_after_coast == pos_after_off) {
        fprintf(stderr, "track_pos did not advance while coasting: %u\n", pos_after_off);
        return 4;
    }

    uint32_t spin_down_cycles =
        (uint32_t)qeaii_total_clocks_per_line *
        (uint32_t)(qeaii_height + qeaii_dummy_lines) *
        12u * 5u + 20000u;
    (void)run_cycles(&pc, spin_down_cycles);
    if (pc.driveII.spinning) {
        fprintf(stderr, "drive did not stop after spin-down window\n");
        return 5;
    }

    return 0;
}
