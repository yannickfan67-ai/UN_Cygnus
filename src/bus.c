#include "cygnus.h"
#include <string.h>

void cygnus_bus_init(CygnusBus *bus){
    if(bus)memset(bus,0,sizeof(*bus));
}

CygnusDevice *cygnus_bus_add_device(CygnusVM *vm,const CygnusDeviceOps *ops,const char *name,void *state){
    if(!vm||!ops||ops->abi_version!=CYGNUS_DEVICE_ABI_VERSION||vm->bus.device_count>=CYGNUS_MAX_DEVICES)return NULL;
    CygnusDevice *d=&vm->bus.devices[vm->bus.device_count];
    memset(d,0,sizeof(*d));
    d->ops=ops;
    d->state=state;
    d->instance_id=(uint32_t)vm->bus.device_count++;
    if(name){
        size_t i=0;
        for(;name[i]&&i+1<sizeof(d->name);i++)d->name[i]=name[i];
        d->name[i]=0;
    }
    if(d->ops->reset&&!d->ops->reset(vm,d))return NULL;
    return d;
}

int cygnus_bus_register_io(CygnusVM *vm,CygnusDevice *dev,uint16_t first,uint16_t last){
    if(!vm||!dev||first>last||vm->bus.io_count>=CYGNUS_MAX_IO_REGIONS)return 0;
    for(size_t i=0;i<vm->bus.io_count;i++){
        if(!(last<vm->bus.io_regions[i].first||first>vm->bus.io_regions[i].last))return 0;
    }
    CygnusIoRegion *r=&vm->bus.io_regions[vm->bus.io_count++];
    r->first=first;r->last=last;r->device=dev;
    return 1;
}

int cygnus_bus_register_mmio(CygnusVM *vm,CygnusDevice *dev,uint64_t first,uint64_t last){
    if(!vm||!dev||first>last||vm->bus.mmio_count>=CYGNUS_MAX_MMIO_REGIONS)return 0;
    for(size_t i=0;i<vm->bus.mmio_count;i++){
        if(!(last<vm->bus.mmio_regions[i].first||first>vm->bus.mmio_regions[i].last))return 0;
    }
    CygnusMmioRegion *r=&vm->bus.mmio_regions[vm->bus.mmio_count++];
    r->first=first;r->last=last;r->device=dev;
    return 1;
}

int cygnus_bus_io_read(CygnusVM *vm,uint16_t port,unsigned width,uint32_t *value){
    if(!vm||!value)return 0;
    for(size_t i=0;i<vm->bus.io_count;i++){
        CygnusIoRegion *r=&vm->bus.io_regions[i];
        if(port>=r->first&&port<=r->last&&r->device->ops->io_read)
            return r->device->ops->io_read(vm,r->device,port,width,value);
    }
    *value=width==1?0xffu:width==2?0xffffu:0xffffffffu;
    return 1;
}

int cygnus_bus_io_write(CygnusVM *vm,uint16_t port,unsigned width,uint32_t value){
    if(!vm)return 0;
    for(size_t i=0;i<vm->bus.io_count;i++){
        CygnusIoRegion *r=&vm->bus.io_regions[i];
        if(port>=r->first&&port<=r->last&&r->device->ops->io_write)
            return r->device->ops->io_write(vm,r->device,port,width,value);
    }
    return 1;
}

int cygnus_bus_mmio_read(CygnusVM *vm,uint64_t addr,unsigned width,uint64_t *value){
    if(!vm||!value)return 0;
    for(size_t i=0;i<vm->bus.mmio_count;i++){
        CygnusMmioRegion *r=&vm->bus.mmio_regions[i];
        if(addr>=r->first&&addr<=r->last&&r->device->ops->mmio_read)
            return r->device->ops->mmio_read(vm,r->device,addr,width,value);
    }
    *value=~0ull;
    return 1;
}

int cygnus_bus_mmio_write(CygnusVM *vm,uint64_t addr,unsigned width,uint64_t value){
    if(!vm)return 0;
    for(size_t i=0;i<vm->bus.mmio_count;i++){
        CygnusMmioRegion *r=&vm->bus.mmio_regions[i];
        if(addr>=r->first&&addr<=r->last&&r->device->ops->mmio_write)
            return r->device->ops->mmio_write(vm,r->device,addr,width,value);
    }
    return 1;
}

void cygnus_bus_tick(CygnusVM *vm,uint64_t guest_cycles){
    if(!vm)return;
    for(size_t i=0;i<vm->bus.device_count;i++){
        CygnusDevice *d=&vm->bus.devices[i];
        if(d->ops&&d->ops->tick)d->ops->tick(vm,d,guest_cycles);
    }
}

void cygnus_bus_destroy(CygnusVM *vm){
    if(!vm)return;
    for(size_t i=0;i<vm->bus.device_count;i++){
        CygnusDevice *d=&vm->bus.devices[i];
        if(d->ops&&d->ops->destroy)d->ops->destroy(vm,d);
    }
    cygnus_bus_init(&vm->bus);
}

void cygnus_irq_init(CygnusIrqFabric *irq){
    if(irq)memset(irq,0,sizeof(*irq));
}

int cygnus_irq_raise(CygnusVM *vm,uint8_t line){
    if(!vm)return 0;
    unsigned bank=line>>6;
    uint64_t bit=1ull<<(line&63);
    vm->irq.asserted[bank]|=bit;
    vm->irq.pending[bank]|=bit;
    return 1;
}

int cygnus_irq_lower(CygnusVM *vm,uint8_t line){
    if(!vm)return 0;
    unsigned bank=line>>6;
    uint64_t bit=1ull<<(line&63);
    vm->irq.asserted[bank]&=~bit;
    return 1;
}

int cygnus_irq_next(CygnusVM *vm,uint8_t *line){
    if(!vm||!line)return 0;
    for(unsigned bank=0;bank<4;bank++){
        uint64_t bits=vm->irq.pending[bank];
        if(!bits)continue;
        unsigned bit=0;
        while(bit<64&&!(bits&(1ull<<bit)))bit++;
        if(bit==64)continue;
        vm->irq.pending[bank]&=~(1ull<<bit);
        *line=(uint8_t)(bank*64+bit);
        return 1;
    }
    return 0;
}
