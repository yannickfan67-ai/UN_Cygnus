#include "cygnus.h"
#include <stdio.h>
#include <string.h>

static int io_reads;
static int io_writes;
static int mmio_reads;
static int mmio_writes;

static int io_read(CygnusVM *vm,CygnusDevice *dev,uint16_t port,unsigned width,uint32_t *value){
    (void)vm;(void)dev;(void)port;(void)width;
    io_reads++;
    *value=0x12;
    return 1;
}

static int io_write(CygnusVM *vm,CygnusDevice *dev,uint16_t port,unsigned width,uint32_t value){
    (void)vm;(void)dev;(void)port;(void)width;(void)value;
    io_writes++;
    return 1;
}

static int mmio_read(CygnusVM *vm,CygnusDevice *dev,uint64_t addr,unsigned width,uint64_t *value){
    (void)vm;(void)dev;(void)addr;(void)width;
    mmio_reads++;
    *value=0x34;
    return 1;
}

static int mmio_write(CygnusVM *vm,CygnusDevice *dev,uint64_t addr,unsigned width,uint64_t value){
    (void)vm;(void)dev;(void)addr;(void)width;(void)value;
    mmio_writes++;
    return 1;
}

static const CygnusDeviceOps OPS={
    .abi_version=CYGNUS_DEVICE_ABI_VERSION,
    .type="width-test",
    .io_read=io_read,
    .io_write=io_write,
    .mmio_read=mmio_read,
    .mmio_write=mmio_write,
};

int main(void){
    CygnusVM vm;
    uint32_t io_value=0;
    uint64_t mmio_value=0;
    memset(&vm,0,sizeof(vm));
    cygnus_bus_init(&vm.bus);

    CygnusDevice *dev=cygnus_bus_add_device(&vm,&OPS,"width",NULL);
    if(!dev||!cygnus_bus_register_io(&vm,dev,0x300,0x30f)||!cygnus_bus_register_mmio(&vm,dev,0x1000,0x10ff)){
        fputs("failed to set up width test device\n",stderr);
        return 1;
    }

    if(cygnus_bus_io_read(&vm,0x300,3,&io_value)||cygnus_bus_io_write(&vm,0x300,0,0)){
        fputs("invalid port-I/O width was accepted\n",stderr);
        return 1;
    }
    if(cygnus_bus_mmio_read(&vm,0x1000,3,&mmio_value)||cygnus_bus_mmio_write(&vm,0x1000,16,0)){
        fputs("invalid MMIO width was accepted\n",stderr);
        return 1;
    }
    if(io_reads||io_writes||mmio_reads||mmio_writes){
        fputs("invalid width reached a device callback\n",stderr);
        return 1;
    }

    if(!cygnus_bus_io_read(&vm,0x300,1,&io_value)||io_value!=0x12||io_reads!=1||
       !cygnus_bus_io_write(&vm,0x300,4,0x12345678)||io_writes!=1||
       !cygnus_bus_mmio_read(&vm,0x1000,8,&mmio_value)||mmio_value!=0x34||mmio_reads!=1||
       !cygnus_bus_mmio_write(&vm,0x1000,2,0x55aa)||mmio_writes!=1){
        fputs("valid width did not reach device callback\n",stderr);
        return 1;
    }

    io_value=0;
    mmio_value=0;
    if(!cygnus_bus_io_read(&vm,0x30e,4,&io_value)||io_value!=0xffffffffu||
       !cygnus_bus_io_write(&vm,0x30e,4,0x12345678)||
       !cygnus_bus_mmio_read(&vm,0x10fc,8,&mmio_value)||mmio_value!=~0ull||
       !cygnus_bus_mmio_write(&vm,0x10fc,8,0x1122334455667788ull)){
        fputs("cross-boundary access was not handled as unmapped\n",stderr);
        return 1;
    }
    if(io_reads!=1||io_writes!=1||mmio_reads!=1||mmio_writes!=1){
        fputs("cross-boundary access reached a device callback\n",stderr);
        return 1;
    }

    if(cygnus_bus_io_read(&vm,0x400,3,&io_value)||cygnus_bus_mmio_read(&vm,0x2000,3,&mmio_value)){
        fputs("unmapped access accepted an invalid width\n",stderr);
        return 1;
    }

    puts("bus width validation test passed");
    return 0;
}
