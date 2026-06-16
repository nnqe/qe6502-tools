#ifndef QEAIIHELPERS_H
#define QEAIIHELPERS_H

#include "qeaii.h"

QE_API
void qeaii_to_rgb(const qeaii_frame_t* frame, uint8_t* rgb_frame);


QE_API
void qeaii_to_audio_samples(const qeaii_speaker_frame_t* frame,
                            int16_t* output, uint32_t output_size);

QE_API
uint64_t qeaii_to_nanos(uint32_t clocks);

QE_API
uint64_t qeaii_to_cycles(uint32_t nanos);

#endif /* QEAIIHELPERS_H */
