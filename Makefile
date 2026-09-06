CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -Iinclude
BUILD := build
OBJS := $(BUILD)/cygnus.o $(BUILD)/main.o
.PHONY: all clean test
all: $(BUILD)/cygnus
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/%.o: src/%.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/cygnus: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD)/hello.img: tools/mkboot.py | $(BUILD)
	python3 tools/mkboot.py
test: all $(BUILD)/hello.img
	./$(BUILD)/cygnus run $(BUILD)/hello.img 10000 > $(BUILD)/guest.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest.out
	./$(BUILD)/cygnus snapshot $(BUILD)/hello.img $(BUILD)/hello.cys
	test -s $(BUILD)/hello.cys
	@echo "Cygnus self-emulation smoke test passed"
clean:
	rm -rf $(BUILD)
