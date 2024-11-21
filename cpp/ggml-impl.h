#pragma once

// GGML internal header

#include "ggml.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h> // load `stdlib.h` before other headers to work around MinGW bug: https://sourceforge.net/p/mingw-w64/bugs/192/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif // __ARM_FEATURE_SVE

#if defined(__ARM_NEON)
// if YCM cannot find <arm_neon.h>, make a symbolic link to it, for example:
//
//   $ ln -sfn /Library/Developer/CommandLineTools/usr/lib/clang/13.1.6/include/arm_neon.h ./src/
//
#include <arm_neon.h>
#endif

#if defined(__F16C__)
#include <immintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#undef MIN
#undef MAX

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// required for mmap as gguf only guarantees 32-byte alignment
#define TENSOR_ALIGNMENT 32

// static_assert should be a #define, but if it's not,
// fall back to the _Static_assert C11 keyword.
// if C99 - static_assert is noop
// ref: https://stackoverflow.com/a/53923785/4039976
#ifndef __cplusplus
    #ifndef static_assert
        #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201100L)
            #define static_assert(cond, msg) _Static_assert(cond, msg)
        #else
            #define static_assert(cond, msg) struct global_scope_noop_trick
        #endif
    #endif
#endif

static inline int wsp_ggml_up32(int n) {
    return (n + 31) & ~31;
}

//static inline int wsp_ggml_up64(int n) {
//    return (n + 63) & ~63;
//}

static inline int wsp_ggml_up(int n, int m) {
    // assert m is a power of 2
    WSP_GGML_ASSERT((m & (m - 1)) == 0);
    return (n + m - 1) & ~(m - 1);
}

//
// logging
//

WSP_GGML_ATTRIBUTE_FORMAT(2, 3)
void wsp_ggml_log_internal        (enum wsp_ggml_log_level level, const char * format, ...);
void wsp_ggml_log_callback_default(enum wsp_ggml_log_level level, const char * text, void * user_data);

#define WSP_GGML_LOG(...)       wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_NONE , __VA_ARGS__)
#define WSP_GGML_LOG_INFO(...)  wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_INFO , __VA_ARGS__)
#define WSP_GGML_LOG_WARN(...)  wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_WARN , __VA_ARGS__)
#define WSP_GGML_LOG_ERROR(...) wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_ERROR, __VA_ARGS__)
#define WSP_GGML_LOG_DEBUG(...) wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define WSP_GGML_LOG_CONT(...)  wsp_ggml_log_internal(WSP_GGML_LOG_LEVEL_CONT , __VA_ARGS__)

#define WSP_GGML_DEBUG 0

#if (WSP_GGML_DEBUG >= 1)
#define WSP_GGML_PRINT_DEBUG(...) WSP_GGML_LOG_DEBUG(__VA_ARGS__)
#else
#define WSP_GGML_PRINT_DEBUG(...)
#endif

#if (WSP_GGML_DEBUG >= 5)
#define WSP_GGML_PRINT_DEBUG_5(...) WSP_GGML_LOG_DEBUG(__VA_ARGS__)
#else
#define WSP_GGML_PRINT_DEBUG_5(...)
#endif

#if (WSP_GGML_DEBUG >= 10)
#define WSP_GGML_PRINT_DEBUG_10(...) WSP_GGML_LOG_DEBUG(__VA_ARGS__)
#else
#define WSP_GGML_PRINT_DEBUG_10(...)
#endif

// tensor params

static void wsp_ggml_set_op_params(struct wsp_ggml_tensor * tensor, const void * params, size_t params_size) {
    WSP_GGML_ASSERT(tensor != NULL); // silence -Warray-bounds warnings
    assert(params_size <= WSP_GGML_MAX_OP_PARAMS);
    memcpy(tensor->op_params, params, params_size);
}

static int32_t wsp_ggml_get_op_params_i32(const struct wsp_ggml_tensor * tensor, uint32_t i) {
    assert(i < WSP_GGML_MAX_OP_PARAMS / sizeof(int32_t));
    return ((const int32_t *)(tensor->op_params))[i];
}

static float wsp_ggml_get_op_params_f32(const struct wsp_ggml_tensor * tensor, uint32_t i) {
    assert(i < WSP_GGML_MAX_OP_PARAMS / sizeof(float));
    return ((const float *)(tensor->op_params))[i];
}

static void wsp_ggml_set_op_params_i32(struct wsp_ggml_tensor * tensor, uint32_t i, int32_t value) {
    assert(i < WSP_GGML_MAX_OP_PARAMS / sizeof(int32_t));
    ((int32_t *)(tensor->op_params))[i] = value;
}

static void wsp_ggml_set_op_params_f32(struct wsp_ggml_tensor * tensor, uint32_t i, float value) {
    assert(i < WSP_GGML_MAX_OP_PARAMS / sizeof(float));
    ((float *)(tensor->op_params))[i] = value;
}

struct wsp_ggml_map_custom1_op_params {
    wsp_ggml_custom1_op_t  fun;
    int                n_tasks;
    void             * userdata;
};

struct wsp_ggml_map_custom2_op_params {
    wsp_ggml_custom2_op_t   fun;
    int                 n_tasks;
    void              * userdata;
};

struct wsp_ggml_map_custom3_op_params {
    wsp_ggml_custom3_op_t fun;
    int n_tasks;
    void * userdata;
};

// bitset

typedef uint32_t wsp_ggml_bitset_t;

static_assert(sizeof(wsp_ggml_bitset_t) == 4, "bitset_t constants must be updated");
#define BITSET_SHR 5 // log2(sizeof(wsp_ggml_bitset_t)*8)
#define BITSET_MASK (sizeof(wsp_ggml_bitset_t)*8 - 1)

static size_t wsp_ggml_bitset_size(size_t n) {
    return (n + BITSET_MASK) >> BITSET_SHR;
}

static inline bool wsp_ggml_bitset_get(const wsp_ggml_bitset_t * bitset, size_t i) {
    return !!(bitset[i >> BITSET_SHR] & (1u << (i & BITSET_MASK)));
}

static inline void wsp_ggml_bitset_set(wsp_ggml_bitset_t * bitset, size_t i) {
    bitset[i >> BITSET_SHR] |= (1u << (i & BITSET_MASK));
}

static inline void wsp_ggml_bitset_clear(wsp_ggml_bitset_t * bitset, size_t i) {
    bitset[i >> BITSET_SHR] &= ~(1u << (i & BITSET_MASK));
}

// hash set

#define WSP_GGML_HASHSET_FULL ((size_t)-1)
#define WSP_GGML_HASHSET_ALREADY_EXISTS ((size_t)-2)

struct wsp_ggml_hash_set {
    size_t size;
    wsp_ggml_bitset_t * used;       // whether or not the keys are in use i.e. set
    struct wsp_ggml_tensor ** keys; // actual tensors in the set, keys[i] is only defined if wsp_ggml_bitset_get(used, i)
};

struct wsp_ggml_hash_set wsp_ggml_hash_set_new(size_t size);
void                 wsp_ggml_hash_set_free(struct wsp_ggml_hash_set * hash_set);

// returns the minimum size for a hash set that can hold min_sz elements
size_t wsp_ggml_hash_size(size_t min_sz);

// remove all elements from the hash set
void wsp_ggml_hash_set_reset(struct wsp_ggml_hash_set * hash_set);

// returns true if key is in the hash set
static bool wsp_ggml_hash_contains(const struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key);

// returns WSP_GGML_HASHSET_FULL if table is full, otherwise the current index of the key or where it should be inserted
static size_t wsp_ggml_hash_find(const struct wsp_ggml_hash_set * hash_set, const struct wsp_ggml_tensor * key);

// returns WSP_GGML_HASHSET_ALREADY_EXISTS if key already exists, index otherwise, asserts if table is full
static size_t wsp_ggml_hash_insert(struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key);

// return index, asserts if table is full
static size_t wsp_ggml_hash_find_or_insert(struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key);

// hash function for wsp_ggml_tensor
static inline size_t wsp_ggml_hash(const struct wsp_ggml_tensor * p) {
    // the last 4 bits are always zero due to alignment
    return (size_t)(uintptr_t)p >> 4;
}

static size_t wsp_ggml_hash_find(const struct wsp_ggml_hash_set * hash_set, const struct wsp_ggml_tensor * key) {
    size_t h = wsp_ggml_hash(key) % hash_set->size;

    // linear probing
    size_t i = h;
    while (wsp_ggml_bitset_get(hash_set->used, i) && hash_set->keys[i] != key) {
        i = (i + 1) % hash_set->size;
        if (i == h) {
            // visited all hash table entries -> not found
            return WSP_GGML_HASHSET_FULL;
        }
    }
    return i;
}

static bool wsp_ggml_hash_contains(const struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key) {
    size_t i = wsp_ggml_hash_find(hash_set, key);
    return i != WSP_GGML_HASHSET_FULL && wsp_ggml_bitset_get(hash_set->used, i);
}

static size_t wsp_ggml_hash_insert(struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key) {
    size_t h = wsp_ggml_hash(key) % hash_set->size;

    // linear probing
    size_t i = h;
    do {
        if (!wsp_ggml_bitset_get(hash_set->used, i)) {
            wsp_ggml_bitset_set(hash_set->used, i);
            hash_set->keys[i] = key;
            return i;
        }
        if (hash_set->keys[i] == key) {
            return WSP_GGML_HASHSET_ALREADY_EXISTS;
        }
        i = (i + 1) % hash_set->size;
    } while (i != h);

    // visited all hash table entries -> not found
    WSP_GGML_ABORT("fatal error");
}

static size_t wsp_ggml_hash_find_or_insert(struct wsp_ggml_hash_set * hash_set, struct wsp_ggml_tensor * key) {
    size_t h = wsp_ggml_hash(key) % hash_set->size;

    // linear probing
    size_t i = h;
    do {
        if (!wsp_ggml_bitset_get(hash_set->used, i)) {
            wsp_ggml_bitset_set(hash_set->used, i);
            hash_set->keys[i] = key;
            return i;
        }
        if (hash_set->keys[i] == key) {
            return i;
        }
        i = (i + 1) % hash_set->size;
    } while (i != h);

    // visited all hash table entries -> not found
    WSP_GGML_ABORT("fatal error");
}

// computation graph

enum wsp_ggml_cgraph_eval_order {
    WSP_GGML_CGRAPH_EVAL_ORDER_LEFT_TO_RIGHT = 0,
    WSP_GGML_CGRAPH_EVAL_ORDER_RIGHT_TO_LEFT,
    WSP_GGML_CGRAPH_EVAL_ORDER_COUNT
};

struct wsp_ggml_cgraph {
    int size;    // maximum number of nodes/leafs/grads/grad_accs
    int n_nodes; // number of nodes currently in use
    int n_leafs; // number of leafs currently in use

    struct wsp_ggml_tensor ** nodes;     // tensors with data that can change if the graph is evaluated
    struct wsp_ggml_tensor ** grads;     // the outputs of these tensors are the gradients of the nodes
    struct wsp_ggml_tensor ** grad_accs; // accumulators for node gradients
    struct wsp_ggml_tensor ** leafs;     // tensors with constant data

    struct wsp_ggml_hash_set visited_hash_set;

    enum wsp_ggml_cgraph_eval_order order;
};

struct wsp_ggml_cgraph wsp_ggml_graph_view(struct wsp_ggml_cgraph * cgraph, int i0, int i1);

// Memory allocation

void * wsp_ggml_aligned_malloc(size_t size);
void wsp_ggml_aligned_free(void * ptr, size_t size);

// FP16 to FP32 conversion

#if defined(__ARM_NEON)
    #ifdef _MSC_VER
        typedef uint16_t wsp_ggml_fp16_internal_t;
    #else
        typedef __fp16 wsp_ggml_fp16_internal_t;
    #endif
#endif

#if defined(__ARM_NEON) && !defined(_MSC_VER)
    #define WSP_GGML_COMPUTE_FP16_TO_FP32(x) wsp_ggml_compute_fp16_to_fp32(x)
    #define WSP_GGML_COMPUTE_FP32_TO_FP16(x) wsp_ggml_compute_fp32_to_fp16(x)

    #define WSP_GGML_FP16_TO_FP32(x) wsp_ggml_compute_fp16_to_fp32(x)

    static inline float wsp_ggml_compute_fp16_to_fp32(wsp_ggml_fp16_t h) {
        wsp_ggml_fp16_internal_t tmp;
        memcpy(&tmp, &h, sizeof(wsp_ggml_fp16_t));
        return (float)tmp;
    }

    static inline wsp_ggml_fp16_t wsp_ggml_compute_fp32_to_fp16(float f) {
        wsp_ggml_fp16_t res;
        wsp_ggml_fp16_internal_t tmp = f;
        memcpy(&res, &tmp, sizeof(wsp_ggml_fp16_t));
        return res;
    }

#elif defined(__F16C__)

    #ifdef _MSC_VER
        #define WSP_GGML_COMPUTE_FP16_TO_FP32(x) _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128(x)))
        #define WSP_GGML_COMPUTE_FP32_TO_FP16(x) _mm_extract_epi16(_mm_cvtps_ph(_mm_set_ss(x), 0), 0)
    #else
        #define WSP_GGML_COMPUTE_FP16_TO_FP32(x) _cvtsh_ss(x)
        #define WSP_GGML_COMPUTE_FP32_TO_FP16(x) _cvtss_sh(x, 0)
    #endif

#elif defined(__POWER9_VECTOR__)

    #define WSP_GGML_COMPUTE_FP16_TO_FP32(x) wsp_ggml_compute_fp16_to_fp32(x)
    #define WSP_GGML_COMPUTE_FP32_TO_FP16(x) wsp_ggml_compute_fp32_to_fp16(x)
    /* the inline asm below is about 12% faster than the lookup method */
    #define WSP_GGML_FP16_TO_FP32(x) WSP_GGML_COMPUTE_FP16_TO_FP32(x)
    #define WSP_GGML_FP32_TO_FP16(x) WSP_GGML_COMPUTE_FP32_TO_FP16(x)

    static inline float wsp_ggml_compute_fp16_to_fp32(wsp_ggml_fp16_t h) {
        register float f;
        register double d;
        __asm__(
            "mtfprd %0,%2\n"
            "xscvhpdp %0,%0\n"
            "frsp %1,%0\n" :
            /* temp */ "=d"(d),
            /* out */  "=f"(f):
            /* in */   "r"(h));
        return f;
    }

    static inline wsp_ggml_fp16_t wsp_ggml_compute_fp32_to_fp16(float f) {
        register double d;
        register wsp_ggml_fp16_t r;
        __asm__( /* xscvdphp can work on double or single precision */
            "xscvdphp %0,%2\n"
            "mffprd %1,%0\n" :
            /* temp */ "=d"(d),
            /* out */  "=r"(r):
            /* in */   "f"(f));
        return r;
    }

#else

    // FP16 <-> FP32
    // ref: https://github.com/Maratyszcza/FP16

    static inline float fp32_from_bits(uint32_t w) {
        union {
            uint32_t as_bits;
            float as_value;
        } fp32;
        fp32.as_bits = w;
        return fp32.as_value;
    }

    static inline uint32_t fp32_to_bits(float f) {
        union {
            float as_value;
            uint32_t as_bits;
        } fp32;
        fp32.as_value = f;
        return fp32.as_bits;
    }

    static inline float wsp_ggml_compute_fp16_to_fp32(wsp_ggml_fp16_t h) {
        const uint32_t w = (uint32_t) h << 16;
        const uint32_t sign = w & UINT32_C(0x80000000);
        const uint32_t two_w = w + w;

        const uint32_t exp_offset = UINT32_C(0xE0) << 23;
    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) || defined(__GNUC__) && !defined(__STRICT_ANSI__)) && (!defined(__cplusplus) || __cplusplus >= 201703L)
        const float exp_scale = 0x1.0p-112f;
    #else
        const float exp_scale = fp32_from_bits(UINT32_C(0x7800000));
    #endif
        const float normalized_value = fp32_from_bits((two_w >> 4) + exp_offset) * exp_scale;

        const uint32_t magic_mask = UINT32_C(126) << 23;
        const float magic_bias = 0.5f;
        const float denormalized_value = fp32_from_bits((two_w >> 17) | magic_mask) - magic_bias;

        const uint32_t denormalized_cutoff = UINT32_C(1) << 27;
        const uint32_t result = sign |
            (two_w < denormalized_cutoff ? fp32_to_bits(denormalized_value) : fp32_to_bits(normalized_value));
        return fp32_from_bits(result);
    }

    static inline wsp_ggml_fp16_t wsp_ggml_compute_fp32_to_fp16(float f) {
    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) || defined(__GNUC__) && !defined(__STRICT_ANSI__)) && (!defined(__cplusplus) || __cplusplus >= 201703L)
        const float scale_to_inf = 0x1.0p+112f;
        const float scale_to_zero = 0x1.0p-110f;
    #else
        const float scale_to_inf = fp32_from_bits(UINT32_C(0x77800000));
        const float scale_to_zero = fp32_from_bits(UINT32_C(0x08800000));
    #endif
        float base = (fabsf(f) * scale_to_inf) * scale_to_zero;

        const uint32_t w = fp32_to_bits(f);
        const uint32_t shl1_w = w + w;
        const uint32_t sign = w & UINT32_C(0x80000000);
        uint32_t bias = shl1_w & UINT32_C(0xFF000000);
        if (bias < UINT32_C(0x71000000)) {
            bias = UINT32_C(0x71000000);
        }

        base = fp32_from_bits((bias >> 1) + UINT32_C(0x07800000)) + base;
        const uint32_t bits = fp32_to_bits(base);
        const uint32_t exp_bits = (bits >> 13) & UINT32_C(0x00007C00);
        const uint32_t mantissa_bits = bits & UINT32_C(0x00000FFF);
        const uint32_t nonsign = exp_bits + mantissa_bits;
        return (sign >> 16) | (shl1_w > UINT32_C(0xFF000000) ? UINT16_C(0x7E00) : nonsign);
    }

    #define WSP_GGML_COMPUTE_FP16_TO_FP32(x) wsp_ggml_compute_fp16_to_fp32(x)
    #define WSP_GGML_COMPUTE_FP32_TO_FP16(x) wsp_ggml_compute_fp32_to_fp16(x)

#endif // defined(__ARM_NEON) && (!defined(__MSC_VER)

// precomputed f32 table for f16 (256 KB)
// defined in ggml.c, initialized in wsp_ggml_init()
WSP_GGML_API float wsp_ggml_table_f32_f16[1 << 16];

// On ARM NEON, it's quicker to directly convert x -> x instead of calling into wsp_ggml_lookup_fp16_to_fp32,
// so we define WSP_GGML_FP16_TO_FP32 and WSP_GGML_FP32_TO_FP16 elsewhere for NEON.
// This is also true for POWER9.
#if !defined(WSP_GGML_FP16_TO_FP32)
inline static float wsp_ggml_lookup_fp16_to_fp32(wsp_ggml_fp16_t f) {
    uint16_t s;
    memcpy(&s, &f, sizeof(uint16_t));
    return wsp_ggml_table_f32_f16[s];
}

#define WSP_GGML_FP16_TO_FP32(x) wsp_ggml_lookup_fp16_to_fp32(x)
#endif

#if !defined(WSP_GGML_FP32_TO_FP16)
#define WSP_GGML_FP32_TO_FP16(x) WSP_GGML_COMPUTE_FP32_TO_FP16(x)
#endif

/**
 * Converts brain16 to float32.
 *
 * The bfloat16 floating point format has the following structure:
 *
 *       ┌sign
 *       │
 *       │   ┌exponent
 *       │   │
 *       │   │      ┌mantissa
 *       │   │      │
 *       │┌──┴───┐┌─┴───┐
 *     0b0000000000000000 brain16
 *
 * Since bf16 has the same number of exponent bits as a 32bit float,
 * encoding and decoding numbers becomes relatively straightforward.
 *
 *       ┌sign
 *       │
 *       │   ┌exponent
 *       │   │
 *       │   │      ┌mantissa
 *       │   │      │
 *       │┌──┴───┐┌─┴───────────────────┐
 *     0b00000000000000000000000000000000 IEEE binary32
 *
 * For comparison, the standard fp16 format has fewer exponent bits.
 *
 *       ┌sign
 *       │
 *       │  ┌exponent
 *       │  │
 *       │  │    ┌mantissa
 *       │  │    │
 *       │┌─┴─┐┌─┴──────┐
 *     0b0000000000000000 IEEE binary16
 *
 * @see IEEE 754-2008
 */
static inline float wsp_ggml_compute_bf16_to_fp32(wsp_ggml_bf16_t h) {
    union {
        float f;
        uint32_t i;
    } u;
    u.i = (uint32_t)h.bits << 16;
    return u.f;
}

/**
 * Converts float32 to brain16.
 *
 * This is binary identical with Google Brain float conversion.
 * Floats shall round to nearest even, and NANs shall be quiet.
 * Subnormals aren't flushed to zero, except perhaps when used.
 * This code should vectorize nicely if using modern compilers.
 */
static inline wsp_ggml_bf16_t wsp_ggml_compute_fp32_to_bf16(float s) {
    wsp_ggml_bf16_t h;
    union {
        float f;
        uint32_t i;
    } u;
    u.f = s;
    if ((u.i & 0x7fffffff) > 0x7f800000) { /* nan */
        h.bits = (u.i >> 16) | 64; /* force to quiet */
        return h;
    }
    h.bits = (u.i + (0x7fff + ((u.i >> 16) & 1))) >> 16;
    return h;
}

#define WSP_GGML_FP32_TO_BF16(x) wsp_ggml_compute_fp32_to_bf16(x)
#define WSP_GGML_BF16_TO_FP32(x) wsp_ggml_compute_bf16_to_fp32(x)

#ifdef __cplusplus
}
#endif
