// Validates supported ELF headers and PT_LOAD program headers.

#include "elf_validate.h"

#include <stdint.h>
#include <unistd.h>

int elf_validate(const Elf64_Ehdr *e) {
  if (!e)
    return -1;

  if (e->e_ident[EI_MAG0] != ELFMAG0 || e->e_ident[EI_MAG1] != ELFMAG1 ||
      e->e_ident[EI_MAG2] != ELFMAG2 || e->e_ident[EI_MAG3] != ELFMAG3)
    return -1;

  if (e->e_ident[EI_CLASS] != ELFCLASS64 ||
      e->e_ident[EI_DATA] != ELFDATA2LSB ||
      e->e_ident[EI_VERSION] != EV_CURRENT)
    return -1;

  if (e->e_version != EV_CURRENT || e->e_ehsize != sizeof(Elf64_Ehdr))
    return -1;

  if (e->e_machine != EM_X86_64 || e->e_type != ET_EXEC)
    return -1;

  if (e->e_phnum == 0 || e->e_phentsize != sizeof(Elf64_Phdr))
    return -1;

  return 0;
}

int elf_validate_loads(const Elf64_Phdr *p, size_t n, off_t file_size) {
  if ((!p && n != 0) || file_size < 0)
    return -1;

  for (size_t i = 0; i < n; i++) {
    if (p[i].p_type != PT_LOAD)
      continue;

    if (p[i].p_filesz > p[i].p_memsz)
      return -1;

    if (p[i].p_offset > (Elf64_Off)file_size)
      return -1;
    if (p[i].p_filesz > (Elf64_Xword)((Elf64_Off)file_size - p[i].p_offset))
      return -1;

    if (p[i].p_vaddr + p[i].p_memsz < p[i].p_vaddr)
      return -1;

    // The mapper rounds both values down to page boundaries. For file-backed
    // data, their within-page offsets must match for the mapping to be valid.
    if (p[i].p_filesz > 0 && (p[i].p_offset & 0xfff) != (p[i].p_vaddr & 0xfff))
      return -1;

    if (p[i].p_align > 1 &&
        ((p[i].p_align & (p[i].p_align - 1)) != 0 ||
         (p[i].p_vaddr % p[i].p_align) != (p[i].p_offset % p[i].p_align)))
      return -1;
  }

  return 0;
}
