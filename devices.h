#ifndef HEADER_DEVICES
#define HEADER_DEVICES

#include "common.h"
#include "memory.h"

/**
 * GPIO 
 */
typedef struct {
  uint32_t pin0 : 3;
  uint32_t pin1 : 3;
  uint32_t pin2 : 3;
  uint32_t pin3 : 3;
  uint32_t pin4 : 3;
  uint32_t pin5 : 3;
  uint32_t pin6 : 3;
  uint32_t pin7 : 3;
  uint32_t pin8 : 3;
  uint32_t pin9 : 3;
  uint32_t      : 2;
} gpio_t;

memory_t* gpio_init();

void gpio_access_callback(memory_t*, uint32_t, bool);

/**
 * RAM stuff
 */
memory_t* ram_init();

/**
 * Timer
 */
memory_t* timer_init();

void timer_access_callback(memory_t*, uint32_t, bool);

/**
 * Mailbox
 */
memory_t* mailbox_init();

void mailbox_access_callback(memory_t*, uint32_t, bool);

#endif

