#include <switch.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include "engine.h"

#define MAX_FILES 180
#define PATH_CAP 768
#define VISIBLE 13

static const char *games[]={"Most Wanted '05 (Xbox 360)","Underground 2 (Xbox)","Carbon (Xbox)"};
static const char *roots[][5]={
 {"sdmc:/switch/nfsmw-nx/nfsmw", "sdmc:/switch/nfsmw/nfsmw", "sdmc:/switch/nfsmw-nx/saves", "sdmc:/switch/nfsmw/saves", NULL},
 {"sdmc:/switch/nfsu2x/game/UDATA","sdmc:/switch/nfsu2x/game/Save",NULL},
 {"sdmc:/switch/nfscx/game/UDATA","sdmc:/switch/nfscx/game/Save",NULL}
};
typedef struct{char path[PATH_CAP];char relative[160];uint32_t bytes;int copy_only;} SaveFile;
static SaveFile files[MAX_FILES];static int nfiles=0,game=0,selected=0;
static LabMatches matches;static int matchidx=0;static uint32_t old_val=0,new_val=9999999;
static int scanerr=0,patcherr=0;static int screen=0;static int digit=4;
static char notice[240]="Choose game / A: search files";
static char backuppath[1024]="";

static int is_dir(const char *path){struct stat st;return stat(path,&st)==0 && S_ISDIR(st.st_mode);}
static int included_file(const char *name){
    if(name[0]=='.')return 0;
    const char *suffixes[]={".nro",".nsp",".xex",".xbe",".iso",".png",".jpg",".jpeg",".dds",".log",".toml",".nfsp",".bak",".tmp",".xml",".txt",".tga",".zip",".7z",NULL};
    for(int k=0;suffixes[k];k++){
        size_t n=strlen(name),l=strlen(suffixes[k]);
        if(n>=l && strcasecmp(name+n-l,suffixes[k])==0)return 0;
    }
    if(strstr(name,".nfslab."))return 0;
    return 1;
}
static void crawl(const char *path,const char *label,int depth,int copy_only){
    if(depth>7 || nfiles>=MAX_FILES)return;
    DIR *dir=opendir(path);if(!dir)return;
    struct dirent *ent;
    while((ent=readdir(dir)) && nfiles<MAX_FILES){
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))continue;
        char full[PATH_CAP];int k=snprintf(full,sizeof(full),"%s/%s",path,ent->d_name);
        if(k<0||(size_t)k>=sizeof(full))continue;
        struct stat st;if(stat(full,&st)!=0)continue;
        if(S_ISDIR(st.st_mode)){crawl(full,label,depth+1,copy_only);continue;}
        if(!S_ISREG(st.st_mode)||st.st_size<32||st.st_size>(off_t)LAB_MAX_FILE||!included_file(ent->d_name))continue;
        SaveFile *f=&files[nfiles++];snprintf(f->path,sizeof(f->path),"%s",full);
        snprintf(f->relative,sizeof(f->relative),"%s / %.100s",label,ent->d_name);
        f->bytes=(uint32_t)st.st_size;f->copy_only=copy_only;
    }
    closedir(dir);
}
static void discover(void){
    nfiles=selected=0;
    for(int r=0;roots[game][r];r++){
        const char *root=roots[game][r];
        if(is_dir(root)){
            const char *leaf=strrchr(root,'/');
            crawl(root,leaf?leaf+1:root,0,game==0 && r>=2);
        }
    }
    snprintf(notice,sizeof(notice),"Found %d candidate files. Choose a save.",nfiles);
}
static uint32_t inc_amount(uint32_t v,int sign,int place){
    static const uint32_t steps[]={1,10,100,1000,10000,100000,1000000,10000000,100000000};
    uint32_t d=steps[place];if(sign>0)return v>999999999u-d?999999999u:v+d;
    return v<d?0:v-d;
}
static int value_input(PadState *pad,uint32_t *v,u64 keys){
    if(keys&HidNpadButton_Left){if(digit<8)digit++;return 0;}
    if(keys&HidNpadButton_Right){if(digit>0)digit--;return 0;}
    if(keys&HidNpadButton_Up){*v=inc_amount(*v,1,digit);return 1;}
    if(keys&HidNpadButton_Down){*v=inc_amount(*v,-1,digit);return 1;}
    (void)pad;return 0;
}
static void draw(void){
    consoleClear();
    printf("NFS SAVE LAB NX  v0.1  |  OFFLINE\n");
    printf("----------------------------------------------------------\n");
    printf("Game: %s\n",games[game]);
    printf("Warning: experimental. CHECKSUMS ARE NOT REPAIRED.\n");
    printf("Exit games before editing. Keep a separate backup.\n\n");
    if(screen==0){
        printf("Select game: L / R\n\n");
        for(int i=0;i<3;i++)printf(" %c %s\n",game==i?'>':' ',games[i]);
        printf("\nA: find local save files    +: exit\n");
    }else if(screen==1){
        printf("File list (%d). Up/Down: choose. A: select. B: back\n",nfiles);
        int start=selected-(VISIBLE/2);if(start<0)start=0;if(start>nfiles-VISIBLE)start=nfiles-VISIBLE;if(start<0)start=0;
        for(int i=start;i<nfiles && i<start+VISIBLE;i++){
            printf("%c%c %.38s  (%u KB)\n",i==selected?'>':' ',files[i].copy_only?'!':' ',files[i].relative,files[i].bytes/1024);
        }
        if(nfiles==0)printf("No files. Make a game save first.\n");
        printf("\n! = port backup copy (view ONLY)\n");
    }else if(screen==2){
        printf("Selected: %.58s\n",files[selected].relative);
        printf("Size: %u bytes   %s\n\n",files[selected].bytes,files[selected].copy_only?"BACKUP COPY":"LIVE CANDIDATE");
        printf("1) Enter CURRENT in-game cash/bounty value:\n");
        printf("    %09u\n",old_val);
        printf("    %*s^ digit\n\n",13-digit,"");
        printf("Up/Down: change highlighted digit (10^%d)\n",digit);
        printf("Left/Right: choose digit\n");
        printf("A: scan file   B: back\n");
    }else if(screen==3){
        printf("Scan result: %d hit(s)%s\n",matches.count,matches.overflow?" (TRUNCATED)":"");
        if(scanerr)printf("Error: %s\n",lab_error(scanerr));
        if(matches.count==0)printf("No plain 32-bit value found. Could be encoded/hashed.\n");
        for(int i=0;i<matches.count && i<8;i++){
            printf(" %c Offset 0x%08X  %s\n",i==matchidx?'>':' ',matches.items[i].offset,matches.items[i].be?"BIG endian":"LITTLE endian");
        }
        if(matches.count>8)printf("... total %d; use Up/Down to select\n",matches.count);
        printf("\nONLY exact current value matches are shown.\n");
        printf("A: change target amount  B: back\n");
    }else if(screen==4){
        printf("New amount (experimental patch):\n\n");
        printf("    %09u\n",new_val);
        printf("    %*s^ digit\n\n",13-digit,"");
        printf("Up/Down: change highlighted digit; Left/Right: digit\n");
        printf("A: review write   B: back\n");
    }else if(screen==5){
        printf("DANGER: Changing raw data may INVALIDATE GAME SAVE!\n\n");
        printf("OLD %u -> NEW %u\n",old_val,new_val);
        printf("Offset 0x%08X, %s\n",matches.items[matchidx].offset,matches.items[matchidx].be?"BE":"LE");
        printf("%s\n\n",files[selected].path);
        printf("Will preserve original as .nfslab.bakN.\n");
        printf("Will NOT repair game checksums/hashes.\n");
        printf("Hold ZL + ZR and press A to write.\n");
        printf("B: cancel\n");
    }else if(screen==6){
        printf("Result: %s\n",patcherr?lab_error(patcherr):"Wrote candidate bytes");
        if(!patcherr)printf("Backup: %s\n",backuppath);
        printf("\nTHIS DOES NOT GUARANTEE THE GAME WILL LOAD SAVE.\n");
        printf("B: go to file list\n");
    }
    printf("\n----------------------------------------------------------\n");
    printf("%s\n",notice);
    consoleUpdate(NULL);
}
int main(int argc,char **argv){
    (void)argc;(void)argv;
    consoleInit(NULL);padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState pad;padInitializeDefault(&pad);
    draw();
    while(appletMainLoop()){
        padUpdate(&pad);u64 k=padGetButtonsDown(&pad);
        if(!k){svcSleepThread(12000000L);continue;}
        if(screen==0){
            if(k&HidNpadButton_Plus)break;
            if(k&HidNpadButton_L)game=(game+2)%3;
            if(k&HidNpadButton_R)game=(game+1)%3;
            if(k&HidNpadButton_A){discover();screen=1;}
        }else if(screen==1){
            if(k&HidNpadButton_B)screen=0;
            if(k&HidNpadButton_Up && selected>0)selected--;
            if(k&HidNpadButton_Down && selected<nfiles-1)selected++;
            if(k&HidNpadButton_A && nfiles>0){old_val=0;digit=4;screen=2;}
        }else if(screen==2){
            if(k&HidNpadButton_B)screen=1;
            else if(k&HidNpadButton_A){scanerr=lab_scan(files[selected].path,old_val,&matches);matchidx=0;screen=3;}
            else value_input(&pad,&old_val,k);
        }else if(screen==3){
            if(k&HidNpadButton_B)screen=2;
            if(k&HidNpadButton_Up && matchidx>0)matchidx--;
            if(k&HidNpadButton_Down && matchidx<matches.count-1)matchidx++;
            if(k&HidNpadButton_A && !scanerr && matches.count>0 && !matches.overflow && !files[selected].copy_only){
                digit=4;screen=4;
            }
        }else if(screen==4){
            if(k&HidNpadButton_B)screen=3;
            else if(k&HidNpadButton_A)screen=5;
            else value_input(&pad,&new_val,k);
        }else if(screen==5){
            if(k&HidNpadButton_B)screen=4;
            else if(k&HidNpadButton_A){
                u64 held=padGetButtons(&pad);
                if((held&HidNpadButton_ZL) && (held&HidNpadButton_ZR)){
                    patcherr=lab_patch(files[selected].path,old_val,new_val,matches.items[matchidx],backuppath,sizeof(backuppath));
                    screen=6;
                }else snprintf(notice,sizeof(notice),"Hold BOTH triggers (ZL + ZR) while pressing A.");
            }
        }else if(screen==6){if(k&HidNpadButton_B){discover();screen=1;}}
        draw();
    }
    consoleExit(NULL);return 0;
}
