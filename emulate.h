#ifndef HEADER_EMULATE
#define HEADER_EMULATE

#include "common.h"

#include "instruction.h"
#include "cpu.h"
#include "memory.h"
#include "devices.h"
#include "utils.h"

int load_binary(memory_t*, int, char**);
void dump_state(); 

#endif
