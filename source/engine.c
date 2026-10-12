#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static uint32_t read_word(const unsigned char *p, int be) {
    if (be) return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
    return ((uint32_t)p[3]<<24)|((uint32_t)p[2]<<16)|((uint32_t)p[1]<<8)|p[0];
}
static void write_word(unsigned char *p, uint32_t v, int be) {
    for (int i=0;i<4;i++) p[be?3-i:i] = (unsigned char)(v >> (8*i));
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
static int exclusive_path(const char *name) {
    struct stat st;
    return stat(name,&st)!=0;
}
int lab_patch(const char *path,uint32_t old_value,uint32_t new_value,
              LabMatch match,char *backup,size_t cap) {
    if(!path||!backup||cap<16)return -5;
    backup[0]='\0';
    unsigned char *data=NULL;size_t sz=0;int r=read_file(path,&data,&sz);if(r)return r;
    if((size_t)match.offset+4>sz || read_word(data+match.offset,match.be)!=old_value){free(data);return -6;}
    char bpath[1024], tmp[1024];int chosen=0;
    for(int n=1;n<=999;n++){
        int k=snprintf(bpath,sizeof(bpath),"%s.nfslab.bak%d",path,n);
        if(k<=0 || (size_t)k>=sizeof(bpath)){free(data);return -5;}
        if(exclusive_path(bpath)){chosen=1;break;}
    }
    if(!chosen || strlen(bpath)+1>cap){free(data);return -7;}
    int k=snprintf(tmp,sizeof(tmp),"%s.nfslab.tmp",path);
    if(k<=0 || (size_t)k>=sizeof(tmp) || !exclusive_path(tmp)){free(data);return -7;}
    write_word(data+match.offset,new_value,match.be);
    FILE *fp=fopen(tmp,"wb");if(!fp){free(data);return -2;}
    int ok=fwrite(data,1,sz,fp)==sz && fflush(fp)==0;
    if(fclose(fp)!=0)ok=0;
    free(data);
    if(!ok){remove(tmp);return -2;}
    if(rename(path,bpath)!=0){remove(tmp);return -2;}
    if(rename(tmp,path)!=0){rename(bpath,path);remove(tmp);return -2;}
    snprintf(backup,cap,"%s",bpath);return 0;
}
const char *lab_error(int r) {
    switch(r){case 0:return "OK";case -1:return "Cannot open";
    case -2:return "Read/write/rename error";case -3:return "Unsupported file size";
    case -4:return "Out of memory";case -5:return "Invalid path/argument";
    case -6:return "Value changed / offset invalid";
    case -7:return "No available backup/temp path";
    default:return "Unknown error";}
}
