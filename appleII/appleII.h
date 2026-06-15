/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE_APPLEII_H__
#define QE_APPLEII_H__

#include <qe_core.h>
#include <qe6502.h>

static const uint16_t aii_width = 280;
static const uint16_t aii_height = 192;
static const uint16_t aii_frame_size = (280 * 192) / 7; // 7 pixels per byte
static const uint16_t aii_pixels_per_clock = 7;
static const uint16_t aii_total_clocks_per_line = 65;
static const uint16_t aii_dummy_lines = 70;
static const uint16_t aii_clocks_per_line_visible_pixels = aii_width / aii_pixels_per_clock;

static const uint16_t aii_disk_tracks = 35;
static const uint16_t aii_disk_track_size = 0x1a00;
static const uint8_t aii_flag_cpu_error = (1 << 7);
static const uint8_t aii_flag_new_frame = (1 << 0);

typedef struct
{
    qe_bool is_text;
    qe_bool is_mixed;
    qe_bool is_hires;
    uint8_t bitmap[280 * 192 / 7];
} aii_frame_t;

typedef struct
{
    qe_bool readonly;
    qe_bool changed;
    uint8_t data[0x1A00 * 35];
} aii_diskette_t;

typedef struct
{
    uint8_t data[0x10000];
} aii_memory_t;

struct aii_appleII;
typedef qe_bool (*user_handler_fn)( struct aii_appleII* pc );

typedef struct
{
    uint8_t key;
    uint8_t key_register;
} aii_keyboard_t;

typedef struct
{
    qe_bool is_text;
    qe_bool is_mixed;
    qe_bool is_page2;
    qe_bool is_hires;

    qe_bool blink;

    uint16_t line;
    uint16_t col;
    qe_word32_t offsets;

    uint8_t current_frame;
    uint16_t frame_pos;
    aii_frame_t frames[2];
    uint8_t font_lines[8][64]; //[line][code]
    uint8_t ifont_lines[8][64]; //[line][code]
} aii_videocard_t;

typedef struct
{
    uint16_t ticks[1024];
    uint64_t frame_start_cycle;
    uint16_t tick_count;
    uint8_t first_value;
} aii_speaker_frame_t;

typedef struct
{
    aii_speaker_frame_t frames[2];
    uint8_t current_frame;
    uint8_t last_value;
} aii_speaker_t;

typedef struct
{
    uint16_t firts_rom_address;
    aii_memory_t memory;
} aii_bus_t;

typedef struct
{
    qe_bool is_mount;
    qe_bool q6;
    qe_bool q7;
    uint8_t phase;
    uint16_t track;           // from 0 to 34
    uint16_t track_pos;
    qe_bool phases[4];
    aii_diskette_t diskette;
} aii_drive_state_t;

typedef struct
{
    qe_bool spinning;
    uint8_t active_drive;
    aii_drive_state_t drives[2];
} aii_driveII_t;

typedef struct aii_appleII
{
    qe6502_t cpu;
    qe6502_cycle_t cycle;
    aii_keyboard_t kbd;
    aii_videocard_t video;
    aii_speaker_t speaker;
    aii_driveII_t driveII;
    aii_bus_t bus;
    qe_bool nmi;
    qe_bool is_ok;
    uint64_t cycle_counter;
    uint8_t stop_flags;
    user_handler_fn ex_video_handler;
    user_handler_fn ex_bus_handler;
    user_handler_fn ex_cpu_handler;
} aii_appleII_t;

typedef struct
{
    uint8_t mem[0x10000];
    uint8_t font_rom[2048];
    aii_diskette_t disk0;
    qe_bool mount_disk0;
    uint16_t first_rom_address;
    user_handler_fn ex_video_handler;
    user_handler_fn ex_bus_handler;
    user_handler_fn ex_cpu_handler;
} aii_bootstrap_t;

QE_API
qe_bool aii_power_on(aii_appleII_t* pc,
                     aii_bootstrap_t* bootstrap);
QE_API
void aii_break(aii_appleII_t* pc);

QE_API
aii_frame_t* aii_frame(aii_appleII_t* pc);


QE_API
uint32_t aii_run(aii_appleII_t* pc,
                 uint16_t max_instructions);
QE_API
uint32_t aii_run_ex(aii_appleII_t* pc,
                    uint16_t max_instructions);

#if(QE6502_ENABLE_CYCLE_MERGE != 1)
    QE_API // returns cycles left, not cycles processed
    uint32_t aii_run_cycles(aii_appleII_t* pc, uint32_t max_cycles);
    QE_API // returns cycles left, not cycles processed
    uint32_t aii_run_cycles_ex(aii_appleII_t* pc, uint32_t max_cycles);
#endif

QE_API
void aii_press_key(aii_appleII_t* pc,
                   uint8_t key);
QE_API
aii_speaker_frame_t*
aii_speaker_frame(aii_appleII_t* pc);

QE_API
void aii_mount_disk0(aii_appleII_t* pc,
                     aii_diskette_t* diskette);
QE_API
void aii_unmount_disk0(aii_appleII_t* pc);
QE_API
qe_bool aii_pc_ok(aii_appleII_t* pc);
QE_API
qe_bool aii_disk_active(aii_appleII_t* pc);
QE_API
qe_bool aii_frame_ready(aii_appleII_t* pc);

#endif // QE_APPLEII_H__
