# UN_Cygnus roadmap

## Soft86
- grow 16-bit instruction coverage enough for real option ROM / DOS-class boot code
- ModRM memory addressing, segment overrides and string instructions
- 32-bit 386 protected mode
- paging and x86_64 long mode after protected mode is stable
- deterministic tracing, breakpoints and register/memory debugger

## Virtual motherboard
- i8259 PIC, PIT 8254 and RTC
- PCI configuration space / PCI root complex
- IDE first, then AHCI/NVMe virtual storage
- VGA/linear framebuffer and PS/2/USB HID
- RTL8139/e1000 compatibility NICs
- APIC/IOAPIC/MSI after protected-mode guests are viable

## C-Bus synthetic devices
- C-Storage, C-Net, C-GPU and C-Input
- shared-ring protocol for hardware-accelerated backends
- versioned guest integration interface
- virtual switch and VM-to-VM networking

## Hardware execution backends
- direct Intel VMX backend using Cygnus' own VMXON/VMCS code
- direct AMD SVM backend using Cygnus' own VMCB/VMRUN code
- no QEMU/KVM/WHPX dependency in the Cygnus execution path
- identical C-Bus/device model across Soft86, VMX and SVM

## Management
- VM inventory and lifecycle service
- checkpoints, pause/resume and clone
- `.cvm` schema expansion for vCPU, disks, network, firmware and display
- future Cygnus Manager GUI inside UN_Orion
