#define _GNU_SOURCE

#include "elf_loader.h"
#include "elf_info.h"
#include "elf_mapper.h"
#include "elf_validate.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void test_truncated_input(void) {
  char path[] = "/tmp/tinyelf-truncated-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);

  Elf64_Ehdr e;
  memset(&e, 0, sizeof(e));
  e.e_ident[EI_MAG0] = ELFMAG0;
  e.e_ident[EI_MAG1] = ELFMAG1;
  e.e_ident[EI_MAG2] = ELFMAG2;
  e.e_ident[EI_MAG3] = ELFMAG3;
  e.e_ident[EI_CLASS] = ELFCLASS64;
  e.e_ident[EI_DATA] = ELFDATA2LSB;
  e.e_ident[EI_VERSION] = EV_CURRENT;
  e.e_type = ET_EXEC;
  e.e_machine = EM_X86_64;
  e.e_version = EV_CURRENT;
  e.e_ehsize = sizeof(e);
  e.e_phoff = sizeof(e);
  e.e_phentsize = sizeof(Elf64_Phdr);
  e.e_phnum = 1;
  assert(write(fd, &e, sizeof(e)) == (ssize_t)sizeof(e));
  close(fd);

  assert(elf_load(path) == -1);
  assert(elf_info(path) == -1);
  unlink(path);
}

static void test_bss_is_zeroed(void) {
  long page_size = sysconf(_SC_PAGESIZE);
  assert(page_size > 0);

  char path[] = "/tmp/tinyelf-bss-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  const char data[] = "file-not-part-of-segment";
  assert(write(fd, data, sizeof(data) - 1) == (ssize_t)(sizeof(data) - 1));

  Elf64_Phdr phdr;
  memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = 0;
  phdr.p_vaddr = 0x500000000000ULL;
  phdr.p_filesz = 4;
  phdr.p_memsz = (Elf64_Xword)page_size * 2 + 16;
  phdr.p_flags = PF_R | PF_W;

  assert(elf_map_segments(fd, &phdr, 1) == 0);
  char *mapped = (char *)(uintptr_t)phdr.p_vaddr;
  assert(memcmp(mapped, data, phdr.p_filesz) == 0);
  for (size_t i = phdr.p_filesz; i < (size_t)phdr.p_memsz; i++)
    assert(mapped[i] == 0);

  assert(munmap(mapped, (size_t)page_size * 3) == 0);
  close(fd);
  unlink(path);
}

static void test_bss_only_segment_with_offset(void) {
  long page_size = sysconf(_SC_PAGESIZE);
  assert(page_size > 0);

  char path[] = "/tmp/tinyelf-bss-only-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);

  Elf64_Phdr phdr;
  memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = 0;
  phdr.p_vaddr = 0x500000000080ULL;
  phdr.p_filesz = 0;
  phdr.p_memsz = 128;
  phdr.p_flags = PF_R | PF_W;

  assert(elf_map_segments(fd, &phdr, 1) == 0);

  char *mapped = (char *)(uintptr_t)phdr.p_vaddr;
  for (size_t i = 0; i < (size_t)phdr.p_memsz; i++)
    assert(mapped[i] == 0);

  uintptr_t base = (uintptr_t)phdr.p_vaddr & ~((uintptr_t)page_size - 1);
  size_t delta = (size_t)((uintptr_t)phdr.p_vaddr - base);
  size_t map_size =
      (delta + (size_t)phdr.p_memsz + (size_t)page_size - 1) &
      ~((size_t)page_size - 1);
  assert(munmap((void *)base, map_size) == 0);

  close(fd);
  unlink(path);
}

static void test_bad_load_headers(void) {
  Elf64_Phdr p;
  memset(&p, 0, sizeof(p));
  p.p_type = PT_LOAD;
  p.p_offset = 0;
  p.p_vaddr = 0x500000000000ULL;
  p.p_filesz = 8;
  p.p_memsz = 4;
  assert(elf_validate_loads(&p, 1, 64) == -1);

  p.p_filesz = 8;
  p.p_memsz = 16;
  p.p_offset = 60;
  assert(elf_validate_loads(&p, 1, 64) == -1);
}

int main(void) {
  test_truncated_input();
  test_bss_is_zeroed();
  test_bss_only_segment_with_offset();
  test_bad_load_headers();
  return 0;
}
