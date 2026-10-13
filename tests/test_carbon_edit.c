#include "../source/carbon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 const char *original=argv[1];
 uint32_t cash=0;
 assert(carbon_read_cash(original,&cash)==0 && cash==10200);
 char dst[1024];
 assert(carbon_create_cash_test(original,10201,50000,dst,sizeof(dst))==-11);
 assert(carbon_create_cash_test(original,10200,100000,dst,sizeof(dst))==0);
 assert(carbon_read_cash(dst,&cash)==0 && cash==100000);
 assert(carbon_read_cash(original,&cash)==0 && cash==10200);
 assert(carbon_create_cash_test(original,10200,200000,dst,sizeof(dst))==-17);
 assert(carbon_read_cash(dst,&cash)==0 && cash==100000);
 assert(remove(dst)==0);
 puts("PASS: Carbon separate test save, arbitrary 100000 Cash, strict no overwrite");
 return 0;
}
