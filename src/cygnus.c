#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CF 0x0001
#define IF 0x0200
#define ZF 0x0040
#define SF 0x0080

static uint8_t mem8(CygnusVM *vm,uint32_t a){return a<vm->ram_size?vm->ram[a]:0xff;}
static uint16_t mem16(CygnusVM *vm,uint32_t a){return (uint16_t)(mem8(vm,a)|(mem8(vm,a+1)<<8));}
static void set8(CygnusVM *vm,uint32_t a,uint8_t v){if(a<vm->ram_size)vm->ram[a]=v;}
static void set16(CygnusVM *vm,uint32_t a,uint16_t v){set8(vm,a,(uint8_t)v);set8(vm,a+1,(uint8_t)(v>>8));}
uint32_t cygnus_linear(uint16_t seg,uint16_t off){return (((uint32_t)seg<<4)+off)&0xfffff;}
static uint8_t fetch8(CygnusVM *vm){uint8_t v=mem8(vm,cygnus_linear(vm->cpu.cs,vm->cpu.ip));vm->cpu.ip++;return v;}
static uint16_t fetch16(CygnusVM *vm){uint16_t v=fetch8(vm);v|=(uint16_t)fetch8(vm)<<8;return v;}

static uint16_t *reg16(CygnusCPU *c,unsigned r){
    switch(r&7){case 0:return &c->ax;case 1:return &c->cx;case 2:return &c->dx;case 3:return &c->bx;case 4:return &c->sp;case 5:return &c->bp;case 6:return &c->si;default:return &c->di;}
}
static uint8_t get_r8(CygnusCPU*c,unsigned r){uint16_t*v=reg16(c,r&3);return r<4?(uint8_t)*v:(uint8_t)(*v>>8);}
static void set_r8(CygnusCPU*c,unsigned r,uint8_t x){uint16_t*v=reg16(c,r&3);if(r<4)*v=(*v&0xff00)|x;else *v=(*v&0x00ff)|((uint16_t)x<<8);}
static void flags8(CygnusCPU*c,uint8_t v){c->flags=(uint16_t)((c->flags&~(ZF|SF))|(v==0?ZF:0)|(v&0x80?SF:0));}
static void push16(CygnusVM*vm,uint16_t v){vm->cpu.sp-=2;set16(vm,cygnus_linear(vm->cpu.ss,vm->cpu.sp),v);}
static uint16_t pop16(CygnusVM*vm){uint16_t v=mem16(vm,cygnus_linear(vm->cpu.ss,vm->cpu.sp));vm->cpu.sp+=2;return v;}

static uint8_t io_in(CygnusVM *vm,uint16_t port){
    uint32_t v=0xff;
    if(!cygnus_bus_io_read(vm,port,1,&v))return 0xff;
    return (uint8_t)v;
}
static void io_out(CygnusVM *vm,uint16_t port,uint8_t v){
    (void)cygnus_bus_io_write(vm,port,1,v);
}
static int bios_int(CygnusVM *vm,uint8_t num){
    uint8_t ah=(uint8_t)(vm->cpu.ax>>8),al=(uint8_t)vm->cpu.ax;
    if(num==0x10 && ah==0x0e){fputc(al,stdout);fflush(stdout);return 1;}
    if(num==0x12){vm->cpu.ax=(uint16_t)(vm->ram_size/1024);return 1;}
    if(num==0x19){vm->cpu.cs=0;vm->cpu.ip=0x7c00;return 1;}
    fprintf(stderr,"Cygnus: unsupported BIOS interrupt 0x%02x\n",num);return 0;
}

static int step(CygnusVM *vm){
    CygnusCPU*c=&vm->cpu;uint8_t op=fetch8(vm);vm->instructions++;
    if(op>=0xb8&&op<=0xbf){*reg16(c,op-0xb8)=fetch16(vm);return 1;}
    if(op>=0xb0&&op<=0xb7){set_r8(c,op-0xb0,fetch8(vm));return 1;}
    if(op>=0x50&&op<=0x57){push16(vm,*reg16(c,op-0x50));return 1;}
    if(op>=0x58&&op<=0x5f){*reg16(c,op-0x58)=pop16(vm);return 1;}
    if(op>=0x40&&op<=0x47){uint16_t*r=reg16(c,op-0x40);(*r)++;c->flags=(uint16_t)((c->flags&~ZF)|(*r==0?ZF:0));return 1;}
    if(op>=0x48&&op<=0x4f){uint16_t*r=reg16(c,op-0x48);(*r)--;c->flags=(uint16_t)((c->flags&~ZF)|(*r==0?ZF:0));return 1;}
    switch(op){
        case 0x90:return 1;
        case 0xf4:c->halted=1;return 1;
        case 0xfa:c->flags=(uint16_t)(c->flags&~IF);return 1;
        case 0xfb:c->flags=(uint16_t)(c->flags|IF);return 1;
        case 0xcd:{uint8_t n=fetch8(vm);return bios_int(vm,n);}
        case 0xeb:{int8_t d=(int8_t)fetch8(vm);c->ip=(uint16_t)(c->ip+d);return 1;}
        case 0xe9:{int16_t d=(int16_t)fetch16(vm);c->ip=(uint16_t)(c->ip+d);return 1;}
        case 0x74:{int8_t d=(int8_t)fetch8(vm);if(c->flags&ZF)c->ip=(uint16_t)(c->ip+d);return 1;}
        case 0x75:{int8_t d=(int8_t)fetch8(vm);if(!(c->flags&ZF))c->ip=(uint16_t)(c->ip+d);return 1;}
        case 0x3c:{uint8_t imm=fetch8(vm),v=(uint8_t)c->ax,r=(uint8_t)(v-imm);flags8(c,r);c->flags=(uint16_t)((c->flags&~CF)|(v<imm?CF:0));return 1;}
        case 0x04:{uint8_t imm=fetch8(vm),v=(uint8_t)c->ax,r=(uint8_t)(v+imm);set_r8(c,0,r);flags8(c,r);c->flags=(uint16_t)((c->flags&~CF)|((uint16_t)v+imm>255?CF:0));return 1;}
        case 0x2c:{uint8_t imm=fetch8(vm),v=(uint8_t)c->ax,r=(uint8_t)(v-imm);set_r8(c,0,r);flags8(c,r);c->flags=(uint16_t)((c->flags&~CF)|(v<imm?CF:0));return 1;}
        case 0xe4:{uint16_t p=fetch8(vm);set_r8(c,0,io_in(vm,p));return 1;}
        case 0xe6:{uint16_t p=fetch8(vm);io_out(vm,p,get_r8(c,0));return 1;}
        case 0xec:set_r8(c,0,io_in(vm,c->dx));return 1;
        case 0xee:io_out(vm,c->dx,get_r8(c,0));return 1;
        case 0x31:{
            uint8_t m=fetch8(vm);if((m&0xc0)!=0xc0){fprintf(stderr,"Cygnus: XOR memory ModRM not implemented\n");return 0;}
            unsigned src=(m>>3)&7,dst=m&7;*reg16(c,dst)^=*reg16(c,src);c->flags=(uint16_t)((c->flags&~(CF|ZF|SF))|(*reg16(c,dst)==0?ZF:0)|(*reg16(c,dst)&0x8000?SF:0));return 1;
        }
        case 0x89:{
            uint8_t m=fetch8(vm);if((m&0xc0)!=0xc0){fprintf(stderr,"Cygnus: MOV memory ModRM not implemented\n");return 0;}*reg16(c,m&7)=*reg16(c,(m>>3)&7);return 1;
        }
        case 0x8e:{
            uint8_t m=fetch8(vm);if((m&0xc0)!=0xc0){fprintf(stderr,"Cygnus: MOV Sreg memory ModRM not implemented\n");return 0;}uint16_t v=*reg16(c,m&7);switch((m>>3)&3){case 0:c->es=v;break;case 2:c->ss=v;break;case 3:c->ds=v;break;default:return 0;}return 1;
        }
        default:fprintf(stderr,"Cygnus: unimplemented opcode %02x at %04x:%04x\n",op,c->cs,(uint16_t)(c->ip-1));return 0;
    }
}

static void exit_fill(CygnusVM *vm,CygnusVmExit *e,CygnusVmExitReason reason){
    if(!e)return;
    memset(e,0,sizeof(*e));
    e->reason=reason;
    e->guest_pc=cygnus_linear(vm->cpu.cs,vm->cpu.ip);
    e->instructions=vm->instructions;
}

static int vm_init_base(CygnusVM *vm,size_t ram_size,int with_serial){
    if(!vm)return 0;
    memset(vm,0,sizeof(*vm));
    if(!ram_size)ram_size=CYGNUS_RAM_DEFAULT;
    if(ram_size>0x100000)ram_size=0x100000;
    vm->ram=calloc(1,ram_size);
    if(!vm->ram)return 0;
    vm->ram_size=ram_size;vm->cpu.flags=0x0002;vm->cpu.sp=0x7c00;vm->state=CYGNUS_VM_CREATED;
    cygnus_bus_init(&vm->bus);cygnus_irq_init(&vm->irq);vm->backend=cygnus_backend_soft86();
    if(with_serial&&!cygnus_attach_serial(vm,0x3f8)){free(vm->ram);memset(vm,0,sizeof(*vm));return 0;}
    vm->state=CYGNUS_VM_READY;return 1;
}
int cygnus_vm_init(CygnusVM *vm,size_t ram_size){return vm_init_base(vm,ram_size,1);}
int cygnus_vm_init_config(CygnusVM *vm,const CygnusVMConfig *cfg){
    if(!cfg||strcmp(cfg->backend,"soft86"))return 0;
    if(!vm_init_base(vm,cfg->ram_size,cfg->serial_enabled))return 0;
    size_t i=0;while(cfg->name[i]&&i+1<sizeof(vm->name)){vm->name[i]=cfg->name[i];i++;}vm->name[i]=0;
    return cygnus_vm_load_bootsector(vm,cfg->boot_path);
}
void cygnus_vm_destroy(CygnusVM *vm){if(!vm)return;cygnus_bus_destroy(vm);if(vm->backend&&vm->backend->destroy)vm->backend->destroy(vm);if(vm->ram)free(vm->ram);memset(vm,0,sizeof(*vm));}
int cygnus_vm_reset(CygnusVM *vm){if(!vm||!vm->backend||!vm->backend->reset)return 0;return vm->backend->reset(vm);}
int cygnus_vm_load_bootsector(CygnusVM *vm,const char *path){
    if(!vm||!vm->ram||!path) return 0;
    FILE*f=fopen(path,"rb");
    if(!f){perror(path);return 0;}
    uint8_t sec[512];
    size_t n=fread(sec,1,sizeof(sec),f);
    fclose(f);
    if(n<1) return 0;
    uint32_t a=0x7c00;
    if(a+n>vm->ram_size) return 0;
    memcpy(vm->ram+a,sec,n);
    vm->cpu.cs=0;
    vm->cpu.ip=0x7c00;
    vm->cpu.dx=(vm->cpu.dx&0xff00)|0x80;
    vm->cpu.halted=0;
    return 1;
}
static int soft86_reset(CygnusVM *vm){
    if(!vm)return 0;
    memset(&vm->cpu,0,sizeof(vm->cpu));vm->cpu.flags=0x0002;vm->cpu.sp=0x7c00;vm->instructions=0;vm->state=CYGNUS_VM_READY;cygnus_irq_init(&vm->irq);return 1;
}
static int soft86_run_slice(CygnusVM *vm,uint64_t budget,CygnusVmExit *exit_info){
    if(!vm||!vm->ram)return 0;
    if(!budget)budget=1;
    uint64_t target=vm->instructions+budget;
    vm->state=CYGNUS_VM_RUNNING;
    while(!vm->cpu.halted&&vm->instructions<target){
        if(!step(vm)){vm->state=CYGNUS_VM_FAILED;exit_fill(vm,exit_info,CYGNUS_EXIT_ERROR);return 0;}
        cygnus_bus_tick(vm,1);
    }
    if(vm->cpu.halted){vm->state=CYGNUS_VM_HALTED;exit_fill(vm,exit_info,CYGNUS_EXIT_HLT);return 1;}
    vm->state=CYGNUS_VM_PAUSED;exit_fill(vm,exit_info,CYGNUS_EXIT_BUDGET);return 1;
}
static int soft86_run(CygnusVM *vm,uint64_t max){
    CygnusVmExit e;
    if(!max)max=1000000;
    if(!soft86_run_slice(vm,max,&e))return 0;
    if(e.reason==CYGNUS_EXIT_HLT)return 1;
    if(e.reason==CYGNUS_EXIT_BUDGET)fprintf(stderr,"Cygnus: instruction limit reached\n");
    return 0;
}
static int soft86_inject_irq(CygnusVM *vm,uint8_t vector){
    if(!vm||!vm->ram||!(vm->cpu.flags&IF))return 0;
    uint32_t ivt=(uint32_t)vector*4u;
    if(ivt+3>=vm->ram_size)return 0;
    push16(vm,vm->cpu.flags);push16(vm,vm->cpu.cs);push16(vm,vm->cpu.ip);
    vm->cpu.flags=(uint16_t)(vm->cpu.flags&~IF);
    vm->cpu.ip=mem16(vm,ivt);vm->cpu.cs=mem16(vm,ivt+2);vm->cpu.halted=0;
    return 1;
}
static const CygnusCpuBackendOps SOFT86_OPS={
    .api_version=CYGNUS_API_VERSION,
    .name="soft86",
    .capabilities=CYGNUS_CAP_SOFT86|CYGNUS_CAP_VMEXIT|CYGNUS_CAP_IRQ_FABRIC,
    .reset=soft86_reset,
    .run=soft86_run,
    .destroy=NULL,
    .run_slice=soft86_run_slice,
    .inject_irq=soft86_inject_irq
};
const CygnusCpuBackendOps *cygnus_backend_soft86(void){return &SOFT86_OPS;}
uint32_t cygnus_api_version(void){return CYGNUS_API_VERSION;}
uint64_t cygnus_capabilities(void){return CYGNUS_CAP_SOFT86|CYGNUS_CAP_SNAPSHOT|CYGNUS_CAP_CBUS|CYGNUS_CAP_CVM_CONFIG|CYGNUS_CAP_VMEXIT|CYGNUS_CAP_IRQ_FABRIC;}
int cygnus_vm_run(CygnusVM *vm,uint64_t max){if(!vm||!vm->backend||!vm->backend->run)return 0;return vm->backend->run(vm,max);}
int cygnus_vm_run_slice(CygnusVM *vm,uint64_t budget,CygnusVmExit *exit_info){
    if(!vm||!vm->backend)return 0;
    if(vm->backend->run_slice)return vm->backend->run_slice(vm,budget,exit_info);
    int ok=vm->backend->run?vm->backend->run(vm,budget):0;
    exit_fill(vm,exit_info,ok?CYGNUS_EXIT_HLT:(vm->state==CYGNUS_VM_PAUSED?CYGNUS_EXIT_BUDGET:CYGNUS_EXIT_ERROR));
    return ok||vm->state==CYGNUS_VM_PAUSED;
}
int cygnus_vm_inject_irq(CygnusVM *vm,uint8_t vector){if(!vm||!vm->backend||!vm->backend->inject_irq)return 0;return vm->backend->inject_irq(vm,vector);}

struct SnapHeader {char magic[8];uint32_t version;uint32_t ram_size;CygnusCPU cpu;uint64_t instructions;};
int cygnus_vm_snapshot_save(const CygnusVM *vm,const char *path){if(!vm||!vm->ram||!path)return 0;FILE*f=fopen(path,"wb");if(!f)return 0;struct SnapHeader h={{'C','Y','G','S','N','A','P','1'},1,(uint32_t)vm->ram_size,vm->cpu,vm->instructions};int ok=fwrite(&h,1,sizeof(h),f)==sizeof(h)&&fwrite(vm->ram,1,vm->ram_size,f)==vm->ram_size;fclose(f);return ok;}
int cygnus_vm_snapshot_load(CygnusVM *vm,const char *path){if(!vm||!path)return 0;FILE*f=fopen(path,"rb");if(!f)return 0;struct SnapHeader h;if(fread(&h,1,sizeof(h),f)!=sizeof(h)||memcmp(h.magic,"CYGSNAP1",8)||h.version!=1||h.ram_size>0x100000){fclose(f);return 0;}cygnus_vm_destroy(vm);if(!cygnus_vm_init(vm,h.ram_size)){fclose(f);return 0;}vm->cpu=h.cpu;vm->instructions=h.instructions;int ok=fread(vm->ram,1,vm->ram_size,f)==vm->ram_size;fclose(f);return ok;}
