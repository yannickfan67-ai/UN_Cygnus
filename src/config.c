#include "cygnus.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static char *trim(char *s){
    while(*s&&isspace((unsigned char)*s))s++;
    char *e=s+strlen(s);
    while(e>s&&isspace((unsigned char)e[-1]))*--e=0;
    return s;
}
static void copy(char *d,size_t cap,const char *s){
    size_t i=0;
    if(!cap)return;
    while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}
    d[i]=0;
}
void cygnus_config_defaults(CygnusVMConfig *c){
    if(!c)return;
    memset(c,0,sizeof(*c));
    copy(c->name,sizeof(c->name),"Cygnus VM");
    copy(c->backend,sizeof(c->backend),"soft86");
    c->ram_size=CYGNUS_RAM_DEFAULT;
    c->serial_enabled=1;
}
int cygnus_config_load(const char *path,CygnusVMConfig *c){
    if(!path||!c)return 0;
    cygnus_config_defaults(c);
    FILE *f=fopen(path,"r");
    if(!f)return 0;
    char line[1024];
    while(fgets(line,sizeof(line),f)){
        char *s=trim(line);
        if(!*s||*s=='#'||*s==';')continue;
        char *eq=strchr(s,'=');
        if(!eq)continue;
        *eq++=0;
        char *k=trim(s),*v=trim(eq);
        if(!strcmp(k,"name"))copy(c->name,sizeof(c->name),v);
        else if(!strcmp(k,"backend"))copy(c->backend,sizeof(c->backend),v);
        else if(!strcmp(k,"boot"))copy(c->boot_path,sizeof(c->boot_path),v);
        else if(!strcmp(k,"memory")){
            unsigned long long m=strtoull(v,NULL,0);
            if(m>=65536&&m<=0x100000)c->ram_size=(size_t)m;
        }else if(!strcmp(k,"serial")){
            c->serial_enabled=!strcmp(v,"on")||!strcmp(v,"1")||!strcmp(v,"true");
        }
    }
    fclose(f);
    return c->boot_path[0]!=0;
}
