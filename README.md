# UN_Cygnus

UN_Cygnus is the virtualization / machine-emulation branch of the UN ecosystem. It is a **from-scratch VMM**, not a QEMU frontend, and does not use QEMU, KVM, WHPX or another VMM as its execution engine.

## Current milestone — 0.2.0-dev

The executable core can already boot and execute a small 16-bit x86 guest using Cygnus itself:

- own x86 real-mode fetch/decode/execute loop (**Soft86**)
- own 20-bit real-mode address translation
- 1 MiB guest RAM
- **C-Bus** port-I/O + MMIO router
- pluggable `CygnusDeviceOps` device ABI
- COM1/16550-lite device implemented through C-Bus
- minimal BIOS service layer (`INT 10h`, `INT 12h`, `INT 19h`)
- bootsector loading at `0000:7C00`
- MOV, PUSH/POP, INC/DEC, XOR, JMP/Jcc, CMP/ADD/SUB AL, IN/OUT, INT, CLI/STI, HLT
- VM snapshots (`.cys` v1: CPU + RAM)
- `.cvm` declarative VM configuration
- CPU backend ABI with Soft86 implemented and direct VMX/SVM slots reserved
- explicit API/device ABI versions and capability bits

`make test` exercises both the legacy image command and the new `.cvm` machine path. The guest prints:

```text
Hello from a guest running on UN_Cygnus!
```

## Stable expansion seams

Cygnus reserves extension interfaces before the machine model grows:

- `CygnusCpuBackendOps` — Soft86 / future direct Intel VMX / AMD SVM / ARM64 EL2
- `CygnusDeviceOps` — reset, port-I/O, MMIO, tick, save/load state, destroy
- `C-Bus` — non-overlapping port-I/O and MMIO region routing
- `.cvm` — VM/machine configuration
- `.cys` — checkpoint format, with per-device state callbacks already reserved for v2
- capability flags — feature discovery without hard-coding a specific backend/device

See `docs/EXTENSION_API.md` and `docs/ARCHITECTURE.md`.

## Long-term architecture

- **Cygnus Core** — VM/partition lifecycle and vCPU scheduling
- **CVM** — guest machine model and configuration
- **C-Bus** — virtual device/address-space fabric
- **C-Net / C-GPU / C-Input / C-Storage** — future synthetic high-speed devices
- **Soft86** — portable deterministic CPU backend
- **Cygnus VMX** — future direct Intel VT-x backend
- **Cygnus SVM** — future direct AMD-V backend
- virtual PIC/APIC/PIT/HPET, PCIe, storage, networking, graphics, USB/input
- virtual switches, checkpoints, deterministic tracing and debugger

The VM/device model is deliberately independent from the execution engine, so adding hardware acceleration later does not require rewriting the virtual motherboard.

## Build

```bash
make
make test
./build/cygnus capabilities
./build/cygnus run build/hello.img
./build/cygnus vm build/hello.cvm
```
