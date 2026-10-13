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

/* Experimental Carbon original-Xbox test-copy writer. IMPORTANT: the game's
 * 20CM wrapper checksum/signature is not fully reverse engineered yet.
 * This creates a test file only; never replaces the live save.
 * JDLZ all-literal encoding is valid JDLZ v2 and fits this profile's
 * 0xB49C-byte reserved area; compression is deliberately simple/reliable.
 */
static void put_le32(unsigned char *p,uint32_t x){
 p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);
 p[2]=(unsigned char)(x>>16);p[3]=(unsigned char)(x>>24);
}
static size_t cash_digits(uint32_t amount,char out[16]){
 int n=snprintf(out,16,"%u",amount);
 return n>0&&n<16?(size_t)n:0;
}
static int find_cash_field(const unsigned char *buf,size_t len,size_t *start,size_t *end){
 static const unsigned char head[]="2287685084:\t";
 size_t hits=0;
 for(size_t i=0;i+sizeof(head)<len;i++){
  if(memcmp(buf+i,head,sizeof(head)-1)|| (i && buf[i-1]!='\n'))continue;
  size_t at=i+sizeof(head)-1;
  for(unsigned k=1;k<7;k++){
   while(at<len && buf[at]!='\n' && buf[at]!='\t')at++;
   if(at>=len||buf[at]!='\t')return -8;
   at++;
  }
  size_t first=at;
  while(at<len&&buf[at]>='0'&&buf[at]<='9')at++;
  if(at==first||at>=len||buf[at]!='\t')return -8;
  *start=first;*end=at;hits++;
 }
 return hits==1?0:-8;
}
static int make_cash_candidate(const unsigned char *input,size_t len,
                               uint32_t old_cash,uint32_t new_cash,
                               unsigned char *candidate){
 if(!input||!candidate||input==candidate)return -3;
 if(new_cash>999999999u)return -10;
 uint32_t check=0;
 int r=carbon_parse_cash(input,len,&check);
 if(r)return r;
 if(check!=old_cash)return -11;
 if(check==new_cash)return -12;
 const size_t base=0x34bc0u;
 const size_t frame=0x34ba8u;
 if(memcmp(input+frame,"\x22\x11\x44\x55",4)||
    le32(input+frame+4)!=32768u)return -13;
 size_t prevlen=le32(input+base+12);
 if(prevlen<16||prevlen>len-base)return -13;
 uint32_t align4=(uint32_t)((prevlen+3u)&~3u);
 if(le32(input+0x4c)!=align4+0x4cu ||
    le32(input+frame+8)!=align4+0x18u)return -13;
 for(size_t i=base+prevlen;i<len;i++)if(input[i])return -13;
 unsigned char *raw=(unsigned char*)malloc(UNPACK_LEN);
 if(!raw)return -9;
 r=jdlz_unpack(input+base,len-base,raw,UNPACK_LEN);
 if(r){free(raw);return r;}
 size_t start=0,end=0;
 r=find_cash_field(raw,UNPACK_LEN,&start,&end);
 if(r){free(raw);return r;}
 char number[16];size_t n=cash_digits(new_cash,number);
 if(!n){free(raw);return -10;}
 size_t old_digits=end-start;
 if(n>old_digits){
  size_t increase=n-old_digits;
  for(size_t i=UNPACK_LEN-increase;i<UNPACK_LEN;i++)
   if(raw[i]){free(raw);return -14;}
  memmove(raw+start+n,raw+end,UNPACK_LEN-(start+n));
 }else if(n<old_digits){
  memmove(raw+start+n,raw+end,UNPACK_LEN-end);
  memset(raw+UNPACK_LEN-(old_digits-n),0,old_digits-n);
 }
 memcpy(raw+start,number,n);
 /* A literal-only stream requires 16 + 4096 flag bytes + 1
  * secondary control byte + 32768 literal bytes = 36881 bytes.
  * The JDLZ reader obtains a new f1 byte every 8 literal tokens,
  * while f2 is read once and remains unused. */
 const size_t packed_size=16u+1u+4096u+UNPACK_LEN;
 if(packed_size>len-base){free(raw);return -14;}
 memcpy(candidate,input,len);
 memset(candidate+base,0,len-base);
 unsigned char *dst=candidate+base;
 memcpy(dst,"JDLZ",4);dst[4]=2;dst[5]=16;
 put_le32(dst+8,UNPACK_LEN);put_le32(dst+12,(uint32_t)packed_size);
 size_t op=16;
 for(size_t i=0;i<UNPACK_LEN;i++){
  if(i%8u==0){dst[op++]=0;if(i==0)dst[op++]=0;}
  dst[op++]=raw[i];
 }
 free(raw);
 if(op!=packed_size)return -14;
 uint32_t newalign=((uint32_t)packed_size+3u)&~3u;
 put_le32(candidate+frame+8,newalign+0x18u);
 put_le32(candidate+0x4c,newalign+0x4cu);
 uint32_t verify=0;
 r=carbon_parse_cash(candidate,len,&verify);
 if(r||verify!=new_cash)return -15;
 /* The PC/Xbox wrapper authentication is unknown: do not recalculate
  * or forge unknown header bytes. The file is TEST ONLY. */
 return 0;
}
int carbon_create_cash_test(const char *path,uint32_t expected_cash,uint32_t new_cash,
                            char *test_path,size_t capacity){
 if(!path||!test_path||capacity<64)return -3;
 test_path[0]=0;
 if(strstr(path,".nfslab.")||strlen(path)>800)return -3;
 int n=snprintf(test_path,capacity,"%s.nfslab.carbon.test",path);
 if(n<=0||(size_t)n>=capacity){test_path[0]=0;return -3;}
 /* Explicit no-overwrite guarantee; never use rb+ or rename on original. */
 FILE *existing=fopen(test_path,"rb");
 if(existing){fclose(existing);return -17;}
 unsigned char *old=(unsigned char*)malloc(CARBON_ORIGINAL_XBOX_FILE_LEN);
 unsigned char *made=(unsigned char*)malloc(CARBON_ORIGINAL_XBOX_FILE_LEN);
 if(!old||!made){free(old);free(made);return -9;}
 FILE *f=fopen(path,"rb");
 if(!f){free(old);free(made);return -4;}
 size_t got=fread(old,1,CARBON_ORIGINAL_XBOX_FILE_LEN,f);
 int trailing=fgetc(f),err=ferror(f),closed=fclose(f);
 if(got!=CARBON_ORIGINAL_XBOX_FILE_LEN||trailing!=EOF||err||closed){
  free(old);free(made);return -1;
 }
 int r=make_cash_candidate(old,CARBON_ORIGINAL_XBOX_FILE_LEN,expected_cash,new_cash,made);
 if(!r){
  /* Exclusive create means that even a concurrent existing test copy
   * cannot be replaced. This routine is only called on explicitly selected
   * real user profile files. */
  FILE *out=fopen(test_path,"wbx");
  if(!out)r=-17;
  else{
   int ok=fwrite(made,1,CARBON_ORIGINAL_XBOX_FILE_LEN,out)==CARBON_ORIGINAL_XBOX_FILE_LEN;
   if(fflush(out))ok=0;
   if(fclose(out))ok=0;
   if(!ok){remove(test_path);r=-18;}
  }
 }
 free(old);free(made);
 return r;
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
  case -10:return "CASH VALUE OUT OF RANGE";
  case -11:return "ORIGINAL CASH HAS CHANGED";
  case -12:return "CASH ALREADY MATCHES";
  case -13:return "UNKNOWN WRAPPER / FRAME LAYOUT";
  case -14:return "CANNOT REPACK JDLZ SAFELY";
  case -15:return "TEST COPY FAILED VALIDATION";
  case -17:return "TEST COPY ALREADY EXISTS";
  case -18:return "ERROR WRITING TEST COPY";
  default:return "UNKNOWN CARBON ERROR";
 }
}
