#ifndef TINYELF_MAPPER_H
#define TINYELF_MAPPER_H

#include <elf.h>
#include <stddef.h>

int elf_map_segments(int fd, const Elf64_Phdr *phdrs, size_t count);

#endif
