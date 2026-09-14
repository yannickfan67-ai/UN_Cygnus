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
$(BUILD)/snapshot-load-test: tests/snapshot_load_test.c src/cygnus.c src/bus.c src/device_serial.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/snapshot_load_test.c src/cygnus.c src/bus.c src/device_serial.c -o $@
$(BUILD)/bus-add-device-test: tests/bus_add_device_test.c src/bus.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/bus_add_device_test.c src/bus.c -o $@
$(BUILD)/bus-width-test: tests/bus_width_test.c src/bus.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/bus_width_test.c src/bus.c -o $@
$(BUILD)/bus-registration-test: tests/bus_registration_test.c src/bus.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/bus_registration_test.c src/bus.c -o $@
$(BUILD)/serial-attach-test: tests/serial_attach_test.c src/bus.c src/device_serial.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/serial_attach_test.c src/bus.c src/device_serial.c -o $@
$(BUILD)/irq-inject-test: tests/irq_inject_test.c src/cygnus.c src/bus.c src/device_serial.c include/cygnus.h | $(BUILD)
	$(CC) $(CFLAGS) tests/irq_inject_test.c src/cygnus.c src/bus.c src/device_serial.c -o $@
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
$(BUILD)/invalid-malformed-line.cvm: $(BUILD)/hello.img | $(BUILD)
	printf 'name=Malformed line VM\nmemory 1048576\nboot=build/hello.img\n' > $@
$(BUILD)/invalid-empty-key.cvm: $(BUILD)/hello.img | $(BUILD)
	printf 'name=Empty key VM\n=ignored\nboot=build/hello.img\n' > $@
test: all $(BUILD)/snapshot-load-test $(BUILD)/bus-add-device-test $(BUILD)/bus-width-test $(BUILD)/bus-registration-test $(BUILD)/serial-attach-test $(BUILD)/irq-inject-test $(BUILD)/hello.img $(BUILD)/hello.cvm $(BUILD)/invalid-memory.cvm $(BUILD)/invalid-serial.cvm $(BUILD)/invalid-long-line.cvm $(BUILD)/invalid-long-boot.cvm $(BUILD)/invalid-malformed-line.cvm $(BUILD)/invalid-empty-key.cvm
	./$(BUILD)/snapshot-load-test
	./$(BUILD)/bus-add-device-test
	./$(BUILD)/bus-width-test
	./$(BUILD)/bus-registration-test
	./$(BUILD)/serial-attach-test
	./$(BUILD)/irq-inject-test
	./$(BUILD)/cygnus capabilities > $(BUILD)/caps.out
	grep -q "C-Bus" $(BUILD)/caps.out
	./$(BUILD)/cygnus run $(BUILD)/hello.img 10000 > $(BUILD)/guest.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest.out
	./$(BUILD)/cygnus vm $(BUILD)/hello.cvm 10000 > $(BUILD)/guest-cvm.out
	grep -q "Hello from a guest running on UN_Cygnus!" $(BUILD)/guest-cvm.out
	! ./$(BUILD)/cygnus run $(BUILD)/hello.img 100junk > $(BUILD)/invalid-limit.out 2>&1
	grep -q "invalid max-instructions" $(BUILD)/invalid-limit.out
	! ./$(BUILD)/cygnus run $(BUILD)/hello.img -1 > $(BUILD)/invalid-negative-limit.out 2>&1
	grep -q "invalid max-instructions" $(BUILD)/invalid-negative-limit.out
	! ./$(BUILD)/cygnus run $(BUILD)/hello.img 0 > $(BUILD)/invalid-zero-limit.out 2>&1
	grep -q "invalid max-instructions" $(BUILD)/invalid-zero-limit.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-memory.cvm 10000 > $(BUILD)/invalid-memory.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-memory.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-serial.cvm 10000 > $(BUILD)/invalid-serial.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-serial.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-long-line.cvm 10000 > $(BUILD)/invalid-long-line.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-long-line.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-long-boot.cvm 10000 > $(BUILD)/invalid-long-boot.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-long-boot.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-malformed-line.cvm 10000 > $(BUILD)/invalid-malformed-line.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-malformed-line.out
	! ./$(BUILD)/cygnus vm $(BUILD)/invalid-empty-key.cvm 10000 > $(BUILD)/invalid-empty-key.out 2>&1
	grep -q "invalid CVM config" $(BUILD)/invalid-empty-key.out
	./$(BUILD)/cygnus snapshot $(BUILD)/hello.img $(BUILD)/hello.cys
	test -s $(BUILD)/hello.cys
	@echo "Cygnus extensible-core smoke test passed"
clean:
	rm -rf $(BUILD)