#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void usage(void){
    puts("UN_Cygnus 0.2.0-dev - from-scratch virtual machine monitor");
    puts("usage:");
    puts("  cygnus run <boot.img> [max-instructions]");
    puts("  cygnus vm <machine.cvm> [max-instructions]");
    puts("  cygnus snapshot <boot.img> <state.cys>");
    puts("  cygnus resume <state.cys> [max-instructions]");
    puts("  cygnus capabilities");
}
static void print_caps(void){
    printf("Cygnus API: 0x%08x\n",cygnus_api_version());
    printf("Capabilities: 0x%016llx\n",(unsigned long long)cygnus_capabilities());
    puts("CPU backends: soft86 (VMX/SVM ABI slots reserved)");
    puts("Buses: C-Bus port-I/O + MMIO");
}
static int parse_max_instructions(const char *s,unsigned long long *value){
    if(!s||!*s||*s=='-')return 0;
    char *end=NULL;
    errno=0;
    unsigned long long n=strtoull(s,&end,0);
    if(errno==ERANGE||end==s||*end||n==0)return 0;
    *value=n;
    return 1;
}
int main(int argc,char**argv){
    if(argc<2){usage();return 2;}
    if(!strcmp(argv[1],"capabilities")){print_caps();return 0;}
    unsigned long long max_instructions=1000000;
    if((!strcmp(argv[1],"run")||!strcmp(argv[1],"vm")||!strcmp(argv[1],"resume"))&&argc>=4&&
       !parse_max_instructions(argv[3],&max_instructions)){
        fputs("Cygnus: invalid max-instructions\n",stderr);
        return 2;
    }
    CygnusVM vm;memset(&vm,0,sizeof(vm));int ok=0;
    if(!strcmp(argv[1],"vm")&&argc>=3){
        CygnusVMConfig cfg;
        if(!cygnus_config_load(argv[2],&cfg)){fprintf(stderr,"Cygnus: invalid CVM config\n");return 1;}
        if(cygnus_vm_init_config(&vm,&cfg))ok=cygnus_vm_run(&vm,max_instructions);
    }else{
        if(!cygnus_vm_init(&vm,CYGNUS_RAM_DEFAULT)){fputs("Cygnus: RAM allocation failed\n",stderr);return 1;}
        if(!strcmp(argv[1],"run")&&argc>=3){
            if(cygnus_vm_load_bootsector(&vm,argv[2]))ok=cygnus_vm_run(&vm,max_instructions);
        }else if(!strcmp(argv[1],"snapshot")&&argc>=4){
            if(cygnus_vm_load_bootsector(&vm,argv[2]))ok=cygnus_vm_snapshot_save(&vm,argv[3]);
        }else if(!strcmp(argv[1],"resume")&&argc>=3){
            cygnus_vm_destroy(&vm);memset(&vm,0,sizeof(vm));
            if(cygnus_vm_snapshot_load(&vm,argv[2]))ok=cygnus_vm_run(&vm,max_instructions);
        }else{
            usage();cygnus_vm_destroy(&vm);return 2;
        }
    }
    fprintf(stderr,"\nCygnus: %s, backend=%s instructions=%llu AX=%04x CS:IP=%04x:%04x\n",ok?"guest halted cleanly":"guest failed",vm.backend?vm.backend->name:"unknown",(unsigned long long)vm.instructions,vm.cpu.ax,vm.cpu.cs,vm.cpu.ip);
    cygnus_vm_destroy(&vm);return ok?0:1;
}
