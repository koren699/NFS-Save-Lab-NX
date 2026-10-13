/*
 * Most Wanted 2005 Xbox 360 recompilation MC02 integrity / cash.
 * MD5: RFC 1321; EA CRC32: non-reflected polynomial 04C11DB7 custom
 * framing verified on two authentic 62688-byte user saves.
 * The user confirmed an independently patched 400000 cash save loads.
 */
#include "mc02.h"
#include "backup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static uint32_t be32(const unsigned char *p){
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void put_be32(unsigned char *p,uint32_t x){
 p[0]=(unsigned char)(x>>24);p[1]=(unsigned char)(x>>16);
 p[2]=(unsigned char)(x>>8);p[3]=(unsigned char)x;
}
static uint32_t rot(uint32_t x,unsigned count){return (x<<count)|(x>>(32-count));}
static const uint32_t md5_k[64]={
0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,0x6d9d6122,
0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
static const unsigned char md5_s[64]={
 7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
 5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
 4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
 6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};
static void md5_step(uint32_t state[4],const unsigned char block[64]){
 uint32_t m[16];
 for(int i=0;i<16;i++){const unsigned char *p=block+4*i;
   m[i]=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
 uint32_t a=state[0],b=state[1],c=state[2],d=state[3];
 for(int i=0;i<64;i++){
  uint32_t f;int g;
  if(i<16){f=(b&c)|((~b)&d);g=i;}
  else if(i<32){f=(d&b)|((~d)&c);g=(5*i+1)%16;}
  else if(i<48){f=b^c^d;g=(3*i+5)%16;}
  else{f=c^(b|~d);g=(7*i)%16;}
  uint32_t tmp=d;d=c;c=b;b=b+rot(a+f+md5_k[i]+m[g],md5_s[i]);a=tmp;
 }
 state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;
}
static void md5_sum(const unsigned char *msg,size_t len,unsigned char result[16]){
 uint32_t state[4]={0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u};
 size_t full=len/64;
 for(size_t i=0;i<full;i++)md5_step(state,msg+64*i);
 unsigned char tail[128]={0};size_t rem=len%64;
 memcpy(tail,msg+full*64,rem);tail[rem]=0x80;
 size_t total=rem<56?64:128;
 uint64_t bits=(uint64_t)len*8;
 for(int i=0;i<8;i++)tail[total-8+i]=(unsigned char)(bits>>(i*8));
 md5_step(state,tail);if(total==128)md5_step(state,tail+64);
 for(int i=0;i<4;i++)for(int j=0;j<4;j++)result[i*4+j]=(unsigned char)(state[i]>>(j*8));
}
static uint32_t ea_crc32(const unsigned char *data,size_t n){
 if(n<4)return 0;
 uint32_t crc=~be32(data);
 for(size_t i=4;i<n;i++){
  uint8_t b=data[i];uint32_t idx=crc>>24;
  uint32_t table=idx<<24;
  for(int k=0;k<8;k++)table=(table<<1)^((table&0x80000000u)?0x04C11DB7u:0u);
  crc=((crc<<8)|b)^table;
 }
 return ~crc;
}
int mc02_verify(const unsigned char *buf,size_t len,uint32_t *cash){
 if(!buf||len!=MC02_LEN||memcmp(buf,"MC02",4)||be32(buf+4)!=MC02_LEN||
    be32(buf+12)!=MC02_LEN-0x24u)return -1;
 unsigned char md5[16];md5_sum(buf+0x34,len-0x34-16,md5);
 if(memcmp(md5,buf+len-16,16)!=0)return -2;
 if(be32(buf+0x10)!=ea_crc32(buf+0x1c,8)||
    be32(buf+0x14)!=ea_crc32(buf+0x24,len-0x24)||
    be32(buf+0x18)!=ea_crc32(buf,0x18))return -2;
 if(cash)*cash=be32(buf+MC02_CASH_OFFSET);
 return 0;
}
int mc02_build(const unsigned char *input,size_t len,uint32_t expected_cash,
               uint32_t desired_cash,unsigned char *output){
 if(!input||!output||input==output)return -3;
 if(desired_cash>MC02_CASH_MAX)return -4;
 uint32_t before=0;
 int r=mc02_verify(input,len,&before);if(r)return r;
 if(before!=expected_cash)return -5;
 if(desired_cash==before)return -6;
 memcpy(output,input,len);
 put_be32(output+MC02_CASH_OFFSET,desired_cash);
 put_be32(output+0x10,ea_crc32(output+0x1c,8));
 md5_sum(output+0x34,len-0x34-16,output+len-16);
 put_be32(output+0x14,ea_crc32(output+0x24,len-0x24));
 put_be32(output+0x18,ea_crc32(output,0x18));
 uint32_t after=0;
 if(mc02_verify(output,len,&after)!=0||after!=desired_cash)return -7;
 return 0;
}
static int read_mc02(const char *path,unsigned char *bytes){
 struct stat st;if(!path||stat(path,&st)||!S_ISREG(st.st_mode)||st.st_size!=MC02_LEN)return -1;
 FILE *f=fopen(path,"rb");if(!f)return -8;
 size_t n=fread(bytes,1,MC02_LEN,f);
 int ok=(n==MC02_LEN && fgetc(f)==EOF && !ferror(f));
 if(fclose(f)!=0)ok=0;
 return ok?0:-8;
}
int mc02_read_cash(const char *path,uint32_t *cash){
 if(!cash)return -3;
 unsigned char *buf=malloc(MC02_LEN);if(!buf)return -9;
 int r=read_mc02(path,buf);
 if(!r)r=mc02_verify(buf,MC02_LEN,cash);
 free(buf);return r;
}
static int available(const char *p){
 struct stat st;
 return stat(p,&st)!=0 && errno==ENOENT;
}
int mc02_apply_cash(const char *path,uint32_t expected_cash,uint32_t desired_cash,
                    char *backup_path,size_t cap){
 if(!path||!backup_path||cap<64)return -3;
 backup_path[0]=0;
 size_t pl=strlen(path);
 if(pl>800||strstr(path,".nfslab.")||!strcmp(path,"")||strstr(path,"/Headers/"))return -3;
 unsigned char *old=malloc(MC02_LEN),*next=malloc(MC02_LEN),*verify=malloc(MC02_LEN);
 if(!old||!next||!verify){free(old);free(next);free(verify);return -9;}
 int r=read_mc02(path,old);
 if(r)goto end;
 r=mc02_build(old,MC02_LEN,expected_cash,desired_cash,next);
 if(r)goto end;
 char temp[1024],part[1024],bk[1024];
 if(snprintf(temp,sizeof(temp),"%s.nfslab.mc02.tmp",path)<0||
    snprintf(part,sizeof(part),"%s.nfslab.mc02.oldpart",path)<0){r=-3;goto end;}
 if(!available(temp)||!available(part)){r=-10;goto end;}
 r=lab_make_backup(path,bk,sizeof(bk));
 if(r){r=-11;goto end;}
 snprintf(backup_path,cap,"%s",bk);
 /* Check the original has not been replaced since it was validated. */
 r=read_mc02(path,verify);
 if(r||memcmp(old,verify,MC02_LEN)){r=-12;goto end;}
 FILE *f=fopen(temp,"wb");
 if(!f){r=-8;goto end;}
 int success=fwrite(next,1,MC02_LEN,f)==MC02_LEN && fflush(f)==0;
 if(fclose(f)!=0)success=0;
 if(!success){remove(temp);r=-8;goto end;}
 r=read_mc02(temp,verify);
 if(r||memcmp(next,verify,MC02_LEN)){remove(temp);r=-12;goto end;}
 if(rename(path,part)!=0){remove(temp);r=-8;goto end;}
 if(rename(temp,path)!=0){
  if(rename(part,path)!=0){r=-13;goto end;}
  remove(temp);r=-8;goto end;
 }
 r=read_mc02(path,verify);
 if(r||memcmp(next,verify,MC02_LEN)||mc02_verify(verify,MC02_LEN,0)){
  /* Attempt rollback. Leave oldpart + backup intact if anything fails. */
  char failed[1024];
  snprintf(failed,sizeof(failed),"%s.nfslab.mc02.failed",path);
  if(available(failed))rename(path,failed);
  if(rename(part,path)!=0){r=-13;goto end;}
  r=-12;goto end;
 }
 remove(part);r=0;
end:
 free(old);free(next);free(verify);return r;
}
const char *mc02_error(int code){
 switch(code){
  case 0:return "OK";
  case -1:return "UNSUPPORTED SAVE FORMAT/SIZE";
  case -2:return "HASH CHECK FAILED";
  case -3:return "INVALID INPUT";
  case -4:return "CASH OUT OF RANGE";
  case -5:return "CASH CHANGED SINCE SELECTION";
  case -6:return "CASH ALREADY MATCHES";
  case -7:return "OUTPUT HASH VALIDATION FAILED";
  case -8:return "SD CARD FILE ERROR";
  case -9:return "OUT OF MEMORY";
  case -10:return "TEMP/RECOVERY FILE EXISTS";
  case -11:return "FAILED TO MAKE VERIFIED BACKUP";
  case -12:return "FILE VERIFICATION FAILED";
  case -13:return "RECOVER ORIGINAL FROM .oldpart OR .bak";
  default:return "UNKNOWN ERROR";
 }
}
