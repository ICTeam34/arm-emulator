#include "memory.h"

void memory_init(memory_t* memory, uint32_t start, uint32_t size) {
  memory->mem      = calloc(size, sizeof(uint8_t));
  if(memory->mem == NULL) {
    fprintf(stderr,"calloc failure");
    exit(EXIT_FAILURE);
  }
  memory->start    = start;
  memory->size     = size;
  memory->callback = NULL;
}

bool memory_write(memory_t* memory, uint32_t address, uint32_t value) {
  // Convert to address to relative
  address -= memory->start;

  if (address + 3 > memory->size - 4) {
    printf("Invalid address, this is probably an error in address_decoder\n");
    return false;   
  }


  if (memory->callback != NULL) {
    memory->callback(memory, address, true);
  }

  memory_write_unsafe(memory, address, value);

  return true;
}

void memory_write_unsafe(memory_t* memory, uint32_t address, uint32_t value) {
  memcpy(memory->mem + address, &value, 4);
}


uint32_t memory_read(memory_t* memory, uint32_t address) {
  // Convert the address to relative
  address -= memory->start;

  if (memory->callback != NULL) {
    memory->callback(memory, address, false);
  }

  return memory_read_unsafe(memory, address);
}

uint32_t memory_read_unsafe(memory_t* memory, uint32_t address) {
  uint32_t* first_address = (uint32_t*) (memory->mem + address);
  return *first_address;
}

void memory_dump_state(memory_t* memory) {
  printf("Non-zero memory:\n");
  for (uint32_t i = 0; i <= (memory->size - 4); i += 4) {
    uint32_t value = memory_read(memory, i);
    if (value) {
      printf("0x%08x: 0x%08x\n", i, endian_swap(value));
    }
  }
}

void memory_free(memory_t* memory) {
  if (memory == NULL) {
    return; 
  }

  if (memory->mem) {
    free(memory->mem);
  }

  free(memory);
}

/**
 * Chooses the device corresponding to the specified address.
 * Returns null if device not found.
 */
memory_t* address_decoder(memory_t** devices, uint8_t devices_c,
    uint32_t address) {

  for (uint8_t i = 0; i < devices_c; i++) {
    uint32_t start = devices[i]->start;
    uint32_t end = start + devices[i]->size - 4;
    if (address >= start && address <= end) {
      return devices[i];
    }
  }

  return NULL;
}

/*
 * Swap the endianness of the input number.
 * Should only be used for outputting values, as by default,
 * the system would print the bytes in the wrong order.
 */
uint32_t endian_swap(uint32_t n) {
  uint32_t result;
  dword_t* from = (dword_t *) &n;
  dword_t* to   = (dword_t *) &result;

  to->bytes._1 = from->bytes._4;
  to->bytes._2 = from->bytes._3;
  to->bytes._3 = from->bytes._2;
  to->bytes._4 = from->bytes._1;

  return result;  
}
