#include "../source/engine.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
int main(void){
    const char *p="tests/test_sample.bin";
    unsigned char buf[128]={0};
    /* Little-endian $123456 @ +16, big-endian $654321 @ +64. */
    buf[16]=0x40;buf[17]=0xe2;buf[18]=0x01;buf[19]=0x00;
    buf[64]=0;buf[65]=9;buf[66]=0xfb;buf[67]=0xf1;
    FILE *f=fopen(p,"wb");assert(f);assert(fwrite(buf,1,sizeof(buf),f)==sizeof(buf));fclose(f);
    LabMatches matches;assert(lab_scan(p,123456,&matches)==0);assert(matches.count==1);
    assert(matches.items[0].offset==16 && matches.items[0].be==0);
    char bk[1024];assert(lab_patch(p,123456,9999999,matches.items[0],bk,sizeof(bk))==0);
    assert(lab_scan(p,9999999,&matches)==0 && matches.count==1 && matches.items[0].offset==16);
    assert(lab_scan(p,654321,&matches)==0 && matches.count==1 && matches.items[0].offset==64 && matches.items[0].be==1);
    assert(lab_patch(p,654321,5000000,matches.items[0],bk,sizeof(bk))==0);
    assert(lab_scan(p,5000000,&matches)==0 && matches.count==1);
    remove(p);remove("tests/test_sample.bin.nfslab.bak1");remove("tests/test_sample.bin.nfslab.bak2");
    puts("PASS: scan LE + BE, safe patch, backups, rescan");return 0;
}
