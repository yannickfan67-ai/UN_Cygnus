# UN_Cygnus

UN_Cygnus is the virtualization / machine-emulation branch of the UN ecosystem.
It is **not a QEMU frontend** and does not use QEMU, KVM, WHPX or another VMM as its execution engine.

## v0.1 prototype

The first executable milestone is a from-scratch x86 real-mode virtual machine:

- own CPU fetch/decode/execute loop
- own 20-bit real-mode address translation
- 1 MiB guest RAM
- own I/O bus
- COM1 output emulation
- minimal BIOS service layer (`INT 10h`, `INT 12h`, `INT 19h`)
- bootsector loading at `0000:7C00`
- first x86 instruction subset: MOV, PUSH/POP, INC/DEC, XOR, JMP/Jcc, CMP/ADD/SUB AL, IN/OUT, INT, CLI/STI, HLT
- VM snapshots (`.cys`)
- deterministic instruction-limit guard

`make test` builds a 512-byte x86 guest bootsector and executes it on Cygnus itself. The expected guest output is:

```text
Hello from a guest running on UN_Cygnus!
```

## Long-term architecture

Cygnus will grow toward a Hyper-V-like partitioned VMM while keeping its own implementation:

- **Cygnus Core**: VM/partition lifecycle and vCPU scheduling
- **CVM**: guest machine model and configuration
- **C-Bus**: synthetic high-speed guest/host device bus
- **Soft86**: portable in-tree x86 interpreter
- **Cygnus VMX**: future direct Intel VT-x backend
- **Cygnus SVM**: future direct AMD-V backend
- virtual APIC/PIC/PIT/HPET, PCI, storage, network, graphics and input
- snapshots, pause/resume, virtual switches and checkpoints

The VMX/SVM backends will call CPU virtualization extensions directly. They are not wrappers around a third-party hypervisor.

## Build

```bash
make
make test
./build/cygnus run build/hello.img
```
