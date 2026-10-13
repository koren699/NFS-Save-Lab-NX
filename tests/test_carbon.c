#include "../source/carbon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_profile(const char *p,unsigned char *buf){
 FILE *f=fopen(p,"rb");assert(f);
 assert(fread(buf,1,CARBON_ORIGINAL_XBOX_FILE_LEN,f)==CARBON_ORIGINAL_XBOX_FILE_LEN);
 assert(fgetc(f)==EOF);assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==3);
 uint32_t cash=0;
 assert(carbon_read_cash(argv[1],&cash)==0 && cash==5000);
 assert(carbon_read_cash(argv[2],&cash)==0 && cash==10200);
 unsigned char *b=malloc(CARBON_ORIGINAL_XBOX_FILE_LEN);
 assert(b);
 read_profile(argv[2],b);
 assert(carbon_parse_cash(b,CARBON_ORIGINAL_XBOX_FILE_LEN,&cash)==0&&cash==10200);
 b[0]=0;
 assert(carbon_parse_cash(b,CARBON_ORIGINAL_XBOX_FILE_LEN,&cash)==-1);
 b[0]='2';
 b[0x34bc0]='Z';
 assert(carbon_parse_cash(b,CARBON_ORIGINAL_XBOX_FILE_LEN,&cash)==-6);
 b[0x34bc0]='J';
 b[0x34bc0+12]=0xff;
 assert(carbon_parse_cash(b,CARBON_ORIGINAL_XBOX_FILE_LEN,&cash)==-7);
 free(b);
 puts("PASS: Carbon read-only original Xbox cash decoder (5,000 -> 10,200), rejected corruption");
 return 0;
}
