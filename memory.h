#ifndef HEADER_MEMORY
#define HEADER_MEMORY

#include "common.h"
#include "instruction.h"
#include "utils.h"

#define RAM_SIZE (1 << 16) // 2^16 memory locations, byte addressable. 
                           // DONT delete the parenthese
                           
typedef struct memory_struct {
  uint32_t start;
  uint32_t size;
  uint8_t *mem;
  void (*callback)(struct memory_struct*, uint32_t rel_address, bool write);

  // Custom value can be stored, probably it should be an array...
  uint64_t custom_buffer;
} memory_t; // device_t maybe?

typedef union {
  uint32_t value;
  struct {
    uint32_t _1 : 8;
    uint32_t _2 : 8;
    uint32_t _3 : 8;
    uint32_t _4 : 8;
  } bytes;
} dword_t;

typedef union {
  uint64_t value;
  struct {
    dword_t lower;
    dword_t higher;
  } dwords;
} qword_t;

void      memory_init(memory_t*, uint32_t, uint32_t);
void      memory_write_unsafe(memory_t*, uint32_t, uint32_t);
bool      memory_write(memory_t*, uint32_t, uint32_t);
uint32_t  memory_read(memory_t*, uint32_t);
uint32_t  memory_read_unsafe(memory_t*, uint32_t);
memory_t* address_decoder(memory_t**, uint8_t, uint32_t);
void      memory_dump_state(memory_t*);
void      memory_free(memory_t*);
uint32_t  endian_swap(uint32_t n);

#endif
