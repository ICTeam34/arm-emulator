#ifndef UTILS_HEADER
#define UTILS_HEADER

#include "common.h"

uint32_t get_bits(uint32_t, uint8_t, uint8_t);
uint32_t set_bits(uint32_t, uint8_t, uint8_t, uint32_t);
bool     get_bit(uint32_t, uint8_t);

void     get_reg_list(uint32_t*, const uint32_t);
int      count_set_bits(uint32_t);

#endif
