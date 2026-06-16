#ifndef QEAII_SOKOL_DSK2NIB_H
#define QEAII_SOKOL_DSK2NIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    qeaii_dsk_image_size = 143360,
    qeaii_nib_image_size = 0x1a00 * 35
};

bool qeaii_dsk2nib(const uint8_t* dsk,
                   size_t dsk_size,
                   uint8_t* nib,
                   size_t nib_size,
                   char* error,
                   size_t error_size);

#endif /* QEAII_SOKOL_DSK2NIB_H */
