# Cygnus Extension API v1

Cygnus reserves stable seams before the device set becomes large. The rule is: **machine policy must not depend on an execution backend or a specific device implementation**.

## Versioning

`CYGNUS_API_VERSION` and `CYGNUS_DEVICE_ABI_VERSION` are encoded as `0xMMMMmmmm` (major/minor). Major changes may break ABI. Minor changes only append capabilities or optional callbacks.

Current core API: **1.1** (`0x00010001`). Device ABI remains **1.0**.

## CPU backend ABI

`CygnusCpuBackendOps` isolates vCPU execution from the VM object. `soft86` is the first implementation. The same VM/device model is reserved for direct `vmx` and `svm` backends later.

The compatibility `run()` callback remains available. API 1.1 adds two append-only callbacks:

- `run_slice(vm, budget, exit)` — execute a bounded slice and return a backend-neutral `CygnusVmExit`
- `inject_irq(vm, vector)` — inject a resolved interrupt vector without exposing backend internals

`CygnusVmExitReason` already reserves the common classes required by future hardware virtualization:

- HLT
- port I/O
- MMIO
- CPUID
- MSR
- exception
- interrupt window
- BIOS/service exit
- instruction budget
- shutdown
- backend error

Soft86 currently handles its own emulated port-I/O through C-Bus and normally surfaces HLT, budget or error exits. A future VMX/SVM backend can surface raw I/O/MMIO exits and let the same VM core dispatch them to C-Bus.

## IRQ fabric

`CygnusIrqFabric` provides 256 backend-neutral pending/asserted lines. Devices raise/lower lines without calling a CPU backend. An interrupt controller (8259 PIC, IOAPIC/LAPIC or a future synthetic controller) resolves a line to a guest vector, then the VM calls `inject_irq()`.

This keeps device IRQ generation independent from Soft86, VMX, SVM or a future ARM64 backend.

## Device ABI

Every virtual device exposes `CygnusDeviceOps` and may implement:

- reset
- port-I/O read/write
- MMIO read/write
- tick
- save/load state
- destroy

Devices are registered on **C-Bus**, not called directly by the CPU engine. This lets software emulation, VMX and SVM share one virtual motherboard.

## Address spaces

C-Bus v1 has two routable spaces:

- x86 port I/O: 16-bit ports
- MMIO: 64-bit guest physical addresses

Regions cannot overlap. Future buses (PCI configuration, USB, I2C, synthetic C-Bus channels) layer on top rather than bypassing the router.

## `.cvm` machine configuration

The development v1 parser intentionally uses a small `key=value` format:

```ini
name=Development VM
backend=soft86
memory=1048576
boot=guest.img
serial=on
```

Unknown keys are ignored for forward compatibility. A future schema will add machine type, vCPU count, firmware, storage controllers, network/switches, display, input, TPM and passthrough policy.

## Snapshot contract

`.cys` exists today for CPU + RAM. Device callbacks `save_state/load_state` are reserved now so snapshot v2 can include tagged per-device state without changing device implementations later.

## Planned extension points

- CPU: soft86, VMX, SVM, ARM64 interpreter/EL2
- interrupt fabric: PIC, IOAPIC, LAPIC, MSI/MSI-X
- clock/timers: PIT, RTC, HPET, APIC timer
- buses: PCI/PCIe ECAM, USB, synthetic C-Bus
- storage: IDE, AHCI, NVMe, Cygnus synthetic storage
- networking: RTL8139/e1000 compatibility, C-Net synthetic NIC, virtual switch
- graphics: VGA compatibility, linear framebuffer, C-GPU synthetic display
- input: PS/2, USB HID, C-Input synthetic input
- firmware: minimal BIOS, UEFI-compatible firmware path
- debugging: breakpoints, trace, register/memory inspection, deterministic replay
