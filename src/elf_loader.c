// Loads a supported executable: parse, validate, map, then jump to its entry.

#include "elf_loader.h"

#include "elf_mapper.h"
#include "elf_parser.h"
#include "elf_validate.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static void elf_run(unsigned long entry, const char *path) {
  size_t stack_size = 2 * 4096;
  char *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack == MAP_FAILED) {
    perror("stack");
    exit(1);
  }

  size_t path_len = strlen(path) + 1;
  if (path_len + 64 > stack_size) {
    fprintf(stderr, "path too long for initial stack\n");
    exit(1);
  }

  char *sp = stack + stack_size;
  sp -= path_len;
  memcpy(sp, path, path_len);
  char *argv0 = sp;

  uintptr_t *table = (uintptr_t *)((uintptr_t)sp & ~(uintptr_t)15) - 4;
  table[0] = 1;
  table[1] = (uintptr_t)argv0;
  table[2] = 0;
  table[3] = 0;

  void *target = (void *)entry;
  __asm__ volatile("mov %0, %%rsp\n\tjmp *%1" : : "r"(table), "r"(target)
                   : "memory");
  __builtin_unreachable();
}

int elf_load(const char *path) {
  if (!path)
    return -1;

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror(path);
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("fstat");
    close(fd);
    return -1;
  }

  Elf64_Ehdr e;
  if (elf_read_header(fd, &e) < 0 || elf_validate(&e) < 0) {
    fprintf(stderr, "unsupported or invalid ELF\n");
    close(fd);
    return -1;
  }

  Elf64_Phdr *p = NULL;
  if (elf_read_phdrs(fd, &e, &p) < 0 ||
      elf_validate_loads(p, e.e_phnum, st.st_size) < 0) {
    fprintf(stderr, "invalid program headers\n");
    free(p);
    close(fd);
    return -1;
  }

  if (elf_map_segments(fd, p, e.e_phnum) < 0) {
    perror("mapping");
    free(p);
    close(fd);
    return -1;
  }

  free(p);
  close(fd);
  fprintf(stderr, "segments mapped; entry point: 0x%lx\n",
          (unsigned long)e.e_entry);
  elf_run((unsigned long)e.e_entry, path);
  return 0;
}
