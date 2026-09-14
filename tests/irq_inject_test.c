#include "cygnus.h"
#include <stdio.h>
#include <string.h>

#define IF 0x0200

static int expect(int condition,const char *message){
    if(condition)return 1;
    fprintf(stderr,"irq_inject_test: %s\n",message);
    return 0;
}

static void put16(CygnusVM *vm,uint32_t addr,uint16_t value){
    vm->ram[addr]=(uint8_t)value;
    vm->ram[addr+1]=(uint8_t)(value>>8);
}

static uint16_t get16(const CygnusVM *vm,uint32_t addr){
    return (uint16_t)(vm->ram[addr]|((uint16_t)vm->ram[addr+1]<<8));
}

int main(void){
    CygnusVM vm;
    if(!cygnus_vm_init(&vm,4096))return 1;

    vm.cpu.flags=0x0202;
    vm.cpu.cs=0x1111;
    vm.cpu.ip=0x2222;
    vm.cpu.ss=0;
    vm.cpu.sp=2;
    vm.cpu.halted=1;
    put16(&vm,0x80,0x3456);
    put16(&vm,0x82,0x1234);

    CygnusCPU before=vm.cpu;
    if(!expect(!cygnus_vm_inject_irq(&vm,0x20),"out-of-range interrupt frame was accepted"))return 1;
    if(!expect(memcmp(&vm.cpu,&before,sizeof(before))==0,"failed injection changed CPU state"))return 1;
    cygnus_vm_destroy(&vm);

    if(!cygnus_vm_init(&vm,65536))return 1;
    vm.cpu.flags=0x0202;
    vm.cpu.cs=0x1111;
    vm.cpu.ip=0x2222;
    vm.cpu.ss=0;
    vm.cpu.sp=0x0100;
    vm.cpu.halted=1;
    put16(&vm,0x80,0x3456);
    put16(&vm,0x82,0x1234);

    if(!expect(cygnus_vm_inject_irq(&vm,0x20),"valid interrupt injection failed"))return 1;
    if(!expect(vm.cpu.sp==0x00fa,"valid injection produced wrong SP"))return 1;
    if(!expect(vm.cpu.cs==0x1234&&vm.cpu.ip==0x3456,"valid injection loaded wrong vector"))return 1;
    if(!expect(!(vm.cpu.flags&IF),"valid injection did not clear IF"))return 1;
    if(!expect(!vm.cpu.halted,"valid injection did not resume halted CPU"))return 1;
    if(!expect(get16(&vm,0x00fa)==0x2222,"saved IP is wrong"))return 1;
    if(!expect(get16(&vm,0x00fc)==0x1111,"saved CS is wrong"))return 1;
    if(!expect(get16(&vm,0x00fe)==0x0202,"saved FLAGS is wrong"))return 1;

    cygnus_vm_destroy(&vm);
    return 0;
}
