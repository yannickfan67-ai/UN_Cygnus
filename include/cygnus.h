#ifndef CYGNUS_H
#define CYGNUS_H
#include <stdint.h>
#include <stddef.h>

#define CYGNUS_VERSION "0.2.0-dev"
#define CYGNUS_API_VERSION 0x00010000u
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

int cygnus_attach_serial(CygnusVM *vm,uint16_t base_port);

void cygnus_config_defaults(CygnusVMConfig *cfg);
int cygnus_config_load(const char *path,CygnusVMConfig *cfg);

#endif
