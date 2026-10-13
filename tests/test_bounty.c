#include "../source/mc02.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void load(const char *p,unsigned char *b){
 FILE *f=fopen(p,"rb");assert(f);
 assert(fread(b,1,MC02_LEN,f)==MC02_LEN);assert(fgetc(f)==EOF);
 assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==4);
 unsigned char *twocar=malloc(MC02_LEN),*onecar=malloc(MC02_LEN),
               *expected=malloc(MC02_LEN),*edited=malloc(MC02_LEN);
 assert(twocar&&onecar&&expected&&edited);
 load(argv[1],twocar);load(argv[2],onecar);load(argv[3],expected);
 Mc02BountyInfo b;
 assert(mc02_bounty_inspect(twocar,MC02_LEN,&b)==0);
 assert(b.sold_history==0 && b.active_count==2 && b.total==31550);
 assert(b.active_slots[0]==0 && b.active_bounty[0]==22550);
 assert(b.active_slots[1]==1 && b.active_bounty[1]==9000 && b.preferred_index==1);
 assert(mc02_bounty_inspect(onecar,MC02_LEN,&b)==0);
 assert(b.sold_history==22550 && b.active_count==1 && b.total==45150);
 assert(b.active_slots[0]==1 && b.active_bounty[0]==22600 && b.preferred_index==0);
 assert(mc02_build_bounty(onecar,MC02_LEN,45150,100000,0,edited)==0);
 assert(memcmp(edited,expected,MC02_LEN)==0);
 assert(mc02_bounty_inspect(edited,MC02_LEN,&b)==0 && b.total==100000);
 assert(b.active_count==1 && b.active_bounty[0]==77450 && b.sold_history==22550);
 assert(mc02_build_bounty(onecar,MC02_LEN,45149,100000,0,edited)==-5);
 assert(mc02_build_bounty(onecar,MC02_LEN,45150,45150,0,edited)==-6);
 assert(mc02_build_bounty(onecar,MC02_LEN,45150,20000,0,edited)==-20);
 assert(mc02_build_bounty(onecar,MC02_LEN,45150,100000,1,edited)==-19);
 assert(mc02_build_bounty(twocar,MC02_LEN,31550,100000,1,edited)==0);
 assert(mc02_bounty_inspect(edited,MC02_LEN,&b)==0 && b.total==100000 && b.sold_history==0);
 assert(mc02_build_bounty(twocar,MC02_LEN,31550,100000,0,edited)==0);
 assert(mc02_bounty_inspect(edited,MC02_LEN,&b)==0 && b.total==100000);
 onecar[0xe32d+7]=1;
 assert(mc02_build_bounty(onecar,MC02_LEN,45150,100000,0,edited)==-2);
 onecar[0xe32d+7]=0;
 char backup[1024];
 const char *live="tests/bounty_live_test.bin";
 FILE *fp=fopen(live,"wb");assert(fp);assert(fwrite(onecar,1,MC02_LEN,fp)==MC02_LEN);
 assert(fclose(fp)==0);
 assert(mc02_apply_bounty(live,45150,100000,0,backup,sizeof(backup))==0);
 load(live,edited);assert(memcmp(edited,expected,MC02_LEN)==0);
 load(backup,edited);assert(memcmp(edited,onecar,MC02_LEN)==0);
 assert(mc02_apply_bounty(live,45150,200000,0,backup,sizeof(backup))==-5);
 load(live,edited);assert(memcmp(edited,expected,MC02_LEN)==0);
 assert(mc02_read_bounty_info(live,&b)==0 && b.total==100000);
 assert(remove(live)==0);assert(remove("tests/bounty_live_test.bin.nfslab.bak1")==0);
 free(twocar);free(onecar);free(expected);free(edited);
 puts("PASS: sold-history aggregation, retired slot filtering, multi-car selection, verified Bounty write+backup");
 return 0;
}
