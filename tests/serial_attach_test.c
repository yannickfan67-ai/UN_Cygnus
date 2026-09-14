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

    if(!cygnus_bus_io_write(&vm,0x3fb,1,0x80))return fail("failed to enable DLAB");
    if(!cygnus_bus_io_write(&vm,0x3f8,1,0x34))return fail("failed to write DLL");
    if(!cygnus_bus_io_write(&vm,0x3f9,1,0x12))return fail("failed to write DLM");
    uint32_t value=0;
    if(!cygnus_bus_io_read(&vm,0x3f8,1,&value)||value!=0x34)return fail("DLL did not round-trip");
    if(!cygnus_bus_io_read(&vm,0x3f9,1,&value)||value!=0x12)return fail("DLM did not round-trip");

    if(!cygnus_bus_io_write(&vm,0x3fb,1,0x00))return fail("failed to disable DLAB");
    if(!cygnus_bus_io_read(&vm,0x3f9,1,&value)||value!=0x00)return fail("DLM leaked into IER");
    if(!cygnus_bus_io_write(&vm,0x3f9,1,0x05))return fail("failed to write IER");
    if(!cygnus_bus_io_write(&vm,0x3fb,1,0x80))return fail("failed to re-enable DLAB");
    if(!cygnus_bus_io_read(&vm,0x3f8,1,&value)||value!=0x34)return fail("DLL changed after IER access");
    if(!cygnus_bus_io_read(&vm,0x3f9,1,&value)||value!=0x12)return fail("DLM changed after IER access");
    if(!cygnus_bus_io_write(&vm,0x3fb,1,0x00))return fail("failed to restore LCR");
    if(!cygnus_bus_io_read(&vm,0x3f9,1,&value)||value!=0x05)return fail("IER did not survive DLAB access");

    cygnus_bus_destroy(&vm);
    if(vm.bus.device_count||vm.bus.io_count)return fail("destroy did not clear bus");
    puts("serial attach and DLAB test passed");
    return 0;
}
