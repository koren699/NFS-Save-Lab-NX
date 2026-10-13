#include "u2.h"
#include "carbon_auth.h"
#include "backup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static uint32_t le32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put_le32(unsigned char *p,uint32_t n){
 for(int i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));
}
int u2_verify(const unsigned char *p,size_t n){
 if(!p||n!=U2_SAVE_LEN||memcmp(p,"20CM",4)||
    le32(p+4)!=U2_SAVE_LEN||le32(p+8)!=8u||
    le32(p+12)!=0xD218u||0x4cu+8u+0xD218u!=n)return -1;
 return xbox_20cm_verify_zero(p,n)==0?0:-2;
}
int u2_build_cash(const unsigned char *source,size_t n,uint32_t expected,
                  uint32_t desired,unsigned char *output){
 if(!source||!output||source==output)return -3;
 int r=u2_verify(source,n);
 if(r)return r;
 if(desired>U2_MAX_CASH)return -4;
 if(le32(source+U2_CASH_OFFSET)!=expected)return -5;
 if(desired==expected)return -6;
 memcpy(output,source,n);
 put_le32(output+U2_CASH_OFFSET,desired);
 if(xbox_20cm_sign_zero(output,n)!=0||u2_verify(output,n)||
    le32(output+U2_CASH_OFFSET)!=desired)return -7;
 /* Only the money field and the last two HMAC digests can differ.
  * The first digest covers the 8-byte invariant container header. */
 for(size_t i=0;i<n;i++){
  if(i>=U2_CASH_OFFSET&&i<U2_CASH_OFFSET+4)continue;
  if(i>=0x24u&&i<0x4cu)continue;
  if(output[i]!=source[i])return -7;
 }
 return 0;
}
static int read_exact(const char *path,unsigned char *buf){
 FILE *f=path?fopen(path,"rb"):NULL;
 if(!f)return -8;
 size_t n=fread(buf,1,U2_SAVE_LEN,f);
 int extra=fgetc(f),err=ferror(f),closed=fclose(f);
 if(n!=U2_SAVE_LEN||extra!=EOF||err||closed)return -1;
 return u2_verify(buf,n);
}
int u2_read_cash(const char *path,uint32_t *cash){
 if(!cash)return -3;
 unsigned char *bytes=(unsigned char*)malloc(U2_SAVE_LEN);
 if(!bytes)return -9;
 int r=read_exact(path,bytes);
 if(!r)*cash=le32(bytes+U2_CASH_OFFSET);
 free(bytes);return r;
}
int u2_verify_file(const char *path){
 unsigned char *bytes=(unsigned char*)malloc(U2_SAVE_LEN);
 if(!bytes)return -9;
 int r=read_exact(path,bytes);
 free(bytes);return r;
}
static int available(const char *path){
 struct stat st;
 return stat(path,&st)!=0&&errno==ENOENT;
}
static int write_candidate(const char *path,const unsigned char *buf){
 FILE *f=fopen(path,"wbx");
 if(!f)return -10;
 int ok=fwrite(buf,1,U2_SAVE_LEN,f)==U2_SAVE_LEN;
 if(fflush(f)!=0)ok=0;
 if(fclose(f)!=0)ok=0;
 if(!ok){remove(path);return -10;}
 return 0;
}
int u2_apply_cash(const char *path,uint32_t expected,uint32_t desired,
                  char *backup_path,size_t cap){
 if(!path||!backup_path||cap<128||strlen(path)>720 ||
    strstr(path,".nfslab.")||strstr(path,"/Headers/")||
    strstr(path,"NFSLAB_TEST_"))return -3;
 backup_path[0]=0;
 unsigned char *old=(unsigned char*)malloc(U2_SAVE_LEN);
 unsigned char *made=(unsigned char*)malloc(U2_SAVE_LEN);
 unsigned char *verify=(unsigned char*)malloc(U2_SAVE_LEN);
 if(!old||!made||!verify){
  free(old);free(made);free(verify);return -9;
 }
 int r=read_exact(path,old);
 if(r)goto done;
 r=u2_build_cash(old,U2_SAVE_LEN,expected,desired,made);
 if(r)goto done;
 char tmp[1024],part[1024],bk[1024];
 int k=snprintf(tmp,sizeof(tmp),"%s.nfslab.u2.tmp",path);
 int j=snprintf(part,sizeof(part),"%s.nfslab.u2.oldpart",path);
 if(k<1||j<1||(size_t)k>=sizeof(tmp)||(size_t)j>=sizeof(part)){
  r=-3;goto done;
 }
 if(!available(tmp)||!available(part)){r=-11;goto done;}
 /* Full independent backup is verified before the active career is touched. */
 r=lab_make_backup(path,bk,sizeof(bk));
 if(r){r=-12;goto done;}
 snprintf(backup_path,cap,"%s",bk);
 r=read_exact(bk,verify);
 if(r||memcmp(old,verify,U2_SAVE_LEN)){r=-13;goto done;}
 r=read_exact(path,verify);
 if(r||memcmp(old,verify,U2_SAVE_LEN)){r=-13;goto done;}
 r=write_candidate(tmp,made);
 if(r)goto done;
 r=read_exact(tmp,verify);
 if(r||memcmp(made,verify,U2_SAVE_LEN)){
  remove(tmp);r=-13;goto done;
 }
 if(rename(path,part)!=0){remove(tmp);r=-10;goto done;}
 if(rename(tmp,path)!=0){
  if(rename(part,path)!=0){r=-14;goto done;}
  remove(tmp);r=-10;goto done;
 }
 r=read_exact(path,verify);
 if(r||memcmp(made,verify,U2_SAVE_LEN)){
  /* Never overwrite recovery files. Keep the verified backup intact. */
  char failed[1024];
  snprintf(failed,sizeof(failed),"%s.nfslab.u2.failed",path);
  if(available(failed))rename(path,failed);
  if(rename(part,path)!=0){r=-14;goto done;}
  r=-13;goto done;
 }
 remove(part);r=0;
done:
 free(old);free(made);free(verify);
 return r;
}
const char *u2_error(int r){
 switch(r){
 case 0:return "OK";
 case -1:return "UNSUPPORTED UNDERGROUND 2 PROFILE";
 case -2:return "INVALID XBOX SAVE SIGNATURE";
 case -3:return "INVALID FILE / PARAMETERS";
 case -4:return "CASH OUT OF RANGE";
 case -5:return "CASH CHANGED SINCE LAST CHECK";
 case -6:return "CASH ALREADY MATCHES";
 case -7:return "NEW SAVE SIGNATURE VALIDATION FAILED";
 case -8:return "CANNOT READ CAREER";
 case -9:return "OUT OF MEMORY";
 case -10:return "SD CARD WRITE ERROR";
 case -11:return "RECOVERY FILE ALREADY EXISTS";
 case -12:return "CANNOT CREATE VERIFIED BACKUP";
 case -13:return "SAVE / BACKUP VERIFICATION FAILED";
 case -14:return "RESTORE FROM .oldpart OR .nfslab.bakN";
 default:return "UNKNOWN U2 EDIT ERROR";
 }
}
