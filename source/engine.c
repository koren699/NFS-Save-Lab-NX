#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_word(const unsigned char *p, int be) {
    if (be) return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
    return ((uint32_t)p[3]<<24)|((uint32_t)p[2]<<16)|((uint32_t)p[1]<<8)|p[0];
}
static int read_file(const char *path, unsigned char **buf, size_t *sz) {
    FILE *fp=fopen(path,"rb"); if(!fp) return -1;
    if(fseek(fp,0,SEEK_END)!=0){fclose(fp);return -2;}
    long len=ftell(fp);
    if(len<32 || (unsigned long)len>LAB_MAX_FILE){fclose(fp);return -3;}
    rewind(fp);
    unsigned char *data=(unsigned char*)malloc((size_t)len);
    if(!data){fclose(fp);return -4;}
    if(fread(data,1,(size_t)len,fp)!=(size_t)len){free(data);fclose(fp);return -2;}
    fclose(fp); *buf=data;*sz=(size_t)len;return 0;
}
int lab_scan(const char *path,uint32_t old_value, LabMatches *out) {
    if(!out)return -5;
    memset(out,0,sizeof(*out));
    unsigned char *data=NULL; size_t sz=0; int r=read_file(path,&data,&sz); if(r)return r;
    for(size_t off=0;off+4<=sz;off++){
        int le=read_word(data+off,0)==old_value;
        int be=read_word(data+off,1)==old_value;
        if(!le && !be)continue;
        if(out->count<LAB_MAX_MATCHES) {
            out->items[out->count].offset=(uint32_t)off;
            out->items[out->count].be=(!le && be);
            ++out->count;
        }else{out->overflow=1;break;}
    }
    free(data);return 0;
}
const char *lab_error(int r) {
    switch(r){case 0:return "OK";case -1:return "Cannot open";
    case -2:return "Read/write/rename error";case -3:return "Unsupported file size";
    case -4:return "Out of memory";case -5:return "Invalid path/argument";
    case -6:return "Value changed / offset invalid";
    case -7:return "No available backup/temp path";
    default:return "Unknown error";}
}
