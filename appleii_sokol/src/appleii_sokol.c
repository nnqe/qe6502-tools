/*
 * Minimal Sokol Apple II runner for qeaii.
 *
 * The program accepts one disk image path (.dsk or .nib), boots the Apple II
 * Plus ROM copied from the old project, and displays the live video frame.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qeaii.h"
#include "qeaiihelpers.h"
#include "dsk2nib.h"

#if !defined(SOKOL_GLCORE) && !defined(SOKOL_DUMMY_BACKEND)
#define SOKOL_GLCORE
#endif
#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#if defined(QEAII_ENABLE_SOKOL_AUDIO)
#include "sokol_audio.h"
#endif

#ifndef QEAII_PATH_MAX
#define QEAII_PATH_MAX 4096
#endif

enum {
    QEAII_SCREEN_WIDTH = 280,
    QEAII_SCREEN_HEIGHT = 192,
    QEAII_RGB_STRIDE = QEAII_SCREEN_WIDTH * QEAII_SCREEN_HEIGHT * 3,
    QEAII_RGBA_STRIDE = QEAII_SCREEN_WIDTH * QEAII_SCREEN_HEIGHT * 4,
    QEAII_AUDIO_CHUNK_NANOS = 8000000
};

static const uint8_t s_appleii_rom[0x10000] =
#include "apple_ii_plus_disk_ii_card_5.hex"
;

static const uint8_t s_appleii_video_rom[2048] =
#include "apple_ii_plus_video_rom.hex"
;

typedef struct {
    float x;
    float y;
    float u;
    float v;
} vertex_t;

typedef struct {
    char disk_path[QEAII_PATH_MAX];
    char error[1024];

    qeaii_t apple;
    qeaii_bootstrap_t bootstrap;

    uint8_t rgb[QEAII_RGB_STRIDE];
    uint8_t rgba[QEAII_RGBA_STRIDE];

    sg_image image;
    sg_view image_view;
    sg_sampler sampler;
    sg_buffer vertex_buffer;
    sg_shader shader;
    sg_pipeline pipeline;
    sg_bindings bindings;
    sg_pass_action pass_action;

    bool booted;
    bool frame_upload_pending;

#if defined(QEAII_ENABLE_SOKOL_AUDIO)
    bool audio_started;
    int audio_sample_rate;
    int audio_channels;
    double audio_sample_remainder;
    size_t audio_capacity_frames;
    int16_t* audio_i16;
    float* audio_f32;
#endif
} app_state_t;

static app_state_t s_app;

static void set_error(char* error, size_t error_size, const char* text)
{
    if ((error == NULL) || (error_size == 0)) {
        return;
    }
    if (text == NULL) {
        error[0] = '\0';
        return;
    }
    snprintf(error, error_size, "%s", text);
    error[error_size - 1] = '\0';
}

static bool ends_with_ext(const char* path, const char* ext)
{
    if ((path == NULL) || (ext == NULL)) {
        return false;
    }
    size_t path_len = strlen(path);
    size_t ext_len = strlen(ext);
    if (path_len < ext_len) {
        return false;
    }
    const char* suffix = path + path_len - ext_len;
    for (size_t i = 0; i < ext_len; i++) {
        if (tolower((unsigned char)suffix[i]) != tolower((unsigned char)ext[i])) {
            return false;
        }
    }
    return true;
}

static bool read_file_exact(const char* path, uint8_t* dst, size_t expected_size, char* error, size_t error_size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "cannot open '%s': %s", path, strerror(errno));
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        snprintf(error, error_size, "cannot seek '%s'", path);
        fclose(file);
        return false;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        snprintf(error, error_size, "cannot read size of '%s'", path);
        fclose(file);
        return false;
    }

    if ((size_t)file_size != expected_size) {
        snprintf(error,
                 error_size,
                 "'%s' must be exactly %u bytes, got %ld",
                 path,
                 (unsigned)expected_size,
                 file_size);
        fclose(file);
        return false;
    }

    rewind(file);
    size_t read_count = fread(dst, 1, expected_size, file);
    if (read_count != expected_size) {
        snprintf(error, error_size, "cannot read '%s'", path);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

static bool load_diskette(const char* path, qeaii_diskette_t* diskette, char* error, size_t error_size)
{
    memset(diskette, 0, sizeof(*diskette));
    diskette->readonly = false;
    diskette->changed = false;

    if (ends_with_ext(path, ".nib")) {
        return read_file_exact(path, diskette->data, qeaii_nib_image_size, error, error_size);
    }

    if (ends_with_ext(path, ".dsk")) {
        uint8_t* dsk = (uint8_t*)malloc(qeaii_dsk_image_size);
        if (dsk == NULL) {
            set_error(error, error_size, "out of memory while loading DSK image");
            return false;
        }
        bool ok = read_file_exact(path, dsk, qeaii_dsk_image_size, error, error_size);
        if (ok) {
            ok = qeaii_dsk2nib(dsk,
                               qeaii_dsk_image_size,
                               diskette->data,
                               sizeof(diskette->data),
                               error,
                               error_size);
        }
        free(dsk);
        return ok;
    }

    snprintf(error, error_size, "unsupported disk image extension for '%s' (expected .dsk or .nib)", path);
    return false;
}

static bool boot_appleii(const char* disk_path, char* error, size_t error_size)
{
    memset(&s_app.apple, 0, sizeof(s_app.apple));
    memset(&s_app.bootstrap, 0, sizeof(s_app.bootstrap));

    if (sizeof(s_appleii_rom) != sizeof(s_app.bootstrap.mem)) {
        set_error(error, error_size, "embedded Apple II ROM has unexpected size");
        return false;
    }
    if (sizeof(s_appleii_video_rom) != sizeof(s_app.bootstrap.font_rom)) {
        set_error(error, error_size, "embedded Apple II video ROM has unexpected size");
        return false;
    }

    memcpy(s_app.bootstrap.mem, s_appleii_rom, sizeof(s_app.bootstrap.mem));
    memcpy(s_app.bootstrap.font_rom, s_appleii_video_rom, sizeof(s_app.bootstrap.font_rom));
    s_app.bootstrap.first_rom_address = 0xc000;

    if (!load_diskette(disk_path, &s_app.bootstrap.disk0, error, error_size)) {
        return false;
    }
    s_app.bootstrap.mount_disk0 = true;

    if (!qeaii_power_on(&s_app.apple, &s_app.bootstrap)) {
        set_error(error, error_size, "qeaii_power_on failed");
        return false;
    }
    (void)qeaii_speaker_frame(&s_app.apple);

    return true;
}

static void frame_rgb_to_rgba(void)
{
    const uint8_t* src = s_app.rgb;
    uint8_t* dst = s_app.rgba;
    for (int i = 0; i < QEAII_SCREEN_WIDTH * QEAII_SCREEN_HEIGHT; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 255;
        src += 3;
        dst += 4;
    }
}

static void build_current_frame_pixels(void)
{
    qeaii_frame_t* frame = qeaii_frame(&s_app.apple);
    qeaii_to_rgb(frame, s_app.rgb);
    frame_rgb_to_rgba();
}

static void upload_current_frame_once(void)
{
    sg_image_data image_data;
    memset(&image_data, 0, sizeof(image_data));
    image_data.mip_levels[0].ptr = s_app.rgba;
    image_data.mip_levels[0].size = sizeof(s_app.rgba);
    sg_update_image(s_app.image, &image_data);
}

static uint32_t frame_cycles(void)
{
    double dt = sapp_frame_duration();
    if ((dt <= 0.0) || (dt > 0.05)) {
        dt = 1.0 / 60.0;
    }
    uint64_t nanos64 = (uint64_t)(dt * 1000000000.0);
    if (nanos64 == 0) {
        nanos64 = 1;
    }
    if (nanos64 > UINT32_MAX) {
        nanos64 = UINT32_MAX;
    }
    uint32_t cycles = (uint32_t)qeaii_to_cycles((uint32_t)nanos64);
    return (cycles == 0) ? 1u : cycles;
}

static uint32_t audio_chunk_cycles(void)
{
    uint32_t cycles = (uint32_t)qeaii_to_cycles(QEAII_AUDIO_CHUNK_NANOS);
    return (cycles == 0) ? 1u : cycles;
}

#if defined(QEAII_ENABLE_SOKOL_AUDIO)
static bool ensure_audio_capacity(size_t frames)
{
    if (frames <= s_app.audio_capacity_frames) {
        return true;
    }

    size_t new_capacity = (s_app.audio_capacity_frames == 0) ? 512u : s_app.audio_capacity_frames;
    while (new_capacity < frames) {
        new_capacity *= 2u;
    }

    int16_t* new_i16 = (int16_t*)realloc(s_app.audio_i16, new_capacity * sizeof(int16_t));
    if (new_i16 == NULL) {
        return false;
    }
    s_app.audio_i16 = new_i16;

    int channels = (s_app.audio_channels > 0) ? s_app.audio_channels : 1;
    float* new_f32 = (float*)realloc(s_app.audio_f32, new_capacity * (size_t)channels * sizeof(float));
    if (new_f32 == NULL) {
        return false;
    }
    s_app.audio_f32 = new_f32;
    s_app.audio_capacity_frames = new_capacity;
    return true;
}

static uint32_t audio_frames_for_cycles(uint64_t cycles)
{
    if ((cycles == 0) || (s_app.audio_sample_rate <= 0)) {
        return 0;
    }

    if (cycles > UINT32_MAX) {
        cycles = UINT32_MAX;
    }

    double nanos = (double)qeaii_to_nanos((uint32_t)cycles);
    double exact_frames = (nanos * (double)s_app.audio_sample_rate / 1000000000.0) +
                          s_app.audio_sample_remainder;
    uint32_t frames = (uint32_t)exact_frames;
    s_app.audio_sample_remainder = exact_frames - (double)frames;
    return frames;
}

static void make_audio_resources(void)
{
    saudio_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.sample_rate = 44100;
    desc.num_channels = 1;
    desc.buffer_frames = 2048;
    desc.packet_frames = 128;
    desc.num_packets = 64;
    desc.logger.func = slog_func;
    saudio_setup(&desc);
    s_app.audio_started = true;

    if (!saudio_isvalid()) {
        fprintf(stderr, "qeaii_sokol_appleii: audio backend unavailable, continuing without sound\n");
        return;
    }

    s_app.audio_sample_rate = saudio_sample_rate();
    s_app.audio_channels = saudio_channels();
    if (s_app.audio_sample_rate <= 0) {
        s_app.audio_sample_rate = 44100;
    }
    if (s_app.audio_channels <= 0) {
        s_app.audio_channels = 1;
    }
}

static void cleanup_audio_resources(void)
{
    if (s_app.audio_started) {
        saudio_shutdown();
        s_app.audio_started = false;
    }
    free(s_app.audio_i16);
    free(s_app.audio_f32);
    s_app.audio_i16 = NULL;
    s_app.audio_f32 = NULL;
    s_app.audio_capacity_frames = 0;
}

static void submit_audio_for_executed_cycles(void)
{
    qeaii_speaker_frame_t* speaker_frame = qeaii_speaker_frame(&s_app.apple);

    if (!s_app.audio_started || !saudio_isvalid()) {
        return;
    }

    /*
     * The old SDL runner discarded empty speaker frames and also kept audio
     * quiet while the Disk II motor was spinning.  Preserve that behavior so
     * a stable speaker output doesn't turn into a DC offset and disk booting
     * doesn't fill the audio queue with unwanted speaker transitions.
     */
    if ((speaker_frame->tick_count == 0) || qeaii_disk_active(&s_app.apple)) {
        return;
    }

    uint64_t cycles = 0;
    if (speaker_frame->end_cycle >= speaker_frame->start_cycle) {
        cycles = speaker_frame->end_cycle - speaker_frame->start_cycle + 1u;
    }
    uint32_t frames = audio_frames_for_cycles(cycles);
    if (frames == 0) {
        return;
    }

    if (!ensure_audio_capacity(frames)) {
        return;
    }

    qeaii_to_audio_samples(speaker_frame, s_app.audio_i16, frames);

    int channels = (s_app.audio_channels > 0) ? s_app.audio_channels : 1;
    for (uint32_t frame = 0; frame < frames; frame++) {
        float sample = (float)s_app.audio_i16[frame] / 32768.0f;
        if (sample > 1.0f) {
            sample = 1.0f;
        } else if (sample < -1.0f) {
            sample = -1.0f;
        }
        for (int channel = 0; channel < channels; channel++) {
            s_app.audio_f32[(size_t)frame * (size_t)channels + (size_t)channel] = sample;
        }
    }

    (void)saudio_push(s_app.audio_f32, (int)frames);
}
#else
static void submit_audio_for_executed_cycles(void)
{
    (void)qeaii_speaker_frame(&s_app.apple);
}
#endif

static void run_machine_for_this_frame(void)
{
    if (!s_app.booted) {
        return;
    }

    uint32_t requested = frame_cycles();
    uint32_t done = 0;
    uint32_t audio_chunk = audio_chunk_cycles();
    while (done < requested) {
        uint32_t remaining = requested - done;
        uint32_t target = (remaining < audio_chunk) ? remaining : audio_chunk;
        uint32_t chunk = qeaii_run(&s_app.apple, target);
        if (chunk == 0) {
            break;
        }
        done += chunk;
        submit_audio_for_executed_cycles();
        if (qeaii_frame_ready(&s_app.apple)) {
            build_current_frame_pixels();
            s_app.frame_upload_pending = true;
        }
    }
}

static void make_shader_and_pipeline(void)
{
    const char* vs_src =
        "#version 410\n"
        "in vec2 position;\n"
        "in vec2 texcoord0;\n"
        "out vec2 uv;\n"
        "void main() {\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "    uv = texcoord0;\n"
        "}\n";

    const char* fs_src =
        "#version 410\n"
        "uniform sampler2D tex0_smp;\n"
        "in vec2 uv;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "    frag_color = texture(tex0_smp, uv);\n"
        "}\n";

    sg_shader_desc shader_desc;
    memset(&shader_desc, 0, sizeof(shader_desc));
    shader_desc.vertex_func.source = vs_src;
    shader_desc.fragment_func.source = fs_src;
    shader_desc.attrs[0].glsl_name = "position";
    shader_desc.attrs[1].glsl_name = "texcoord0";
    shader_desc.views[0].texture.stage = SG_SHADERSTAGE_FRAGMENT;
    shader_desc.views[0].texture.image_type = SG_IMAGETYPE_2D;
    shader_desc.views[0].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
    shader_desc.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
    shader_desc.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
    shader_desc.texture_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
    shader_desc.texture_sampler_pairs[0].view_slot = 0;
    shader_desc.texture_sampler_pairs[0].sampler_slot = 0;
    shader_desc.texture_sampler_pairs[0].glsl_name = "tex0_smp";
    shader_desc.label = "appleii-shader";
    s_app.shader = sg_make_shader(&shader_desc);

    sg_pipeline_desc pipeline_desc;
    memset(&pipeline_desc, 0, sizeof(pipeline_desc));
    pipeline_desc.shader = s_app.shader;
    pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.label = "appleii-pipeline";
    s_app.pipeline = sg_make_pipeline(&pipeline_desc);
}

static void make_graphics_resources(void)
{
    sg_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    for (size_t i = 0; i < sizeof(s_app.rgba); i += 4) {
        s_app.rgba[i + 0] = 0;
        s_app.rgba[i + 1] = 0;
        s_app.rgba[i + 2] = 0;
        s_app.rgba[i + 3] = 255;
    }

    sg_image_desc image_desc;
    memset(&image_desc, 0, sizeof(image_desc));
    image_desc.usage.stream_update = true;
    image_desc.width = QEAII_SCREEN_WIDTH;
    image_desc.height = QEAII_SCREEN_HEIGHT;
    image_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    /*
     * Sokol validation rule: images marked dynamic_update/stream_update are
     * update targets and must be created without initial data.  The first
     * pixels are uploaded by frame_cb() via sg_update_image(), before draw.
     */
    image_desc.label = "appleii-frame";
    s_app.image = sg_make_image(&image_desc);

    sg_view_desc view_desc;
    memset(&view_desc, 0, sizeof(view_desc));
    view_desc.texture.image = s_app.image;
    view_desc.label = "appleii-frame-view";
    s_app.image_view = sg_make_view(&view_desc);

    sg_sampler_desc sampler_desc;
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.min_filter = SG_FILTER_NEAREST;
    sampler_desc.mag_filter = SG_FILTER_NEAREST;
    sampler_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    sampler_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    sampler_desc.label = "appleii-sampler";
    s_app.sampler = sg_make_sampler(&sampler_desc);

    const vertex_t vertices[] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f,  1.0f, 0.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f, 1.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f, 0.0f }
    };

    sg_buffer_desc buffer_desc;
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.data.ptr = vertices;
    buffer_desc.data.size = sizeof(vertices);
    buffer_desc.label = "appleii-quad";
    s_app.vertex_buffer = sg_make_buffer(&buffer_desc);

    make_shader_and_pipeline();

    memset(&s_app.bindings, 0, sizeof(s_app.bindings));
    s_app.bindings.vertex_buffers[0] = s_app.vertex_buffer;
    s_app.bindings.views[0] = s_app.image_view;
    s_app.bindings.samplers[0] = s_app.sampler;

    memset(&s_app.pass_action, 0, sizeof(s_app.pass_action));
    s_app.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    s_app.pass_action.colors[0].clear_value.r = 0.0f;
    s_app.pass_action.colors[0].clear_value.g = 0.0f;
    s_app.pass_action.colors[0].clear_value.b = 0.0f;
    s_app.pass_action.colors[0].clear_value.a = 1.0f;
}

static void draw_frame(void)
{
    int window_width = sapp_width();
    int window_height = sapp_height();
    float sx = (float)window_width / (float)QEAII_SCREEN_WIDTH;
    float sy = (float)window_height / (float)QEAII_SCREEN_HEIGHT;
    float scale = (sx < sy) ? sx : sy;
    if (scale < 1.0f) {
        scale = 1.0f;
    }

    int view_width = (int)((float)QEAII_SCREEN_WIDTH * scale);
    int view_height = (int)((float)QEAII_SCREEN_HEIGHT * scale);
    if (view_width > window_width) {
        view_width = window_width;
    }
    if (view_height > window_height) {
        view_height = window_height;
    }
    int view_x = (window_width - view_width) / 2;
    int view_y = (window_height - view_height) / 2;

    sg_pass pass;
    memset(&pass, 0, sizeof(pass));
    pass.action = s_app.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_apply_viewport(view_x, view_y, view_width, view_height, true);
    sg_apply_pipeline(s_app.pipeline);
    sg_apply_bindings(&s_app.bindings);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

static void apple_key(uint8_t code)
{
    if (s_app.booted) {
        qeaii_press_key(&s_app.apple, (uint8_t)(code | 0x80));
    }
}

static bool handle_control_key(const sapp_event* event)
{
    if ((event->modifiers & SAPP_MODIFIER_CTRL) == 0) {
        return false;
    }

    if ((event->key_code >= SAPP_KEYCODE_A) && (event->key_code <= SAPP_KEYCODE_Z)) {
        apple_key((uint8_t)(event->key_code - SAPP_KEYCODE_A + 1));
        return true;
    }

    switch (event->key_code) {
        case SAPP_KEYCODE_SPACE:
        case SAPP_KEYCODE_2:
        case SAPP_KEYCODE_GRAVE_ACCENT:
            apple_key(0x00);
            return true;
        case SAPP_KEYCODE_3:
        case SAPP_KEYCODE_LEFT_BRACKET:
            apple_key(0x1b);
            return true;
        case SAPP_KEYCODE_4:
        case SAPP_KEYCODE_BACKSLASH:
            apple_key(0x1c);
            return true;
        case SAPP_KEYCODE_5:
        case SAPP_KEYCODE_RIGHT_BRACKET:
            apple_key(0x1d);
            return true;
        case SAPP_KEYCODE_6:
            apple_key(0x1e);
            return true;
        case SAPP_KEYCODE_7:
        case SAPP_KEYCODE_MINUS:
        case SAPP_KEYCODE_SLASH:
            apple_key(0x1f);
            return true;
        default:
            return false;
    }
}

static void handle_key_down(const sapp_event* event)
{
    switch (event->key_code) {
        case SAPP_KEYCODE_ENTER:
        case SAPP_KEYCODE_KP_ENTER:
            apple_key(0x0d);
            break;
        case SAPP_KEYCODE_ESCAPE:
            apple_key(0x1b);
            break;
        case SAPP_KEYCODE_TAB:
            apple_key(0x09);
            break;
        case SAPP_KEYCODE_BACKSPACE:
        case SAPP_KEYCODE_DELETE:
        case SAPP_KEYCODE_LEFT:
            apple_key(0x08);
            break;
        case SAPP_KEYCODE_RIGHT:
            apple_key(0x15);
            break;
        case SAPP_KEYCODE_UP:
        case SAPP_KEYCODE_PAGE_UP:
            apple_key(0x0b);
            break;
        case SAPP_KEYCODE_DOWN:
        case SAPP_KEYCODE_PAGE_DOWN:
            apple_key(0x0a);
            break;
        case SAPP_KEYCODE_HOME:
            apple_key(0x01);
            break;
        case SAPP_KEYCODE_END:
            apple_key(0x05);
            break;
        default:
            (void)handle_control_key(event);
            break;
    }
}

static void handle_char(uint32_t codepoint)
{
    if ((codepoint >= 'a') && (codepoint <= 'z')) {
        codepoint -= ('a' - 'A');
    }
    if ((codepoint >= 0x20) && (codepoint <= 0x7e)) {
        apple_key((uint8_t)codepoint);
    }
}

static void init_cb(void)
{
    make_graphics_resources();
#if defined(QEAII_ENABLE_SOKOL_AUDIO)
    make_audio_resources();
#endif

    if (!boot_appleii(s_app.disk_path, s_app.error, sizeof(s_app.error))) {
        fprintf(stderr, "qeaii_sokol_appleii: %s\n", s_app.error);
        sapp_request_quit();
        return;
    }
    s_app.booted = true;
    /*
     * Upload the black startup texture on the first Sokol frame unless the
     * emulator produces a real Apple II video frame first.  Do not call
     * qeaii_frame() here: that function flips the Apple II video double buffer
     * and must only be called after qeaii_frame_ready() reports a completed
     * emulated frame.
     */
    s_app.frame_upload_pending = true;
}

static void frame_cb(void)
{
    run_machine_for_this_frame();
    if (s_app.frame_upload_pending) {
        upload_current_frame_once();
        s_app.frame_upload_pending = false;
    }
    draw_frame();
}

static void cleanup_cb(void)
{
#if defined(QEAII_ENABLE_SOKOL_AUDIO)
    cleanup_audio_resources();
#endif
    sg_shutdown();
}

static void event_cb(const sapp_event* event)
{
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
        handle_key_down(event);
    } else if (event->type == SAPP_EVENTTYPE_CHAR) {
        handle_char(event->char_code);
    }
}

sapp_desc sokol_main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <disk.dsk|disk.nib>\n", (argc > 0) ? argv[0] : "qeaii_sokol_appleii");
        exit(1);
    }

    snprintf(s_app.disk_path, sizeof(s_app.disk_path), "%s", argv[1]);
    s_app.disk_path[sizeof(s_app.disk_path) - 1] = '\0';

    sapp_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.init_cb = init_cb;
    desc.frame_cb = frame_cb;
    desc.cleanup_cb = cleanup_cb;
    desc.event_cb = event_cb;
    desc.width = 840;
    desc.height = 576;
    desc.high_dpi = true;
    desc.sample_count = 1;
    desc.swap_interval = 1;
    desc.window_title = "qe6502 Apple II";
    desc.logger.func = slog_func;
#if defined(_WIN32)
    desc.win32.console_attach = true;
#endif
    return desc;
}
