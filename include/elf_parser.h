#ifndef TINYELF_PARSER_H
#define TINYELF_PARSER_H

#include <elf.h>
#include <stddef.h>
#include <sys/types.h>

int elf_read_header(int fd, Elf64_Ehdr *eh);
int elf_read_phdrs(int fd, const Elf64_Ehdr *eh, Elf64_Phdr **out);

#endif
