#include "../source/engine.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
int main(void){
 const char *p="tests/test_sample.bin";
 unsigned char bytes[128]={0};
 /* LE 123456 @ 16, BE 654321 @ 64 */
 bytes[16]=0x40;bytes[17]=0xe2;bytes[18]=0x01;bytes[19]=0x00;
 bytes[64]=0x00;bytes[65]=0x09;bytes[66]=0xfb;bytes[67]=0xf1;
 FILE *f=fopen(p,"wb");assert(f);
 assert(fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes));fclose(f);
 LabMatches matches;
 assert(lab_scan(p,123456,&matches)==0 && matches.count==1);
 assert(matches.items[0].offset==16 && matches.items[0].be==0);
 assert(lab_scan(p,654321,&matches)==0 && matches.count==1);
 assert(matches.items[0].offset==64 && matches.items[0].be==1);
 assert(lab_scan(p,765432,&matches)==0 && matches.count==0);
 unsigned char original[128];
 f=fopen(p,"rb");assert(f);assert(fread(original,1,sizeof(original),f)==sizeof(original));fclose(f);
 assert(memcmp(original,bytes,sizeof(bytes))==0);
 remove(p);
 puts("PASS: read-only LE/BE number scan and zero mutations");
 return 0;
}
