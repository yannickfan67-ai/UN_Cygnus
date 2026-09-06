# Cygnus architecture

## Isolation model

A Cygnus `Partition` owns guest RAM, one or more virtual CPUs, a device namespace and an event queue. The management process is analogous to a privileged root partition, while guests are child partitions. This is an architectural model only; no Hyper-V code is used.

## Execution engines

1. **Soft86** — in-tree x86 interpreter. Always available; deterministic and useful for firmware/device work.
2. **VMX** — future Intel backend using VMXON/VMCS/VM entry and VM exit directly.
3. **SVM** — future AMD backend using VMCB/VMRUN directly.

A VM must not depend on which engine executes its vCPU.

## Device model

Guest port-I/O and MMIO are routed through **C-Bus**. A device registers address ranges and callbacks. Initial devices are BIOS teletype and COM1. Planned devices include i8259 PIC, PIT 8254, PCI root, IDE/AHCI/NVMe, RTL8139/e1000, framebuffer and USB HID.

## File formats

- `.cvm` — declarative machine configuration (planned)
- `.cys` — Cygnus snapshot/state image (v1 implemented)
- disk images remain raw sector-addressable media initially

## Hardware acceleration roadmap

Intel VMX support will be discovered through CPUID and VMX capability MSRs, with one VMCS per virtual CPU. AMD SVM support will use VMCBs and VMRUN. Both backends remain in-tree Cygnus implementations and preserve the same Partition/C-Bus interfaces used by Soft86.
