#ifndef TINYELF_VALIDATE_H
#define TINYELF_VALIDATE_H

#include <elf.h>
#include <stddef.h>
#include <sys/types.h>

int elf_validate(const Elf64_Ehdr *eh);
int elf_validate_loads(const Elf64_Phdr *phdrs, size_t count, off_t file_size);

#endif
