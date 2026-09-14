#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>

static int reset_calls;
static int reset_ok=1;

static int test_reset(CygnusVM *vm,CygnusDevice *dev){
    (void)vm;
    (void)dev;
    reset_calls++;
    return reset_ok;
}

static const CygnusDeviceOps OPS={
    .abi_version=CYGNUS_DEVICE_ABI_VERSION,
    .type="reset-test",
    .reset=test_reset
};

int main(void){
    CygnusVM vm;
    if(!cygnus_vm_init(&vm,0x10000))return 1;

    if(!cygnus_bus_add_device(&vm,&OPS,"reset-test",NULL)){
        cygnus_vm_destroy(&vm);
        return 2;
    }
    reset_calls=0;
    vm.cpu.ax=0x1234;
    vm.instructions=77;
    if(!cygnus_vm_reset(&vm)){
        cygnus_vm_destroy(&vm);
        return 3;
    }
    if(reset_calls!=1||vm.cpu.ax!=0||vm.instructions!=0||vm.state!=CYGNUS_VM_READY){
        cygnus_vm_destroy(&vm);
        return 4;
    }

    reset_ok=0;
    if(cygnus_vm_reset(&vm)){
        cygnus_vm_destroy(&vm);
        return 5;
    }
    if(vm.state!=CYGNUS_VM_FAILED){
        cygnus_vm_destroy(&vm);
        return 6;
    }

    cygnus_vm_destroy(&vm);
    puts("vm reset device coverage passed");
    return 0;
}
