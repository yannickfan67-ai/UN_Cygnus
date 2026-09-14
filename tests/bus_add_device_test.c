#include "cygnus.h"
#include <stdio.h>
#include <string.h>

static int tick_calls;
static int destroy_calls;

static int fail_reset(CygnusVM *vm,CygnusDevice *dev){
    (void)vm;
    (void)dev;
    return 0;
}

static void count_tick(CygnusVM *vm,CygnusDevice *dev,uint64_t guest_cycles){
    (void)vm;
    (void)dev;
    (void)guest_cycles;
    tick_calls++;
}

static void count_destroy(CygnusVM *vm,CygnusDevice *dev){
    (void)vm;
    (void)dev;
    destroy_calls++;
}

static const CygnusDeviceOps FAILING_OPS={
    .abi_version=CYGNUS_DEVICE_ABI_VERSION,
    .type="failing-test-device",
    .reset=fail_reset,
    .tick=count_tick,
    .destroy=count_destroy,
};

int main(void){
    CygnusVM vm;
    memset(&vm,0,sizeof(vm));
    cygnus_bus_init(&vm.bus);

    if(cygnus_bus_add_device(&vm,&FAILING_OPS,"fail",NULL)!=NULL){
        fputs("failing reset unexpectedly added device\n",stderr);
        return 1;
    }
    if(vm.bus.device_count!=0){
        fputs("failed device remained registered\n",stderr);
        return 1;
    }

    cygnus_bus_tick(&vm,1);
    cygnus_bus_destroy(&vm);
    if(tick_calls!=0||destroy_calls!=0){
        fputs("failed device still received bus callbacks\n",stderr);
        return 1;
    }

    puts("bus add-device rollback test passed");
    return 0;
}
