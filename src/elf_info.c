// Info mode: dump header and program headers without mapping.

#include "elf_info.h"

#include "elf_parser.h"
#include "elf_validate.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int elf_info(const char *path) {
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

  printf("ELF64 x86-64\nType: EXEC\nEntry: 0x%lx\nProgram headers: %u\n",
         (unsigned long)e.e_entry, e.e_phnum);

  for (unsigned i = 0; i < e.e_phnum; i++) {
    if (p[i].p_type != PT_LOAD)
      continue;

    printf("PT_LOAD offset=0x%lx vaddr=0x%lx filesz=0x%lx memsz=0x%lx "
           "flags=%c%c%c\n",
           (unsigned long)p[i].p_offset, (unsigned long)p[i].p_vaddr,
           (unsigned long)p[i].p_filesz, (unsigned long)p[i].p_memsz,
           (p[i].p_flags & PF_R) ? 'R' : '-', (p[i].p_flags & PF_W) ? 'W' : '-',
           (p[i].p_flags & PF_X) ? 'X' : '-');
  }

  free(p);
  close(fd);
  return 0;
}
