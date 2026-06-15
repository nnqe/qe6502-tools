/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE6502_ABI_H
#define QE6502_ABI_H

#include <stdint.h>

#include <qe6502/qe6502_version.h>

#if defined(__cplusplus)
#   define QE6502_ABI_ALIGNAS(n) alignas(n)
#   define QE6502_ABI_ALIGNOF(type) alignof(type)
#   define QE6502_ABI_STATIC_ASSERT(condition, message) static_assert((condition), message)
#else
#   include <stdalign.h>
#   define QE6502_ABI_ALIGNAS(n) alignas(n)
#   define QE6502_ABI_ALIGNOF(type) alignof(type)
#   define QE6502_ABI_STATIC_ASSERT(condition, message) _Static_assert((condition), message)
#endif

#if defined(QE6502_STATIC)
#   define QE6502_ABI_API
#elif defined(_WIN32)
#   if defined(QE6502_BUILDING_LIBRARY)
#       define QE6502_ABI_API __declspec(dllexport)
#   elif defined(QE6502_SHARED)
#       define QE6502_ABI_API __declspec(dllimport)
#   else
#       define QE6502_ABI_API
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define QE6502_ABI_API __attribute__((visibility("default")))
#else
#   define QE6502_ABI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ---------------------------------------------------------------------------
 * Stable ABI wrapper
 * ---------------------------------------------------------------------------
 *
 * Shared-library, WASM, and FFI entry points using scalar values and
 * caller-owned opaque storage. Callers must pass valid objects and must not
 * edit context bytes directly.
 */

#define QE6502_ABI_CONTEXT_SIZE    64u
#define QE6502_ABI_CONTEXT_ALIGN   8u
#define QE6502_ABI_CONTEXT_MAGIC   0x6502u
#define QE6502_ABI_CONTEXT_VERSION 1u

/* Portable 64-byte snapshot buffer used by save/load APIs. */
#define QE6502_ABI_SNAPSHOT_SIZE QE6502_ABI_CONTEXT_SIZE
typedef uint8_t qe6502abi_snapshot_t[QE6502_ABI_SNAPSHOT_SIZE];

/*
 * ---------------------------------------------------------------------------
 * ABI context storage
 * ---------------------------------------------------------------------------
 *
 * Opaque 64-byte, 8-byte-aligned caller-owned context. The used bytes contain
 * private ABI state; the remaining bytes are reserved and must not be edited.
 */
typedef struct qe6502abi_context
{
    QE6502_ABI_ALIGNAS(QE6502_ABI_CONTEXT_ALIGN)
    uint8_t bytes[QE6502_ABI_CONTEXT_SIZE];
} qe6502abi_context_t;

QE6502_ABI_STATIC_ASSERT(sizeof(qe6502abi_context_t) == QE6502_ABI_CONTEXT_SIZE,
                         "qe6502abi_context_t must be exactly 64 bytes");
QE6502_ABI_STATIC_ASSERT(QE6502_ABI_ALIGNOF(qe6502abi_context_t) == QE6502_ABI_CONTEXT_ALIGN,
                         "qe6502abi_context_t must be 8-byte aligned");

typedef uint32_t qe6502abi_tick_t;

/*
 * ---------------------------------------------------------------------------
 * ABI constants and packed tick helpers
 * ---------------------------------------------------------------------------
 *
 * Stable scalar constants for processor models, status flags, and packed tick
 * decoding. These values are part of the shared/WASM/FFI ABI contract.
 */

#define QE6502_ABI_MODEL_NMOS  0u
#define QE6502_ABI_MODEL_NES   1u
#define QE6502_ABI_MODEL_WDC   2u
#define QE6502_ABI_MODEL_RW    3u
#define QE6502_ABI_MODEL_ST    4u
#define QE6502_ABI_MODEL_COUNT 5u

#define QE6502_ABI_FLAG_C  (1u << 0u)
#define QE6502_ABI_FLAG_Z  (1u << 1u)
#define QE6502_ABI_FLAG_I  (1u << 2u)
#define QE6502_ABI_FLAG_D  (1u << 3u)
#define QE6502_ABI_FLAG_B  (1u << 4u)
#define QE6502_ABI_FLAG_UN (1u << 5u)
#define QE6502_ABI_FLAG_V  (1u << 6u)
#define QE6502_ABI_FLAG_N  (1u << 7u)

/* Packed tick layout: address bits 0..15, status bits 16..23, bus bits 24..31. */
#define QE6502_ABI_TICK_ADDRESS_SHIFT 0u
#define QE6502_ABI_TICK_STATUS_SHIFT  16u
#define QE6502_ABI_TICK_BUS_SHIFT     24u

#define QE6502_ABI_TICK_WRITING_SHIFT        16u
#define QE6502_ABI_TICK_FETCH_SHIFT          17u
#define QE6502_ABI_TICK_INTERNAL_RESET_SHIFT 22u
#define QE6502_ABI_TICK_CPU_JAMMED_SHIFT     23u

#define QE6502_ABI_TICK_WRITING        (UINT32_C(1) << QE6502_ABI_TICK_WRITING_SHIFT)
#define QE6502_ABI_TICK_FETCH          (UINT32_C(1) << QE6502_ABI_TICK_FETCH_SHIFT)
#define QE6502_ABI_TICK_INTERNAL_RESET (UINT32_C(1) << QE6502_ABI_TICK_INTERNAL_RESET_SHIFT)
#define QE6502_ABI_TICK_CPU_JAMMED     (UINT32_C(1) << QE6502_ABI_TICK_CPU_JAMMED_SHIFT)

#define QE6502_ABI_TICK_ADDRESS(tick) \
    ((uint16_t)(((uint32_t)(tick) >> QE6502_ABI_TICK_ADDRESS_SHIFT) & 0xffffu))
#define QE6502_ABI_TICK_BUS(tick) \
    ((uint8_t)(((uint32_t)(tick) >> QE6502_ABI_TICK_BUS_SHIFT) & 0xffu))
#define QE6502_ABI_TICK_STATUS(tick) \
    ((uint8_t)(((uint32_t)(tick) >> QE6502_ABI_TICK_STATUS_SHIFT) & 0xffu))

/*
 * ---------------------------------------------------------------------------
 * ABI entry points
 * ---------------------------------------------------------------------------
 *
 * Shared/WASM/FFI functions for versioning, CPU execution, interrupt pins,
 * snapshots, and scalar CPU-state access through the opaque ABI context.
 */

/* ABI version. */
QE6502_ABI_API uint32_t qe6502abi_version(void);

/* Initialize an ABI context for the selected processor model. */
QE6502_ABI_API void qe6502abi_setup(qe6502abi_context_t *ctx, uint32_t model);

/* Restart the CPU context and return an initial dummy read request at address 0x00ff. */
QE6502_ABI_API qe6502abi_tick_t qe6502abi_restart(qe6502abi_context_t *ctx);

/* Enter execution at address and return the first bus request. */
QE6502_ABI_API qe6502abi_tick_t qe6502abi_goto(qe6502abi_context_t *ctx, uint32_t address);

/* Execute one CPU bus phase and return the next packed bus request. */
QE6502_ABI_API qe6502abi_tick_t qe6502abi_tick(qe6502abi_context_t *ctx, uint32_t bus);

/* Interrupt pin control. */
QE6502_ABI_API void    qe6502abi_nmi_assert(qe6502abi_context_t *ctx, uint8_t assert_nmi);
QE6502_ABI_API void    qe6502abi_irq_assert(qe6502abi_context_t *ctx, uint8_t assert_irq);
QE6502_ABI_API uint8_t qe6502abi_is_nmi_asserted(const qe6502abi_context_t *ctx);
QE6502_ABI_API uint8_t qe6502abi_is_irq_asserted(const qe6502abi_context_t *ctx);

/* Save and restore a portable 64-byte ABI snapshot. */
QE6502_ABI_API void             qe6502abi_save(const qe6502abi_context_t *ctx, qe6502abi_tick_t tick, qe6502abi_snapshot_t snapshot);
QE6502_ABI_API qe6502abi_tick_t qe6502abi_load(qe6502abi_context_t *ctx, const qe6502abi_snapshot_t snapshot);

/* CPU register accessors. */
QE6502_ABI_API uint32_t qe6502abi_get_model(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_model(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_pc(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_pc(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_s(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_s(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_a(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_a(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_x(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_x(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_y(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_y(qe6502abi_context_t *ctx, uint32_t value);
QE6502_ABI_API uint32_t qe6502abi_get_p(const qe6502abi_context_t *ctx);
QE6502_ABI_API void     qe6502abi_set_p(qe6502abi_context_t *ctx, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* QE6502_ABI_H */
