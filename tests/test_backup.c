#include "../source/backup.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
static void check(const char *p, const char *want) {
 FILE *f=fopen(p,"rb");assert(f);
 char buf[128]={0};size_t n=fread(buf,1,sizeof(buf)-1,f);fclose(f);
 assert(n==strlen(want) && memcmp(buf,want,n)==0);
}
static void write_file(const char *p, const char *s) {
 FILE *f=fopen(p,"wb");assert(f);assert(fwrite(s,1,strlen(s),f)==strlen(s));fclose(f);
}
int main(void){
 const char *p="tests/restore_sample.bin";
 write_file(p,"original save data");
 char backup[1024]={0}, safety[1024]={0};
 assert(lab_make_backup(p,backup,sizeof(backup))==0);
 assert(strstr(backup,".nfslab.bak1")!=NULL);
 check(backup,"original save data");
 write_file(p,"changed contents!");
 assert(lab_restore_backup(p,backup,safety,sizeof(safety))==0);
 assert(strstr(safety,".nfslab.bak2")!=NULL);
 check(p,"original save data");
 check(backup,"original save data");
 check(safety,"changed contents!");
 assert(lab_restore_backup(p,"tests/otherfile",safety,sizeof(safety))==-7);
 remove(p);remove(backup);remove("tests/restore_sample.bin.nfslab.bak2");
 puts("PASS: unique backup / verified restore / safety snapshot / invalid source");
 return 0;
}
