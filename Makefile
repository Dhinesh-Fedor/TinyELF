CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

BUILD = build
SRC = src/main.c src/elf_parser.c src/elf_validate.c src/elf_mapper.c src/elf_loader.c src/elf_info.c
OBJ = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

$(BUILD)/tinyelf: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

$(BUILD)/test_parser: tests/test_parser.c src/elf_validate.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

$(BUILD)/test_elf: tests/test_elf.c src/elf_parser.c src/elf_validate.c src/elf_mapper.c src/elf_loader.c src/elf_info.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

test: $(BUILD)/test_parser $(BUILD)/test_elf $(BUILD)/tinyelf hello_print
	./$(BUILD)/test_parser
	./$(BUILD)/test_elf
	test "$$(./$(BUILD)/tinyelf ./$(BUILD)/hello_print 2>/dev/null)" = "hello from tinyelf"

clean:
	rm -rf $(BUILD)

hello: tests/hello.S | $(BUILD)
	$(CC) -c $< -o $(BUILD)/hello.o
	ld $(BUILD)/hello.o -o $(BUILD)/hello

hello_print: tests/hello_print.S | $(BUILD)
	$(CC) -c $< -o $(BUILD)/hello_print.o
	ld $(BUILD)/hello_print.o -o $(BUILD)/hello_print

.PHONY: all test clean hello hello_print
all: $(BUILD)/tinyelf
