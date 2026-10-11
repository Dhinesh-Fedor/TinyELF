// Maps PT_LOAD segments into memory at their requested addresses.

#define _GNU_SOURCE

#include "elf_mapper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int prot(uint32_t f) {
  int p = 0;
  if (f & PF_R) p |= PROT_READ;
  if (f & PF_W) p |= PROT_WRITE;
  if (f & PF_X) p |= PROT_EXEC;
  return p;
}

static int round_up_size(size_t value, size_t page_size, size_t *out) {
  if (value > SIZE_MAX - (page_size - 1))
    return -1;
  *out = (value + page_size - 1) & ~(page_size - 1);
  return 0;
}

int elf_map_segments(int fd, const Elf64_Phdr *p, size_t n) {
  if (!p && n != 0)
    return -1;

  long ps_long = sysconf(_SC_PAGESIZE);
  if (ps_long <= 0)
    return -1;
  size_t ps = (size_t)ps_long;

  for (size_t i = 0; i < n; i++) {
    if (p[i].p_type != PT_LOAD || p[i].p_memsz == 0)
      continue;
    if (p[i].p_filesz > p[i].p_memsz)
      return -1;

    Elf64_Addr base = p[i].p_vaddr & ~((Elf64_Addr)ps - 1);
    Elf64_Off aligned_off = p[i].p_offset & ~((Elf64_Off)ps - 1);
    if (aligned_off > (Elf64_Off)INT64_MAX)
      return -1;
    off_t off = (off_t)aligned_off;
    size_t delta = (size_t)(p[i].p_vaddr - base);

    if (p[i].p_filesz > SIZE_MAX - delta ||
        p[i].p_memsz > SIZE_MAX - delta)
      return -1;
    size_t file_size = delta + (size_t)p[i].p_filesz;
    size_t mem_size = delta + (size_t)p[i].p_memsz;
    size_t map_size, file_map_size;
    if (round_up_size(mem_size, ps, &map_size) < 0 ||
        round_up_size(file_size, ps, &file_map_size) < 0)
      return -1;

    // This loader maps each segment independently, so an occupied target
    // address is treated as a mapping error rather than overwritten.
    if (p[i].p_filesz > 0) {
      void *m = mmap((void *)(uintptr_t)base, file_size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, off);
      if (m == MAP_FAILED)
        return -1;

      size_t zero_end = mem_size < file_map_size ? mem_size : file_map_size;
      if (zero_end > file_size)
        memset((char *)m + file_size, 0, zero_end - file_size);
    } else {
      void *bss = mmap((void *)(uintptr_t)base, map_size,
                       PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
      if (bss == MAP_FAILED)
        return -1;
    }

    if (p[i].p_filesz > 0 && file_map_size < map_size) {
      void *bss = mmap((void *)(uintptr_t)(base + file_map_size),
                       map_size - file_map_size,
                       PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
      if (bss == MAP_FAILED) {
        munmap((void *)(uintptr_t)base, file_map_size);
        return -1;
      }
    }

    if (mprotect((void *)(uintptr_t)base, map_size, prot(p[i].p_flags)) < 0) {
      munmap((void *)(uintptr_t)base, map_size);
      return -1;
    }
  }

  return 0;
}
