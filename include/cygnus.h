#ifndef CYGNUS_H
#define CYGNUS_H
#include <stdint.h>
#include <stddef.h>

#define CYGNUS_VERSION "0.2.0-dev"
#define CYGNUS_API_VERSION 0x00010001u
#define CYGNUS_DEVICE_ABI_VERSION 0x00010000u
#define CYGNUS_RAM_DEFAULT (1024u * 1024u)
#define CYGNUS_MAX_SNAPSHOT_NAME 64
#define CYGNUS_MAX_DEVICES 32
#define CYGNUS_MAX_IO_REGIONS 64
#define CYGNUS_MAX_MMIO_REGIONS 64

#define CYGNUS_CAP_SOFT86       (1ull << 0)
#define CYGNUS_CAP_SNAPSHOT     (1ull << 1)
#define CYGNUS_CAP_CBUS         (1ull << 2)
#define CYGNUS_CAP_CVM_CONFIG   (1ull << 3)
#define CYGNUS_CAP_VMEXIT       (1ull << 4)
#define CYGNUS_CAP_IRQ_FABRIC   (1ull << 5)
#define CYGNUS_CAP_VMX_RESERVED (1ull << 16)
#define CYGNUS_CAP_SVM_RESERVED (1ull << 17)

struct CygnusVM;
struct CygnusDevice;

typedef enum {
    CYGNUS_VM_CREATED = 0,
    CYGNUS_VM_READY,
    CYGNUS_VM_RUNNING,
    CYGNUS_VM_PAUSED,
    CYGNUS_VM_HALTED,
    CYGNUS_VM_FAILED
} CygnusVMState;

typedef enum {
    CYGNUS_EXIT_NONE = 0,
    CYGNUS_EXIT_HLT,
    CYGNUS_EXIT_IO,
    CYGNUS_EXIT_MMIO,
    CYGNUS_EXIT_CPUID,
    CYGNUS_EXIT_MSR,
    CYGNUS_EXIT_EXCEPTION,
    CYGNUS_EXIT_INTERRUPT_WINDOW,
    CYGNUS_EXIT_BIOS,
    CYGNUS_EXIT_BUDGET,
    CYGNUS_EXIT_SHUTDOWN,
    CYGNUS_EXIT_ERROR
} CygnusVmExitReason;

typedef struct {
    CygnusVmExitReason reason;
    uint64_t guest_pc;
    uint64_t instructions;
    uint64_t address;
    uint64_t value;
    uint32_t error_code;
    uint16_t port;
    uint8_t width;
    uint8_t is_write;
    uint8_t vector;
    uint8_t reserved[5];
} CygnusVmExit;

typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp;
    uint16_t cs,ds,es,ss,ip,flags;
    int halted;
} CygnusCPU;

typedef struct CygnusCpuBackendOps {
    uint32_t api_version;
    const char *name;
    uint64_t capabilities;
    int (*reset)(struct CygnusVM *vm);
    int (*run)(struct CygnusVM *vm,uint64_t max_instructions);
    void (*destroy)(struct CygnusVM *vm);
    /* API 1.1+: bounded execution with a backend-neutral exit record. */
    int (*run_slice)(struct CygnusVM *vm,uint64_t budget,CygnusVmExit *exit_info);
    /* API 1.1+: inject a resolved guest interrupt vector. */
    int (*inject_irq)(struct CygnusVM *vm,uint8_t vector);
} CygnusCpuBackendOps;

typedef struct CygnusDeviceOps {
    uint32_t abi_version;
    const char *type;
    uint64_t capabilities;
    int (*reset)(struct CygnusVM *vm,struct CygnusDevice *dev);
    int (*io_read)(struct CygnusVM *vm,struct CygnusDevice *dev,uint16_t port,unsigned width,uint32_t *value);
    int (*io_write)(struct CygnusVM *vm,struct CygnusDevice *dev,uint16_t port,unsigned width,uint32_t value);
    int (*mmio_read)(struct CygnusVM *vm,struct CygnusDevice *dev,uint64_t addr,unsigned width,uint64_t *value);
    int (*mmio_write)(struct CygnusVM *vm,struct CygnusDevice *dev,uint64_t addr,unsigned width,uint64_t value);
    void (*tick)(struct CygnusVM *vm,struct CygnusDevice *dev,uint64_t guest_cycles);
    int (*save_state)(struct CygnusVM *vm,struct CygnusDevice *dev,void *dst,size_t cap,size_t *used);
    int (*load_state)(struct CygnusVM *vm,struct CygnusDevice *dev,const void *src,size_t len);
    void (*destroy)(struct CygnusVM *vm,struct CygnusDevice *dev);
} CygnusDeviceOps;

typedef struct CygnusDevice {
    const CygnusDeviceOps *ops;
    void *state;
    char name[32];
    uint32_t instance_id;
} CygnusDevice;

typedef struct {
    uint16_t first;
    uint16_t last;
    CygnusDevice *device;
} CygnusIoRegion;

typedef struct {
    uint64_t first;
    uint64_t last;
    CygnusDevice *device;
} CygnusMmioRegion;

typedef struct {
    CygnusDevice devices[CYGNUS_MAX_DEVICES];
    size_t device_count;
    CygnusIoRegion io_regions[CYGNUS_MAX_IO_REGIONS];
    size_t io_count;
    CygnusMmioRegion mmio_regions[CYGNUS_MAX_MMIO_REGIONS];
    size_t mmio_count;
} CygnusBus;

/* Backend-neutral interrupt-line staging. Controllers such as 8259/APIC will
 * translate device IRQ lines into vectors before backend injection. */
typedef struct {
    uint64_t pending[4];
    uint64_t asserted[4];
} CygnusIrqFabric;

typedef struct {
    char name[64];
    char backend[24];
    char boot_path[512];
    size_t ram_size;
    int serial_enabled;
} CygnusVMConfig;

typedef struct CygnusVM {
    uint8_t *ram;
    size_t ram_size;
    CygnusCPU cpu;
    uint64_t instructions;
    CygnusVMState state;
    const CygnusCpuBackendOps *backend;
    CygnusBus bus;
    CygnusIrqFabric irq;
    char name[64];
} CygnusVM;

uint32_t cygnus_api_version(void);
uint64_t cygnus_capabilities(void);
const CygnusCpuBackendOps *cygnus_backend_soft86(void);

int cygnus_vm_init(CygnusVM *vm,size_t ram_size);
int cygnus_vm_init_config(CygnusVM *vm,const CygnusVMConfig *cfg);
void cygnus_vm_destroy(CygnusVM *vm);
int cygnus_vm_reset(CygnusVM *vm);
int cygnus_vm_load_bootsector(CygnusVM *vm,const char *path);
int cygnus_vm_run(CygnusVM *vm,uint64_t max_instructions);
int cygnus_vm_run_slice(CygnusVM *vm,uint64_t budget,CygnusVmExit *exit_info);
int cygnus_vm_inject_irq(CygnusVM *vm,uint8_t vector);
int cygnus_vm_snapshot_save(const CygnusVM *vm,const char *path);
int cygnus_vm_snapshot_load(CygnusVM *vm,const char *path);
uint32_t cygnus_linear(uint16_t seg,uint16_t off);

void cygnus_bus_init(CygnusBus *bus);
CygnusDevice *cygnus_bus_add_device(CygnusVM *vm,const CygnusDeviceOps *ops,const char *name,void *state);
int cygnus_bus_register_io(CygnusVM *vm,CygnusDevice *dev,uint16_t first,uint16_t last);
int cygnus_bus_register_mmio(CygnusVM *vm,CygnusDevice *dev,uint64_t first,uint64_t last);
int cygnus_bus_io_read(CygnusVM *vm,uint16_t port,unsigned width,uint32_t *value);
int cygnus_bus_io_write(CygnusVM *vm,uint16_t port,unsigned width,uint32_t value);
int cygnus_bus_mmio_read(CygnusVM *vm,uint64_t addr,unsigned width,uint64_t *value);
int cygnus_bus_mmio_write(CygnusVM *vm,uint64_t addr,unsigned width,uint64_t value);
void cygnus_bus_tick(CygnusVM *vm,uint64_t guest_cycles);
void cygnus_bus_destroy(CygnusVM *vm);

void cygnus_irq_init(CygnusIrqFabric *irq);
int cygnus_irq_raise(CygnusVM *vm,uint8_t line);
int cygnus_irq_lower(CygnusVM *vm,uint8_t line);
int cygnus_irq_next(CygnusVM *vm,uint8_t *line);

int cygnus_attach_serial(CygnusVM *vm,uint16_t base_port);

void cygnus_config_defaults(CygnusVMConfig *cfg);
int cygnus_config_load(const char *path,CygnusVMConfig *cfg);

#endif
