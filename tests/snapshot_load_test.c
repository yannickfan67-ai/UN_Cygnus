#include "cygnus.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int copy_without_last_byte(const char *src,const char *dst){
    FILE *in=fopen(src,"rb");
    if(!in)return 0;
    if(fseek(in,0,SEEK_END)!=0){fclose(in);return 0;}
    long size=ftell(in);
    if(size<=0||fseek(in,0,SEEK_SET)!=0){fclose(in);return 0;}
    FILE *out=fopen(dst,"wb");
    if(!out){fclose(in);return 0;}
    for(long i=0;i<size-1;i++){
        int ch=fgetc(in);
        if(ch==EOF||fputc(ch,out)==EOF){fclose(in);fclose(out);return 0;}
    }
    int ok=fclose(out)==0;
    fclose(in);
    return ok;
}

static int copy_with_trailing_byte(const char *src,const char *dst){
    FILE *in=fopen(src,"rb");
    if(!in)return 0;
    FILE *out=fopen(dst,"wb");
    if(!out){fclose(in);return 0;}
    int ch;
    while((ch=fgetc(in))!=EOF){if(fputc(ch,out)==EOF){fclose(in);fclose(out);return 0;}}
    fclose(in);
    int ok=fputc(0xa5,out)!=EOF&&fclose(out)==0;
    return ok;
}

static int make_zero_ram_snapshot(const char *src,const char *dst){
    FILE *in=fopen(src,"rb");
    if(!in)return 0;
    FILE *out=fopen(dst,"wb");
    if(!out){fclose(in);return 0;}
    int ch;
    while((ch=fgetc(in))!=EOF){if(fputc(ch,out)==EOF){fclose(in);fclose(out);return 0;}}
    fclose(in);
    if(fclose(out)!=0)return 0;
    out=fopen(dst,"rb+");
    if(!out)return 0;
    unsigned char zero[4]={0,0,0,0};
    int ok=fseek(out,12,SEEK_SET)==0&&fwrite(zero,1,sizeof(zero),out)==sizeof(zero);
    fclose(out);
    return ok;
}

static int state_is_intact(const CygnusVM *vm){
    return vm->ram_size==65536&&vm->ram&&vm->ram[0]==0xa5&&vm->cpu.ax==0xbeef&&vm->instructions==99&&vm->bus.device_count==1;
}

static int failed_config_init_is_clean(void){
    CygnusVM vm;
    CygnusVMConfig cfg;
    memset(&vm,0,sizeof(vm));
    memset(&cfg,0,sizeof(cfg));
    strcpy(cfg.backend,"soft86");
    strcpy(cfg.name,"missing boot image");
    strcpy(cfg.boot_path,"build/definitely-missing-boot.img");
    cfg.ram_size=65536;
    cfg.serial_enabled=1;
    if(cygnus_vm_init_config(&vm,&cfg)){
        cygnus_vm_destroy(&vm);
        return 0;
    }
    return vm.ram==NULL&&vm.ram_size==0&&vm.backend==NULL&&vm.bus.device_count==0&&vm.bus.io_count==0&&vm.bus.mmio_count==0;
}

static int run_slice_handles_counter_wrap(void){
    CygnusVM vm;
    CygnusVmExit exit_info;
    if(!cygnus_vm_init(&vm,65536))return 0;
    vm.ram[0]=0x90;
    vm.ram[1]=0xf4;
    vm.cpu.cs=0;
    vm.cpu.ip=0;
    vm.instructions=UINT64_MAX-1;
    int ok=cygnus_vm_run_slice(&vm,4,&exit_info);
    int passed=ok&&vm.cpu.halted&&vm.cpu.ip==2&&vm.instructions==0&&exit_info.reason==CYGNUS_EXIT_HLT;
    cygnus_vm_destroy(&vm);
    return passed;
}

int main(void){
    const char *good="build/snapshot-good.cys";
    const char *truncated="build/snapshot-truncated.cys";
    const char *trailing="build/snapshot-trailing.cys";
    const char *zero_ram="build/snapshot-zero-ram.cys";
    if(!failed_config_init_is_clean()||!run_slice_handles_counter_wrap())return 1;
    CygnusVM vm;
    if(!cygnus_vm_init(&vm,65536))return 1;
#ifdef __linux__
    CygnusVM tiny;
    if(!cygnus_vm_init(&tiny,1)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    if(cygnus_vm_snapshot_save(&tiny,"/dev/full")){
        cygnus_vm_destroy(&tiny);
        cygnus_vm_destroy(&vm);
        return 1;
    }
    cygnus_vm_destroy(&tiny);
#endif
    vm.ram[0]=0x5a;
    vm.cpu.ax=0x1234;
    vm.instructions=42;
    if(!cygnus_vm_snapshot_save(&vm,good)||!copy_without_last_byte(good,truncated)||!copy_with_trailing_byte(good,trailing)||!make_zero_ram_snapshot(good,zero_ram)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    vm.ram[0]=0xa5;
    vm.cpu.ax=0xbeef;
    vm.instructions=99;
    if(cygnus_vm_snapshot_load(&vm,truncated)||!state_is_intact(&vm)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    if(cygnus_vm_snapshot_load(&vm,trailing)||!state_is_intact(&vm)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    if(cygnus_vm_snapshot_load(&vm,zero_ram)||!state_is_intact(&vm)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    cygnus_vm_destroy(&vm);
    remove(good);
    remove(truncated);
    remove(trailing);
    remove(zero_ram);
    return 0;
}
