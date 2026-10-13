/* JDLZ v2 compressor adapted to ISO C from the zombie28 compressor
 * documented at MWisBest/OpenNFSTools, LibNFS/Compression/JDLZ.cs.
 * Used only after decoding and full revalidation of an authentic 20CM save.
 */
#include "carbon_jdlz.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define HASH_SIZE 8192u
#define MAX_DEPTH 16
#define MIN_LEN 3
static int emit(unsigned char *dst,size_t cap,size_t *at,unsigned char x){
 if(*at>=cap)return -1;
 dst[(*at)++]=x;return 0;
}
static void put_le32(unsigned char *p,uint32_t n){
 for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));
}
int carbon_jdlz_compress(const unsigned char *src,size_t n,
                         unsigned char *dst,size_t cap,size_t *written){
 if(!src||!dst||!written||n<1||n>0x8000u||cap<24)return -1;
 *written=0;
 int *hashpos=(int *)calloc(HASH_SIZE,sizeof(int));
 int *chain=(int *)calloc(n,sizeof(int));
 if(!hashpos||!chain){free(hashpos);free(chain);return -2;}
 size_t out=0,in=0,remaining=n;
 unsigned bits1=1,bits2=1,flag1=0,flag2=0;
 unsigned f1pos=16u,f2pos=17u;
 int result=-1;
 do{
  memcpy(dst,"JDLZ",4);dst[4]=2;dst[5]=16;dst[6]=dst[7]=0;
  put_le32(dst+8,(uint32_t)n);put_le32(dst+12,0);
  out=18;
  /* The initial byte is always a literal; reserve both flag bytes. */
  bits1<<=1;
  if(emit(dst,cap,&out,src[in++]))break;
  remaining--;
  while(remaining){
   size_t bestlen=2,bestdist=0;
   if(remaining>=MIN_LEN){
    unsigned mix=(unsigned)src[in]^(((unsigned)src[in+1]^((unsigned)src[in+2]<<4))<<4);
    unsigned hash=((uint32_t)(0u-0x1a1u)*mix)&(HASH_SIZE-1u);
    int matchpos=hashpos[hash];
    hashpos[hash]=(int)in;
    chain[in]=matchpos;
    int prev=(int)in;
    for(int depth=0;depth<MAX_DEPTH;depth++){
     int distance=(int)in-matchpos;
     if(distance>2064||matchpos>=prev||distance<=0)break;
     size_t limit=(size_t)distance<=16?4098u:34u;
     if(limit>remaining)limit=remaining;
     if(bestlen>=limit)break;
     size_t ml=0;
     while(ml<limit&&src[in+ml]==src[(size_t)matchpos+ml])ml++;
     if(ml>bestlen){bestlen=ml;bestdist=(size_t)distance;}
     prev=matchpos;
     matchpos=chain[matchpos];
    }
   }
   if(bestlen>=MIN_LEN){
    flag1|=bits1;
    in+=bestlen;remaining-=bestlen;
    unsigned count=(unsigned)(bestlen-MIN_LEN);
    if(bestdist<17){
     flag2|=bits2;
     if(emit(dst,cap,&out,(unsigned char)((bestdist-1)|((count>>4)&0xf0))))break;
     if(emit(dst,cap,&out,(unsigned char)count))break;
    }else{
     unsigned distance=(unsigned)(bestdist-17);
     if(emit(dst,cap,&out,(unsigned char)(count|((distance>>3)&0xe0))))break;
     if(emit(dst,cap,&out,(unsigned char)distance))break;
    }
    bits2<<=1;
   }else{
    if(emit(dst,cap,&out,src[in++]))break;
    remaining--;
   }
   bits1<<=1;
   if(bits1==256u){
    dst[f1pos]=(unsigned char)flag1;
    flag1=0;f1pos=(unsigned)out;bits1=1;
    if(emit(dst,cap,&out,0))break;
   }
   if(bits2==256u){
    dst[f2pos]=(unsigned char)flag2;
    flag2=0;f2pos=(unsigned)out;bits2=1;
    if(emit(dst,cap,&out,0))break;
   }
  }
  if(remaining)break;
  if(bits2>1)dst[f2pos]=(unsigned char)flag2;
  else if(f2pos==out-1)out=f2pos;
  if(bits1>1)dst[f1pos]=(unsigned char)flag1;
  else if(f1pos==out-1)out=f1pos;
  if(out>UINT32_MAX)break;
  put_le32(dst+12,(uint32_t)out);
  *written=out;
  result=0;
 }while(0);
 free(hashpos);free(chain);return result;
}
