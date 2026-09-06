# Cygnus architecture

## Isolation model

A Cygnus `Partition` will own guest RAM, one or more virtual CPUs, a C-Bus device namespace and an event queue. The management process is analogous to a privileged root partition, while guests are child partitions. This is an architectural model only; no Hyper-V code is used.

## Layering

```text
Cygnus Manager / API
        |
   VM / Partition Core
    /            \
CPU Backend      C-Bus
Soft86           |-- port I/O
VMX (reserved)   |-- MMIO
SVM (reserved)   |-- future PCI/synthetic channels
                 |
             Virtual Devices
```

No virtual device may reach into a CPU backend. No CPU backend may special-case a device. VM exits and interpreter accesses converge on C-Bus.

## Execution engines

1. **Soft86** — in-tree x86 interpreter. Always available; deterministic and useful for firmware/device work.
2. **VMX** — reserved Intel backend using VMXON/VMCS/VM entry/VM exit directly.
3. **SVM** — reserved AMD backend using VMCB/VMRUN directly.
4. **ARM64/EL2** — future architecture port after the machine/device interfaces stabilize.

A VM must not depend on which engine executes its vCPU.

## Device model

Guest port-I/O and MMIO are routed through **C-Bus**. Devices register non-overlapping address ranges and callbacks. COM1 already uses this path. Planned devices include i8259 PIC, PIT 8254, PCI root, IDE/AHCI/NVMe, RTL8139/e1000, framebuffer and USB HID.

## Management formats

- `.cvm` — declarative machine configuration; development parser implemented
- `.cys` — Cygnus snapshot/state image; v1 CPU + RAM implemented
- raw disk images — sector-addressable media initially

## Compatibility rule

Public API/ABI versions are explicit. New optional callbacks and capability bits are appended. The VM core should preserve old device modules wherever practical instead of forcing every driver/device model to be rewritten on each release.
