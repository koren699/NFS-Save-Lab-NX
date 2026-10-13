#include "../source/mc02.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void load(const char *p,unsigned char *b){
 FILE *f=fopen(p,"rb");assert(f);
 assert(fread(b,1,MC02_LEN,f)==MC02_LEN);assert(fgetc(f)==EOF);
 assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==3);
 unsigned char *a=malloc(MC02_LEN),*want=malloc(MC02_LEN),*got=malloc(MC02_LEN);
 assert(a&&want&&got);
 load(argv[1],a);load(argv[2],want);
 uint32_t cash=0;
 assert(mc02_verify(a,MC02_LEN,&cash)==0 && cash==900);
 assert(mc02_verify(want,MC02_LEN,&cash)==0 && cash==400000);
 assert(mc02_build(a,MC02_LEN,900,400000,got)==0);
 assert(memcmp(got,want,MC02_LEN)==0);
 assert(mc02_build(a,MC02_LEN,901,400000,got)==-5);
 assert(mc02_build(a,MC02_LEN,900,900,got)==-6);
 a[0x4039]^=1;
 assert(mc02_verify(a,MC02_LEN,&cash)==-2);
 assert(mc02_build(a,MC02_LEN,900,400000,got)==-2);
 a[0x4039]^=1;
 assert(mc02_verify(a,MC02_LEN-1,&cash)==-1);
 char backup[1024];
 const char *live="tests/mc02_live_test.bin";
 FILE *f=fopen(live,"wb");assert(f);
 assert(fwrite(a,1,MC02_LEN,f)==MC02_LEN);assert(fclose(f)==0);
 assert(mc02_apply_cash(live,900,400000,backup,sizeof(backup))==0);
 load(live,got);assert(memcmp(got,want,MC02_LEN)==0);
 load(backup,got);assert(memcmp(got,a,MC02_LEN)==0);
 assert(mc02_apply_cash(live,900,500000,backup,sizeof(backup))==-5);
 load(live,got);assert(memcmp(got,want,MC02_LEN)==0);
 assert(mc02_read_cash(live,&cash)==0 && cash==400000);
 remove(live);remove("tests/mc02_live_test.bin.nfslab.bak1");
 free(a);free(want);free(got);
 puts("PASS: native MC02 validation, Python byte-identical hashes, backup + safe cash write");
 return 0;
}
