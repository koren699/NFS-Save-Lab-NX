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
 {"sdmc:/switch/nfscx/game/UDATA","sdmc:/switch/nfscx/game/Save",NULL}
};
static SaveFile files[MAX_FILES];
static int nfiles=0,selected=0,game=0,screen=0,match_idx=0,digit=4;
static int metric=0, action=0, restore_index=0, restore_count=0, backup_result=0;
static const char *metric_names[]={"CASH","BOUNTY","HEAT LEVEL"};
static char restore_paths[64][1024];
static uint32_t old_val=0,new_val=9999999;
static LabMatches matches;
static int scan_err=0,patch_err=0;
static char backup_path[1024]={0};
static int backup_error=0;
static char status_line[190]="READY TO SCAN LOCAL SAVES";
static Framebuffer fb;
static uint32_t *pixels;
static int pitch;
static uint32_t accent=GREEN;

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
 txt(980,42,2,"BUILD 0.3.0",WHITE);
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
 label(67,685,"CLOSE THE GAME BEFORE EDITING  /  SAVE CHECKSUMS NOT VERIFIED");
}
static void game_card(int idx,int x,int y){
 int active=game==idx,w=360,h=355;
 frame(x,y,w,h,active);
 uint32_t c=idx==0?GREEN:idx==1?COLOR(69,209,241):COLOR(247,164,67);
 rect(x+15,y+15,w-30,118,COLOR(12,19,25));
 for(int z=0;z<13;z++){
  int ax=x+132+z*24;
  line(ax,y+15,ax-85,y+132,COLOR(38,52,55),1);
 }
 rect(x+18,y+15,9,118,c);
 txt(x+40,y+37,5,idx==0?"MW":idx==1?"U2":"CX",WHITE);
 txt(x+44,y+102,2,idx==0?"2005":idx==1?"2004":"2006",c);
 txt(x+22,y+164,3,game_names[idx],active?WHITE:COLOR(201,213,218));
 txt(x+22,y+220,2,game_descriptions[idx],MUTED);
 rect(x+22,y+266,316,1,COLOR(60,73,79));
 txt(x+22,y+290,2,idx==0?"PORT / RECOMP X360":"PORT / RECOMP XBOX",active?c:MUTED);
 if(active){pill(x+210,y+20,"SELECTED",c);}
}
static void dashboard(void){
 header("SELECT A PORT");
 txt(65,185,4,"YOUR GARAGE.",WHITE);
 txt(65,231,2,"CHOOSE THE GAME TO ANALYZE LOCAL SAVE DATA",MUTED);
 for(int i=0;i<3;i++)game_card(i,65+i*392,274);
 footer("L / R  CHANGE GAME       A  OPEN SAVE BROWSER       +  EXIT");
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
 label(862,331,"CURRENT VALUE");
 char b[50];snprintf(b,sizeof(b),"%09u",old_val);txt(862,362,3,b,WHITE);
 label(862,416,"PATCH MODE");
 pill(862,451,"EXPERIMENTAL",COLOR(245,178,72));
 label(862,520,"BACKUP: AUTO ON WRITE");
}
static void files_page(void){
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
 }
 if(!nfiles){txt(88,335,2,"NO SAVE FILES FOUND IN KNOWN PORT FOLDERS",RED);label(88,373,"CREATE A GAME SAVE AND TRY AGAIN.");}
 footer("UP / DOWN  SELECT       A  CONTINUE       B  BACK");
}
static void action_page(void){
 header("02 / MODIFY OR PROTECT SAVE");
 info_panel("CHOOSE AN ACTION","SELECT A METRIC OR BACKUP OPERATION");
 const char *entries[]={"CASH","BOUNTY","HEAT LEVEL","CREATE BACKUP","RESTORE BACKUP"};
 const char *notes[]={"IN-GAME MONEY","CHASE REWARD POINTS","CHASE POLICE HEAT (1 - 5)","KEEP ORIGINAL SNAPSHOT","RESTORE A PREVIOUS SNAPSHOT"};
 for(int i=0;i<5;i++){
  int y=284+i*58,sel=i==action;
  if(sel){rect(78,y-10,725,52,COLOR(43,64,63));rect(78,y-10,5,52,accent);}
  char s[75];snprintf(s,sizeof(s),"0%d   %s",i+1,entries[i]);
  txt(96,y,2,s,sel?WHITE:MUTED);
  txt(395,y,2,notes[i],sel?accent:MUTED);
 }
 footer("UP / DOWN  SELECT       A  OPEN       B  FILES");
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
 label(89,509,next?"A  REVIEW CHANGE":"A  SCAN FOR CURRENT VALUE");
 footer(next?"A  REVIEW       B  BACK":"A  SCAN       B  BACK");
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
 footer(matches.overflow?"TOO MANY MATCHES  /  WRITING DISABLED":
       metric==2&&matches.count!=1?"HEAT NEEDS EXACTLY ONE MATCH  /  WRITE BLOCKED":
       "UP / DOWN  OFFSET       A  EDIT       B  BACK");
}
static void review_page(void){
 header("05 / SAFETY CHECK");
 info_panel("CONFIRM CHANGE","VERIFY THE FILE AND AMOUNTS BEFORE WRITING");
 char b[140];
 snprintf(b,sizeof(b),"%s",metric_names[metric]);txt(95,279,2,b,WHITE);
 snprintf(b,sizeof(b),"CURRENT   %09u",old_val);txt(95,324,3,b,MUTED);
 snprintf(b,sizeof(b),"NEW       %09u",new_val);txt(95,381,3,b,accent);
 snprintf(b,sizeof(b),"OFFSET 0x%08X / %s",matches.items[match_idx].offset,matches.items[match_idx].be?"BE":"LE");
 txt(95,437,2,b,WHITE);
 txt(95,485,2,"GAME SAVE MAY FAIL CHECKSUM VALIDATION",RED);
 txt(95,523,2,"HOLD ZL + ZR, THEN PRESS A TO WRITE",WHITE);
 footer("ZL + ZR + A  WRITE AND BACKUP ORIGINAL       B  CANCEL");
}
static void result_page(void){
 header("06 / WRITE RESULT");
 info_panel("OPERATION COMPLETE","RETURN TO BROWSER FOR ANOTHER SAVE");
 txt(91,321,3,patch_err?"WRITE FAILED":"BYTE PATCH WRITTEN",patch_err?RED:accent);
 if(patch_err){txt(91,373,2,lab_error(patch_err),RED);}
 else {label(91,385,"ORIGINAL COPIED TO .NFSLAB.BAKN");label(91,431,"NOT YET VERIFIED TO LOAD IN GAME");}
 footer("B  RETURN TO FILES");
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
 case 4:value_page(1);break;case 5:review_page();break;case 6:result_page();break;
 case 7:action_page();break;case 8:backups_page();break;
 case 9:restore_confirm_page();break;case 10:backup_result_page();break;}
 if(status_line[0]){
  rect(64,605,1150,23,COLOR(36,45,50));
  cliptext(73,609,2,status_line,WHITE,68);
 }
 framebufferEnd(&fb);
}
static int is_dir(const char *p){struct stat st;return stat(p,&st)==0&&S_ISDIR(st.st_mode);}
static int suitable(const char *name){
 if(name[0]=='.'||strstr(name,".nfslab."))return 0;
 const char *bad[]={".nro",".nsp",".xex",".xbe",".iso",".png",".jpg",".dds",".log",".toml",".nfsp",".bak",".tmp",".xml",".txt",".zip",".7z",NULL};
 for(int i=0;bad[i];i++){size_t n=strlen(name),s=strlen(bad[i]);if(n>=s&&!strcasecmp(name+n-s,bad[i]))return 0;}
 return 1;
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
  if(S_ISDIR(st.st_mode)){crawl(p,depth+1,backup);continue;}
  if(!S_ISREG(st.st_mode)||st.st_size<32||st.st_size>(off_t)LAB_MAX_FILE||!suitable(ent->d_name))continue;
  SaveFile *f=&files[nfiles++];
  snprintf(f->path,sizeof(f->path),"%s",p);snprintf(f->name,sizeof(f->name),"%s",ent->d_name);
  f->size=(uint32_t)st.st_size;f->backup=backup;
 }
 closedir(d);
}
static void discover(void){
 nfiles=0;selected=0;
 for(int r=0;game_root[game][r];r++)if(is_dir(game_root[game][r]))crawl(game_root[game][r],0,game==0&&r>=2);
 snprintf(status_line,sizeof(status_line),"%d FILES FOUND / CHOOSE A CANDIDATE SAVE",nfiles);
}
static uint32_t change(uint32_t v,int sign,int digit_place){
 uint32_t step=1;for(int i=0;i<digit_place;i++)step*=10;
 if(sign>0)return v>999999999-step?999999999:v+step;
 return v<step?0:v-step;
}
static void handle(u64 key,u64 held){
 if(screen==0){
  if(key&HidNpadButton_L)game=(game+2)%3;
  if(key&HidNpadButton_R)game=(game+1)%3;
  if(key&HidNpadButton_A){discover();screen=1;}
 } else if(screen==1){
  if(key&HidNpadButton_B)screen=0;
  else if(key&HidNpadButton_Up&&selected>0)selected--;
  else if(key&HidNpadButton_Down&&selected<nfiles-1)selected++;
  else if(key&HidNpadButton_A&&nfiles){old_val=0;digit=4;action=0;screen=7;snprintf(status_line,sizeof(status_line),"SELECTED FILE: %.130s",files[selected].name);}
 } else if(screen==7){
  if(key&HidNpadButton_B)screen=1;
  else if(key&HidNpadButton_Up&&action>0)action--;
  else if(key&HidNpadButton_Down&&action<4)action++;
  else if(key&HidNpadButton_A){
   if(action<3){
    if(files[selected].backup){
     snprintf(status_line,sizeof(status_line),"READ-ONLY PORT COPY / CHOOSE A LIVE FILE");return;
    }
    metric=action;old_val=metric==2?1:0;new_val=metric==2?2:50000;digit=4;screen=2;
   } else if(action==3) {
    if(files[selected].backup){snprintf(status_line,sizeof(status_line),"CANNOT BACK UP A PORT MIRROR AS LIVE SAVE");return;}
    backup_result=0;
    backup_error=lab_make_backup(files[selected].path,backup_path,sizeof(backup_path));
    screen=10;
   } else {
    if(files[selected].backup){snprintf(status_line,sizeof(status_line),"CHOOSE A LIVE SAVE TO RESTORE");return;}
    load_backups();screen=8;
   }
  }
 } else if(screen==2||screen==4){
  uint32_t *v=screen==2?&old_val:&new_val;
  if(key&HidNpadButton_B)screen=screen==2?7:3;
  else if(key&HidNpadButton_A){
   if(screen==2){scan_err=lab_scan(files[selected].path,old_val,&matches);match_idx=0;screen=3;snprintf(status_line,sizeof(status_line),"%d CANDIDATE BYTE MATCHES",matches.count);}
   else screen=5;
  } else if(metric==2) {
   if(key&HidNpadButton_Up&&*v<5)(*v)++;
   else if(key&HidNpadButton_Down&&*v>1)(*v)--;
  } else if(key&HidNpadButton_Left&&digit<8)digit++;
  else if(key&HidNpadButton_Right&&digit>0)digit--;
  else if(key&HidNpadButton_Up)*v=change(*v,1,digit);
  else if(key&HidNpadButton_Down)*v=change(*v,-1,digit);
 } else if(screen==3){
  if(key&HidNpadButton_B)screen=2;
  else if(key&HidNpadButton_Up&&match_idx>0)match_idx--;
  else if(key&HidNpadButton_Down&&match_idx<matches.count-1)match_idx++;
  else if(key&HidNpadButton_A&&!scan_err&&matches.count>0&&!matches.overflow&&!files[selected].backup&&
          (metric!=2||matches.count==1)){digit=4;screen=4;}
  else if(key&HidNpadButton_A&&files[selected].backup)snprintf(status_line,sizeof(status_line),"BACKUP COPY: CANNOT PATCH THIS FILE");
 } else if(screen==5){
  if(key&HidNpadButton_B)screen=4;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    patch_err=lab_patch(files[selected].path,old_val,new_val,matches.items[match_idx],backup_path,sizeof(backup_path));screen=6;
    snprintf(status_line,sizeof(status_line),patch_err?"WRITE FAILED / ORIGINAL SHOULD BE INTACT":"RAW BYTES PATCHED / SAVE INTEGRITY NOT VERIFIED");
   }else snprintf(status_line,sizeof(status_line),"HOLD ZL + ZR WHILE PRESSING A TO CONFIRM");
  }
 }else if(screen==6&&key&HidNpadButton_B){screen=7;}
 else if(screen==8){
  if(key&HidNpadButton_B)screen=7;
  else if(key&HidNpadButton_Up&&restore_index>0)restore_index--;
  else if(key&HidNpadButton_Down&&restore_index<restore_count-1)restore_index++;
  else if(key&HidNpadButton_A&&restore_count>0)screen=9;
 }else if(screen==9){
  if(key&HidNpadButton_B)screen=8;
  else if(key&HidNpadButton_A){
   if((held&HidNpadButton_ZL)&&(held&HidNpadButton_ZR)){
    backup_result=1;backup_error=lab_restore_backup(files[selected].path,
      restore_paths[restore_index],backup_path,sizeof(backup_path));
    screen=10;snprintf(status_line,sizeof(status_line),"RESTORE %s",
      backup_error?"FAILED":"COMPLETE / CURRENT SAVE SAFETY COPY SAVED");
   }else snprintf(status_line,sizeof(status_line),"HOLD ZL + ZR WHILE PRESSING A");
  }
 }else if(screen==10&&key&HidNpadButton_B)screen=7;
}
int main(int argc,char **argv){
 (void)argc;(void)argv;
 NWindow *win=nwindowGetDefault();
 if(R_FAILED(framebufferCreate(&fb,win,SW,SH,PIXEL_FORMAT_RGBA_8888,2)))return 1;
 if(R_FAILED(framebufferMakeLinear(&fb))){framebufferClose(&fb);return 1;}
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
 framebufferClose(&fb);return 0;
}
