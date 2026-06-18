#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <qeaii.h>

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
    bootstrap.mem[0xfffc] = 0x00;
    bootstrap.mem[0xfffd] = 0x80;

    uint16_t pc_addr = 0x8000;
    for (int i = 0; i < 257; ++i) {
        bootstrap.mem[pc_addr++] = 0xad; /* LDA $C030 toggles the speaker. */
        bootstrap.mem[pc_addr++] = 0x30;
        bootstrap.mem[pc_addr++] = 0xc0;
    }
    uint16_t loop_addr = pc_addr;
    bootstrap.mem[pc_addr++] = 0x4c; /* JMP to self, with no further speaker access. */
    bootstrap.mem[pc_addr++] = (uint8_t)loop_addr;
    bootstrap.mem[pc_addr++] = (uint8_t)(loop_addr >> 8);

    qeaii_t apple;
    memset(&apple, 0, sizeof(apple));
    if (!qeaii_power_on(&apple, &bootstrap)) {
        return 1;
    }
    (void)qeaii_speaker_frame(&apple);

    (void)run_cycles(&apple, 2000);
    qeaii_speaker_frame_t* frame = qeaii_speaker_frame(&apple);

    if (frame->tick_count != 256) {
        fprintf(stderr, "expected saturated tick_count 256, got %u\n", (unsigned)frame->tick_count);
        return 2;
    }
    if (apple.speaker.last_value != 1) {
        fprintf(stderr, "speaker state did not reflect the unrecorded overflow toggle\n");
        return 3;
    }

    return 0;
}
