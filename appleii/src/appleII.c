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

#include <qe_appleII.h>

///////////////////////////////////////////////////////
//                  KEYBOARD
///////////////////////////////////////////////////////

QE_SIC
void kbd_init(qeaii_t* pc, qeaii_bootstrap_t* bootstrap)
{
    (void)bootstrap;
    pc->kbd.key = 0;
    pc->kbd.key_register = 0;
}

QE_SIC
uint8_t kbd_softswitch_read(qeaii_t* pc, uint8_t softswitch)
{
    if (softswitch == 0x00 && (pc->kbd.key & 0x80))
    {
        pc->kbd.key_register = pc->kbd.key;
        pc->kbd.key &= 0x7f;
    }
    else if (softswitch == 0x10)
    {
        pc->kbd.key_register &= 0x7f;
    }
    return pc->kbd.key_register;
}

QE_SIC
void kbd_softswitch_write(qeaii_t* pc)
{
    pc->kbd.key_register &= 0x7f;
}

///////////////////////////////////////////////////////
//                  VIDEO
///////////////////////////////////////////////////////

static const uint32_t s_video_offsets[192] =
{
    // 0xGGGGTTTT ((gr_address << 16) | txt_address)
    0x00000000,0x04000000,0x08000000,0x0C000000,0x10000000,0x14000000,0x18000000,0x1C000000,
    0x00800080,0x04800080,0x08800080,0x0C800080,0x10800080,0x14800080,0x18800080,0x1C800080,
    0x01000100,0x05000100,0x09000100,0x0D000100,0x11000100,0x15000100,0x19000100,0x1D000100,
    0x01800180,0x05800180,0x09800180,0x0D800180,0x11800180,0x15800180,0x19800180,0x1D800180,
    0x02000200,0x06000200,0x0A000200,0x0E000200,0x12000200,0x16000200,0x1A000200,0x1E000200,
    0x02800280,0x06800280,0x0A800280,0x0E800280,0x12800280,0x16800280,0x1A800280,0x1E800280,
    0x03000300,0x07000300,0x0B000300,0x0F000300,0x13000300,0x17000300,0x1B000300,0x1F000300,
    0x03800380,0x07800380,0x0B800380,0x0F800380,0x13800380,0x17800380,0x1B800380,0x1F800380,
    0x00280028,0x04280028,0x08280028,0x0C280028,0x10280028,0x14280028,0x18280028,0x1C280028,
    0x00A800A8,0x04A800A8,0x08A800A8,0x0CA800A8,0x10A800A8,0x14A800A8,0x18A800A8,0x1CA800A8,
    0x01280128,0x05280128,0x09280128,0x0D280128,0x11280128,0x15280128,0x19280128,0x1D280128,
    0x01A801A8,0x05A801A8,0x09A801A8,0x0DA801A8,0x11A801A8,0x15A801A8,0x19A801A8,0x1DA801A8,
    0x02280228,0x06280228,0x0A280228,0x0E280228,0x12280228,0x16280228,0x1A280228,0x1E280228,
    0x02A802A8,0x06A802A8,0x0AA802A8,0x0EA802A8,0x12A802A8,0x16A802A8,0x1AA802A8,0x1EA802A8,
    0x03280328,0x07280328,0x0B280328,0x0F280328,0x13280328,0x17280328,0x1B280328,0x1F280328,
    0x03A803A8,0x07A803A8,0x0BA803A8,0x0FA803A8,0x13A803A8,0x17A803A8,0x1BA803A8,0x1FA803A8,
    0x00500050,0x04500050,0x08500050,0x0C500050,0x10500050,0x14500050,0x18500050,0x1C500050,
    0x00D000D0,0x04D000D0,0x08D000D0,0x0CD000D0,0x10D000D0,0x14D000D0,0x18D000D0,0x1CD000D0,
    0x01500150,0x05500150,0x09500150,0x0D500150,0x11500150,0x15500150,0x19500150,0x1D500150,
    0x01D001D0,0x05D001D0,0x09D001D0,0x0DD001D0,0x11D001D0,0x15D001D0,0x19D001D0,0x1DD001D0,
    0x02500250,0x06500250,0x0A500250,0x0E500250,0x12500250,0x16500250,0x1A500250,0x1E500250,
    0x02D002D0,0x06D002D0,0x0AD002D0,0x0ED002D0,0x12D002D0,0x16D002D0,0x1AD002D0,0x1ED002D0,
    0x03500350,0x07500350,0x0B500350,0x0F500350,0x13500350,0x17500350,0x1B500350,0x1F500350,
    0x03D003D0,0x07D003D0,0x0BD003D0,0x0FD003D0,0x13D003D0,0x17D003D0,0x1BD003D0,0x1FD003D0
};

QE_API_IMPL
qeaii_frame_t* qeaii_frame(qeaii_t* pc)
{
    qeaii_frame_t* frame = &(pc->video.frames[ pc->video.current_frame ]);
    pc->video.current_frame = !(pc->video.current_frame);
    frame->is_text = pc->video.is_text;
    frame->is_mixed = pc->video.is_mixed;
    frame->is_hires = pc->video.is_hires;
    return frame;
}

QE_SIC
void video_init(qeaii_t* pc, qeaii_bootstrap_t* bootstrap)
{
    QE_CLEAR_OBJ(pc->video);
    pc->video.is_text = qe_true;
    pc->video.current_frame = 1;

    // Generate font
    unsigned rom_idx = 0;
    for (unsigned code = 0; code < 64; code++)
    {
        for (unsigned line = 0; line < 8; line++)
        {
            uint8_t pixels = 0;
            for(unsigned b = 0; b < 8; b++)
            {
                pixels |= ((bootstrap->font_rom[rom_idx] >> (7-b)) & 1) << b;
            }
            pixels >>= 1;
            pc->video.font_lines[line][code] = pixels;
            pc->video.ifont_lines[line][code] = (~pixels) & 0x7f;
            rom_idx++;
        }
    }
}

QE_SIC
uint32_t video_address(qeaii_t* pc)
{
    return (pc->video.is_page2 ? 0x40000800:0x20000400);
}

QE_SIC
void video_softswitch_write(qeaii_t* pc, uint8_t softswitch)
{
    switch(softswitch)
    {
    case 0x50: pc->video.is_text  = qe_false;   break;  // 0xc050
    case 0x51: pc->video.is_text  = qe_true;    break;  // 0xc051
    case 0x52: pc->video.is_mixed = qe_false;   break;  // 0xc052
    case 0x53: pc->video.is_mixed = qe_true;    break;  // 0xc053
    case 0x54: pc->video.is_page2 = qe_false;   break;  // 0xc054
    case 0x55: pc->video.is_page2 = qe_true;    break;  // 0xc055
    case 0x56: pc->video.is_hires = qe_false;   break;  // 0xc056
    case 0x57: pc->video.is_hires = qe_true;    break;  // 0xc057
    default:break;
    }
}

QE_SIC
uint8_t video_softswitch_read(qeaii_t* pc, uint8_t softswitch)
{
    switch (softswitch)
    {
    case 0x1a: return QE_U8( (pc->video.is_text?1:0)  << 7 );
    case 0x1b: return QE_U8( (pc->video.is_mixed?1:0) << 7 );
    case 0x1c: return QE_U8( (pc->video.is_page2?1:0) << 7 );
    case 0x1d: return QE_U8( (pc->video.is_hires?1:0) << 7 );
    default: break;
    }
    return (video_softswitch_write(pc, softswitch), 0);
}

QE_SIC
void video_clock(qeaii_t* pc)
{
    qeaii_videocard_t* video = &pc->video;
    qeaii_bus_t* bus = &pc->bus;
    if (video->line < qeaii_height)
    {
        if (video->col < qeaii_clocks_per_line_visible_pixels)
        {
            qeaii_frame_t* frame = &video->frames[ video->current_frame ];
            if (video->is_text ||
                (video->is_mixed && video->line >= 160))
            {
                // draw text
                uint8_t symbol = bus->memory.data[ video->offsets.lsw.u16 ];
                uint8_t symbol_line = video->line % 8;
                uint8_t bitmap_idx = symbol % 64;

                    // 0b11000000     0b01000000
                if (((symbol & 0xC0) == 0x40) && video->blink)
                {
                    symbol ^= 0x80;
                }
                if (symbol & 0x80)
                {
                    uint8_t pixels = video->font_lines[symbol_line][bitmap_idx];
                    frame->bitmap[ video->frame_pos++ ] = pixels;
                }
                else
                {
                    uint8_t pixels = video->ifont_lines[symbol_line][bitmap_idx];
                    frame->bitmap[ video->frame_pos++ ] = pixels;
                }

            }
            else if (video->is_hires)
            {
                // draw hires
                frame->bitmap[ video->frame_pos++ ] = bus->memory.data[ video->offsets.msw.u16 ];
            }
            else
            {
                // draw lores
                uint8_t code = bus->memory.data[ video->offsets.lsw.u16 ];
                if (video->line & 0xC) //0b00001100
                {
                    code >>= 4;
                }
                else
                {
                    code &= 0x0f;
                }
                frame->bitmap[ video->frame_pos++ ] = code;
            }
            video->col++;
            // incrementh both counters (text and graphics)
            video->offsets.u32 += 0x00010001;
        }
        else
        {
            video->col++;
            if (video->col == qeaii_total_clocks_per_line)
            {
                video->col = 0;
                video->line++;
                video->offsets.u32 = s_video_offsets[video->line] + video_address(pc);
            }
        }
    }
    else
    {
        video->col++;
        if (video->col == qeaii_dummy_lines * qeaii_total_clocks_per_line)
        {
            pc->stop_flags |= qeaii_flag_new_frame;
            video->frame_pos = 0;
            video->col = 0;
            video->line = 0;
            video->offsets.u32 = s_video_offsets[0] + video_address(pc);
        }
    }
}

///////////////////////////////////////////////////////
//                  SPEAKER
///////////////////////////////////////////////////////

QE_API_IMPL
qeaii_speaker_frame_t*
qeaii_speaker_frame(qeaii_t* pc)
{
    uint8_t old_frame = pc->speaker.current_frame;
    uint8_t new_frame = !old_frame;

    pc->speaker.current_frame = new_frame;
    pc->speaker.frames[new_frame].start_cycle = pc->cycle_counter + 1;
    pc->speaker.frames[new_frame].tick_count = 0;
    pc->speaker.frames[new_frame].speaker_state = pc->speaker.last_value;

    pc->speaker.frames[old_frame].end_cycle = pc->cycle_counter;
    return &pc->speaker.frames[old_frame];
}

QE_SIC
void speaker_init(qeaii_t* pc, qeaii_bootstrap_t* bootstrap)
{
    (void)bootstrap;
    QE_CLEAR_OBJ(pc->speaker);
}

QE_SIC
void speaker_io(qeaii_t* pc)
{
    qeaii_speaker_frame_t* frame = &pc->speaker.frames[pc->speaker.current_frame];
    if (frame->tick_count < QE_ARRAY_SIZE(frame->ticks))
    {
        pc->speaker.last_value = !pc->speaker.last_value;
        frame->ticks[frame->tick_count] = QE_U32(pc->cycle_counter - frame->start_cycle);
        frame->tick_count++;
    }
}

///////////////////////////////////////////////////////
//                  DRIVE II
///////////////////////////////////////////////////////

QE_API_IMPL
void qeaii_mount_disk0(qeaii_t* pc, qeaii_diskette_t* diskette)
{
    pc->driveII.drives[0].is_mount = qe_true;
    pc->driveII.drives[0].diskette.changed = qe_false;
    QE_COPY_OBJ(pc->driveII.drives[0].diskette, *diskette);
}

QE_API_IMPL
void qeaii_unmount_disk0(qeaii_t* pc)
{
    pc->driveII.drives[0].is_mount = qe_false;
}

QE_SIC
void driveII_init(qeaii_t* pc, qeaii_bootstrap_t* bootstrap)
{
    QE_CLEAR_OBJ(pc->driveII);
    if (bootstrap->mount_disk0)
    {
        pc->driveII.drives[0].is_mount = qe_true;
        QE_COPY_OBJ(pc->driveII.drives[0].diskette, bootstrap->disk0);
    }
}

QE_SIC
void driveII_phase_on(qeaii_drive_state_t* drive, uint8_t phase)
{
    drive->phases[phase] = qe_true;
    uint8_t direction = QE_U8( (phase - drive->phase + 4) % 4 );
    if (direction == 1)
    {
        drive->phase = QE_U8( (drive->phase + 1) % 4 );
        if ((drive->phase & 1) == 0 && drive->track < qeaii_disk_tracks - 1)
        {
            drive->track++;
        }
    }
    else if (direction == 3)
    {
        drive->phase = QE_U8( (drive->phase + 3) % 4 );
        if ((drive->phase & 1) && drive->track > 0)
        {
            drive->track--;
        }
    }
}

QE_SIC
uint8_t driveII_latch_event(qeaii_drive_state_t* drive, uint8_t data, qe_bool sw_reading)
{
    if (drive->q7)
    {
        if (!drive->q6)
        {
            return 0x80;
        }
        if (!sw_reading && !drive->diskette.readonly) // switch not reading (switch writing)
        {
            drive->diskette.data[ drive->track * qeaii_disk_track_size + drive->track_pos] = data;
            drive->track_pos = QE_U16( (drive->track_pos + 1) % qeaii_disk_track_size );
            drive->diskette.changed = qe_true;
        }
        return 0x0;
    }
    else
    {
        if (drive->q6)
        {
            return drive->diskette.readonly ? 0xff : 0; // return write-protected flag
        }

        // read mode and cpu do not need info
        // prepare and retun the data
        if (drive->phase != 0 && drive->phase != 2)
        {
            return 0x0;
        }
        uint8_t latch = drive->diskette.data[ drive->track * qeaii_disk_track_size + drive->track_pos];
        drive->track_pos = QE_U16( (drive->track_pos + 1) % qeaii_disk_track_size );
        return latch;
    }
}

QE_SIC
uint8_t driveII_process_softswitch(qeaii_t* pc, uint8_t softswitch, uint8_t data, qe_bool sw_reading)
{
    qeaii_drive_state_t* drive = &pc->driveII.drives[ pc->driveII.active_drive ];
    if (!drive->is_mount)
    {
        return 0x0;
    }
    if (!pc->driveII.spinning)
    {
        if (softswitch == 0xe9)
        {
            // Disk spinning
            pc->driveII.spinning = qe_true;
        }
        return 0x0;
    }

    switch (softswitch)
    {
    // Track step movement phases [0; 3]
    case 0xe0:   drive->phases[0] = qe_false;        return 0x00;
    case 0xe1:   driveII_phase_on(drive, 0);         return 0x00;
    case 0xe2:   drive->phases[1] = qe_false;        return 0x00;
    case 0xe3:   driveII_phase_on(drive, 1);         return 0x00;
    case 0xe4:   drive->phases[2] = qe_false;        return 0x00;
    case 0xe5:   driveII_phase_on(drive, 2);         return 0x00;
    case 0xe6:   drive->phases[3] = qe_false;        return 0x00;
    case 0xe7:   driveII_phase_on(drive, 3);         return 0x00;
    // Disk spinning
    case 0xe8:   pc->driveII.spinning = qe_false;    return 0x0;
    // Select drive
    case 0xeA:   pc->driveII.active_drive = 0;       return 0x00;
    case 0xeB:   pc->driveII.active_drive = 1;       return 0x00;
    // Data read/write
    case 0xeC:   drive->q6 = qe_false;               return driveII_latch_event(drive, data, sw_reading);
    case 0xeD:   drive->q6 = qe_true;                return driveII_latch_event(drive, data, sw_reading);
    case 0xeE:   drive->q7 = qe_false;               return driveII_latch_event(drive, data, sw_reading);
    case 0xeF:   drive->q7 = qe_true;                return driveII_latch_event(drive, data, sw_reading);
    default: break;
    }
    return 0x00;
}

QE_SIC
uint8_t driveII_read(qeaii_t* pc, uint8_t softswitch)
{
    return driveII_process_softswitch(pc, softswitch, 0, qe_true);
}

QE_SIC
void driveII_write(qeaii_t* pc, uint8_t softswitch, uint8_t data)
{
    driveII_process_softswitch(pc, softswitch, data, qe_false);
}

///////////////////////////////////////////////////////
//                  MEMORY
///////////////////////////////////////////////////////

QE_SIC
void bus_init(qeaii_t* pc, qeaii_bootstrap_t* bootstrap)
{
    QE_COPY_OBJ(pc->bus.memory, bootstrap->mem);
    pc->bus.firts_rom_address = bootstrap->first_rom_address;
}

QE_SIC
void bus_write(qeaii_t* pc, qe_word_t address, uint8_t data)
{
    if (QE_LIKELY( address.u8_msb != 0xc0 ))
    {
        if (QE_LIKELY(address.u16 < pc->bus.firts_rom_address))
        {
            pc->bus.memory.data[address.u16] = data;
        }
    }
    else
    {
        uint8_t softswitch = address.u8_lsb;
        if ((softswitch >= 0x1a && softswitch <= 0x1d) ||
            (softswitch >= 0x50 && softswitch <= 0x57))
        {
            video_softswitch_write(pc, softswitch);
        }
        else if (softswitch == 0x00)
        {
            kbd_softswitch_write(pc);
        }
        else if (softswitch == 0x30)
        {
            speaker_io(pc);
        }
        else if (softswitch >= 0xe0 && softswitch <= 0xef)
        {
            driveII_write(pc, softswitch, data);
        }
    }
}

QE_SIC
uint8_t bus_read(qeaii_t* pc, qe_word_t address)
{
    if (QE_LIKELY( address.u8_msb != 0xc0 ))
    {
        return pc->bus.memory.data[address.u16];
    }
    else
    {
        uint8_t softswitch = address.u8_lsb;
        if ((softswitch >= 0x1a && softswitch <= 0x1d) ||
            (softswitch >= 0x50 && softswitch <= 0x57))
        {
            return video_softswitch_read(pc, softswitch);
        }
        else if (softswitch == 0x00 || softswitch == 0x10)
        {
            return kbd_softswitch_read(pc, softswitch);
        }
        else if (softswitch == 0x30)
        {
            speaker_io(pc);
        }
        else if (softswitch >= 0xe0 && softswitch <= 0xef)
        {
            return driveII_read(pc, softswitch);
        }
    }
    return 0;
}

QE_SIC
void bus_clock(qeaii_t *pc)
{
    qe_word_t address;
    address.u16 = qe6502_address(&pc->cpu);
    if (qe6502_has_data(&pc->cpu))
    {
        // Write request
        uint8_t data = qe6502_data(&pc->cpu);
        bus_write(pc, address, data);
    }
    else
    {
        // Read request
        uint8_t data = bus_read(pc, address);
        qe6502_feed_data(&pc->cpu, data);
    }
}

///////////////////////////////////////////////////////
//                  CPU
///////////////////////////////////////////////////////

QE_SIC
void cpu_init(qeaii_t *pc)
{
    pc->nmi = qe_false;
    pc->cycle = qe6502_power_on(&pc->cpu, qe6502_mos);
    pc->is_ok = qe6502_ok(&pc->cpu);
}

QE_SIC
void cpu_clock(qeaii_t *pc)
{
    pc->cycle = pc->cycle.execute(&pc->cpu);
    pc->cycle_counter++;
    pc->cycle_counter += pc->cpu.merged;
    if (!qe6502_ok(&pc->cpu))
    {
        pc->stop_flags |= qeaii_flag_cpu_error;
    }
}

QE_SIC
void cpu_handle_nmi(qeaii_t *pc)
{
    if (pc->nmi)
    {
        pc->cpu.PC.u8_lsb = pc->bus.memory.data[ 0xFFFA ];
        pc->cpu.PC.u8_msb = pc->bus.memory.data[ 0xFFFB ];
        pc->nmi = qe_false;
    }
}

///////////////////////////////////////////////////////
//                  APPLE II PC
///////////////////////////////////////////////////////

QE_API_IMPL
qe_bool qeaii_power_on(qeaii_t *pc, qeaii_bootstrap_t* bootstrap)
{
    pc->ex_video_handler = bootstrap->ex_video_handler;
    pc->ex_bus_handler = bootstrap->ex_bus_handler;
    pc->ex_cpu_handler = bootstrap->ex_cpu_handler;

    bus_init(pc, bootstrap);
    kbd_init(pc, bootstrap);
    video_init(pc, bootstrap);
    speaker_init(pc, bootstrap);
    driveII_init(pc, bootstrap);
    cpu_init(pc);
    return qeaii_pc_ok(pc);
}

QE_API_IMPL
qe_bool qeaii_pc_ok(qeaii_t* pc)
{
    return qe6502_ok(&pc->cpu);
}

QE_API_IMPL
uint32_t qeaii_run(qeaii_t *pc, uint32_t requested_cycles)
{
    if (!pc->is_ok)
    {
        return 0;
    }
    uint64_t target_cycles = pc->cycle_counter + requested_cycles;
    pc->stop_flags = 0;
    cpu_handle_nmi(pc);
    pc->video.blink = pc->cycle_counter & ( 1 << 19 ) ? 1 : 0;

    while((pc->cycle_counter < target_cycles) && (!pc->stop_flags))
    {
        video_clock(pc);
        bus_clock(pc);
        cpu_clock(pc);
    }
    return QE_U32(pc->cycle_counter - requested_cycles);
}

QE_API_IMPL
uint32_t qeaii_run_ex(qeaii_t* pc, uint32_t requested_cycles)
{
    if (!pc->is_ok)
    {
        return 0;
    }
    uint64_t target_cycles = pc->cycle_counter + requested_cycles;
    pc->stop_flags = 0;
    cpu_handle_nmi(pc);
    pc->video.blink = pc->cycle_counter & ( 1 << 19 ) ? 1 : 0;

    while((pc->cycle_counter < target_cycles) && (!pc->stop_flags))
    {
        if (QE_NULL == pc->ex_video_handler ||
            qe_false == pc->ex_video_handler(pc))
        {
            video_clock(pc);
        }
        if (QE_NULL == pc->ex_bus_handler ||
            qe_false == pc->ex_bus_handler(pc))
        {
            bus_clock(pc);
        }
        if (QE_NULL == pc->ex_cpu_handler ||
            qe_false == pc->ex_cpu_handler(pc))
        {
            cpu_clock(pc);
        }
    }
    return QE_U32(pc->cycle_counter - requested_cycles);
}

QE_API_IMPL
void qeaii_press_key(qeaii_t *pc, uint8_t key)
{
    pc->kbd.key = key;
}

QE_API_IMPL
qe_bool qeaii_disk_active(qeaii_t *pc)
{
    return pc->driveII.spinning;
}

QE_API_IMPL
qe_bool qeaii_frame_ready(qeaii_t *pc)
{
    return (pc->stop_flags & qeaii_flag_new_frame) != 0;
}

QE_API_IMPL
void qeaii_break(qeaii_t *pc)
{
    pc->nmi = qe_true;
}
