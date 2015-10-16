#include "instruction.h"

/**
 * Decodes the instruction pointed to by instruction_ptr.
 * Returns a decoded struct
 */
decoded_t instruction_decode(uint32_t instruction) {
  decoded_t result;

  // Get the value of the instruction from the pointer
  result.fields.instruction = instruction;
  
  // If the instruction is 0, HALT
  if (instruction == 0) {
    result.type = HALT;
    return result;
  }

  if (result.fields.bx.magic == BX_MAGIC) {
    result.type = BX;
    return result;
  }

  switch (result.fields.generic.type_bits) { 
    case 0: // 00
       if (result.fields.mult.magic == MULT_MAGIC) {
         result.type = MULT;
       } else {
         result.type = PROC;
       }
      break;
    case 1: // 01
      result.type = SDT;
      break;
    case 2: // 10
      if (result.fields.generic.bit_25) {
        result.type = BRANCH;
      } else {
        result.type = BDT;
      }
      break;
   default:break;
  }

  return result;
}
