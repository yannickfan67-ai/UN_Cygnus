#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint16_t base;
    uint8_t regs[8];
} SerialState;

static int ser_reset(CygnusVM *vm,CygnusDevice *dev){
    (void)vm;
    SerialState *s=dev->state;
    for(int i=0;i<8;i++)s->regs[i]=0;
    s->regs[5]=0x60;
    return 1;
}
static int ser_read(CygnusVM *vm,CygnusDevice *dev,uint16_t port,unsigned width,uint32_t *value){
    (void)vm;
    SerialState *s=dev->state;
    if(width!=1||port<s->base||port>s->base+7)return 0;
    *value=s->regs[port-s->base];
    return 1;
}
static int ser_write(CygnusVM *vm,CygnusDevice *dev,uint16_t port,unsigned width,uint32_t value){
    (void)vm;
    SerialState *s=dev->state;
    if(width!=1||port<s->base||port>s->base+7)return 0;
    if(port==s->base){fputc((int)(value&0xff),stdout);fflush(stdout);}
    else s->regs[port-s->base]=(uint8_t)value;
    return 1;
}
static void ser_destroy(CygnusVM *vm,CygnusDevice *dev){
    (void)vm;
    free(dev->state);
    dev->state=NULL;
}
static const CygnusDeviceOps OPS={
    CYGNUS_DEVICE_ABI_VERSION,
    "uart16550-lite",
    0,
    ser_reset,
    ser_read,
    ser_write,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ser_destroy
};

int cygnus_attach_serial(CygnusVM *vm,uint16_t base){
    SerialState *s=calloc(1,sizeof(*s));
    if(!s)return 0;
    s->base=base;
    CygnusDevice *dev=cygnus_bus_add_device(vm,&OPS,"com1",s);
    if(!dev){free(s);return 0;}
    if(!cygnus_bus_register_io(vm,dev,base,(uint16_t)(base+7)))return 0;
    return 1;
}
