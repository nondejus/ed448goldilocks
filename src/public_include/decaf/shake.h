/**
 * @file decaf/shake.h
 * @copyright
 *   Based on CC0 code by David Leon Gil, 2015 \n
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief SHA-3-n and DECAF_SHAKE-n instances.
 */

#ifndef __DECAF_SHAKE_H__
#define __DECAF_SHAKE_H__

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h> /* for NULL */

#include <decaf/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INTERNAL_SPONGE_STRUCT
    /** Sponge container object for the various primitives. */
    typedef struct decaf_keccak_sponge_s {
        /** @cond internal */
        uint64_t opaque[26];
        /** @endcond */
    } decaf_keccak_sponge_s;

    /** Convenience GMP-style one-element array version */
    typedef struct decaf_keccak_sponge_s decaf_keccak_sponge_t[1];

    /** Parameters for sponge construction, distinguishing DECAF_SHA3 and
     * DECAF_SHAKE instances.
     */
    struct decaf_kparams_s;
#endif

/**
 * @brief Initialize a sponge context object.
 * @param [out] sponge The object to initialize.
 * @param [in] params The sponge's parameter description.
 */
void decaf_sponge_init (
    decaf_keccak_sponge_t sponge,
    const struct decaf_kparams_s *params
) API_VIS;

/**
 * @brief Absorb data into a DECAF_SHA3 or DECAF_SHAKE hash context.
 * @param [inout] sponge The context.
 * @param [in] in The input data.
 * @param [in] len The input data's length in bytes.
 */
void decaf_sha3_update (
    struct decaf_keccak_sponge_s * __restrict__ sponge,
    const uint8_t *in,
    size_t len
) API_VIS;

/**
 * @brief Squeeze output data from a DECAF_SHA3 or DECAF_SHAKE hash context.
 * This does not destroy or re-initialize the hash context, and
 * decaf_sha3 output can be called more times.
 *
 * @param [inout] sponge The context.
 * @param [out] out The output data.
 * @param [in] len The requested output data length in bytes.
 * @return DECAF_FAILURE if the sponge has exhausted its output capacity.
 * @return DECAF_SUCCESS otherwise.
 */  
decaf_error_t decaf_sha3_output (
    decaf_keccak_sponge_t sponge,
    uint8_t * __restrict__ out,
    size_t len
) API_VIS;

/**
 * @brief Squeeze output data from a DECAF_SHA3 or DECAF_SHAKE hash context.
 * This re-initializes the context to its starting parameters.
 *
 * @param [inout] sponge The context.
 * @param [out] out The output data.
 * @param [in] len The requested output data length in bytes.
 */  
decaf_error_t decaf_sha3_final (
    decaf_keccak_sponge_t sponge,
    uint8_t * __restrict__ out,
    size_t len
) API_VIS;

/**
 * @brief Reset the sponge to the empty string.
 *
 * @param [inout] sponge The context.
 */  
void decaf_sha3_reset (
    decaf_keccak_sponge_t sponge
) API_VIS;

/**
 * @brief Return the default output length of the sponge construction,
 * for the purpose of C++ default operators.
 *
 * Returns n/8 for DECAF_SHA3-n and 2n/8 for DECAF_SHAKE-n.
 */  
size_t decaf_sponge_default_output_bytes (
    const decaf_keccak_sponge_t sponge /**< [inout] The context. */
) API_VIS;

/**
 * @brief Return the default output length of the sponge construction,
 * for the purpose of C++ default operators.
 *
 * Returns n/8 for DECAF_SHA3-n and SIZE_MAX for DECAF_SHAKE-n.
 */  
size_t decaf_sponge_max_output_bytes (
    const decaf_keccak_sponge_t sponge /**< [inout] The context. */
) API_VIS;

/**
 * @brief Destroy a DECAF_SHA3 or DECAF_SHAKE sponge context by overwriting it with 0.
 * @param [out] sponge The context.
 */  
void decaf_sponge_destroy (
    decaf_keccak_sponge_t sponge
) API_VIS;

/**
 * @brief Hash (in) to (out)
 * @param [in] in The input data.
 * @param [in] inlen The length of the input data.
 * @param [out] out A buffer for the output data.
 * @param [in] outlen The length of the output data.
 * @param [in] params The parameters of the sponge hash.
 */  
void decaf_sponge_hash (
    const uint8_t *in,
    size_t inlen,
    uint8_t *out,
    size_t outlen,
    const struct decaf_kparams_s *params
) API_VIS;

/* FUTURE: expand/doxygenate individual DECAF_SHAKE/DECAF_SHA3 instances? */

/** @cond internal */
#define DEC_SHAKE(n) \
    extern const struct decaf_kparams_s DECAF_SHAKE##n##_params_s API_VIS; \
    typedef struct decaf_shake##n##_ctx_s { decaf_keccak_sponge_t s; } decaf_shake##n##_ctx_t[1]; \
    static inline void NONNULL decaf_shake##n##_init(decaf_shake##n##_ctx_t sponge) { \
        decaf_sponge_init(sponge->s, &DECAF_SHAKE##n##_params_s); \
    } \
    static inline void NONNULL decaf_shake##n##_gen_init(decaf_keccak_sponge_t sponge) { \
        decaf_sponge_init(sponge, &DECAF_SHAKE##n##_params_s); \
    } \
    static inline void NONNULL decaf_shake##n##_update(decaf_shake##n##_ctx_t sponge, const uint8_t *in, size_t inlen ) { \
        decaf_sha3_update(sponge->s, in, inlen); \
    } \
    static inline void  NONNULL decaf_shake##n##_final(decaf_shake##n##_ctx_t sponge, uint8_t *out, size_t outlen ) { \
        decaf_sha3_output(sponge->s, out, outlen); \
        decaf_sponge_init(sponge->s, &DECAF_SHAKE##n##_params_s); \
    } \
    static inline void  NONNULL decaf_shake##n##_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { \
        decaf_sponge_hash(in,inlen,out,outlen,&DECAF_SHAKE##n##_params_s); \
    } \
    static inline void  NONNULL decaf_shake##n##_destroy( decaf_shake##n##_ctx_t sponge ) { \
        decaf_sponge_destroy(sponge->s); \
    }

#define DEC_SHA3(n) \
    extern const struct decaf_kparams_s DECAF_SHA3_##n##_params_s API_VIS; \
    typedef struct decaf_sha3_##n##_ctx_s { decaf_keccak_sponge_t s; } decaf_sha3_##n##_ctx_t[1]; \
    static inline void NONNULL decaf_sha3_##n##_init(decaf_sha3_##n##_ctx_t sponge) { \
        decaf_sponge_init(sponge->s, &DECAF_SHA3_##n##_params_s); \
    } \
    static inline void NONNULL decaf_sha3_##n##_gen_init(decaf_keccak_sponge_t sponge) { \
        decaf_sponge_init(sponge, &DECAF_SHA3_##n##_params_s); \
    } \
    static inline void NONNULL decaf_sha3_##n##_update(decaf_sha3_##n##_ctx_t sponge, const uint8_t *in, size_t inlen ) { \
        decaf_sha3_update(sponge->s, in, inlen); \
    } \
    static inline void NONNULL decaf_sha3_##n##_final(decaf_sha3_##n##_ctx_t sponge, uint8_t *out, size_t outlen ) { \
        decaf_sha3_output(sponge->s, out, outlen); \
        decaf_sponge_init(sponge->s, &DECAF_SHA3_##n##_params_s); \
    } \
    static inline void NONNULL decaf_sha3_##n##_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) { \
        decaf_sponge_hash(in,inlen,out,outlen,&DECAF_SHA3_##n##_params_s); \
    } \
    static inline void NONNULL decaf_sha3_##n##_destroy(decaf_sha3_##n##_ctx_t sponge) { \
        decaf_sponge_destroy(sponge->s); \
    }
/** @endcond */

DEC_SHAKE(128)
DEC_SHAKE(256)
DEC_SHA3(224)
DEC_SHA3(256)
DEC_SHA3(384)
DEC_SHA3(512)
#undef DEC_SHAKE
#undef DEC_SHA3

#ifdef __cplusplus
} /* extern "C" */
#endif
    
#endif /* __DECAF_SHAKE_H__ */
