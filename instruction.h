#ifndef HEADER_OPCODE
#define HEADER_OPCODE

#include "common.h"
#include "memory.h"
#include "utils.h"

#define OP_AND 0x0
#define OP_EOR 0x1
#define OP_SUB 0x2
#define OP_RSB 0x3
#define OP_ADD 0x4
#define OP_TST 0x8
#define OP_TEQ 0x9
#define OP_CMP 0xA
#define OP_ORR 0xC
#define OP_MOV 0xD

#define MULT_MAGIC 0x9
#define BX_MAGIC   0x12FFF1

/**
 * Enum type describing the instruction types
 */
typedef enum {
  PROC,        // Data Processing
  MULT,        // Multiply
  SDT,         // Single Data Transfer
  BRANCH,      // Branch
  BX,          // Branch and exchange
  BDT,         // Block data transfer
  HALT,        // Special state, halting the system
  EMPTY        // No instruction on the next iteration
} inst_t;

// WARNING: ALWAYS SUBJECT TO CHANGE
typedef struct {
  uint32_t           : 4;
  uint32_t bits4_7   : 4;
  uint32_t           : 17;
  uint32_t bit_25    : 1;                     
  uint32_t type_bits : 2;
  uint32_t cond      : 4; 
} inst_generic_t;

typedef struct {
  uint32_t r_m   : 4;
  uint32_t magic : 4;
  uint32_t r_s   : 4;
  uint32_t r_n   : 4;
  uint32_t r_d   : 4;
  uint32_t s     : 1;
  uint32_t a     : 1;
  uint32_t       : 6;
  uint32_t cond  : 4;
} inst_mult_t;

typedef struct {
  uint32_t op2    : 12;
  uint32_t r_d    : 4;
  uint32_t r_n    : 4;
  uint32_t s      : 1;
  uint32_t opcode : 4;
  uint32_t i      : 1;
  uint32_t        : 2;
  uint32_t cond   : 4;
} inst_data_proc_t;

typedef struct {
  uint32_t offset : 24;
  uint32_t l      : 1;
  uint32_t magic  : 3;
  uint32_t cond   : 4;
} inst_branch_t;

typedef struct {
  uint32_t r_n    : 4;
  uint32_t magic  : 24;
  uint32_t cond   : 4;
} inst_bx_t;

typedef struct {
  uint32_t offset : 12;
  uint32_t r_d     : 4;
  uint32_t r_n     : 4;
  uint32_t l      : 1;
  uint32_t w      : 1;
  uint32_t b      : 1;
  uint32_t u      : 1;
  uint32_t p      : 1;
  uint32_t i      : 1;
  uint32_t magic  : 2;
  uint32_t cond   : 4;
} inst_sdt_t;

//TODO: replace the one in emulate.c
typedef struct {
  uint32_t reg_bits :  16;
  uint32_t r_n   :      4;
  uint32_t l     :      1;
  uint32_t w     :      1;
  uint32_t s     :      1;
  uint32_t p_u   :      2;
  uint32_t magic :      3;
  uint32_t cond  :      4;
} inst_bdt_t;

typedef struct {
  uint32_t i : 32;
} inst_halt_t;

/**
 * Struct holding the decoded instruction's data
 */
typedef struct {
  inst_t         type; 
  union {
    uint32_t          instruction;
    inst_data_proc_t  data_proc;
    inst_branch_t     branch;
    inst_bx_t         bx;
    inst_sdt_t        sdt;
    inst_bdt_t        bdt;
    inst_mult_t       mult;
    inst_halt_t       halt;
    inst_generic_t    generic;
  } fields;
} decoded_t;

decoded_t instruction_decode(uint32_t);

#endif
