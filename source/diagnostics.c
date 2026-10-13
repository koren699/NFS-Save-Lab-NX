#include "diagnostics.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_data(const char *path,unsigned char **dst,size_t *size){
    if(!path||!dst||!size)return -1;
    FILE *f=fopen(path,"rb");if(!f)return -1;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return -1;}
    long len=ftell(f);
    if(len<32 || (unsigned long long)len>LAB_MAX_FILE){fclose(f);return -2;}
    rewind(f);
    unsigned char *data=malloc((size_t)len);
    if(!data){fclose(f);return -3;}
    int okay=fread(data,1,(size_t)len,f)==(size_t)len;
    if(fclose(f)!=0)okay=0;
    if(!okay){free(data);return -1;}
    *dst=data;*size=(size_t)len;return 0;
}
static uint32_t word(const unsigned char *data,int big){
    if(big)return ((uint32_t)data[0]<<24)|((uint32_t)data[1]<<16)|((uint32_t)data[2]<<8)|data[3];
    return ((uint32_t)data[3]<<24)|((uint32_t)data[2]<<16)|((uint32_t)data[1]<<8)|data[0];
}
int lab_diff_values(const char *before_path,const char *after_path,
                    uint32_t before_value,uint32_t after_value,LabDiff *out) {
    if(!out||!before_path||!after_path)return -1;
    memset(out,0,sizeof(*out));
    if(before_value==after_value)return -4;
    unsigned char *a=NULL,*b=NULL;size_t len_a=0,len_b=0;
    int rc=read_data(before_path,&a,&len_a);if(rc)return rc;
    rc=read_data(after_path,&b,&len_b);
    if(rc){free(a);return rc;}
    if(len_a!=len_b){free(a);free(b);return -5;}
    out->size=(uint32_t)len_a;
    int inside=0;
    for(size_t i=0;i<len_a;i++){
        if(a[i]!=b[i]){
            out->changed_bytes++;
            if(!inside){
                out->changed_ranges++;
                if(!out->first_difference)out->first_difference=(uint32_t)i;
            }
            out->last_difference=(uint32_t)i;
            inside=1;
        }else inside=0;
    }
    for(size_t i=0;i+4<=len_a;i++){
        for(int endian=0;endian<2;endian++){
            if(word(a+i,endian)==before_value && word(b+i,endian)==after_value){
                if(out->matches<LAB_DIFF_MATCHES){
                    out->entries[out->matches].offset=(uint32_t)i;
                    out->entries[out->matches].big_endian=endian;
                    out->matches++;
                }else out->too_many_matches=1;
            }
        }
    }
    free(a);free(b);return 0;
}
const char *lab_diff_error(int rc){
    switch(rc){
        case 0:return "OK";
        case -1:return "CANNOT READ SAVE";
        case -2:return "SAVE SIZE UNSUPPORTED";
        case -3:return "OUT OF MEMORY";
        case -4:return "BEFORE AND AFTER VALUES MUST DIFFER";
        case -5:return "SAVE SIZES DIFFER";
        default:return "UNKNOWN ERROR";
    }
}
