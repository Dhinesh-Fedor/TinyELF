// CLI entry: `tinyelf --info <elf>` dumps headers, `tinyelf <elf>` runs it.

#include "elf_info.h"
#include "elf_loader.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "--info") == 0)
    return elf_info(argv[2]) == 0 ? 0 : 1;

  if (argc != 2) {
    fprintf(stderr, "usage: tinyelf [--info] <elf>\n");
    return 2;
  }

  return elf_load(argv[1]) == 0 ? 0 : 1;
}
