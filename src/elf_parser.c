// Parses the ELF header and program headers from an open file.

#define _XOPEN_SOURCE 700

#include "elf_parser.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

static int read_full(int fd, void *buf, size_t n, off_t off) {
  size_t done = 0;

  while (done < n) {
    if (off < 0 || (uintmax_t)off + done > (uintmax_t)INT64_MAX)
      return -1;

    ssize_t r = pread(fd, (char *)buf + done, n - done, off + (off_t)done);
    if (r < 0 && errno == EINTR)
      continue;
    if (r <= 0)
      return -1;
    done += (size_t)r;
  }

  return 0;
}

int elf_read_header(int fd, Elf64_Ehdr *eh) {
  if (!eh)
    return -1;
  return read_full(fd, eh, sizeof(*eh), 0);
}

int elf_read_phdrs(int fd, const Elf64_Ehdr *eh, Elf64_Phdr **out) {
  if (!eh || !out)
    return -1;
  *out = NULL;

  if (eh->e_phnum == 0 || eh->e_phentsize != sizeof(Elf64_Phdr))
    return -1;

  // e_phoff is unsigned in the ELF header; reject values that cannot be
  // represented by the signed file offset accepted by pread().
  if (eh->e_phoff > (Elf64_Off)INT64_MAX)
    return -1;

  size_t n = (size_t)eh->e_phnum * sizeof(Elf64_Phdr);
  Elf64_Phdr *p = malloc(n);
  if (!p)
    return -1;

  if (read_full(fd, p, n, (off_t)eh->e_phoff) < 0) {
    free(p);
    return -1;
  }

  *out = p;
  return 0;
}
