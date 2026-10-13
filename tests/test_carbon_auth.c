#include "../source/carbon_auth.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void le32(unsigned char *p,uint32_t v){
 p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);
 p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);
}
int main(void){
 unsigned char title[16],expected[60]={
 0xea,0xb5,0x7e,0x93,0xeb,0x94,0xc4,0x3a,0x98,0xb9,0x6b,0x9d,0xbd,0x03,0xcb,0x28,0x1f,0x34,0x22,0x75,
 0xea,0xe9,0x51,0x00,0x63,0xe4,0xb7,0x76,0xd0,0xb4,0x7e,0x51,0x79,0x8f,0xa0,0x2c,0x1f,0x1d,0x35,0x6c,
 0x71,0x0c,0xca,0x4f,0x32,0xa4,0x85,0x92,0xbb,0xed,0xd2,0x22,0x26,0x08,0x9c,0x58,0x85,0xbd,0x9f,0x08
 };
 for(unsigned i=0;i<16;i++)title[i]=(unsigned char)(i+1);
 unsigned char *save=calloc(1,0x4005c);
 assert(save);
 memcpy(save,"20CM",4);
 le32(save+4,0x4005c);
 le32(save+8,16);
 le32(save+12,0x40000);
 memcpy(save+0x4c,"WRAPPER!",8);
 memcpy(save+0x54,"CARBONZZZ",9);
 assert(carbon_auth_sign_buffer(save,0x4005c,title)==0);
 assert(memcmp(save+0x10,expected,60)==0);
 assert(carbon_auth_verify_buffer(save,0x4005c,title)==0);
 save[0x100]^=1;
 assert(carbon_auth_verify_buffer(save,0x4005c,title)==-3);
 save[0x100]^=1;
 assert(carbon_auth_verify_buffer(save,0x4005c,title)==0);
 unsigned char xbe[0x400]={0};
 memcpy(xbe,"XBEH",4);
 le32(xbe+0x104,0x10000);
 le32(xbe+0x108,0x400);
 le32(xbe+0x118,0x10180);
 le32(xbe+0x180,0x1dc);
 le32(xbe+0x188,0x4541009e);
 memcpy(xbe+0x240,title,16);
 const char *xbe_path="tests/carbon_auth_mock.xbe";
 const char *save_path="tests/carbon_auth_mock.sav";
 FILE *f=fopen(xbe_path,"wb");assert(f);
 assert(fwrite(xbe,1,sizeof(xbe),f)==sizeof(xbe));assert(!fclose(f));
 f=fopen(save_path,"wb");assert(f);
 assert(fwrite(save,1,0x4005c,f)==0x4005c);assert(!fclose(f));
 assert(carbon_auth_verify_file(save_path,xbe_path)==0);
 xbe[0x188]=0;
 f=fopen(xbe_path,"wb");assert(f);
 assert(fwrite(xbe,1,sizeof(xbe),f)==sizeof(xbe));assert(!fclose(f));
 assert(carbon_auth_verify_file(save_path,xbe_path)==-2);
 assert(!remove(xbe_path));
 assert(!remove(save_path));
 free(save);
 puts("PASS: Carbon title XBE lookup, 3x Xbox HMAC SHA1 known vectors and tamper detection");
 return 0;
}
