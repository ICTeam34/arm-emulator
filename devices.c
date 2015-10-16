#include "devices.h"

/**
 * Timer initialiser
 */
memory_t* ram_init() {
  memory_t* memory  = malloc(sizeof(memory_t));
  if(memory == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }
  memory_init(memory, 0, RAM_SIZE);
  memory->callback = NULL;

  return memory;
}

/**
 * GPIO initialiser
 */
memory_t* gpio_init() {
  memory_t* device = malloc(sizeof(memory_t));
  if(device == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }
  memory_init(device, 0x20200000, 64);
  device->callback = &gpio_access_callback;
  memory_write_unsafe(device, 0,   0x20200000);
  memory_write_unsafe(device, 0x4, 0x20200004);
  memory_write_unsafe(device, 0x8, 0x20200008);
  return device;
}

/**
 * GPIO memory access handler
 */
void gpio_access_callback(memory_t* device, uint32_t rel_addr, bool write) {

  switch(rel_addr) {
    case 0x0 :
      printf ("One GPIO pin from 0 to 9 has been accessed\n");
      break;
    case 0x4 :
      printf ("One GPIO pin from 10 to 19 has been accessed\n");
      break;
    case 0x8 :
      printf ("One GPIO pin from 20 to 29 has been accessed\n");
      break;
    case 0x28 :
      //if (write && device->mem[relative_address] != 0) {
        printf("PIN OFF\n");
      //}
      break;
    case 0x1c :
      //if (write && device->mem[relative_address] != 0) {
        printf("PIN ON\n");
      //}
      break;
    default : break;
  }
}

/**
 * Timer initialiser
 */
memory_t* timer_init() {
  memory_t* device = malloc(sizeof(memory_t));
  if(device == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }
  memory_init(device, 0x20003000, 22);
  device->callback = &timer_access_callback;

  return device;
}

void timer_access_callback(memory_t* timer, uint32_t rel_addr, bool write) {
  switch(rel_addr) {
    //TODO: other cases
    case 0x4: ;
        if (!write) {
          qword_t qword;
          qword.dwords.lower.value  = memory_read_unsafe(timer, 0x4);
          qword.dwords.higher.value = memory_read_unsafe(timer, 0x8);
          qword.value = clock() - timer->custom_buffer;

          memory_write_unsafe(timer, 0x4, qword.dwords.lower.value);
          memory_write_unsafe(timer, 0x8, qword.dwords.higher.value);
          printf ("Time requested\n");
        }
      break;
    default:break;
  }
}

/**
 * Mailbox initialiser
 */
memory_t* mailbox_init() {
  memory_t* device = malloc(sizeof(memory_t));
  if(device == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }
  memory_init(device, 0x2000B880, 36);
  device->callback = &mailbox_access_callback;

  return device;
}

void mailbox_access_callback(memory_t* device, uint32_t rel_addr, bool write) {
  switch(rel_addr) {
    case 0x0: // Read Receiving mail.
    break;
    case 0x10: // Poll Receive without retrieving.
    break;
    case 0x14: // Sender Sender information.
    break;
    case 0x18: // Status Information.
    break;
    case 0x1C: // Configuration Settings.
    break;
    case 0x20: // Write Sending mail.
    break;
    default:break;
  }
}

