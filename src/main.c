#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int main(int argc,char**argv){
    if(argc<2){usage();return 2;}
    if(!strcmp(argv[1],"capabilities")){print_caps();return 0;}
    CygnusVM vm;memset(&vm,0,sizeof(vm));int ok=0;
    if(!strcmp(argv[1],"vm")&&argc>=3){
        CygnusVMConfig cfg;
        if(!cygnus_config_load(argv[2],&cfg)){fprintf(stderr,"Cygnus: invalid CVM config\n");return 1;}
        if(cygnus_vm_init_config(&vm,&cfg))ok=cygnus_vm_run(&vm,argc>=4?strtoull(argv[3],0,0):1000000);
    }else{
        if(!cygnus_vm_init(&vm,CYGNUS_RAM_DEFAULT)){fputs("Cygnus: RAM allocation failed\n",stderr);return 1;}
        if(!strcmp(argv[1],"run")&&argc>=3){
            if(cygnus_vm_load_bootsector(&vm,argv[2]))ok=cygnus_vm_run(&vm,argc>=4?strtoull(argv[3],0,0):1000000);
        }else if(!strcmp(argv[1],"snapshot")&&argc>=4){
            if(cygnus_vm_load_bootsector(&vm,argv[2]))ok=cygnus_vm_snapshot_save(&vm,argv[3]);
        }else if(!strcmp(argv[1],"resume")&&argc>=3){
            cygnus_vm_destroy(&vm);memset(&vm,0,sizeof(vm));
            if(cygnus_vm_snapshot_load(&vm,argv[2]))ok=cygnus_vm_run(&vm,argc>=4?strtoull(argv[3],0,0):1000000);
        }else{
            usage();cygnus_vm_destroy(&vm);return 2;
        }
    }
    fprintf(stderr,"\nCygnus: %s, backend=%s instructions=%llu AX=%04x CS:IP=%04x:%04x\n",ok?"guest halted cleanly":"guest failed",vm.backend?vm.backend->name:"unknown",(unsigned long long)vm.instructions,vm.cpu.ax,vm.cpu.cs,vm.cpu.ip);
    cygnus_vm_destroy(&vm);return ok?0:1;
}
