#include <qe6502/qe6502_abi.h>

#include <stdint.h>
#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#   define QE6502JS_API __attribute__((visibility("default")))
#else
#   define QE6502JS_API
#endif

#define QE6502JS_CONTEXT_POOL_CAPACITY 256u

QE6502_ABI_STATIC_ASSERT(QE6502JS_CONTEXT_POOL_CAPACITY > 0u,
                         "qe6502js context pool must not be empty");

typedef uint8_t qe6502js_bool_t;

static qe6502abi_context_t qe6502js_context_pool[QE6502JS_CONTEXT_POOL_CAPACITY];
static qe6502abi_context_t *qe6502js_free_contexts[QE6502JS_CONTEXT_POOL_CAPACITY];
static qe6502js_bool_t qe6502js_is_context_free[QE6502JS_CONTEXT_POOL_CAPACITY];
static uint32_t qe6502js_free_context_count = 0u;
static qe6502js_bool_t qe6502js_context_pool_ready = 0u;

QE6502JS_API void qe6502js_context_pool_reset(void);
QE6502JS_API qe6502abi_context_t *qe6502js_context_alloc(void);
QE6502JS_API void qe6502js_context_free(qe6502abi_context_t *ctx);

QE6502JS_API void qe6502js_context_pool_reset(void)
{
    uint32_t i;

    for(i = 0u; i < QE6502JS_CONTEXT_POOL_CAPACITY; ++i) {
        qe6502js_free_contexts[i] = &qe6502js_context_pool[i];
        qe6502js_is_context_free[i] = 1u;
    }

    qe6502js_free_context_count = QE6502JS_CONTEXT_POOL_CAPACITY;
    qe6502js_context_pool_ready = 1u;
}

QE6502JS_API qe6502abi_context_t *qe6502js_context_alloc(void)
{
    qe6502abi_context_t *ctx;
    ptrdiff_t pool_index;

    if(qe6502js_context_pool_ready == 0u) {
        qe6502js_context_pool_reset();
    }

    if(qe6502js_free_context_count == 0u) {
        return (qe6502abi_context_t *)0;
    }

    --qe6502js_free_context_count;
    ctx = qe6502js_free_contexts[qe6502js_free_context_count];
    pool_index = ctx - qe6502js_context_pool;
    qe6502js_is_context_free[(uint32_t)pool_index] = 0u;

    return ctx;
}

QE6502JS_API void qe6502js_context_free(qe6502abi_context_t *ctx)
{
    uintptr_t ptr_value;
    uintptr_t pool_first;
    uintptr_t pool_end;
    ptrdiff_t pool_index;

    if(qe6502js_context_pool_ready == 0u) {
        return;
    }

    if(ctx == (qe6502abi_context_t *)0) {
        return;
    }

    ptr_value = (uintptr_t)(void *)ctx;
    pool_first = (uintptr_t)(void *)qe6502js_context_pool;
    pool_end = (uintptr_t)(void *)(&qe6502js_context_pool[QE6502JS_CONTEXT_POOL_CAPACITY]);

    if((ptr_value < pool_first) || (ptr_value >= pool_end)) {
        return;
    }

    if(((ptr_value - pool_first) % sizeof(qe6502abi_context_t)) != 0u) {
        return;
    }

    pool_index = ctx - qe6502js_context_pool;
    if(qe6502js_is_context_free[(uint32_t)pool_index] != 0u) {
        return;
    }

    if(qe6502js_free_context_count >= QE6502JS_CONTEXT_POOL_CAPACITY) {
        return;
    }

    qe6502js_free_contexts[qe6502js_free_context_count] = ctx;
    qe6502js_is_context_free[(uint32_t)pool_index] = 1u;
    ++qe6502js_free_context_count;
}
