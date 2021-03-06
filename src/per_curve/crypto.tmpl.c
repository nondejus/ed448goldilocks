/**
 * @cond internal
 * @brief Example Decaf crypto routines
 */

#include <decaf/crypto.h>
#include <string.h>

#define API_NAME "$(c_ns)"
#define API_NS(_id) $(c_ns)_##_id
#define API_NS_TOY(_id) $(c_ns)_TOY_##_id
#define SCALAR_BITS $(C_NS)_SCALAR_BITS
#define SCALAR_BYTES ((SCALAR_BITS + 7)/8)
#define SER_BYTES $(C_NS)_SER_BYTES

 /* TODO: canonicalize and freeze the STROBE constants in this file
  * (and STROBE itself for that matter)
  */
static const char *DERIVE_MAGIC = API_NAME"::derive_private_key";
static const char *SIGN_MAGIC = API_NAME"::sign";
static const char *SHARED_SECRET_MAGIC = API_NAME"::shared_secret";
static const uint16_t SHARED_SECRET_MAX_BLOCK_SIZE = 1<<12;
static const unsigned int SCALAR_OVERKILL_BYTES = SCALAR_BYTES + 8;

void API_NS_TOY(derive_private_key) (
    API_NS_TOY(private_key_t) priv,
    const API_NS_TOY(symmetric_key_t) proto
) {
    uint8_t encoded_scalar[SCALAR_OVERKILL_BYTES];
    API_NS(point_t) pub;
    
    keccak_decaf_TOY_strobe_t strobe;
    decaf_TOY_strobe_init(strobe, &STROBE_256, DERIVE_MAGIC, 0);
    decaf_TOY_strobe_fixed_key(strobe, proto, sizeof(API_NS_TOY(symmetric_key_t)));
    decaf_TOY_strobe_prng(strobe, encoded_scalar, sizeof(encoded_scalar));
    decaf_TOY_strobe_destroy(strobe);
    
    memcpy(priv->sym, proto, sizeof(API_NS_TOY(symmetric_key_t)));
    API_NS(scalar_decode_long)(priv->secret_scalar, encoded_scalar, sizeof(encoded_scalar));
    
    API_NS(precomputed_scalarmul)(pub, API_NS(precomputed_base), priv->secret_scalar);
    API_NS(point_encode)(priv->pub, pub);
    
    decaf_bzero(encoded_scalar, sizeof(encoded_scalar));
}

void API_NS_TOY(destroy_private_key) (
    API_NS_TOY(private_key_t) priv
)  {
    decaf_bzero((void*)priv, sizeof(API_NS_TOY(private_key_t)));
}

void API_NS_TOY(private_to_public) (
    API_NS_TOY(public_key_t) pub,
    const API_NS_TOY(private_key_t) priv
) {
    memcpy(pub, priv->pub, sizeof(API_NS_TOY(public_key_t)));
}

/* Performance vs consttime tuning.
 * Specifying true here might give better DOS resistance in certain corner
 * cases.  Specifying false gives a tighter result in test_ct.
 */
#ifndef DECAF_CRYPTO_SHARED_SECRET_SHORT_CIRUIT
#define DECAF_CRYPTO_SHARED_SECRET_SHORT_CIRUIT DECAF_FALSE
#endif

decaf_error_t API_NS_TOY(shared_secret) (
    uint8_t *shared,
    size_t shared_bytes,
    const API_NS_TOY(private_key_t) my_privkey,
    const API_NS_TOY(public_key_t) your_pubkey,
    int me_first
) {
    keccak_decaf_TOY_strobe_t strobe;
    decaf_TOY_strobe_init(strobe, &STROBE_256, SHARED_SECRET_MAGIC, 0);
    
    uint8_t ss_ser[SER_BYTES];
    
    if (me_first) {
        decaf_TOY_strobe_ad(strobe,my_privkey->pub,sizeof(API_NS_TOY(public_key_t)));
        decaf_TOY_strobe_ad(strobe,your_pubkey,sizeof(API_NS_TOY(public_key_t)));
    } else {
        decaf_TOY_strobe_ad(strobe,your_pubkey,sizeof(API_NS_TOY(public_key_t)));
        decaf_TOY_strobe_ad(strobe,my_privkey->pub,sizeof(API_NS_TOY(public_key_t)));
    }
    decaf_error_t ret = API_NS(direct_scalarmul)(
        ss_ser, your_pubkey, my_privkey->secret_scalar, DECAF_FALSE,
        DECAF_CRYPTO_SHARED_SECRET_SHORT_CIRUIT
    );
    
    decaf_TOY_strobe_transact(strobe,NULL,ss_ser,sizeof(ss_ser),STROBE_CW_DH_KEY);
    
    while (shared_bytes) {
        uint16_t cando = (shared_bytes > SHARED_SECRET_MAX_BLOCK_SIZE)
                       ? SHARED_SECRET_MAX_BLOCK_SIZE : shared_bytes;
        decaf_TOY_strobe_prng(strobe,shared,cando);
        shared_bytes -= cando;
        shared += cando;
    }

    decaf_TOY_strobe_destroy(strobe);
    decaf_bzero(ss_ser, sizeof(ss_ser));
    
    return ret;
}

void API_NS_TOY(sign_strobe) (
    keccak_decaf_TOY_strobe_t strobe,
    API_NS_TOY(signature_t) sig,
    const API_NS_TOY(private_key_t) priv
) {
    uint8_t overkill[SCALAR_OVERKILL_BYTES];
    API_NS(point_t) point;
    API_NS(scalar_t) nonce, challenge;
    
    /* Stir pubkey */
    decaf_TOY_strobe_transact(strobe,NULL,priv->pub,sizeof(API_NS_TOY(public_key_t)),STROBE_CW_SIG_PK);
    
    /* Derive nonce */
    keccak_decaf_TOY_strobe_t strobe2;
    memcpy(strobe2,strobe,sizeof(strobe2));
    decaf_TOY_strobe_fixed_key(strobe2,priv->sym,sizeof(API_NS_TOY(symmetric_key_t)));
    decaf_TOY_strobe_prng(strobe2,overkill,sizeof(overkill));
    decaf_TOY_strobe_destroy(strobe2);
    
    API_NS(scalar_decode_long)(nonce, overkill, sizeof(overkill));
    API_NS(precomputed_scalarmul)(point, API_NS(precomputed_base), nonce);
    API_NS(point_encode)(sig, point);
    

    /* Derive challenge */
    decaf_TOY_strobe_transact(strobe,NULL,sig,SER_BYTES,STROBE_CW_SIG_EPH);
    decaf_TOY_strobe_transact(strobe,overkill,NULL,sizeof(overkill),STROBE_CW_SIG_CHAL);
    API_NS(scalar_decode_long)(challenge, overkill, sizeof(overkill));
    
    /* Respond */
    API_NS(scalar_mul)(challenge, challenge, priv->secret_scalar);
    API_NS(scalar_sub)(nonce, nonce, challenge);
    
    /* Save results */
    API_NS(scalar_encode)(overkill, nonce);
    decaf_TOY_strobe_transact(strobe,&sig[SER_BYTES],overkill,SCALAR_BYTES,STROBE_CW_SIG_RESP);
    
    /* Clean up */
    API_NS(scalar_destroy)(nonce);
    API_NS(scalar_destroy)(challenge);
    decaf_bzero(overkill,sizeof(overkill));
}

decaf_error_t API_NS_TOY(verify_strobe) (
    keccak_decaf_TOY_strobe_t strobe,
    const API_NS_TOY(signature_t) sig,
    const API_NS_TOY(public_key_t) pub
) {
    decaf_bool_t ret;
    
    uint8_t overkill[SCALAR_OVERKILL_BYTES];
    API_NS(point_t) point, pubpoint;
    API_NS(scalar_t) challenge, response;
    
    /* Stir pubkey */
    decaf_TOY_strobe_transact(strobe,NULL,pub,sizeof(API_NS_TOY(public_key_t)),STROBE_CW_SIG_PK);
    
    /* Derive nonce */
    decaf_TOY_strobe_transact(strobe,NULL,sig,SER_BYTES,STROBE_CW_SIG_EPH);
    ret = decaf_successful( API_NS(point_decode)(point, sig, DECAF_TRUE) );
    
    /* Derive challenge */
    decaf_TOY_strobe_transact(strobe,overkill,NULL,sizeof(overkill),STROBE_CW_SIG_CHAL);
    API_NS(scalar_decode_long)(challenge, overkill, sizeof(overkill));
    
    /* Decode response */
    decaf_TOY_strobe_transact(strobe,overkill,&sig[SER_BYTES],SCALAR_BYTES,STROBE_CW_SIG_RESP);
    ret &= decaf_successful( API_NS(scalar_decode)(response, overkill) );
    ret &= decaf_successful( API_NS(point_decode)(pubpoint, pub, DECAF_FALSE) );

    API_NS(base_double_scalarmul_non_secret) (
        pubpoint, response, pubpoint, challenge
    );

    ret &= API_NS(point_eq)(pubpoint, point);
    
    /* Nothing here is secret, so don't do these things:
        decaf_bzero(overkill,sizeof(overkill));
        API_NS(point_destroy)(point);
        API_NS(point_destroy)(pubpoint);
        API_NS(scalar_destroy)(challenge);
        API_NS(scalar_destroy)(response);
    */
    
    return decaf_succeed_if(ret);
}

void
API_NS_TOY(sign) (
    API_NS_TOY(signature_t) sig,
    const API_NS_TOY(private_key_t) priv,
    const unsigned char *message,
    size_t message_len
) {
    keccak_decaf_TOY_strobe_t ctx;
    decaf_TOY_strobe_init(ctx,&STROBE_256,SIGN_MAGIC,0);
    decaf_TOY_strobe_transact(ctx, NULL, message, message_len, STROBE_CW_STREAMING_PLAINTEXT);
    API_NS_TOY(sign_strobe)(ctx, sig, priv);
    decaf_TOY_strobe_destroy(ctx);
}

decaf_error_t
API_NS_TOY(verify) (
    const API_NS_TOY(signature_t) sig,
    const API_NS_TOY(public_key_t) pub,
    const unsigned char *message,
    size_t message_len
) {
    keccak_decaf_TOY_strobe_t ctx;
    decaf_TOY_strobe_init(ctx,&STROBE_256,SIGN_MAGIC,0);
    decaf_TOY_strobe_transact(ctx, NULL, message, message_len, STROBE_CW_STREAMING_PLAINTEXT);
    decaf_error_t ret = API_NS_TOY(verify_strobe)(ctx, sig, pub);
    decaf_TOY_strobe_destroy(ctx);
    return ret;
}
