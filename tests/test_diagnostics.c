#include "../source/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
int main(void){
 const char *old="tests/compare_before.bin",*now="tests/compare_after.bin";
 unsigned char a[128]={0},b[128]={0};
 /* Cash: 123456 => 234567, LE offset 44 */
 a[44]=0x40;a[45]=0xe2;a[46]=0x01;a[47]=0x00;
 b[44]=0x47;b[45]=0x94;b[46]=0x03;b[47]=0x00;
 a[69]=0x05;b[69]=0x06;
 FILE *f=fopen(old,"wb");assert(f);assert(fwrite(a,1,sizeof(a),f)==sizeof(a));fclose(f);
 f=fopen(now,"wb");assert(f);assert(fwrite(b,1,sizeof(b),f)==sizeof(b));fclose(f);
 LabDiff d;
 assert(lab_diff_values(old,now,123456,234567,&d)==0);
 assert(d.matches==1 && d.entries[0].offset==44 && d.entries[0].big_endian==0);
 assert(d.changed_bytes==4 && d.changed_ranges==2);
 assert(lab_diff_values(old,now,123456,123456,&d)==-4);
 remove(old);remove(now);
 puts("PASS: read-only diff and numeric-pair candidate finder");
 return 0;
}
