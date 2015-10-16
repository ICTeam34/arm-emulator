#include "cpu.h"

void cpu_init(cpu_t* this) {
  // Set registers to 0
  this->registers = calloc(REG_NUM, sizeof(uint32_t));
  if(this->registers == NULL) {
    fprintf(stderr,"calloc failure");
    exit(EXIT_FAILURE);
  }
  this->flags = (flags_t *) &(this->registers[16]);
  this->has_instruction = false;
  this->instruction_to_execute = &(this->decoded_inst.fields.instruction);

  // Set up default devices: ram and timer
  this->devicesc = 0;
  
  // Allocate pointer to size zero so that it can be realloc'd
  // (although this is implementation specific, TODO: test)
  this->devices = malloc(0);
  if(this->devices == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }

  memory_t* ram = ram_init();
  cpu_add_device(this, ram);
  this->ram     = ram; 

  memory_t* timer = timer_init();
  cpu_add_device(this, timer);
  this->timer    = timer; 
  timer->custom_buffer = clock();

  memory_t* mailbox = mailbox_init();
  cpu_add_device(this, mailbox);
  this->mailbox = mailbox; 

  memory_t* gpio = gpio_init();
  cpu_add_device(this, gpio);

  this->c_temp   = 0;
}

void cpu_add_device(cpu_t* cpu, memory_t* device) {
  ++cpu->devicesc;
  cpu->devices = realloc(cpu->devices, (cpu->devicesc) * sizeof(memory_t *));
  cpu->devices[cpu->devicesc - 1] = device;
}

void cpu_loop(cpu_t* cpu) {
  while (cpu->decoded_inst.type != HALT) {

    cpu_execute(cpu);
    if (cpu->has_instruction) {
      cpu->decoded_inst = instruction_decode(cpu->fetched_inst);
    } else {
      cpu->decoded_inst.type = EMPTY;
      cpu->has_instruction = true;
    }

    cpu_fetch_instruction(cpu);

    // The program counter is incremented by 4
    // because of the addressing mode of the machine (4 byte words)
    //TODO: pc
    cpu->registers[15] += 4;
  }
}

void cpu_fetch_instruction(cpu_t* cpu) {
  cpu->fetched_inst = memory_read(cpu->ram, cpu->registers[15]);
}

/**
 * Evaluate the condition based on the flags in CPSR
 */
bool cpu_eval(cpu_t* cpu, uint8_t condition) {
  bool n_flag = (bool) cpu->flags->n;
  bool z_flag = (bool) cpu->flags->z;
  bool v_flag = (bool) cpu->flags->v;
  bool c_flag = (bool) cpu->flags->c;

  switch(condition) {
    case COND_EQ:
      return z_flag;
    case COND_NE:
      return !z_flag;
    case COND_CS:
      return c_flag; 
    case COND_CC:
      return !c_flag;
    case COND_MI:
      return n_flag;
    case COND_PL:
      return !n_flag;
    case COND_VS:
      return v_flag;
    case COND_VC:
      return !v_flag;
    case COND_HI:
      return (c_flag && !z_flag);
    case COND_LS:
      return (!c_flag || z_flag);
    case COND_GE:
      return !(n_flag ^ v_flag);
    case COND_LT:
      return (n_flag ^ v_flag);
    case COND_GT:
      return (!z_flag && !(n_flag ^ v_flag));
    case COND_LE:
      return (z_flag || (n_flag ^ v_flag));
    case COND_AL:
      return true;
    default:
      return false;
  }
}

/**
 * Executes the current decoded instruction
 * returns true if the system has to halt, false otherwise
 */
bool cpu_execute(cpu_t* cpu) {
  if (cpu->decoded_inst.type == HALT) {
    return true; 
  }

  inst_generic_t* g = cpu->instruction_to_execute;

  // Evaluate the condition then execute
  if (cpu_eval(cpu, g->cond)) {
    switch (cpu->decoded_inst.type) {
      case EMPTY:
        break;
      case PROC: 
        cpu_execute_proc(cpu);
        break;
      case MULT:  
        cpu_execute_mult(cpu);
        break;      
      case SDT: 
        cpu_execute_sdt(cpu);
        break; 
      case BRANCH:
        cpu_execute_branch(cpu);
        break; 
      case BDT:
        cpu_execute_bdt(cpu);
        break;
      default:
        break;
    }
  }
  return false;
}

/**
 * Extract the offset and shift by 2 to the left. 
 * Extend the offset to a 32-bit two's compliment. 
 */
void cpu_execute_branch(cpu_t* cpu) {
  inst_branch_t* inst = cpu->instruction_to_execute;
  uint32_t offset = inst->offset << 2;
  uint32_t sign_bit = (uint32_t) (get_bit(offset, 23) << 31);
  
  for (int i = 0; i < 8; i++) {
    offset = offset | sign_bit;
    sign_bit >>= 1;
  }

  // BL instruction
  if (inst->l) { 
    // Save next instruction in lr
    cpu->registers[14] = cpu->registers[15] - 4;
  }
  
  //TODO: pc nicer
  cpu->registers[15] += offset;
  
  cpu_flush_pipeline(cpu);
  
}

/**
 * TODO: comment cba, test
 */
void cpu_execute_bx(cpu_t* cpu) {
  inst_bx_t* inst = cpu->instruction_to_execute;
  if (inst->r_n == 15) {
    //TODO: undefined state
    return; 
  }
  cpu->registers[15] = cpu->registers[inst->r_n] & 0xFFFFFFFE;
  
  cpu_flush_pipeline(cpu);
}

//TODO: Need to implement exception handling; P30 in the ARM Document

/**
 * TODO: S bit is ignored
 */
void cpu_execute_bdt(cpu_t* cpu) {
  inst_bdt_t* inst = cpu->instruction_to_execute;
  uint32_t*   reg  = cpu->registers;

  uint16_t addr = (uint16_t) reg[inst->r_n];
  uint8_t address_mode = (uint8_t) inst->p_u;

  int regc = count_set_bits(inst->reg_bits);
  uint32_t regv[regc];
  get_reg_list(regv, inst->reg_bits);    

  // points to the memory location after operation
  uint16_t inst_p;

  if (inst->l) {
    inst_p = cpu_load_blocks(cpu, regv, regc, addr, address_mode);
  } else {
    inst_p = cpu_store_blocks(cpu, regv, regc, addr, address_mode);
  } 

  if (inst->w) {
    reg[inst->r_n] = inst_p;
  }
}

void cpu_execute_sdt(cpu_t* cpu) {
  inst_sdt_t* inst = cpu->instruction_to_execute;
  uint8_t r_sourcedest  = (uint8_t) inst->r_d; // source/dest reg
  uint8_t r_n           = (uint8_t) inst->r_n; // base register
  bool load             = (bool) inst->l;
  bool up               = (bool) inst->u;
  bool pre              = (bool) inst->p;
  bool immediate_offset = (bool) inst->i;
  uint8_t r_m = (uint8_t) get_bits(inst->offset, 0, 3); // offset register

  int32_t offset_val = (int32_t) inst->offset;
  int32_t r_n_content = (int32_t) cpu->registers[r_n];

  // check if PC (R15) is the source register (invalid)
  if (!load && r_sourcedest == 15) {

    fprintf(stderr, "Error: PC is source register in str");
    return;
  }

  // get offset
  if (immediate_offset) {

    offset_val = (int32_t) get_not_immediate(cpu, inst->offset, false);

  } else {

    // check if postindexing and not immediate then Rn != Rm (offset reg)
    if (!pre && r_n == r_m) {
      fprintf(stderr, "Error: Offset register = base register in postindexing");
      return;
    }

    // check if PC (R15) is the register offset when offset isn't immediate
    if (r_m == 15) {
      fprintf(stderr, "Error: PC is specified as register offset");
      return;
    } 

  }

  if (!up) {
    offset_val = -offset_val;
  }

  // preindexing
  if (pre) {
    r_n_content += offset_val;
  }
  
  uint32_t address = (uint32_t) r_n_content;
  memory_t* device = address_decoder(cpu->devices, cpu->devicesc, address);
  
  if (device == NULL) {
      printf("Error: Out of bounds memory access at address %#010x\n", address);
      return;
  }

  // transfer data
  if (load) {
    // read
    cpu->registers[r_sourcedest] = memory_read(device, address);
  } else {
    // write
    memory_write(device, r_n_content, (uint32_t) cpu->registers[r_sourcedest]);
  }

  // postindexing
  if (!pre) {
    cpu->registers[r_n] += offset_val;
  }

}

void cpu_execute_proc(cpu_t* cpu) {
  cpu->c_temp = 0;
  inst_data_proc_t* i = cpu->instruction_to_execute;

  uint16_t operand2 = (uint16_t) i->op2;
  uint8_t r_dest = (uint8_t) i->r_d;
  uint8_t opcode = (uint8_t) i->opcode;
  uint8_t r_n    = (uint8_t) i->r_n; // register r_n
  bool set_condition = i->s;
  bool immediate_operand = i->i;

  uint32_t operand_val = 0;

  if (immediate_operand) {

    // Rotate by twice the value in the 4 bit rotate field
    uint8_t rotate_by = (uint8_t) (get_bits(operand2, 8, 11) * 2);
    uint32_t imm = get_bits(operand2, 0, 7);
    operand_val = rotate_right(cpu, imm, rotate_by);


  } else {

    operand_val = get_not_immediate(cpu, operand2, true);
  }

  int32_t result = 0;

  // Store the 64-bit result of the calculations.
  // This ensures that no overflow happens, and it becomes
  // trivial to check for it.
  uint64_t real_result;

  switch (opcode) {
    case OP_AND:
      result = cpu->registers[r_n] & operand_val;
      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_EOR:
      result = cpu->registers[r_n] ^ operand_val;
      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_SUB:
      result = cpu->registers[r_n] - operand_val;

      if (((uint32_t) result < cpu->registers[r_n]) != (operand_val > 0)) {
        //Overflow
        cpu->c_temp = 0;
      } else {
        cpu->c_temp = 1;
      }

      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_RSB:
      result = operand_val - cpu->registers[r_n];

      if (((uint32_t) result < cpu->registers[r_n]) != (operand_val > 0)) {
        //Overflow
        cpu->c_temp = 0;
      } else {
        cpu->c_temp = 1;
      }

      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_ADD:
      real_result = (uint64_t) cpu->registers[r_n] + (uint64_t) operand_val;
      result = (int32_t) real_result;
      cpu->c_temp += (uint32_t) (real_result >> 32); //Shift to check for overflows
      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_TST:
      result = cpu->registers[r_n] & operand_val;
      break;
    case OP_TEQ:
      result = cpu->registers[r_n] ^ operand_val;
      break;
    case OP_CMP:
      result = cpu->registers[r_n] - operand_val;

      if (((uint32_t) result < cpu->registers[r_n]) != (operand_val > 0)) {
        //Overflow
        cpu->c_temp = 0;
      } else {
        cpu->c_temp = 1;
      }
      break;
    case OP_ORR:
      result = cpu->registers[r_n] | operand_val;
      cpu->registers[r_dest] = (uint32_t) result;
      break;
    case OP_MOV:
      result = operand_val;
      cpu->registers[r_dest] = (uint32_t) result;
      if (r_dest == 15) {
        cpu_flush_pipeline(cpu);
      }
      break;
    default:break;
  }

  if (set_condition) {
    cpu->flags->z = (uint32_t) result == 0;
    cpu->flags->n = (int32_t) result < 0;
    cpu->flags->c = (uint32_t) cpu->c_temp > 0;
  }
}

void cpu_execute_mult(cpu_t* cpu) {
  inst_mult_t* inst = cpu->instruction_to_execute;
  uint8_t r_dest = (uint8_t) inst->r_d; 
  uint8_t r_n = (uint8_t) inst->r_n;
  uint8_t r_s = (uint8_t) inst->r_s; 
  uint8_t r_m = (uint8_t) inst->r_m; 

  if(inst->a) {
    cpu->registers[r_dest] 
      = cpu->registers[r_m] * cpu->registers[r_s] + cpu->registers[r_n];
  } else {
    cpu->registers[r_dest] 
      = cpu->registers[r_m] * cpu->registers[r_s];
  }
  if(inst->s) {
    cpu->flags->n = get_bit(cpu->registers[r_dest], 31);
    if(cpu->registers[r_dest] == 0) {
      cpu->flags->z = 1;
    }
  }
}

uint16_t cpu_store_blocks(cpu_t* cpu, uint32_t* regv, int regc, uint32_t addr,
    uint8_t address_mode) {

  uint16_t csp;
  uint32_t* reg = cpu->registers;
  memory_t* device = address_decoder(cpu->devices, cpu->devicesc, addr);

  if (device == NULL) {
    //TODO: error message
    return 0;
  }

  switch(address_mode) {
    case ADDR_PRE_INC:
      for (int i = 0; i<regc; i++) {
        addr += 4;
        memory_write(device, addr, reg[regv[i]]);
      }
      break;
    case ADDR_POST_INC:
      for (int i = 0; i<regc; i++) {
        memory_write(device, addr, reg[regv[i]]);
        addr += 4;
      }
      break;
    case ADDR_PRE_DEC:
      addr -= 4 * regc;
      csp   = addr;
      for (int i = 0; i < regc; i++) {
        memory_write(device, csp, reg[regv[i]]);
        csp += 4;
      }
      break;
    case ADDR_POST_DEC:
      addr -= 4 * regc;
      csp   = addr;
      for (int i = 0; i< regc; i++) {
        csp += 4;
        memory_write(device, csp, reg[regv[i]]);
      }
      break;
  }

  return addr;
}

uint16_t cpu_load_blocks(cpu_t* cpu, uint32_t* regv, int regc, uint32_t addr,
    uint8_t address_mode) {

  int csp;
  uint32_t* reg = cpu->registers;
  memory_t* device = address_decoder(cpu->devices, cpu->devicesc, addr);

  if (device == NULL) {
    printf("asd");
    //TODO: error message
    return 0;
  }

  switch(address_mode) {
    case ADDR_PRE_INC:
      for (int i = 0; i < regc; i++) {
        addr += 4;
        reg[regv[i]] = memory_read(device, addr);
        if (regv[i] == 15) {
          cpu_flush_pipeline(cpu);
        }
      }
      break;
    case ADDR_POST_INC:
      for (int i = 0; i < regc; i++) {
        reg[regv[i]] = memory_read(device, addr);
        addr += 4;
        if (regv[i] == 15) {
          cpu_flush_pipeline(cpu);
        }
      }
      break;
    case ADDR_PRE_DEC:
      addr -= 4 * regc;
      csp = addr;
      for (int i = 0; i < regc; i++) {
        reg[regv[i]] = memory_read(device, csp);
        csp += 4;
        if (regv[i] == 15) {
          cpu_flush_pipeline(cpu);
        }
      }
      break;
    case ADDR_POST_DEC:
      addr -= 4 * regc;
      csp = addr;
      for (int i = 0; i < regc; i++) {
        csp += 4;
        reg[regv[i]] = memory_read(device, csp);
        if (regv[i] == 15) {
          cpu_flush_pipeline(cpu);
        }
      }
      break;
  }
  return addr;
}

void cpu_flush_pipeline(cpu_t* cpu) {
  cpu->has_instruction = false;
  cpu->decoded_inst.type = EMPTY;
}

void cpu_dump_state(cpu_t* cpu) {
  printf("Registers:\n");
  for (int i = 0; i < REG_NUM; i++) {
    if(i<13) {
      printf("$%-3d: %10d (0x%08x)\n", i, cpu->registers[i], cpu->registers[i]);
    } else if(i == 15) {
      printf("PC  : %10d (0x%08x)\n", cpu->registers[i], cpu->registers[i]);
    } else if(i == 16) {
      printf("CPSR: %10d (0x%08x)\n", cpu->registers[i], cpu->registers[i]);
    }
  }
}

void cpu_free(cpu_t* cpu) {
  if (cpu == NULL) {
    return; 
  }

  free(cpu->registers);
  for (int i = 0; i < cpu->devicesc; i++) {
    memory_free(cpu->devices[i]);
  }
  free(cpu);
}

/**
 * Rotate x right by n bits
 */
uint32_t rotate_right(cpu_t* cpu, uint32_t x, uint8_t n) {
  uint32_t y = (x >> n) & ~(-1 << (32 - n));
  uint32_t z = x << (32 - n);

  if (n > 0) {
    cpu->c_temp += get_bits(x, 0, n - 1);
  }
  return y | z;
}

/**
 * Calculate the offset of operand, where operand
 * is not an immediate offset, but either
 * a shifted register of a shifted number
 */
uint32_t get_not_immediate(cpu_t* cpu, uint16_t operand, bool set_c_temp) {
  // set c_temp global value if set_c_temp is true

  uint8_t shift_type = (uint8_t) get_bits(operand, 5, 6);
  uint8_t r_m = (uint8_t) get_bits(operand, 0, 3);
  bool shift_by_reg = (bool) get_bit(operand, 4);
  uint8_t shift_int;

  if (shift_by_reg) {
    uint8_t shift_reg  = (uint8_t) get_bits(operand, 8, 11);
    shift_int  = (uint8_t) cpu->registers[shift_reg];
  } else {
    shift_int  = (uint8_t) get_bits(operand, 7, 11);
  }

  uint32_t operand_val = cpu->registers[r_m];
  uint32_t c_temp_new = cpu->c_temp;

  switch (shift_type) {
    case SHFT_LSL:      
      c_temp_new += (uint8_t) get_bits(operand_val, 32 - shift_int, 31);     
      operand_val <<= shift_int;
      break;
    case SHFT_LSR:
      if (shift_int > 0) {        
        c_temp_new += (uint8_t) get_bits(operand_val, 0, shift_int - 1);       
        operand_val >>= shift_int;
      }
      break;
    case SHFT_ASR: ;
      bool neg = (int32_t) operand_val < 0;
      c_temp_new += (uint8_t) get_bits(operand_val, 32 - shift_int, 31);
      operand_val >>= shift_int;
      if (neg && operand_val > 0) {
        operand_val = (uint32_t) -operand_val;
      }
      break;
    case SHFT_ROR:
      if (shift_int > 0) {
        c_temp_new += (uint8_t) get_bits(operand_val, 0, shift_int);
      }
      operand_val = rotate_right(cpu, operand_val, shift_int);
      break;
    default:break;
  }

  if (set_c_temp) {
    cpu->c_temp = c_temp_new;
  }

  return operand_val;
}
