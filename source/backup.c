#include "backup.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>

static int file_ok(const char *path) {
    struct stat s;
    if (stat(path, &s) != 0 || !S_ISREG(s.st_mode)) return -1;
    if (s.st_size < 1 || (unsigned long long)s.st_size > LAB_MAX_FILE) return -2;
    return 0;
}
static int available(const char *path) {
    struct stat s;
    return stat(path, &s) != 0 && errno == ENOENT;
}
static int next_backup_name(const char *path, char *out, size_t cap) {
    if (!path || !out || cap == 0) return -3;
    for (int i = 1; i <= 999; i++) {
        int n = snprintf(out, cap, "%s.nfslab.bak%d", path, i);
        if (n < 1 || (size_t)n >= cap) return -3;
        if (available(out)) return 0;
    }
    return -4;
}
/* fnv64 verifies that an SD read produced the same bytes as the source.
   It is NOT a game-save signature or checksum repair. */
static uint64_t checksum_file(const char *path, int *err) {
    uint64_t h = UINT64_C(14695981039346656037);
    unsigned char block[8192];
    FILE *f = fopen(path, "rb");
    if (!f) {*err=-5;return 0;}
    size_t n;
    while ((n = fread(block, 1, sizeof(block), f)) > 0) {
        for (size_t i=0;i<n;i++) h=(h^block[i])*UINT64_C(1099511628211);
    }
    *err=ferror(f)?-5:0;
    if(fclose(f)!=0)*err=-5;
    return h;
}
static int copy_verify(const char *source, const char *dest) {
    FILE *in = fopen(source,"rb");
    if (!in) return -5;
    FILE *out = fopen(dest,"wb");
    if (!out){fclose(in);return -5;}
    int error=0;
    size_t n;
    unsigned char block[8192];
    while((n=fread(block,1,sizeof(block),in))>0) {
        if(fwrite(block,1,n,out)!=n){error=-5;break;}
    }
    if(ferror(in))error=-5;
    if(fflush(out)!=0)error=-5;
    if(fclose(out)!=0)error=-5;
    if(fclose(in)!=0)error=-5;
    if(error){remove(dest);return error;}
    int e1=0,e2=0;
    uint64_t src=checksum_file(source,&e1),dst=checksum_file(dest,&e2);
    if(e1||e2||src!=dst){remove(dest);return -6;}
    return 0;
}
int lab_make_backup(const char *save,char *created,size_t cap) {
    if(!save || !created || !cap) return -3;
    created[0]=0;
    int rc=file_ok(save);
    if(rc)return rc;
    char path[1024],part[1024];
    rc=next_backup_name(save,path,sizeof(path));
    if(rc)return rc;
    int k=snprintf(part,sizeof(part),"%s.nfslab.part",save);
    if(k<1 || (size_t)k>=sizeof(part) || !available(part))return -4;
    if(strlen(path)+1>cap)return -3;
    rc=copy_verify(save,part);
    if(rc)return rc;
    if(rename(part,path)!=0){remove(part);return -5;}
    snprintf(created,cap,"%s",path);
    return 0;
}
static int valid_snapshot(const char *save,const char *backup) {
    if(!save || !backup)return 0;
    char prefix[1024];
    int k=snprintf(prefix,sizeof(prefix),"%s.nfslab.bak",save);
    if(k<0 || (size_t)k>=sizeof(prefix))return 0;
    size_t n=strlen(prefix);
    if(strncmp(backup,prefix,n)!=0)return 0;
    const char *s=backup+n;
    if(!*s)return 0;
    int value=0;
    while(*s){if(*s<'0'||*s>'9')return 0;
        value=value*10+(*s++-'0');if(value>999)return 0;}
    return value>=1;
}
int lab_restore_backup(const char *save,const char *snapshot,
                       char *safety_backup,size_t cap) {
    if(!safety_backup || cap==0)return -3;
    safety_backup[0]=0;
    if(!valid_snapshot(save,snapshot))return -7;
    int rc=file_ok(save);if(rc)return rc;
    rc=file_ok(snapshot);if(rc)return rc;
    char temp[1024];
    int k=snprintf(temp,sizeof(temp),"%s.nfslab.restore.part",save);
    if(k<0 || (size_t)k>=sizeof(temp)||!available(temp))return -4;
    rc=copy_verify(snapshot,temp);if(rc)return rc;
    /* Before replacing current file, retain its intact contents. */
    rc=lab_make_backup(save,safety_backup,cap);
    if(rc){remove(temp);return rc;}
    if(rename(temp,save)!=0){remove(temp);return -5;}
    return 0;
}
const char *lab_backup_error(int code) {
    switch(code) {
       case 0:return "SUCCESS";
       case -1:return "FILE MISSING / NOT REGULAR";
       case -2:return "FILE SIZE UNSUPPORTED";
       case -3:return "BAD PATH / BUFFER";
       case -4:return "NO FREE BACKUP NAME";
       case -5:return "FILE I/O ERROR";
       case -6:return "COPY VERIFICATION FAILED";
       case -7:return "INVALID RESTORE SOURCE";
       default:return "UNKNOWN ERROR";
    }
}
