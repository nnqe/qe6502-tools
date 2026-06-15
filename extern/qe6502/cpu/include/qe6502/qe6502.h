#ifndef QE6502_H
#define QE6502_H

#include <stdint.h>

#include <qe6502/qe6502_version.h>

#ifndef QE6502_STATIC_ASSERT
# ifdef __cplusplus
#  define QE6502_STATIC_ASSERT(condition, message) static_assert((condition), message)
# else
#  define QE6502_STATIC_ASSERT(condition, message) _Static_assert((condition), message)
# endif
#endif

#if defined(__GNUC__) || defined(__clang__)
# define QE6502_MAYBE_UNUSED __attribute__((unused))
#else
# define QE6502_MAYBE_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ---------------------------------------------------------------------------
 * Processor model identifiers
 * ---------------------------------------------------------------------------
 *
 * Selects the supported 6502-family CPU model and its model-specific behavior.
 * Use these values with qe6502_setup() and qe6502_set_model().
 */

enum
{
    qe6502_model_nmos = 0,
    qe6502_model_nes  = 1,
    qe6502_model_wdc  = 2,
    qe6502_model_rw   = 3,
    qe6502_model_st   = 4,

    qe6502_supported_models_count = 5
};

/*
 * ---------------------------------------------------------------------------
 * Native CPU state, bus ticks, and control store
 * ---------------------------------------------------------------------------
 *
 * Public native types used by the tick loop: CPU storage, single-cycle bus
 * requests, processor-status flags, tick-status flags, and control-store entry
 * points. Prefer accessor functions over direct qe6502_t field access.
 */

enum
{
    /* Number of microcode entries stored in each opcode/service slot. */
    qe6502_microcode_per_slot = 8,

    /* Total number of microcode entries stored in the complete control store. */
    qe6502_control_store_size = 2 * 256 * qe6502_microcode_per_slot * qe6502_supported_models_count
};
QE6502_STATIC_ASSERT((qe6502_microcode_per_slot & (qe6502_microcode_per_slot - 1u)) == 0u,
                     "qe6502_microcode_per_slot must be a power of two");

#define QE6502_SERVICE_MODE_SLOT 0x100u
#define QE6502_IDX(model, slot, cycle)                      \
        (uint16_t)( (((uint32_t)(model) & 0x0Fu) << 12u) |  \
                    (((uint32_t)(slot) & 0x1FFu) << 3u)  |  \
                    (uint32_t)(cycle)   )

#define QE6502_SERVICE_SLOT_IDX(model, service, cycle) QE6502_IDX(model, service, cycle)

/* Native CPU storage. Prefer accessor functions over direct field access. */
typedef struct qe6502_cpu
{
    /* Configuration. */
    uint8_t  model;

    /* Internal execution state. */
    uint8_t  reserved_extension;
    uint16_t microcode;
    uint16_t latch_addr;
    uint8_t  latch_data;
    uint8_t  service_mode;

    /* CPU registers. */
    uint16_t PC;
    uint8_t  S;
    uint8_t  A;
    uint8_t  X;
    uint8_t  Y;
    uint8_t  P;

    /* Internal interrupt state. */
    uint8_t  interrupts;
} qe6502_t;
QE6502_STATIC_ASSERT(sizeof(qe6502_t) == 16, "qe6502_t must be 16 bytes");

/* Single bus-cycle request/result. */
typedef struct qe6502_tick_result
{
    uint16_t address;
    uint8_t  bus;
    uint8_t  status;
} qe6502_tick_t;
QE6502_STATIC_ASSERT(sizeof(qe6502_tick_t) == 4, "qe6502_tick_t must be 4 bytes");

/* Portable 64-byte snapshot buffer used by save/load APIs. */
#define QE6502_SNAPSHOT_SIZE (64u)
typedef uint8_t qe6502_snapshot_t[QE6502_SNAPSHOT_SIZE];

/* Processor status register flags. */
enum
{
    qe6502_flag_C  = (1u << 0),
    qe6502_flag_Z  = (1u << 1),
    qe6502_flag_I  = (1u << 2),
    qe6502_flag_D  = (1u << 3),
    qe6502_flag_B  = (1u << 4),
    qe6502_flag_UN = (1u << 5),
    qe6502_flag_V  = (1u << 6),
    qe6502_flag_N  = (1u << 7)
};

/* Private tick status flags. Prefer qe6502_is_*() helpers. */
enum
{
    qe6502_status_writing        = (1u << 0),
    qe6502_status_opcode_fetch   = (1u << 1),
    qe6502_status_internal_reset = (1u << 6),
    qe6502_status_cpu_jammed     = (1u << 7)
};

/* Microcode entry. */
typedef qe6502_tick_t (*qe6502_microcode_fn)(qe6502_t *cpu, uint8_t bus);

/* Control store. */
extern const qe6502_microcode_fn qe6502_control_store[qe6502_control_store_size];

/*
 * ---------------------------------------------------------------------------
 * Native C API
 * ---------------------------------------------------------------------------
 *
 * These functions operate directly on qe6502_t and qe6502_tick_t.
 * Prefer this accessor API over direct qe6502_t field access in application code.
 */

/* Initialize a native CPU context for the selected processor model. */
qe6502_t qe6502_setup(uint32_t model);

/* Restart the CPU context and return an initial dummy read request at address 0x00ff. */
qe6502_tick_t qe6502_restart(qe6502_t *cpu);

/* Enter execution at address and return the first bus request. */
qe6502_tick_t qe6502_goto(qe6502_t *cpu, uint16_t address);

/* Exported native C symbol equivalent to the inline qe6502_tick() helper. */
qe6502_tick_t qe6502_tick_exported(qe6502_t *cpu, uint8_t bus);

/* Interrupt pin control. */
void    qe6502_nmi_assert(qe6502_t *cpu, uint8_t assert_nmi);
void    qe6502_irq_assert(qe6502_t *cpu, uint8_t assert_irq);
uint8_t qe6502_is_nmi_asserted(const qe6502_t *cpu);
uint8_t qe6502_is_irq_asserted(const qe6502_t *cpu);

/* CPU register accessors. */
uint8_t  qe6502_get_model(const qe6502_t *cpu);
void     qe6502_set_model(qe6502_t *cpu, uint8_t value);
uint16_t qe6502_get_pc(const qe6502_t *cpu);
void     qe6502_set_pc(qe6502_t *cpu, uint16_t value);
uint8_t  qe6502_get_s(const qe6502_t *cpu);
void     qe6502_set_s(qe6502_t *cpu, uint8_t value);
uint8_t  qe6502_get_a(const qe6502_t *cpu);
void     qe6502_set_a(qe6502_t *cpu, uint8_t value);
uint8_t  qe6502_get_x(const qe6502_t *cpu);
void     qe6502_set_x(qe6502_t *cpu, uint8_t value);
uint8_t  qe6502_get_y(const qe6502_t *cpu);
void     qe6502_set_y(qe6502_t *cpu, uint8_t value);
uint8_t  qe6502_get_p(const qe6502_t *cpu);
void     qe6502_set_p(qe6502_t *cpu, uint8_t value);

/* Processor status flag accessors. Getters return the flag mask or zero. */
uint8_t qe6502_get_flag_c(const qe6502_t *cpu);
void    qe6502_set_flag_c(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_z(const qe6502_t *cpu);
void    qe6502_set_flag_z(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_i(const qe6502_t *cpu);
void    qe6502_set_flag_i(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_d(const qe6502_t *cpu);
void    qe6502_set_flag_d(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_b(const qe6502_t *cpu);
void    qe6502_set_flag_b(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_un(const qe6502_t *cpu);
void    qe6502_set_flag_un(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_v(const qe6502_t *cpu);
void    qe6502_set_flag_v(qe6502_t *cpu, uint8_t value);
uint8_t qe6502_get_flag_n(const qe6502_t *cpu);
void    qe6502_set_flag_n(qe6502_t *cpu, uint8_t value);

/* Save and restore a portable 64-byte native CPU snapshot. */
void          qe6502_save(const qe6502_t *cpu, qe6502_tick_t tick, qe6502_snapshot_t snapshot);
qe6502_tick_t qe6502_load(qe6502_t *cpu, const qe6502_snapshot_t snapshot);

/*
 * ---------------------------------------------------------------------------
 * Hot-path inline API declarations
 * ---------------------------------------------------------------------------
 *
 * Inline helpers for the CPU tick loop and tick-status decoding.
 * Definitions are kept at the end of this header.
 */

/* Execute one CPU bus phase and return the next packed bus request. */
static inline qe6502_tick_t qe6502_tick(qe6502_t *cpu, uint8_t bus);

/* Decode tick status bits returned by qe6502_tick(). */
static inline uint8_t       qe6502_is_write(qe6502_tick_t tick);
static inline uint8_t       qe6502_is_fetch(qe6502_tick_t tick);
static inline uint8_t       qe6502_is_reset(qe6502_tick_t tick);
static inline uint8_t       qe6502_is_jammed(qe6502_tick_t tick);

/*
 * ---------------------------------------------------------------------------
 * Inline implementation section
 * ---------------------------------------------------------------------------
 *
 * Public inline helpers are declared above with the rest of the API.
 * Their definitions are kept here to keep the main declaration section compact.
 */

static inline QE6502_MAYBE_UNUSED qe6502_tick_t
qe6502_tick(qe6502_t *cpu, uint8_t bus)
{
    if (cpu->service_mode != 0u)
    {
        return qe6502_control_store[QE6502_SERVICE_SLOT_IDX(0, QE6502_SERVICE_MODE_SLOT, 0)](cpu, bus);
    }

    qe6502_tick_t tick = qe6502_control_store[cpu->microcode](cpu, bus);
    cpu->microcode++;
    return tick;
}

static inline QE6502_MAYBE_UNUSED uint8_t
qe6502_is_write(qe6502_tick_t tick)
{
    return (uint8_t)(tick.status & qe6502_status_writing);
}

static inline QE6502_MAYBE_UNUSED uint8_t
qe6502_is_fetch(qe6502_tick_t tick)
{
    return (uint8_t)(tick.status & qe6502_status_opcode_fetch);
}

static inline QE6502_MAYBE_UNUSED uint8_t
qe6502_is_reset(qe6502_tick_t tick)
{
    return (uint8_t)(tick.status & qe6502_status_internal_reset);
}

static inline QE6502_MAYBE_UNUSED uint8_t
qe6502_is_jammed(qe6502_tick_t tick)
{
    return (uint8_t)(tick.status & qe6502_status_cpu_jammed);
}

#ifdef __cplusplus
}
#endif

#endif /* QE6502_H */
