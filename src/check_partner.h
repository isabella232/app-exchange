#ifndef _CHECK_PARTNER_H_
#define _CHECK_PARTNER_H_

#include "swap_app_context.h"
#include "send_function.h"

int check_partner(swap_app_context_t *ctx,                               //
                  unsigned char *input_buffer, int input_buffer_length,  //
                  SendFunction send);

#endif  // _CHECK_PARTNER_H_