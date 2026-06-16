/*
 * Minimal in-memory DSK to NIB converter for the Sokol Apple II boot app.
 * Based on the old project's examples/Libs/Utils/Dsk2Nib.cpp copy of
 * dsk2nib.c, copyright (C) 1996, 2017 slotek@nym.hush.com, MIT licensed.
 */

#include "dsk2nib.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TRACKS_PER_DISK       35
#define SECTORS_PER_TRACK     16
#define BYTES_PER_SECTOR      256
#define BYTES_PER_TRACK       4096
#define PRIMARY_BUF_LEN       256
#define SECONDARY_BUF_LEN     86
#define DATA_LEN              (PRIMARY_BUF_LEN + SECONDARY_BUF_LEN)
#define PROLOG_LEN            3
#define EPILOG_LEN            3
#define GAP1_LEN              48
#define GAP2_LEN              5
#define BYTES_PER_NIB_SECTOR  416
#define BYTES_PER_NIB_TRACK   6656
#define DEFAULT_VOLUME        254
#define GAP_BYTE              0xff

typedef uint8_t uchar;

typedef struct {
    uchar prolog[PROLOG_LEN];
    uchar volume[2];
    uchar track[2];
    uchar sector[2];
    uchar checksum[2];
    uchar epilog[EPILOG_LEN];
} addr_t;

typedef struct {
    uchar prolog[PROLOG_LEN];
    uchar data[DATA_LEN];
    uchar data_checksum;
    uchar epilog[EPILOG_LEN];
} data_t;

typedef struct {
    uchar gap1[GAP1_LEN];
    addr_t addr;
    uchar gap2[GAP2_LEN];
    data_t data;
} nib_sector_t;

typedef struct {
    const uint8_t* dsk;
    uint8_t* nib;
    uchar primary_buf[PRIMARY_BUF_LEN];
    uchar secondary_buf[SECONDARY_BUF_LEN];
    nib_sector_t nib_sector;
} dsk2nib_ctx_t;

static const uchar addr_prolog[PROLOG_LEN] = { 0xd5, 0xaa, 0x96 };
static const uchar addr_epilog[EPILOG_LEN] = { 0xde, 0xaa, 0xeb };
static const uchar data_prolog[PROLOG_LEN] = { 0xd5, 0xaa, 0xad };
static const uchar data_epilog[EPILOG_LEN] = { 0xde, 0xaa, 0xeb };

static const int soft_interleave[SECTORS_PER_TRACK] =
    { 0, 7, 0xE, 6, 0xD, 5, 0xC, 4, 0xB, 3, 0xA, 2, 9, 1, 8, 0xF };
static const int phys_interleave[SECTORS_PER_TRACK] =
    { 0, 0xD, 0xB, 9, 7, 5, 3, 1, 0xE, 0xC, 0xA, 8, 6, 4, 2, 0xF };

static const uchar translate_table[0x40] = {
    0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
    0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
    0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
    0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
    0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
    0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
    0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static void set_error(char* error, size_t error_size, const char* fmt, ...)
{
    if (error == NULL || error_size == 0) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(error, error_size, fmt, args);
    va_end(args);
    error[error_size - 1] = '\0';
}

static void odd_even_encode(uchar out[2], int value)
{
    out[0] = (uchar)(((value >> 1) & 0x55) | 0xaa);
    out[1] = (uchar)((value & 0x55) | 0xaa);
}

static uchar translate(uchar byte)
{
    return translate_table[byte & 0x3f];
}

static const uchar* dsk_get(const dsk2nib_ctx_t* ctx, int track, int sector)
{
    return ctx->dsk + (track * BYTES_PER_TRACK) + (sector * BYTES_PER_SECTOR);
}

static uchar* nib_get(dsk2nib_ctx_t* ctx, int track, int sector)
{
    return ctx->nib + (track * BYTES_PER_NIB_TRACK) + (sector * BYTES_PER_NIB_SECTOR);
}

static void nibbilize(dsk2nib_ctx_t* ctx, int track, int sector)
{
    const uchar* src = dsk_get(ctx, track, sector);
    uchar* dest = ctx->nib_sector.data.data;

    memset(ctx->primary_buf, 0, sizeof(ctx->primary_buf));
    memset(ctx->secondary_buf, 0, sizeof(ctx->secondary_buf));

    for (int i = 0; i < PRIMARY_BUF_LEN; i++) {
        ctx->primary_buf[i] = (uchar)(src[i] >> 2);

        int index = i % SECONDARY_BUF_LEN;
        int section = i / SECONDARY_BUF_LEN;
        uchar pair = (uchar)(((src[i] & 2) >> 1) | ((src[i] & 1) << 1));
        ctx->secondary_buf[index] |= (uchar)(pair << (section * 2));
    }

    int index = 0;
    dest[index++] = translate(ctx->secondary_buf[0]);

    for (int i = 1; i < SECONDARY_BUF_LEN; i++) {
        dest[index++] = translate((uchar)(ctx->secondary_buf[i] ^ ctx->secondary_buf[i - 1]));
    }

    dest[index++] = translate((uchar)(ctx->primary_buf[0] ^ ctx->secondary_buf[SECONDARY_BUF_LEN - 1]));

    for (int i = 1; i < PRIMARY_BUF_LEN; i++) {
        dest[index++] = translate((uchar)(ctx->primary_buf[i] ^ ctx->primary_buf[i - 1]));
    }

    ctx->nib_sector.data.data_checksum = translate(ctx->primary_buf[PRIMARY_BUF_LEN - 1]);
}

bool qeaii_dsk2nib(const uint8_t* dsk,
                   size_t dsk_size,
                   uint8_t* nib,
                   size_t nib_size,
                   char* error,
                   size_t error_size)
{
    if (dsk == NULL || nib == NULL) {
        set_error(error, error_size, "missing DSK or NIB buffer");
        return false;
    }
    if (dsk_size != qeaii_dsk_image_size) {
        set_error(error, error_size, "DSK image must be %u bytes", (unsigned)qeaii_dsk_image_size);
        return false;
    }
    if (nib_size != qeaii_nib_image_size) {
        set_error(error, error_size, "NIB output must be %u bytes", (unsigned)qeaii_nib_image_size);
        return false;
    }

    dsk2nib_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.dsk = dsk;
    ctx.nib = nib;

    memset(nib, GAP_BYTE, nib_size);
    memcpy(ctx.nib_sector.addr.prolog, addr_prolog, sizeof(addr_prolog));
    memcpy(ctx.nib_sector.addr.epilog, addr_epilog, sizeof(addr_epilog));
    memcpy(ctx.nib_sector.data.prolog, data_prolog, sizeof(data_prolog));
    memcpy(ctx.nib_sector.data.epilog, data_epilog, sizeof(data_epilog));
    odd_even_encode(ctx.nib_sector.addr.volume, DEFAULT_VOLUME);
    memset(ctx.nib_sector.gap1, GAP_BYTE, sizeof(ctx.nib_sector.gap1));
    memset(ctx.nib_sector.gap2, GAP_BYTE, sizeof(ctx.nib_sector.gap2));

    for (int track = 0; track < TRACKS_PER_DISK; track++) {
        for (int sec = 0; sec < SECTORS_PER_TRACK; sec++) {
            int softsec = soft_interleave[sec];
            int physsec = phys_interleave[sec];
            int checksum = DEFAULT_VOLUME ^ track ^ sec;

            odd_even_encode(ctx.nib_sector.addr.track, track);
            odd_even_encode(ctx.nib_sector.addr.sector, sec);
            odd_even_encode(ctx.nib_sector.addr.checksum, checksum);
            nibbilize(&ctx, track, softsec);
            memcpy(nib_get(&ctx, track, physsec), &ctx.nib_sector, sizeof(ctx.nib_sector));
        }
    }

    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }
    return true;
}
