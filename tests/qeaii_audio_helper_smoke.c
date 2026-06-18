#include <stdint.h>
#include <string.h>

#include <qeaii.h>
#include <qeaiihelpers.h>

int main(void)
{
    qeaii_speaker_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.start_cycle = 1000;
    frame.end_cycle = 2023;
    frame.speaker_state = 0;
    frame.tick_count = 2;
    frame.ticks[0] = 256;
    frame.ticks[1] = 768;

    int16_t samples[32];
    memset(samples, 0, sizeof(samples));
    qeaii_to_audio_samples(&frame, samples, (uint32_t)(sizeof(samples) / sizeof(samples[0])));

    int has_negative = 0;
    int has_positive = 0;
    for (unsigned i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        if (samples[i] < 0) {
            has_negative = 1;
        }
        if (samples[i] > 0) {
            has_positive = 1;
        }
    }

    return (has_negative && has_positive) ? 0 : 1;
}
