#include "cygnus.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *msg){
    fprintf(stderr,"serial_attach_test: %s\n",msg);
    return 1;
}

int main(void){
    CygnusVM vm;
    memset(&vm,0,sizeof(vm));
    cygnus_bus_init(&vm.bus);

    if(!cygnus_attach_serial(&vm,0x3f8))return fail("initial attach failed");
    if(vm.bus.device_count!=1||vm.bus.io_count!=1)return fail("initial attach counts wrong");

    if(cygnus_attach_serial(&vm,0x3f8))return fail("overlapping attach unexpectedly succeeded");
    if(vm.bus.device_count!=1||vm.bus.io_count!=1)return fail("failed overlapping attach changed bus state");

    if(cygnus_attach_serial(&vm,0xfffc))return fail("wrapping port range unexpectedly succeeded");
    if(vm.bus.device_count!=1||vm.bus.io_count!=1)return fail("failed wrapping attach changed bus state");

    cygnus_bus_destroy(&vm);
    if(vm.bus.device_count||vm.bus.io_count)return fail("destroy did not clear bus");
    puts("serial attach rollback test passed");
    return 0;
}
