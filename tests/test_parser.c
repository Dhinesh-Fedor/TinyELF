#include "elf_validate.h"

#include <assert.h>
#include <string.h>

static Elf64_Ehdr valid_header(void) {
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
  e.e_ehsize = sizeof(Elf64_Ehdr);
  e.e_phentsize = sizeof(Elf64_Phdr);
  e.e_phnum = 1;
  return e;
}

int main(void) {
  Elf64_Ehdr e = valid_header();
  assert(elf_validate(&e) == 0);

  e.e_ident[EI_MAG0] = 0;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_ident[EI_CLASS] = ELFCLASS32;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_ident[EI_DATA] = ELFDATA2MSB;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_machine = EM_386;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_type = ET_DYN;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_ehsize--;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_phentsize--;
  assert(elf_validate(&e) == -1);

  e = valid_header();
  e.e_phnum = 0;
  assert(elf_validate(&e) == -1);

  Elf64_Phdr p;
  memset(&p, 0, sizeof(p));
  p.p_type = PT_LOAD;
  p.p_offset = 0;
  p.p_vaddr = 0x400000;
  p.p_filesz = 8;
  p.p_memsz = 16;
  p.p_align = 0x1000;
  assert(elf_validate_loads(&p, 1, 32) == 0);

  p.p_filesz = 17;
  assert(elf_validate_loads(&p, 1, 32) == -1); // filesz > memsz

  p.p_filesz = 8;
  p.p_offset = 30;
  assert(elf_validate_loads(&p, 1, 32) == -1); // bytes past EOF

  p.p_offset = 0;
  p.p_vaddr = UINT64_MAX - 7;
  p.p_memsz = 16;
  assert(elf_validate_loads(&p, 1, 32) == -1); // address overflow

  p.p_vaddr = 0x400080;
  p.p_memsz = 16;
  p.p_filesz = 8;
  assert(elf_validate_loads(&p, 1, 32) == -1); // page offsets differ

  return 0;
}
