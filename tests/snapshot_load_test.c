#include "cygnus.h"
#include <stdio.h>
#include <string.h>

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

int main(void){
    const char *good="build/snapshot-good.cys";
    const char *truncated="build/snapshot-truncated.cys";
    const char *zero_ram="build/snapshot-zero-ram.cys";
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
    if(!cygnus_vm_snapshot_save(&vm,good)||!copy_without_last_byte(good,truncated)||!make_zero_ram_snapshot(good,zero_ram)){
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
    if(cygnus_vm_snapshot_load(&vm,zero_ram)||!state_is_intact(&vm)){
        cygnus_vm_destroy(&vm);
        return 1;
    }
    cygnus_vm_destroy(&vm);
    remove(good);
    remove(truncated);
    remove(zero_ram);
    return 0;
}
