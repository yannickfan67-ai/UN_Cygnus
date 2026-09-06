CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11 -Iinclude
BUILD := build
SRCS := src/cygnus.c src/bus.c src/device_serial.c src/config.c src/main.c
OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))
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
$(BUILD)/hello.cvm: $(BUILD)/hello.img | $(BUILD)
	printf 'name=Hello VM\nbackend=soft86\nmemory=1048576\nboot=build/hello.img\nserial=on\n' > $@
test: all $(BUILD)/hello.img $(BUILD)/hello.cvm
	./$(BUILD)/cygnus capabilities > $(BUILD)/caps.out
	grep -q "C-Bus" $(BUILD)/caps.out
	./$(BUILD)/cygnus run $(BUILD)/hello.img 10000 > $(BUILD)/guest.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest.out
	./$(BUILD)/cygnus vm $(BUILD)/hello.cvm 10000 > $(BUILD)/guest-cvm.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest-cvm.out
	./$(BUILD)/cygnus snapshot $(BUILD)/hello.img $(BUILD)/hello.cys
	test -s $(BUILD)/hello.cys
	@echo "Cygnus extensible-core smoke test passed"
clean:
	rm -rf $(BUILD)
