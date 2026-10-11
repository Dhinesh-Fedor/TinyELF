# TinyELF

Minimal ELF64 loader for Linux x86-64. Parses, validates, and maps static non-PIE executables and then jumps into their entry point with a minimal process stack.

## Features

- ELF64 header parsing
- Program-header parsing
- PT_LOAD validation
- Page-aligned segment mapping
- BSS mapping
- ELF permission handling
- ELF information mode
- Minimal process entry stack (argc, argv, envp)
- Entry-point execution via direct jump

## Architecture

```text
CLI
 |
ELF Parser
 |
Validator
 |
Segment Mapper
 |
Loader
```

## Project Structure

```text
include/       Public module interfaces
src/           Parser, validation, mapping, loader, CLI
tests/         Parser test and assembly fixture
build/         Object files and final binaries (generated)
```

## Requirements

- Linux x86-64
- GCC or Clang
- GNU binutils
- GNU Make

## Build

```bash
make
```

## Usage

Inspect an ELF:

```bash
./build/tinyelf --info ./binary
```

Load, map, and run a supported executable:

```bash
./build/tinyelf ./binary
```

Try it with the bundled fixtures:

```bash
make hello_print
./build/tinyelf ./build/hello_print
```
Expected output:
```
segments mapped; entry point: 0x401000
hello from tinyelf
```

The loader maps the program's `PT_LOAD` segments and then transfers control to its ELF entry point. The entry code is expected to be a freestanding program (static non-PIE `ET_EXEC`) that sets up its own exit path, e.g. via the
`exit`/`write` syscalls as in `tests/hello_print.S`.

## Tests

```bash
make test
```
The test suite covers:
- Valid and invalid ELF headers
- Truncated ELF input
- Invalid segment sizes and file ranges
- Virtual-address overflow and alignment
- BSS zero-initialization
- BSS-only segments with nonzero page offsets
- Execution of the bundled sample

## Limitations

- Static non-PIE ELF64 only
- No dynamic linker support
- No PIE support
- Minimal inital stact setup: no auxiliary vector, no extra argv, no envp propagation

## Future Work

- PIE support
- PT_INTERP handling
- Dynamic linking
- Auxiliary vector setup
- Full argc/argv/envp pass-through

## License

MIT License
