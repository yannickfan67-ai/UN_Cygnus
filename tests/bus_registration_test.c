#include "cygnus.h"
#include <stdio.h>
#include <string.h>

static const CygnusDeviceOps OPS = {
    CYGNUS_DEVICE_ABI_VERSION,
    "registration-test",
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int main(void){
    CygnusVM vm;
    CygnusDevice foreign;
    memset(&vm,0,sizeof(vm));
    memset(&foreign,0,sizeof(foreign));
    cygnus_bus_init(&vm.bus);

    if(cygnus_bus_register_io(&vm,&foreign,0x300,0x307)){
        fprintf(stderr,"foreign I/O device was accepted\n");
        return 1;
    }
    if(cygnus_bus_register_mmio(&vm,&foreign,0x1000,0x1fff)){
        fprintf(stderr,"foreign MMIO device was accepted\n");
        return 1;
    }
    if(vm.bus.io_count!=0||vm.bus.mmio_count!=0){
        fprintf(stderr,"failed registrations changed region counts\n");
        return 1;
    }

    CygnusDevice *dev=cygnus_bus_add_device(&vm,&OPS,"registered",NULL);
    if(!dev){
        fprintf(stderr,"failed to add valid device\n");
        return 1;
    }
    if(!cygnus_bus_register_io(&vm,dev,0x300,0x307)){
        fprintf(stderr,"valid I/O device was rejected\n");
        return 1;
    }
    if(!cygnus_bus_register_mmio(&vm,dev,0x1000,0x1fff)){
        fprintf(stderr,"valid MMIO device was rejected\n");
        return 1;
    }

    cygnus_bus_destroy(&vm);
    return 0;
}
