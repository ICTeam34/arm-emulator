#include "emulate.h"

int main(int argc, char **argv) {

  //if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
  //  printf("something wrong\n");
  //}
  //SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);

  //SDL_WM_SetCaption("Video test", NULL);

  cpu_t* cpu = malloc(sizeof(cpu_t));
  if(cpu == NULL) {
    fprintf(stderr,"malloc failure");
    exit(EXIT_FAILURE);
  }
  cpu_init(cpu);

  if (load_binary(cpu->devices[0], argc, argv)) {
    return EXIT_FAILURE;
  };

  // The execute-decode-fetch "pipeline"
  cpu_loop(cpu);

  dump_state(cpu, cpu->ram);

  cpu_free(cpu);

  return EXIT_SUCCESS;
}

void dump_state(cpu_t* cpu, memory_t* memory) {
  cpu_dump_state(cpu);
  memory_dump_state(memory);
  //TODO: debug messages
  //uint64_t time = memory_read(cpu->timer, 0x20003004);
  //float t = (float) time / CLOCKS_PER_SEC;
  //printf("timer returned: %f\n", t);
}

/**
 * Load the binary executable into memory
 */
int load_binary(memory_t* memory, int numargs, char **argv) {
  if (numargs != 2) {
    fprintf(stderr, "Error: the number of arguments is %d.\n", numargs-1);
    return 1;
  }

  FILE *fileptr = fopen(argv[1], "rb");
  
  if (fileptr == NULL) {
    fprintf(stderr, "Error: Something went wrong while reading the file.\n");
    return 1;
  }

  fseek(fileptr, 0, SEEK_END);
  size_t filelength = (size_t) ftell(fileptr);

  if (filelength % 4 != 0) {
    fprintf(stderr,
     "Error: The number of bytes in the binary is not divisible by 4.\n");
    return 1;
  }

  rewind(fileptr);                    
  
  fread(memory->mem, filelength, 4, fileptr);
  fclose(fileptr);

  return 0;
}

