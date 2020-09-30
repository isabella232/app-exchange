#include "check_tx_signature.h"
#include "os.h"
#include "globals.h"
#include "swap_errors.h"
#include "reply_error.h"
#include "der.h"

#define MAX_DER_INT_SIZE(size) (1 + 3 + (size) + 1)

// This function receive transaction signature
// Input should be in the form of DER serialized signature
// the length should be CURVE_SIZE_BYTES * 2 + 6 (DER encoding)
int check_tx_signature(subcommand_e subcommand,                                        //
                       swap_app_context_t *ctx,                                        //
                       unsigned char *input_buffer, unsigned int input_buffer_length,  //
                       SendFunction send) {
    if (subcommand == SWAP) {
        if (input_buffer_length < MIN_DER_SIGNATURE_LENGTH ||
            input_buffer_length > MAX_DER_SIGNATURE_LENGTH ||
            input_buffer[1] + 2 != input_buffer_length) {
            PRINTF("Error: Input buffer length don't correspond to DER length");
            return reply_error(ctx, INCORRECT_COMMAND_DATA, send);
        }
        if (cx_ecdsa_verify(&ctx->partner.public_key, CX_LAST, CX_SHA256, ctx->sha256_digest,
                            CURVE_SIZE_BYTES, input_buffer, input_buffer_length) == 0) {
            PRINTF("Error: Failed to verify signature of received transaction");
            return reply_error(ctx, SIGN_VERIFICATION_FAIL, send);
        }
    }

    if (subcommand == SELL) {
        if (input_buffer_length != 64) {
            PRINTF("Error: Input buffer length don't correspond to (R, S) length");
            return reply_error(ctx, INCORRECT_COMMAND_DATA, send);
        }

        size_t der_r_len = asn1_get_encoded_integer_size(input_buffer, 32);
        size_t der_s_len = asn1_get_encoded_integer_size(input_buffer + 32, 32);
        size_t size = der_r_len + der_s_len + 2;

        unsigned char der_sig[MAX_DER_INT_SIZE(32) * 2 + 2];
        if (size > sizeof(der_sig)) {
            PRINTF("Error: Unexpected der integer size");
            return reply_error(ctx, SIGN_VERIFICATION_FAIL, send);
        }


        encode_sig_der(der_sig, size, input_buffer, 32, input_buffer + 32, 32);

        PRINTF("DER sig: %.*H\n", size, der_sig);
        PRINTF("SHA256(payload): %.*H\n", sizeof(ctx->sha256_digest), ctx->sha256_digest);

        if (cx_ecdsa_verify(&ctx->partner.public_key, CX_LAST, CX_SHA256, ctx->sha256_digest,
                            CURVE_SIZE_BYTES, der_sig, size) == 0) {
            PRINTF("Error: Failed to verify signature of received transaction");
            return reply_error(ctx, SIGN_VERIFICATION_FAIL, send);
        }
    }

    unsigned char output_buffer[2] = {0x90, 0x00};

    if (send(output_buffer, 2) < 0) {
        PRINTF("Error: Can't send message");
        return -1;
    }

    ctx->state = SIGNATURE_CHECKED;

    return 0;
}
