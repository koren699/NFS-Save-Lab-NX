#include "../source/mc02.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
int main(int argc,char **argv){
 assert(argc==3);
 uint32_t bounty=0,offset=0;
 assert(mc02_read_bounty(argv[1],&bounty,&offset)==0);
 assert(bounty==31550 && offset==0x010e);
 assert(mc02_read_bounty(argv[2],&bounty,&offset)==0);
 assert(bounty==45150 && offset==0x015a);
 puts("PASS: MC02 bounty float discovered by unique record marker at moving offsets");
 return 0;
}
