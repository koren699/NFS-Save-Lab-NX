#include "../source/u2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void read_it(const char *p,unsigned char *out){
 FILE *f=fopen(p,"rb");assert(f);
 assert(fread(out,1,U2_SAVE_LEN,f)==U2_SAVE_LEN);
 assert(fgetc(f)==EOF);assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==3);
 const char *save=argv[1],*expected=argv[2];
 unsigned char *old=malloc(U2_SAVE_LEN),*patch=malloc(U2_SAVE_LEN),
               *ref=malloc(U2_SAVE_LEN);
 assert(old&&patch&&ref);
 read_it(save,old);read_it(expected,ref);
 assert(u2_verify(old,U2_SAVE_LEN)==0);
 assert(u2_verify(ref,U2_SAVE_LEN)==0);
 assert(u2_build_cash(old,U2_SAVE_LEN,125,100000,patch)==0);
 assert(memcmp(ref,patch,U2_SAVE_LEN)==0);
 assert(u2_build_cash(old,U2_SAVE_LEN,126,100000,patch)==-5);
 assert(u2_build_cash(old,U2_SAVE_LEN,125,125,patch)==-6);
 assert(u2_build_cash(old,U2_SAVE_LEN,125,1000000000u,patch)==-4);
 old[0x200]^=0x80;
 assert(u2_verify(old,U2_SAVE_LEN)==-2);
 old[0x200]^=0x80;
 assert(u2_verify(old,U2_SAVE_LEN)==0);
 uint32_t money=0;
 assert(u2_read_cash(save,&money)==0 && money==125);
 char bk[1024]={0};
 assert(u2_apply_cash(save,126,100000,bk,sizeof(bk))==-5);
 assert(u2_read_cash(save,&money)==0 && money==125);
 assert(u2_apply_cash(save,125,100000,bk,sizeof(bk))==0);
 assert(u2_read_cash(save,&money)==0 && money==100000);
 assert(u2_verify_file(save)==0);
 read_it(save,patch);
 assert(memcmp(patch,ref,U2_SAVE_LEN)==0);
 assert(u2_read_cash(bk,&money)==0 && money==125);
 char successful_bk[1024];snprintf(successful_bk,sizeof(successful_bk),"%s",bk);
 assert(u2_apply_cash(save,125,500000,bk,sizeof(bk))==-5);
 assert(u2_read_cash(save,&money)==0 && money==100000);
 assert(remove(successful_bk)==0);
 free(old);free(patch);free(ref);
 puts("PASS: Underground 2 verified 20CM/HMAC Cash write, isolated field, backup, rollback guards");
 return 0;
}
