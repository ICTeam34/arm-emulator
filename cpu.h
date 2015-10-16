#ifndef HEADER_CPU
#define HEADER_CPU

#include "common.h"

#include "instruction.h"
#include "memory.h"
#include "devices.h"

/**
 * Number of registers.
 * TODO: consider the exception register
 */
#define REG_NUM 17

/**
 * Condition codes
 */
#define COND_EQ 0x0
#define COND_NE 0x1
#define COND_CS 0x2
#define COND_CC 0x3
#define COND_MI 0x4
#define COND_PL 0x5
#define COND_VS 0x6
#define COND_VC 0x7
#define COND_HI 0x8
#define COND_LS 0x9
#define COND_GE 0xA
#define COND_LT 0xB
#define COND_GT 0xC
#define COND_LE 0xD
#define COND_AL 0xE

/**
 * Shift types
 */
#define SHFT_LSL 0
#define SHFT_LSR 1
#define SHFT_ASR 2
#define SHFT_ROR 3

/**
 * Block data transfer 
 * addressing modes
 */
#define ADDR_PRE_INC  3
#define ADDR_POST_INC 1
#define ADDR_PRE_DEC  2
#define ADDR_POST_DEC 0

/**
 * CPU flags (CPSR)
 */
typedef struct {
  uint32_t   : 28;
  uint32_t v : 1;
  uint32_t c : 1;
  uint32_t z : 1;
  uint32_t n : 1;
} flags_t;

typedef struct {
  bool        has_instruction;
  decoded_t   decoded_inst;
  void*       instruction_to_execute;
  flags_t*    flags;
  memory_t**  devices;
  memory_t*  ram;
  memory_t*  timer;
  memory_t*  mailbox;

  uint8_t    devicesc; 

  uint32_t  c_temp;
  uint32_t  fetched_inst;
  uint32_t* registers;
} cpu_t;

void     cpu_init(cpu_t*);
void     cpu_add_device(cpu_t*, memory_t*);
void     cpu_loop(cpu_t* cpu);
void     cpu_fetch_instruction(cpu_t* cpu);
bool     cpu_eval(cpu_t*, uint8_t);

void     cpu_execute_proc(cpu_t*);
void     cpu_execute_mult(cpu_t*);
void     cpu_execute_sdt(cpu_t*);
void     cpu_execute_bdt(cpu_t*);
void     cpu_execute_branch(cpu_t*);
void     cpu_execute_bx(cpu_t*);
bool     cpu_execute(cpu_t*);

bool     cpu_get_flag(cpu_t*, uint8_t);
void     cpu_set_flag(cpu_t*, uint8_t, bool);
void     cpu_flush_pipeline(cpu_t*);
void     cpu_dump_state(cpu_t*);
void     cpu_free(cpu_t*);

uint16_t cpu_store_blocks(cpu_t*, uint32_t*, int, uint32_t, uint8_t);
uint16_t cpu_load_blocks(cpu_t*, uint32_t*, int, uint32_t, uint8_t);

uint32_t get_not_immediate(cpu_t*, uint16_t, bool);
uint32_t rotate_right(cpu_t*, uint32_t, uint8_t);

#endif
