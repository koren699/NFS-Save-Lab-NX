#include "carbon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t le32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
#define UNPACK_LEN 32768u
#define BLOCK_MIN 0x34bc0u
#define MAX_COMPRESSED 0xB49Cu
/*
 * Strict bounded decoder of EA's original JDLZ v2. Both control bytes
 * are fetched when their counters expire; backreferences may overlap.
 * This is solely a decoder; Carbon editing is NOT enabled yet.
 */
static int jdlz_unpack(const unsigned char *data,size_t stored,
                       unsigned char *out,size_t outlen){
 if(stored<16||memcmp(data,"JDLZ",4)||data[4]!=2)return -7;
 uint32_t raw=le32(data+8),used=le32(data+12);
 if(raw!=outlen||used<16||used>stored||used>MAX_COMPRESSED)return -7;
 size_t in=16,at=0; unsigned f1=1,f2=1;
 while(at<outlen){
  if(f1==1){
   if(in>=used)return -7;
   f1=(unsigned)data[in++]|0x100u;
  }
  if(f2==1){
   if(in>=used)return -7;
   f2=(unsigned)data[in++]|0x100u;
  }
  if(f1&1u){
   if(in+2>used)return -7;
   unsigned a=data[in],b=data[in+1],len,back;
   if(f2&1u){
    len=(b|((a&0xf0u)<<4))+3;
    back=(a&0x0fu)+1;
   }else{
    back=(b|((a&0xe0u)<<3))+17;
    len=(a&0x1fu)+3;
   }
   in+=2;
   if(!back||back>at||len>outlen-at)return -7;
   for(unsigned i=0;i<len;i++){out[at]=out[at-back];at++;}
   f2>>=1;
  }else{
   if(in>=used)return -7;
   out[at++]=data[in++];
  }
  f1>>=1;
 }
 /* EA compressor may leave a final 1-2 padding bytes. */
 if(used-in>16)return -7;
 return 0;
}
static int parse_money(const unsigned char *text,size_t size,uint32_t *cash){
 static const char name[]="2287685084:\t";
 const size_t sl=sizeof(name)-1;
 unsigned nfound=0;
 for(size_t i=0;i+sl<size;i++){
  if(memcmp(text+i,name,sl)!=0)continue;
  if(i&&text[i-1]!='\n')continue;
  size_t at=i+sl;
  /* name[0] + fields[1..6] precede field[7] (money). */
  for(int f=1;f<7;f++){
   while(at<size && text[at]!='\n' && text[at]!='\t')at++;
   if(at==size||text[at]!='\t')return -8;
   at++;
  }
  if(at>=size||text[at]<'0'||text[at]>'9')return -8;
  uint32_t val=0;unsigned digits=0;
  while(at<size&&text[at]>='0'&&text[at]<='9'){
   unsigned d=(unsigned)(text[at++]-'0');
   if(val>99999999u || (val==99999999u && d>9))return -8;
   val=val*10+d;digits++;
   if(digits>9)return -8;
  }
  if(at==size||text[at]!='\t')return -8;
  *cash=val;nfound++;
 }
 return nfound==1?0:-8;
}
int carbon_parse_cash(const unsigned char *buf,size_t len,uint32_t *cash){
 if(!buf||!cash)return -3;
 if(len!=CARBON_ORIGINAL_XBOX_FILE_LEN||memcmp(buf,"20CM",4)||
    le32(buf+4)!=len)return -1;
 /* Known game profile layout has exactly one JDLZ block.
  * The physical location can vary in future profiles; fail on ambiguity. */
 unsigned count=0;
 size_t at=0;
 for(size_t i=BLOCK_MIN;i+16<=len;i++){
  if(memcmp(buf+i,"JDLZ",4)==0&&buf[i+4]==2){count++;at=i;}
 }
 if(count!=1)return -6;
 unsigned char *out=malloc(UNPACK_LEN);
 if(!out)return -9;
 int r=jdlz_unpack(buf+at,len-at,out,UNPACK_LEN);
 if(!r)r=parse_money(out,UNPACK_LEN,cash);
 free(out);return r;
}
int carbon_read_cash(const char *path,uint32_t *cash){
 if(!path||!cash)return -3;
 FILE *f=fopen(path,"rb");
 if(!f)return -4;
 unsigned char *buf=malloc(CARBON_ORIGINAL_XBOX_FILE_LEN);
 if(!buf){fclose(f);return -9;}
 size_t count=fread(buf,1,CARBON_ORIGINAL_XBOX_FILE_LEN,f);
 int trailing=fgetc(f);
 int closed=fclose(f);
 int r=(count!=CARBON_ORIGINAL_XBOX_FILE_LEN||trailing!=EOF||closed)?-1:
       carbon_parse_cash(buf,count,cash);
 free(buf);return r;
}
const char *carbon_error(int code){
 switch(code){
  case 0:return "OK";
  case -1:return "UNRECOGNIZED CARBON PROFILE";
  case -3:return "INVALID INPUT";
  case -4:return "CANNOT OPEN SAVE FILE";
  case -6:return "JDLZ BLOCK NOT UNIQUE";
  case -7:return "JDLZ BLOCK INVALID";
  case -8:return "MONEY RECORD NOT FOUND";
  case -9:return "OUT OF MEMORY";
  default:return "UNKNOWN CARBON ERROR";
 }
}
