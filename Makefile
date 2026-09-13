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
$(BUILD)/invalid-memory.cvm: $(BUILD)/hello.img | $(BUILD)
	printf 'name=Invalid memory VM\nbackend=soft86\nmemory=65536junk\nboot=build/hello.img\nserial=on\n' > $@
$(BUILD)/invalid-serial.cvm: $(BUILD)/hello.img | $(BUILD)
	printf 'name=Invalid serial VM\nbackend=soft86\nmemory=1048576\nboot=build/hello.img\nserial=tru\n' > $@
$(BUILD)/invalid-long-line.cvm: $(BUILD)/hello.img | $(BUILD)
	python3 -c 'print("name=" + "x" * 1100 + "\\nboot=build/hello.img")' > $@
$(BUILD)/invalid-long-boot.cvm: $(BUILD)/hello.img | $(BUILD)
	python3 -c 'print("name=Long boot VM\\nboot=" + "x" * 600)' > $@
test: all $(BUILD)/hello.img $(BUILD)/hello.cvm $(BUILD)/invalid-memory.cvm $(BUILD)/invalid-serial.cvm $(BUILD)/invalid-long-line.cvm $(BUILD)/invalid-long-boot.cvm
	./$(BUILD)/cygnus capabilities > $(BUILD)/caps.out
	grep -q "C-Bus" $(BUILD)/caps.out
	./$(BUILD)/cygnus run $(BUILD)/hello.img 10000 > $(BUILD)/guest.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest.out
	./$(BUILD)/cygnus vm $(BUILD)/hello.cvm 10000 > $(BUILD)/guest-cvm.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest-cvm.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-memory.cvm 10000 > $(BUILD)/invalid-memory.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-memory.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-serial.cvm 10000 > $(BUILD)/invalid-serial.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-serial.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-long-line.cvm 10000 > $(BUILD)/invalid-long-line.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-long-line.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-long-boot.cvm 10000 > $(BUILD)/invalid-long-boot.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-long-boot.out
	./$(BUILD)/cygnus snapshot $(BUILD)/hello.img $(BUILD)/hello.cys
	test -s $(BUILD)/hello.cys
	@echo "Cygnus extensible-core smoke test passed"
clean:
	rm -rf $(BUILD)