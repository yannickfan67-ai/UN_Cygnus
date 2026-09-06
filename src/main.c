#include "cygnus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void){
    puts("UN_Cygnus 0.1.0 - from-scratch virtual machine monitor");
    puts("usage:");
    puts("  cygnus run <boot.img> [max-instructions]");
    puts("  cygnus snapshot <boot.img> <state.cys>");
    puts("  cygnus resume <state.cys> [max-instructions]");
}
int main(int argc,char**argv){
    if(argc<2){usage();return 2;}
    CygnusVM vm;
    if(!cygnus_vm_init(&vm,CYGNUS_RAM_DEFAULT)){
        fputs("Cygnus: RAM allocation failed\n",stderr);return 1;
    }
    int ok=0;
    if(!strcmp(argv[1],"run")&&argc>=3){
        if(cygnus_vm_load_bootsector(&vm,argv[2]))
            ok=cygnus_vm_run(&vm,argc>=4?strtoull(argv[3],0,0):1000000);
    } else if(!strcmp(argv[1],"snapshot")&&argc>=4){
        if(cygnus_vm_load_bootsector(&vm,argv[2]))
            ok=cygnus_vm_snapshot_save(&vm,argv[3]);
    } else if(!strcmp(argv[1],"resume")&&argc>=3){
        cygnus_vm_destroy(&vm);memset(&vm,0,sizeof(vm));
        if(cygnus_vm_snapshot_load(&vm,argv[2]))
            ok=cygnus_vm_run(&vm,argc>=4?strtoull(argv[3],0,0):1000000);
    } else {
        usage();cygnus_vm_destroy(&vm);return 2;
    }
    fprintf(stderr,"\nCygnus: %s, instructions=%llu AX=%04x CS:IP=%04x:%04x\n",
            ok?"guest halted cleanly":"guest failed",
            (unsigned long long)vm.instructions,vm.cpu.ax,vm.cpu.cs,vm.cpu.ip);
    cygnus_vm_destroy(&vm);return ok?0:1;
}
