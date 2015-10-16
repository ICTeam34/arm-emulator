#include "utils.h"

/**
 * Get the bits of the number between the specified start and end positions
 */
uint32_t get_bits(uint32_t number, uint8_t start, uint8_t end) {
  assert(start >= 0);
  assert(32 >= end);

  //This behaviour is exploited by the shift overflow
  if (start > end) {
    return 0; 
  }

  uint32_t mask = (uint32_t) (~0);
  mask >>= 31 + start - end;
  number >>= start;

  return number & mask;
}

/**
 * Get a single bit of a number
 */
inline bool get_bit(uint32_t number, uint8_t bit) {
  return (bool) (number & (1 << bit));
}

/**
 * Set the the bits in number from start to end to those of n.
 * The first |end - start| bits of n are used.
 */
uint32_t set_bits(uint32_t number, uint8_t start, uint8_t end, uint32_t n) {
  assert(start >= 0);
  assert(end >= start);

  uint32_t mask = 0;
  mask = ~mask;

  mask = get_bits(mask, start, end) << start;
  uint32_t number_masked = ~(~number | mask);
  n <<= start;
  n = n & mask;
  return n | number_masked;
}


int count_set_bits(uint32_t number){
 int size;
 for (size = 0; number; size++) {
  number &= number - 1; 
 }
 return size;
}
//Return an array of register numbers specified by reg_bits
//store in the ascending order of reg_num
//TODO: can it be reduced to one loop only?(get the size immediately)
void get_reg_list(uint32_t* reg_list, const uint32_t reg_bits) {
  int index = 0;
  for (int i = 0; i < 16; ++i) {
    if (get_bit(reg_bits, i)) {
      reg_list[index] = i;
      ++index; 
    }
  }
}

