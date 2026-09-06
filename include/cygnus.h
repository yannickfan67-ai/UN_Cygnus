#ifndef CYGNUS_H
#define CYGNUS_H
#include <stdint.h>
#include <stddef.h>

#define CYGNUS_VERSION "0.1.0"
#define CYGNUS_RAM_DEFAULT (1024u * 1024u)

typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp;
    uint16_t cs,ds,es,ss,ip,flags;
    int halted;
} CygnusCPU;

typedef struct {
    uint8_t *ram;
    size_t ram_size;
    CygnusCPU cpu;
    uint64_t instructions;
    uint8_t serial_lsr;
} CygnusVM;

int cygnus_vm_init(CygnusVM *vm,size_t ram_size);
void cygnus_vm_destroy(CygnusVM *vm);
int cygnus_vm_load_bootsector(CygnusVM *vm,const char *path);
int cygnus_vm_run(CygnusVM *vm,uint64_t max_instructions);
int cygnus_vm_snapshot_save(const CygnusVM *vm,const char *path);
int cygnus_vm_snapshot_load(CygnusVM *vm,const char *path);
uint32_t cygnus_linear(uint16_t seg,uint16_t off);
#endif
