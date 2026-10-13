#include <switch.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include "engine.h"
#include "backup.h"
#include "diagnostics.h"
#include "mc02.h"
#include "carbon.h"
#include "carbon_auth.h"
#include "poster_embedded.h"
#include "font8x8_basic.h"

#define SW 1280
#define SH 720
#define MAX_FILES 180
#define PATH_CAP 768
#define MAX_LABEL 112
#define MAX_ROWS 7
#define COLOR(r,g,b) RGBA8_MAXALPHA(r,g,b)
#define BG COLOR(9,13,18)
#define PANEL COLOR(18,25,32)
#define PANEL2 COLOR(24,32,40)
#define WHITE COLOR(238,244,247)
#define MUTED COLOR(139,155,165)
#define GREEN COLOR(181,248,54)
#define RED COLOR(255,102,93)

typedef struct {char path[PATH_CAP];char name[MAX_LABEL];uint32_t size;int backup;} SaveFile;
static const char *game_names[]={"MOST WANTED", "UNDERGROUND 2", "CARBON"};
static const char *game_descriptions[]={"ROCKPORT  //  XBOX 360", "BAYVIEW  //  ORIGINAL XBOX", "PALMONT CITY  //  ORIGINAL XBOX"};
static const char *game_root[][6]={
 {"sdmc:/switch/nfsmw-nx/nfsmw","sdmc:/switch/nfsmw/nfsmw",
  "sdmc:/switch/nfsmw-nx/saves","sdmc:/switch/nfsmw/saves",NULL},
 {"sdmc:/switch/nfsu2x/game/UDATA","sdmc:/switch/nfsu2x/game/Save",NULL},
 {"sdmc:/switch/nfscx/save","sdmc:/switch/nfscx/game/UDATA",
  "sdmc:/switch/nfscx/game/Save",NULL}
};
static SaveFile files[MAX_FILES];
static int nfiles=0,selected=0,game=0,screen=0,match_idx=0,digit=4;
static int metric=0, action=0, restore_index=0, restore_count=0, backup_result=0;
static const char *metric_names[]={"CASH","BOUNTY","HEAT LEVEL"};
static char restore_paths[64][1024];
static uint32_t old_val=0,new_val=9999999;
static LabMatches matches;
static LabDiff diff_result;
static int diff_error=0;
static int scan_err=0,patch_err=0;
static char backup_path[1024]={0};
static int backup_error=0;
static int cash_error=0;
static int bounty_error=0;
static uint32_t bounty_detected=0,bounty_offset=0;
static Mc02BountyInfo bounty_info;
static unsigned bounty_selected=0;
static int bounty_write_error=0;
static int carbon_cash_error=0;
static uint32_t carbon_cash_value=0;
static int carbon_hmac_status=-2;
static uint32_t carbon_test_amount=0;
static int carbon_test_error=0;
static char carbon_test_path[1024]={0};
static char status_line[190]="READY TO SCAN LOCAL SAVES";
static Framebuffer fb;
static uint32_t *pixels;
static int pitch;
static uint32_t accent=GREEN;
#define HD_POSTER_W 336
#define HD_POSTER_H 360
#define HD_POSTER_PIXELS (HD_POSTER_W*HD_POSTER_H)
#define HD_POSTER_BYTES (3u*HD_POSTER_PIXELS*2u)
#define HD_TRAILER_BYTES 20u
static unsigned char *hd_posters=NULL;
/* v0.8 single-file poster bundle: an optional 725760-byte RGB565 payload
   followed by "NFSLABP8", version 1, dimensions and payload length.
   Everything is read-only. Existing unbundled NROs use the old posters. */
static uint32_t get_le32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t get_le16(const unsigned char *p){
 return (uint16_t)p[0]|((uint16_t)p[1]<<8);
}
static int poster_load_file(const char *path){
 if(!path || !*path || strlen(path)>850)return 0;
 FILE *fp=fopen(path,"rb");if(!fp)return 0;
 int ok=0;
 do {
  if(fseek(fp,0,SEEK_END)!=0)break;
  long len=ftell(fp);
  if(len<0 || (unsigned long)len<HD_POSTER_BYTES+HD_TRAILER_BYTES)break;
  if(fseek(fp,-(long)HD_TRAILER_BYTES,SEEK_END)!=0)break;
  unsigned char t[HD_TRAILER_BYTES];
  if(fread(t,1,sizeof(t),fp)!=sizeof(t))break;
  if(memcmp(t,"NFSLABP8",8)!=0 || get_le32(t+8)!=1 ||
     get_le16(t+12)!=HD_POSTER_W || get_le16(t+14)!=HD_POSTER_H ||
     get_le32(t+16)!=HD_POSTER_BYTES)break;
  if(fseek(fp,-(long)(HD_POSTER_BYTES+HD_TRAILER_BYTES),SEEK_END)!=0)break;
  unsigned char *buf=(unsigned char*)malloc(HD_POSTER_BYTES);
  if(!buf)break;
  if(fread(buf,1,HD_POSTER_BYTES,fp)!=HD_POSTER_BYTES){free(buf);break;}
  hd_posters=buf;ok=1;
 }while(0);
 fclose(fp);return ok;
}
static void poster_hd_init(int argc,char **argv){
 if(argc>0 && argv && argv[0] && poster_load_file(argv[0]))return;
 /* HBMenu can run an NRO without passing its own path in argv. */
 const char *paths[]={
  "sdmc:/switch/nfs_save_lab_nx.nro",
  "sdmc:/switch/NFS_Save_Lab_NX/nfs_save_lab_nx.nro",
  "sdmc:/switch/nfs_save_lab_nx_v0.8.nro",
  NULL
 };
 for(int i=0;paths[i];i++)if(poster_load_file(paths[i]))return;
}
static void poster_draw(int idx,int x,int y,int w,int h){
 if(idx<0||idx>=3)return;
 if(hd_posters){
  const unsigned char *rgb=hd_posters+idx*(HD_POSTER_PIXELS*2u);
  for(int dy=0;dy<h;dy++){
   int sy=dy*HD_POSTER_H/h;
   uint32_t *dest=pixels+(y+dy)*pitch+x;
   for(int dx=0;dx<w;dx++){
    int sx=dx*HD_POSTER_W/w;
    size_t at=2u*((size_t)sy*HD_POSTER_W+(size_t)sx);
    uint16_t v=(uint16_t)(rgb[at]|((uint16_t)rgb[at+1]<<8));
    int r=((v>>11)&31)*255/31;
    int g=((v>>5)&63)*255/63;
    int b=(v&31)*255/31;
    dest[dx]=COLOR(r,g,b);
   }
  }
  return;
 }
 const unsigned char *palette=poster_palettes[idx];
 const unsigned char *indices=poster_indices[idx];
 for(int dy=0;dy<h;dy++){
  int sy=(dy*POSTER_H)/h;
  uint32_t *dest=pixels+(y+dy)*pitch+x;
  for(int dx=0;dx<w;dx++){
   int sx=(dx*POSTER_W)/w;
   unsigned char pi=indices[sy*POSTER_W+sx];
   const unsigned char *rgb=palette+3*pi;
   dest[dx]=COLOR(rgb[0],rgb[1],rgb[2]);
  }
 }
}
static void rect(int x,int y,int w,int h,uint32_t c){
 if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
 if(x+w>SW)w=SW-x;if(y+h>SH)h=SH-y;if(w<=0||h<=0)return;
 for(int yy=y;yy<y+h;yy++){uint32_t *p=pixels+yy*pitch+x;for(int xx=0;xx<w;xx++)p[xx]=c;}
}
static void stroke(int x,int y,int w,int h,uint32_t c,int t){
 rect(x,y,w,t,c);rect(x,y+h-t,w,t,c);rect(x,y,t,h,c);rect(x+w-t,y,t,h,c);
}
static void txt(int x,int y,int scale,const char *s,uint32_t c){
 if(!s)return;
 int start=x;
 for(const unsigned char *p=(const unsigned char*)s;*p;p++){
  if(*p=='\n'){x=start;y+=scale*11;continue;}
  if(x+scale*8>SW)break;
  unsigned ch=*p<128?*p:'?';
  for(int cy=0;cy<8;cy++){unsigned char bits=(unsigned char)font8x8_basic[ch][cy];
   for(int cx=0;cx<8;cx++)if(bits&(1u<<cx))rect(x+cx*scale,y+cy*scale,scale,scale,c);
  }
  x+=scale*8;
 }
}
static void cliptext(int x,int y,int scale,const char *s,uint32_t c,int chars){
 char b[190];snprintf(b,sizeof(b),"%.*s",chars,s?s:"");txt(x,y,scale,b,c);
}
static void line(int x0,int y0,int x1,int y1,uint32_t c,int thickness){
 int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
 for(;;){rect(x0,y0,thickness,thickness,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
}
static void label(int x,int y,const char *s){txt(x,y,2,s,MUTED);}
static void button_hint(int x,int y,const char *key,const char *caption){
 int ww=(int)strlen(key)*16+22;rect(x,y-6,ww,31,PANEL2);stroke(x,y-6,ww,31,COLOR(70,83,91),1);
 txt(x+11,y,2,key,WHITE);txt(x+ww+11,y,2,caption,MUTED);
}
static void empty(void){
 for(int y=0;y<SH;y++){
  int shift=(y*9)/720;
  uint32_t color=COLOR(8+shift,12+shift,18+shift);
  uint32_t *p=pixels+y*pitch;
  for(int x=0;x<SW;x++)p[x]=color;
 }
 for(int i=0;i<19;i++){
  int x=780+i*35;
  line(x,-12,x-400,720,COLOR(21,30,37),1);
 }
 rect(0,0,SW,6,accent);
 rect(0,630,SW,90,COLOR(13,19,24));
 rect(0,629,SW,1,COLOR(41,54,58));
}
static void header(const char *sub){
 rect(65,30,52,52,accent);
 txt(74,46,3,"N",BG);
 txt(133,30,4,"NFS",WHITE);txt(231,30,4,"SAVE LAB",accent);
 txt(135,73,2,"NINTENDO SWITCH  /  HOMEBREW",MUTED);
 txt(980,42,2,"BUILD 0.15.0",WHITE);
 rect(65,111,1150,2,COLOR(46,61,67));
 txt(66,136,2,sub,MUTED);
}
static void pill(int x,int y,const char *s,uint32_t color){
 int w=(int)strlen(s)*16+24;rect(x,y,w,29,color);txt(x+12,y+6,2,s,BG);
}
static void frame(int x,int y,int w,int h,int sel){
 rect(x,y,w,h,sel?COLOR(34,47,53):PANEL);
 rect(x,y,sel?6:2,h,sel?accent:COLOR(64,75,79));
 rect(x,y+h-2,w,2,sel?accent:COLOR(39,53,59));
 if(sel){rect(x+7,y,w-14,2,COLOR(65,86,70));}
}
static void big_number(int x,int y,const char *s){
 txt(x+3,y+4,5,s,COLOR(46,76,42));
 txt(x,y,5,s,WHITE);
}
static void footer(const char *shortline){
 label(67,648,shortline);
 label(67,685,game==2?"CARBON: CASH EDIT / VERIFIED 3X HMAC / BACKUP":
      "MOST WANTED: CASH + BOUNTY MC02    /    HEAT: READ ONLY");
}
static void game_card(int idx,int x,int y){
 int active=(game==idx),w=356,h=426;
 uint32_t c=idx==0?GREEN:(idx==1?COLOR(69,209,241):COLOR(247,164,67));
 rect(x-3,y-3,w+6,h+6,active?c:COLOR(37,71,90));
 rect(x,y,w,h,COLOR(11,19,25));
 poster_draw(idx,x+10,y+9,w-20,360);
 rect(x+10,y+373,w-20,44,COLOR(11,21,26));
 rect(x+10,y+373,w-20,2,c);
 txt(x+20,y+387,2,game_names[idx],active?c:WHITE);
 txt(x+w-65,y+387,2,idx==0?"2005":idx==1?"2004":"2006",MUTED);
 if(active){
  rect(x-9,y-9,30,5,c);rect(x-9,y-9,5,30,c);
  rect(x+w-21,y-9,30,5,c);rect(x+w+4,y-9,5,30,c);
  rect(x-9,y+h+4,30,5,c);rect(x-9,y+h-21,5,30,c);
  rect(x+w-21,y+h+4,30,5,c);rect(x+w+4,y+h-21,5,30,c);
 }
}
static void dashboard(void){
 header("SELECT A GAME");
 txt(66,169,2,game==2?"L / R  CHOOSE GAME       A  OPEN CARBON CAREER":"L / R  CHOOSE GAME       A  OPEN SAVE BROWSER",MUTED);
 for(int i=0;i<3;i++)game_card(i,61+i*394,195);
 footer(game==2?"L / R  CHANGE GAME       A  VIEW CARBON CASH       +  EXIT":
        "L / R  CHANGE GAME       A  OPEN SAVE BROWSER       +  EXIT");
}
static void info_panel(const char *heading,const char *subtitle){
 frame(64,182,752,420,0);
 txt(84,204,3,heading,WHITE);
 label(85,250,subtitle);
 rect(835,182,380,420,COLOR(22,31,37));
 rect(835,182,6,420,accent);
 txt(862,204,3,"SAVE STATUS",WHITE);
 label(862,252,"TARGET GAME");
 txt(862,284,2,game_names[game],accent);
 label(862,331,game==2?"ORIGINAL CASH":"CURRENT VALUE");
 char b[50];
 snprintf(b,sizeof(b),"%09u",game==2?carbon_cash_value:old_val);
 txt(862,362,3,b,WHITE);
 label(862,416,"EDITOR STATUS");
 pill(862,451,game==0?"CASH MC02":game==2?"TEST COPY":"READ ONLY",accent);
 label(862,520,game==2?"GAME SIGNATURE UNVERIFIED":"VERIFIED HASH / BACKUP");
}
static void files_page(void){
 if(game==2){
  header("CARBON / CAREER PROFILES");
  frame(64,182,1150,420,0);
  txt(89,205,3,"SELECT A CAREER",WHITE);
  label(89,253,"ONLY REAL ALIAS PROFILES ARE SHOWN");
  if(!nfiles){
   txt(89,341,2,"NO CARBON CAREER PROFILE FOUND",RED);
   label(89,394,"CHECK THE GAME/UDATA SAVE FOLDER");
  }
  int liststart=selected-3;
  if(liststart<0)liststart=0;
  if(liststart>nfiles-MAX_ROWS)liststart=nfiles-MAX_ROWS;
  if(liststart<0)liststart=0;
  for(int i=liststart;i<nfiles&&i<liststart+MAX_ROWS;i++){
   int y=294+(i-liststart)*40;
   if(selected==i){
    rect(89,y-6,1083,38,COLOR(42,62,67));
    rect(89,y-6,5,38,accent);
   }
   char b[140];
   snprintf(b,sizeof(b),"%02d  %.65s",i+1,files[i].name);
   txt(106,y+3,2,b,selected==i?WHITE:MUTED);
  }
  footer("UP / DOWN  PROFILE       A  VIEW CASH       B  GAMES");
  return;
 }
 header("01 / LOCAL SAVE FILES");
 info_panel("SELECT A SAVE","UP / DOWN TO PICK A FILE.  A TO CONTINUE.");
 int start=selected-3;if(start<0)start=0;if(start>nfiles-MAX_ROWS)start=nfiles-MAX_ROWS;if(start<0)start=0;
 for(int z=0;z<MAX_ROWS;z++){
  int i=start+z;if(i>=nfiles)break;
  int y=277+z*44,active=i==selected;
  if(active){rect(78,y-5,723,41,COLOR(48,67,69));rect(78,y-5,5,41,accent);}
  char b[80];snprintf(b,sizeof(b),"%02d  %.31s",i+1,files[i].name);
  txt(91,y+4,2,b,active?WHITE:MUTED);
  if(files[i].backup)txt(680,y+4,2,"[COPY]",COLOR(248,180,69));
  else txt(680,y+4,2,"[LIVE]",accent);
 }
 if(!nfiles){txt(88,335,2,"NO SAVE FILES FOUND IN KNOWN PORT FOLDERS",RED);label(88,373,"CREATE A GAME SAVE AND TRY AGAIN.");}
 footer("UP / DOWN  SELECT       A  CONTINUE       B  BACK");
}
static void action_page(void){
 header("02 / SAVE TOOLS");
 info_panel("CHOOSE A TOOL","MC02 FORMAT, CRC + MD5 VERIFIED BEFORE WRITING");
 const char *entries[]={"EDIT CASH (MC02)","EDIT BOUNTY (MC02)","BOUNTY INFO",
                       "CASH SCAN","HEAT SCAN","COMPARE SNAPSHOTS",
                       "CREATE BACKUP","RESTORE BACKUP"};
 const char *notes[]={"CHOOSE YOUR CASH","SOLD + LIVE CARS",
                     "VIEW PURSUIT TOTAL","SCAN READ ONLY","SCAN READ ONLY",
                     "BEFORE AND AFTER","KEEP AN ORIGINAL","RECOVER ORIGINAL"};
 for(int i=0;i<8;i++){
  int y=273+i*39,sel=i==action;
  if(sel){rect(78,y-5,725,36,COLOR(43,64,63));rect(78,y-5,5,36,accent);}
  char text[85];snprintf(text,sizeof(text),"%02d   %s",i+1,entries[i]);
  if(i==3 && game==2)snprintf(text,sizeof(text),"04   CARBON CASH INFO");
  txt(94,y,2,text,sel?WHITE:MUTED);
  cliptext(461,y,2,i==3&&game==2?"20CM / JDLZ / READ ONLY":notes[i],sel?accent:MUTED,19);
 }
 footer("UP / DOWN  TOOL       A  OPEN       B  FILES");
}
static void load_backups(void) {
 restore_count=0;restore_index=0;
 if(!nfiles || selected<0 || selected>=nfiles)return;
 for(int n=1;n<=999 && restore_count<64;n++){
  char b[1024];
  int len=snprintf(b,sizeof(b),"%s.nfslab.bak%d",files[selected].path,n);
  if(len<0||(size_t)len>=sizeof(b))break;
  struct stat st;
  if(!stat(b,&st)&&S_ISREG(st.st_mode)&&st.st_size>0 && st.st_size<=(off_t)LAB_MAX_FILE){
   snprintf(restore_paths[restore_count++],sizeof(restore_paths[0]),"%s",b);
  }
 }
}
static void backups_page(void){
 header("03 / RESTORE SNAPSHOT");
 info_panel("SELECT A BACKUP","ONLY SNAPSHOTS CREATED BY SAVE LAB ARE SHOWN");
 if(!restore_count)txt(89,319,2,"NO BACKUPS AVAILABLE FOR THIS SAVE",RED);
 int start=restore_index-3;if(start<0)start=0;
 if(start>restore_count-6)start=restore_count-6;if(start<0)start=0;
 for(int i=start;i<restore_count && i<start+6;i++){
  int y=291+(i-start)*48,active=i==restore_index;
  if(active){rect(79,y-9,725,39,COLOR(45,65,62));rect(79,y-9,5,39,accent);}
  const char *suffix=strstr(restore_paths[i],".nfslab.bak");
  char b[90];snprintf(b,sizeof(b),"SNAPSHOT %02d    %s",i+1,suffix?suffix:"BACKUP");
  txt(95,y,2,b,active?WHITE:MUTED);
 }
 footer("UP / DOWN  SELECT       A  REVIEW RESTORE       B  BACK");
}
static void restore_confirm_page(void){
 header("04 / RESTORE CONFIRMATION");
 info_panel("RESTORE THIS BACKUP?","CURRENT SAVE WILL BE COPIED BEFORE REPLACING");
 txt(91,295,2,"SELECTED BACKUP:",MUTED);
 char b[60];snprintf(b,sizeof(b),"%d OF %d",restore_index+1,restore_count);
 txt(91,342,3,b,WHITE);
 txt(91,410,2,"EXISTING GAME SAVE WILL BE OVERWRITTEN",RED);
 txt(91,454,2,"A NEW SAFETY SNAPSHOT IS CREATED FIRST",accent);
 txt(91,523,2,"HOLD ZL + ZR, THEN PRESS A TO RESTORE",WHITE);
 footer("ZL + ZR + A  RESTORE       B  CANCEL");
}
static void backup_result_page(void){
 header("05 / BACKUP OPERATION");
 info_panel("BACKUP / RESTORE","VERIFIED FILE COPY  /  ORIGINAL RETAINED");
 txt(91,320,3,backup_error?"OPERATION FAILED":(backup_result?"RESTORE COMPLETE":"BACKUP CREATED"),
     backup_error?RED:accent);
 if(backup_error)txt(91,390,2,lab_backup_error(backup_error),RED);
 else {
  label(91,397,backup_result?"CURRENT FILE REPLACED / OLD SAVE BACKED UP":"SOURCE FILE UNCHANGED");
  label(91,445,"SNAPSHOT STORED NEXT TO GAME SAVE");
 }
 footer("B  RETURN TO OPTIONS");
}
static void value_page(int next){
 header(next?"04 / NEW VALUE":"02 / CURRENT GAME VALUE");
 info_panel(next?"SET DESIRED VALUE":"FIND CURRENT VALUE",next?"CHOOSE YOUR OWN TARGET AMOUNT":"ENTER THE EXACT CURRENT IN-GAME VALUE");
 txt(91,270,2,metric_names[metric],accent);
 uint32_t v=next?new_val:old_val;
 char b[32];
 if(metric==2)snprintf(b,sizeof(b),"%u / 5",v);
 else snprintf(b,sizeof(b),"%09u",v);
 big_number(91,326,b);
 if(metric!=2){
  int cursorx=91+(8-digit)*40;
  rect(cursorx,380,37,5,accent);
  label(89,422,"UP / DOWN  ADJUST SELECTED DIGIT");
  label(89,456,"LEFT / RIGHT  CHANGE DIGIT");
 }else{
  label(89,422,"UP / DOWN  SELECT LEVEL 1 - 5");
  label(89,456,"ONLY 1 MATCH MAY BE EDITED IN HEAT MODE");
 }
 label(89,509,"A  SCAN FOR NUMBER - READ ONLY");
 footer("A  SCAN CURRENT NUMBER       B  BACK");
}
static void matches_page(void){
 header("03 / MATCHES");
 info_panel("POSSIBLE VALUES","RAW 32-BIT MATCHES, NOT VERIFIED GAME FIELDS");
 char s[125];snprintf(s,sizeof(s),"%d MATCHES FOUND %s",matches.count,matches.overflow?"(LIMIT HIT)":"");
 txt(87,281,2,s,accent);
 if(scan_err){txt(87,321,2,lab_error(scan_err),RED);}
 if(!scan_err && matches.count==0)label(87,330,"NO PLAIN 32-BIT VALUE FOUND IN FILE.");
 for(int z=0;z<6;z++){
  int start=match_idx>2?match_idx-2:0;
  int i=start+z;if(i>=matches.count)break;
  int y=322+z*43;
  if(i==match_idx){rect(79,y-7,726,38,COLOR(46,66,64));rect(80,y-7,5,38,accent);}
  char b[85];snprintf(b,sizeof(b),"%02d / 0x%08X  / %s",i+1,matches.items[i].offset,matches.items[i].be?"BIG ENDIAN":"LITTLE ENDIAN");
  txt(93,y,2,b,i==match_idx?WHITE:MUTED);
 }
 footer("READ ONLY. MATCH IS NOT VERIFIED CASH / BOUNTY / HEAT.    B  BACK");
}
static void compare_pick_page(void){
 header("03 / PAIR COMPARISON");
 info_panel("SELECT BEFORE SAVE","A KNOWN EARLIER SNAPSHOT OF THIS FILE");
 txt(90,274,2,"METRIC:",MUTED);
 txt(228,274,2,metric_names[metric],accent);
 label(90,316,"L / R SWITCH BETWEEN CASH, BOUNTY, HEAT");
 if(!restore_count)txt(92,370,2,"NO .NFSLAB.BAK SNAPSHOTS FOUND",RED);
 int start=restore_index-2;if(start<0)start=0;
 if(start>restore_count-5)start=restore_count-5;if(start<0)start=0;
 for(int i=start;i<restore_count && i<start+5;i++){
  int y=364+(i-start)*44,active=i==restore_index;
  if(active){rect(82,y-6,717,36,COLOR(44,64,61));rect(82,y-6,5,36,accent);}
  char b[96];snprintf(b,sizeof(b),"BACKUP %02d    .nfslab.bak",i+1);
  txt(96,y,2,b,active?WHITE:MUTED);
 }
 footer("L / R  METRIC     UP / DOWN  BACKUP     A  CONTINUE     B  BACK");
}
static void compare_value_page(int after){
 header(after?"05 / CURRENT SNAPSHOT":"04 / EARLIER SNAPSHOT");
 info_panel(after?"ENTER CURRENT VALUE":"ENTER EARLIER VALUE",
  after?"VALUE DISPLAYED AFTER THE GAME CHANGED":"VALUE DISPLAYED BEFORE THE GAME CHANGED");
 txt(93,271,2,metric_names[metric],accent);
 uint32_t v=after?new_val:old_val;
 char b[40];
 if(metric==2)snprintf(b,sizeof(b),"%u / 5",v);
 else snprintf(b,sizeof(b),"%09u",v);
 big_number(93,326,b);
 if(metric!=2){
  int cursorx=93+(8-digit)*40;
  rect(cursorx,382,37,5,accent);
  label(90,424,"UP / DOWN  CHANGE DIGIT");
  label(90,456,"LEFT / RIGHT  SELECT DIGIT");
 } else label(90,424,"UP / DOWN  SELECT LEVEL 1 - 5");
 label(90,518,after?"A  COMPARE FILES WITHOUT WRITING":"A  ENTER CURRENT VALUE");
 footer(after?"A  ANALYZE     B  BACK":"A  NEXT     B  BACK");
}
static void comparison_result_page(void){
 header("06 / DIAGNOSTIC RESULTS");
 info_panel("FILE DIFFERENCES","READ-ONLY COMPARISON. NO SAVE MODIFICATIONS.");
 if(diff_error){
  txt(89,295,2,lab_diff_error(diff_error),RED);
 }else{
  char b[100];snprintf(b,sizeof(b),"DIFFERENT BYTES: %u",diff_result.changed_bytes);
  txt(89,282,2,b,WHITE);
  snprintf(b,sizeof(b),"CHANGED RANGES: %u",diff_result.changed_ranges);
  txt(89,314,2,b,WHITE);
  snprintf(b,sizeof(b),"NUMBER PAIRS: %d",diff_result.matches);
  txt(89,349,2,b,accent);
  if(diff_result.too_many_matches)label(89,384,"TOO MANY CANDIDATES - DISPLAY LIMITED");
  int top=392;
  for(int i=0;i<diff_result.matches&&i<5;i++){
   snprintf(b,sizeof(b),"%02d  BYTE 0x%08X  %s",i+1,diff_result.entries[i].offset,
    diff_result.entries[i].big_endian?"BIG ENDIAN":"LITTLE ENDIAN");
   txt(89,top+i*37,2,b,MUTED);
  }
  if(diff_result.matches==0)label(89,401,"NO DIRECT 32-BIT NUMBER PAIR WAS FOUND");
 }
 footer("RESULTS ARE HYPOTHESES  /  B  BACK TO SNAPSHOTS");
}
static void cash_edit_page(void){
 header("03 / MOST WANTED CASH");
 info_panel("CHOOSE YOUR CASH","LIVE MC02 SAVE VALIDATED / CUSTOM AMOUNT");
 txt(91,273,2,"CURRENT:",MUTED);
 char b[64];snprintf(b,sizeof(b),"%09u",old_val);
 txt(245,273,2,b,WHITE);
 txt(91,322,2,"NEW CASH:",accent);
 snprintf(b,sizeof(b),"%09u",new_val);
 big_number(91,357,b);
 rect(91+(8-digit)*40,412,37,5,accent);
 label(91,456,"UP / DOWN  CHANGE SELECTED DIGIT");
 label(91,488,"LEFT / RIGHT  SELECT DIGIT");
 label(91,541,"A  REVIEW THE BACKUP AND REPAIR");
 footer("A  REVIEW       B  BACK");
}
static void cash_confirm_page(void){
 header("04 / CONFIRM CASH EDIT");
 info_panel("SAVE CHECK","LIVE MC02 ONLY - VERIFIED CRC + MD5");
 char b[80];
 snprintf(b,sizeof(b),"CASH     %u -> %u",old_val,new_val);txt(89,306,2,b,WHITE);
 txt(89,365,2,"FIRST: CREATE VERIFIED ORIGINAL BACKUP",accent);
 txt(89,410,2,"SECOND: REBUILD MD5 AND THREE EA CRCS",accent);
 txt(89,465,2,"THIRD: VERIFY FILE AND ALLOW ROLLBACK",accent);
 txt(89,525,2,"HOLD ZL + ZR AND PRESS A TO APPLY",WHITE);
 footer("ZL + ZR + A  WRITE       B  CANCEL");
}
static void cash_result_page(void){
 header("05 / CASH EDIT RESULT");
 info_panel(cash_error?"SAVE NOT MODIFIED":"CASH SAVED","MC02 INTEGRITY VALIDATED");
 txt(89,312,2,cash_error?mc02_error(cash_error):"NEW CASH APPLIED",cash_error?RED:accent);
 if(!cash_error){
  char b[88];snprintf(b,sizeof(b),"CASH: %u",new_val);
  txt(89,373,3,b,WHITE);
  label(89,439,"ORIGINAL KEPT IN .nfslab.bakN");
  label(89,487,"GAME-SIDE LOAD NOT TESTED FOR THIS VERSION");
 }else label(89,445,"IF RECOVERY IS NEEDED, CHECK .oldpart / .bakN");
 footer("B  RETURN TO TOOL MENU");
}
static void bounty_view_page(void){
 header("03 / BOUNTY DATA");
 info_panel("ACTUAL CHASE RATING","LIVE CARS PLUS SOLD-CAR HISTORY");
 if(bounty_error){
  txt(91,320,2,mc02_error(bounty_error),RED);
 }else{
  char b[100];
  snprintf(b,sizeof(b),"%09u",bounty_info.total);
  big_number(90,308,b);
  snprintf(b,sizeof(b),"SOLD CARS: %u",bounty_info.sold_history);
  txt(91,405,2,b,MUTED);
  snprintf(b,sizeof(b),"ACTIVE CARS: %u",bounty_info.active_count);
  txt(91,446,2,b,accent);
  snprintf(b,sizeof(b),"CURRENT CAR SLOT: %u",bounty_info.active_slots[bounty_info.preferred_index]+1);
  txt(91,488,2,b,WHITE);
  label(91,536,"CACHE IS NOT THE ACTUAL BOUNTY SOURCE");
 }
 footer("B  BACK TO TOOLS");
}
static void bounty_edit_page(void){
 header("03 / EDIT BOUNTY");
 info_panel("SET TOTAL RATING","SOLD HISTORY REMAINS UNCHANGED");
 char b[90];
 snprintf(b,sizeof(b),"CURRENT TOTAL: %u",old_val);
 txt(90,277,2,b,WHITE);
 snprintf(b,sizeof(b),"SOLD HISTORY: %u",bounty_info.sold_history);
 txt(90,311,2,b,MUTED);
 snprintf(b,sizeof(b),"%09u",new_val);
 big_number(91,347,b);
 rect(91+(8-digit)*40,398,37,5,accent);
 unsigned slot=bounty_info.active_slots[bounty_selected];
 uint32_t minimum=old_val-bounty_info.active_bounty[bounty_selected];
 snprintf(b,sizeof(b),"L / R  CAR %u / %u  (SLOT %u)",
          bounty_selected+1,bounty_info.active_count,slot+1);
 txt(91,437,2,b,accent);
 snprintf(b,sizeof(b),"MINIMUM FOR THIS CAR: %u",minimum);
 txt(91,476,2,b,new_val<minimum?RED:MUTED);
 label(91,521,"UP/DOWN: VALUE    LEFT/RIGHT: DIGIT");
 footer("A  REVIEW EDIT       B  BACK       L/R  SELECT CAR");
}
static void bounty_confirm_page(void){
 header("04 / CONFIRM BOUNTY EDIT");
 info_panel("VERIFY RATING","ONE ACTIVE CAR CHANGES, HISTORY IS PRESERVED");
 char b[105];
 snprintf(b,sizeof(b),"TOTAL: %u -> %u",old_val,new_val);
 txt(91,294,2,b,WHITE);
 snprintf(b,sizeof(b),"SOLD: %u / ACTIVE CARS: %u",
          bounty_info.sold_history,bounty_info.active_count);
 txt(91,350,2,b,MUTED);
 snprintf(b,sizeof(b),"EDIT CAREER SLOT %u",bounty_info.active_slots[bounty_selected]+1);
 txt(91,393,2,b,accent);
 txt(91,443,2,"ORIGINAL SAVED TO .nfslab.bakN",WHITE);
 txt(91,478,2,"REBUILD MD5 + ALL THREE EA CRCS",WHITE);
 txt(91,533,2,"HOLD ZL + ZR AND PRESS A",accent);
 footer("ZL + ZR + A  WRITE       B  CANCEL");
}
static void bounty_result_page(void){
 header("05 / BOUNTY EDIT RESULT");
 info_panel(bounty_write_error?"BOUNTY NOT UPDATED":"RATING UPDATED",
            "LIVE CAR PURSUIT RECORD");
 txt(91,327,2,bounty_write_error?mc02_error(bounty_write_error):
                  "VALIDATED MC02 SAVE WRITTEN",bounty_write_error?RED:accent);
 if(!bounty_write_error){
  char b[80];snprintf(b,sizeof(b),"NEW TOTAL: %u",new_val);
  txt(91,393,3,b,WHITE);
  label(91,474,"ORIGINAL IS IN .nfslab.bakN");
 }else label(91,467,"CHECK ORIGINAL / BACKUP BEFORE RETRYING");
 footer("B  BACK TO TOOL MENU");
}
static void carbon_cash_info_page(void){
 header("CARBON / CAREER");
 frame(64,182,1150,420,0);
 txt(94,211,3,"YOUR CARBON CAREER",WHITE);
 label(95,269,"ORIGINAL XBOX 20CM / JDLZ");
 rect(95,307,684,4,accent);
 if(carbon_cash_error){
  txt(94,344,2,carbon_error(carbon_cash_error),RED);
 }else{
  txt(95,346,2,"CURRENT CASH",MUTED);
  char money[45];snprintf(money,sizeof(money),"$%u",carbon_cash_value);
  big_number(95,392,money);
 }
 txt(95,512,2,carbon_hmac_status==0?"PRESS A TO EDIT CASH":"SAVE EDITING IS LOCKED",carbon_hmac_status==0?accent:RED);
 rect(813,209,374,360,COLOR(22,31,37));
 rect(813,209,6,360,accent);
 txt(836,232,2,"PROFILE",MUTED);
 if(nfiles&&selected>=0&&selected<nfiles)
  cliptext(836,268,2,files[selected].name,WHITE,20);
 txt(836,328,2,"XBOX SAVE SIGNATURE",MUTED);
 txt(836,371,2,carbon_hmac_status==0?"HMAC 1 / 2 / 3: MATCH":
       "NOT VERIFIED",carbon_hmac_status==0?accent:RED);
 label(836,428,carbon_hmac_status==0?"READY / VERIFIED 20CM":carbon_auth_error(carbon_hmac_status));
 label(836,497,"BACKUP BEFORE EVERY WRITE");
 footer(carbon_hmac_status==0 && !carbon_cash_error?
        "A  EDIT CASH       B  GAMES":
        "B  BACK       SAVE IS READ ONLY");
}
static void carbon_cash_edit_page(void){
 header("CARBON / EDIT CASH");
 info_panel("CHOOSE YOUR CASH","LIVE 20CM VERIFIED / CUSTOM AMOUNT");
 char b[80];snprintf(b,sizeof(b),"CURRENT: $%u",carbon_cash_value);
 txt(91,286,2,b,WHITE);
 txt(91,338,2,"NEW CASH:",accent);
 snprintf(b,sizeof(b),"%09u",carbon_test_amount);
 big_number(91,376,b);
 rect(91+(8-digit)*40,432,37,5,accent);
 label(91,476,"UP / DOWN  CHANGE SELECTED DIGIT");
 label(91,512,"LEFT / RIGHT  SELECT DIGIT");
 footer("A  REVIEW CHANGE       B  CANCEL");
}
static void carbon_cash_test_confirm_page(void){
 header("CARBON / CONFIRM CASH EDIT");
 info_panel("SAVE SECURITY CHECK","COMPACT JDLZ / 3 X HMAC-SHA1");
 char b[100];snprintf(b,sizeof(b),"CASH: $%u -> $%u",
                             carbon_cash_value,carbon_test_amount);
 txt(91,283,2,b,WHITE);
 txt(91,338,2,"FIRST: VERIFIED ORIGINAL BACKUP",accent);
 txt(91,380,2,"SECOND: REBUILD JDLZ / ALL 3 HMACS",accent);
 label(91,433,"THIRD: RE-READ AND VERIFY NEW SAVE");
 label(91,473,"IF WRITE FAILS, RESTORE ORIGINAL");
 label(91,525,"HOLD ZL + ZR AND PRESS A");
 footer("ZL + ZR + A  WRITE       B  CANCEL");
}
static void carbon_cash_test_result_page(void){
 header("CARBON / CASH EDIT RESULT");
 info_panel(carbon_test_error?"CASH NOT MODIFIED":"CASH UPDATED",
            "20CM / JDLZ / ORIGINAL-XBOX HMAC");
 if(carbon_test_error){
  txt(91,320,2,carbon_error(carbon_test_error),RED);
  label(91,393,"CHECK .nfslab.bakN IF RECOVERY IS NEEDED");
 }else{
  char b[64];snprintf(b,sizeof(b),"NEW CASH: $%u",carbon_test_amount);
  txt(91,317,3,b,accent);
  txt(91,401,2,"VERIFIED BACKUP CREATED: .nfslab.bakN",WHITE);
  label(91,456,"CARBON SHOULD NOW READ UPDATED MONEY");
  label(91,506,"KEEP ORIGINAL BACKUP UNTIL GAME LOAD TEST");
 }
 footer("B  BACK TO CARBON CAREER");
}
static void draw(void){
 uint32_t stride;
 pixels=(uint32_t*)framebufferBegin(&fb,&stride);
 if(!pixels)return;
 pitch=(int)(stride/4);
 accent=game==0?GREEN:game==1?COLOR(69,209,241):COLOR(247,164,67);
 empty();
 switch(screen){case 0:dashboard();break;case 1:files_page();break;
 case 2:value_page(0);break;case 3:matches_page();break;
 case 7:action_page();break;case 8:backups_page();break;
 case 9:restore_confirm_page();break;case 10:backup_result_page();break;
 case 11:compare_pick_page();break;
 case 12:compare_value_page(0);break;case 13:compare_value_page(1);break;
 case 14:comparison_result_page();break;
 case 15:cash_edit_page();break;case 16:cash_confirm_page();break;
 case 17:cash_result_page();break;
 case 18:bounty_view_page();break;
 case 19:bounty_edit_page();break;
 case 20:bounty_confirm_page();break;
 case 21:bounty_result_page();break;
 case 22:carbon_cash_info_page();break;
 case 23:carbon_cash_edit_page();break;
 case 24:carbon_cash_test_confirm_page();break;
 case 25:carbon_cash_test_result_page();break;}
 if(status_line[0]){
  rect(64,605,1150,23,COLOR(36,45,50));
  cliptext(73,609,2,status_line,WHITE,68);
 }
 framebufferEnd(&fb);
}
static int is_dir(const char *p){struct stat st;return stat(p,&st)==0&&S_ISDIR(st.st_mode);}
static int suitable(const char *name){
 if(name[0]=='.'||strstr(name,".nfslab."))return 0;
 const char *bad[]={".nro",".nsp",".xex",".xbe",".iso",".png",".jpg",".dds",".log",".toml",".nfsp",".bak",".tmp",".xml",".txt",".zip",".7z",".header",".img",NULL};
 for(int i=0;bad[i];i++){size_t n=strlen(name),s=strlen(bad[i]);if(n>=s&&!strcasecmp(name+n-s,bad[i]))return 0;}
 return 1;
}
/* Original Xbox Carbon uses ALIAS_<profile> for career data.
 * TitleMeta.xbx, TitleImage.xbx, SaveMeta.xbx etc. are never a career.
 * Verify the 20CM magic before showing ALIAS candidates. */
static int carbon_is_career(const char *path,const char *name,off_t length){
 if(strncasecmp(name,"ALIAS_",6)!=0 || length<64 ||
    length>(off_t)LAB_MAX_FILE)return 0;
 FILE *f=fopen(path,"rb");if(!f)return 0;
 unsigned char head[8];
 int ok=fread(head,1,sizeof(head),f)==sizeof(head);
 if(fclose(f)!=0)ok=0;
 return ok && memcmp(head,"20CM",4)==0 &&
        get_le32(head+4)==(uint32_t)length;
}
static void crawl(const char *root,int depth,int backup){
 if(depth>7||nfiles>=MAX_FILES)return;
 DIR *d=opendir(root);if(!d)return;
 struct dirent *ent;
 while((ent=readdir(d))&&nfiles<MAX_FILES){
  if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))continue;
  char p[PATH_CAP];int len=snprintf(p,sizeof(p),"%s/%s",root,ent->d_name);
  if(len<0||(size_t)len>=sizeof(p))continue;
  struct stat st;if(stat(p,&st))continue;
  if(S_ISDIR(st.st_mode)){
   if(!strcasecmp(ent->d_name,"Headers"))continue;
   crawl(p,depth+1,backup);continue;}
  if(!S_ISREG(st.st_mode)||st.st_size<32||st.st_size>(off_t)LAB_MAX_FILE||!suitable(ent->d_name))continue;
   if(game==2&&!carbon_is_career(p,ent->d_name,st.st_size))continue;
  SaveFile *f=&files[nfiles++];
  snprintf(f->path,sizeof(f->path),"%s",p);snprintf(f->name,sizeof(f->name),"%s",ent->d_name);
  f->size=(uint32_t)st.st_size;f->backup=backup;
 }
 closedir(d);
}
static void discover(void){
 nfiles=0;selected=0;
 for(int r=0;game_root[game][r];r++)if(is_dir(game_root[game][r]))crawl(game_root[game][r],0,game==0&&r>=2);
 if(game==2)
  snprintf(status_line,sizeof(status_line),nfiles?
      "%d CARBON CAREER PROFILE(S) FOUND":"NO CARBON CAREER PROFILES FOUND",nfiles);
 else
  snprintf(status_line,sizeof(status_line),"%d FILES FOUND / CHOOSE A CANDIDATE SAVE",nfiles);
}
static uint32_t change(uint32_t v,int sign,int digit_place){
 uint32_t step=1;for(int i=0;i<digit_place;i++)step*=10;
 if(sign>0)return v>999999999-step?999999999:v+step;
 return v<step?0:v-step;
}
static void change_number(u64 key,uint32_t *v){
 if(metric==2){
  if(key&HidNpadButton_Up&&*v<5)(*v)++;
  else if(key&HidNpadButton_Down&&*v>1)(*v)--;
  return;
 }
 if(key&HidNpadButton_Left&&digit<8)digit++;
 else if(key&HidNpadButton_Right&&digit>0)digit--;
 else if(key&HidNpadButton_Up)*v=change(*v,1,digit);
 else if(key&HidNpadButton_Down)*v=change(*v,-1,digit);
}
static void handle(u64 key,u64 held){
 if(screen==0){
  if(key&HidNpadButton_L){game=(game+2)%3;status_line[0]=0;}
  if(key&HidNpadButton_R){game=(game+1)%3;status_line[0]=0;}
  if(key&HidNpadButton_A){
   discover();
   if(game==2 && nfiles==1){
    carbon_cash_error=carbon_read_cash(files[0].path,&carbon_cash_value);
    carbon_hmac_status=carbon_auth_verify_zero_file(files[0].path);
    status_line[0]=0;screen=22;
   }else{
    if(game==2 && nfiles>0)status_line[0]=0;
    screen=1;
   }
  }
 }else if(screen==1){
  if(key&HidNpadButton_B){screen=0;status_line[0]=0;}
  else if(key&HidNpadButton_Up&&selected>0)selected--;
  else if(key&HidNpadButton_Down&&selected<nfiles-1)selected++;
  else if(key&HidNpadButton_A&&nfiles){
   if(game==2){
    carbon_cash_error=carbon_read_cash(files[selected].path,&carbon_cash_value);
    carbon_hmac_status=carbon_auth_verify_zero_file(files[selected].path);
    status_line[0]=0;screen=22;
   }else{
    old_val=0;new_val=0;digit=4;action=0;screen=7;
    snprintf(status_line,sizeof(status_line),"SELECTED: %.130s",files[selected].name);
   }
  }
 }else if(screen==7){
  if(key&HidNpadButton_B)screen=1;
  else if(key&HidNpadButton_Up&&action>0)action--;
  else if(key&HidNpadButton_Down&&action<7)action++;
  else if(key&HidNpadButton_A){
   if(action==0){
    if(game!=0||files[selected].backup){
     snprintf(status_line,sizeof(status_line),"CASH EDIT: LIVE MOST WANTED MC02 ONLY");
    }else{
     cash_error=mc02_read_cash(files[selected].path,&old_val);
     if(cash_error)snprintf(status_line,sizeof(status_line),
          "EDIT BLOCKED: %s",mc02_error(cash_error));
     else {new_val=old_val;digit=4;screen=15;}
    }
   }else if(action==1||action==2){
    if(game!=0){
     bounty_error=-1;
    }else{
     bounty_error=mc02_read_bounty_info(files[selected].path,&bounty_info);
    }
    if(action==2){screen=18;}
    else if(files[selected].backup){
     snprintf(status_line,sizeof(status_line),"BOUNTY EDIT: SELECT A LIVE SAVE, NOT A COPY");
    }else if(bounty_error){
     snprintf(status_line,sizeof(status_line),"BOUNTY EDIT BLOCKED: %s",mc02_error(bounty_error));
    }else{
     bounty_selected=bounty_info.preferred_index;
     old_val=bounty_info.total;new_val=old_val;
     digit=4;screen=19;
    }
   }else if(action==3 && game==2){
    carbon_cash_error=carbon_read_cash(files[selected].path,&carbon_cash_value);
    screen=22;
   }else if(action==3||action==4){
    metric=action==3?0:2;
    old_val=metric==2?1:0;digit=4;screen=2;
   }else if(action==5){
    metric=0;load_backups();screen=11;
   }else if(action==6){
    if(files[selected].backup)
      snprintf(status_line,sizeof(status_line),"BACKUP: PLEASE SELECT LIVE SAVE");
    else{
     backup_result=0;
     backup_error=lab_make_backup(files[selected].path,backup_path,sizeof(backup_path));
     screen=10;
    }
   }else{
    if(files[selected].backup)
     snprintf(status_line,sizeof(status_line),"RESTORE: SELECT LIVE SAVE");
    else{load_backups();screen=8;}
   }
  }
 }else if(screen==2){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_A){
   scan_err=lab_scan(files[selected].path,old_val,&matches);
   match_idx=0;screen=3;
   snprintf(status_line,sizeof(status_line),"SCAN COMPLETE: %d MATCHES. WRITE DISABLED.",matches.count);
  }else change_number(key,&old_val);
 }else if(screen==3){
  if(key&HidNpadButton_B)screen=2;
  else if(key&HidNpadButton_Up&&match_idx>0)match_idx--;
  else if(key&HidNpadButton_Down&&match_idx<matches.count-1)match_idx++;
  else if(key&HidNpadButton_A)snprintf(status_line,sizeof(status_line),
            "WRITE DISABLED UNTIL FORMAT AND HASHES ARE VERIFIED");
 }else if(screen==8){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_Up&&restore_index>0)restore_index--;
  else if(key&HidNpadButton_Down&&restore_index<restore_count-1)restore_index++;
  else if(key&HidNpadButton_A&&restore_count>0)screen=9;
 }else if(screen==9){
  if(key&HidNpadButton_B)screen=8;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    backup_result=1;
    backup_error=lab_restore_backup(files[selected].path,
        restore_paths[restore_index],backup_path,sizeof(backup_path));
    screen=10;
    snprintf(status_line,sizeof(status_line),backup_error?"RESTORE FAILED":"RESTORED VERIFIED COPY");
   }else snprintf(status_line,sizeof(status_line),"HOLD ZL + ZR WHILE PRESSING A");
  }
 }else if(screen==10){
  if(key&HidNpadButton_B)screen=7;
 }else if(screen==11){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_L)metric=(metric+2)%3;
  else if(key&HidNpadButton_R)metric=(metric+1)%3;
  else if(key&HidNpadButton_Up&&restore_index>0)restore_index--;
  else if(key&HidNpadButton_Down&&restore_index<restore_count-1)restore_index++;
  else if(key&HidNpadButton_A&&restore_count>0){
   old_val=metric==2?1:0;new_val=metric==2?2:0;digit=4;screen=12;
  }
 }else if(screen==12 || screen==13){
  uint32_t *v=screen==12?&old_val:&new_val;
  if(key&HidNpadButton_B)screen=screen==12?11:12;
  else if(key&HidNpadButton_A){
   if(screen==12){digit=4;screen=13;}
   else{
    diff_error=lab_diff_values(restore_paths[restore_index],
       files[selected].path,old_val,new_val,&diff_result);
    screen=14;
    snprintf(status_line,sizeof(status_line),
       "READ-ONLY COMPARISON COMPLETE / %u CHANGED BYTES",diff_result.changed_bytes);
   }
  }else change_number(key,v);
 }else if(screen==14&&key&HidNpadButton_B)screen=11;
 else if(screen==15){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_A)screen=16;
  else {int prior=metric;metric=0;change_number(key,&new_val);metric=prior;}
 }else if(screen==16){
  if(key&HidNpadButton_B)screen=15;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    cash_error=mc02_apply_cash(files[selected].path,old_val,new_val,
                                backup_path,sizeof(backup_path));
    screen=17;
    snprintf(status_line,sizeof(status_line),
     cash_error?"CASH EDIT FAILED / CHECK BACKUP PATH":"CASH UPDATED / VERIFIED BACKUP RETAINED");
   }else snprintf(status_line,sizeof(status_line),
      "HOLD ZL + ZR AND PRESS A TO CONFIRM");
  }
 }else if(screen==17&&key&HidNpadButton_B)screen=7;
 else if(screen==18&&key&HidNpadButton_B)screen=7;
 else if(screen==19){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_L){
   bounty_selected=bounty_selected?bounty_selected-1:bounty_info.active_count-1;
  }else if(key&HidNpadButton_R){
   bounty_selected=(bounty_selected+1)%bounty_info.active_count;
  }else if(key&HidNpadButton_A){
   uint32_t min_value=old_val-bounty_info.active_bounty[bounty_selected];
   if(new_val<min_value)
    snprintf(status_line,sizeof(status_line),"TOO LOW: MINIMUM %u (OTHER CARS + SOLD)",min_value);
   else if(new_val==old_val)
    snprintf(status_line,sizeof(status_line),"NEW BOUNTY MUST DIFFER FROM CURRENT");
   else screen=20;
  }else{int before=metric;metric=0;change_number(key,&new_val);metric=before;}
 }else if(screen==20){
  if(key&HidNpadButton_B)screen=19;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    bounty_write_error=mc02_apply_bounty(files[selected].path,old_val,new_val,
                        bounty_selected,backup_path,sizeof(backup_path));
    screen=21;
    snprintf(status_line,sizeof(status_line),bounty_write_error?
            "BOUNTY FAILED: CHECK BACKUP":"BOUNTY UPDATED: VERIFIED BACKUP CREATED");
   }else snprintf(status_line,sizeof(status_line),"HOLD ZL + ZR THEN PRESS A");
  }
 }else if(screen==21&&key&HidNpadButton_B)screen=7;
 else if(screen==22){
  if(key&HidNpadButton_B){screen=nfiles==1?0:1;status_line[0]=0;}
  else if(key&HidNpadButton_A && !carbon_cash_error && !carbon_hmac_status){
   carbon_test_amount=carbon_cash_value;
   digit=4;screen=23;status_line[0]=0;
  }
 }else if(screen==23){
  if(key&HidNpadButton_B)screen=22;
  else if(key&HidNpadButton_A){
   if(carbon_test_amount==carbon_cash_value)
    snprintf(status_line,sizeof(status_line),"NEW CASH MUST DIFFER FROM CURRENT VALUE");
   else{screen=24;status_line[0]=0;}
  }else{
   int previous=metric;metric=0;
   change_number(key,&carbon_test_amount);metric=previous;
  }
 }else if(screen==24){
  if(key&HidNpadButton_B)screen=23;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    carbon_test_error=carbon_apply_cash(files[selected].path,
      carbon_cash_value,carbon_test_amount,
      backup_path,sizeof(backup_path));
    if(!carbon_test_error){
     carbon_cash_value=carbon_test_amount;
     carbon_hmac_status=carbon_auth_verify_zero_file(files[selected].path);
    }
    screen=25;status_line[0]=0;
   }else snprintf(status_line,sizeof(status_line),"HOLD ZL + ZR AND PRESS A");
  }
 }else if(screen==25&&key&HidNpadButton_B){screen=22;status_line[0]=0;}
}
int main(int argc,char **argv){
 (void)argc;(void)argv;
 NWindow *win=nwindowGetDefault();
 if(R_FAILED(framebufferCreate(&fb,win,SW,SH,PIXEL_FORMAT_RGBA_8888,2)))return 1;
 if(R_FAILED(framebufferMakeLinear(&fb))){framebufferClose(&fb);return 1;}
 poster_hd_init(argc,argv);
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);
 PadState pad;padInitializeDefault(&pad);
 draw();
 while(appletMainLoop()){
  padUpdate(&pad);
  u64 key=padGetButtonsDown(&pad);
  if(screen==0&&(key&HidNpadButton_Plus))break;
  if(key){handle(key,padGetButtons(&pad));draw();}
  svcSleepThread(16000000L);
 }
 free(hd_posters);hd_posters=NULL;
 framebufferClose(&fb);return 0;
}
