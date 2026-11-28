#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cell.inc"
#include "skip.inc"
/* Exact reference cell/column validation after the known-valid name. */
static int reference(const char *p,const char *end){
 int col=1,bad=0;char cell[256];
 while(p<end){p=Cod1SS_Cell(p,end,cell,sizeof(cell),&bad);if(col>=64)bad=1;++col;}
 return !bad;
}
static uint64_t rng=UINT64_C(0xe38a61278179b62f);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static void check(const char *p,size_t n){assert(reference(p,p+n)==Cod1SS_UnselectedRow56(p,p+n));}
int main(void){
 char row[21000];unsigned i,j,k,yes=0,no=0;size_t n;
 for(i=0;i<200000;i++){
  n=0;
  if(i%3==0){const char alphabet[]="abcXYZ,\"\t 123";n=rnd()%sizeof(row);for(j=0;j<n;j++)row[j]=alphabet[rnd()%(sizeof(alphabet)-1)];}
  else{
   unsigned cols=rnd()%70;
   for(j=0;j<cols;j++){
    unsigned len=rnd()%280;int mode=(int)(rnd()%6);
    if(mode==1||mode==2)row[n++]='"';
    for(k=0;k<len;k++){
     if(mode==1&&k%17==0){row[n++]='"';row[n++]='"';}
     else row[n++]=mode==2&&k%17==0?',':'a';
    }
    if(mode==1||mode==2)row[n++]='"';
    if(mode==4)row[n++]='"';
    if(j+1<cols||i%5==0)row[n++]=',';
    if(n>sizeof(row)-600)break;
   }
  }
  check(row,n);if(reference(row,row+n))++yes;else ++no;
 }
 memset(row,'a',256);check(row,255);check(row,256);assert(Cod1SS_UnselectedRow56(row,row+255));assert(!Cod1SS_UnselectedRow56(row,row+256));
 memset(row,',',64);check(row,63);check(row,64);assert(Cod1SS_UnselectedRow56(row,row+63));assert(!Cod1SS_UnselectedRow56(row,row+64));
 check("\"a,b\",\"a\"\"b\",c",sizeof("\"a,b\",\"a\"\"b\",c")-1);check("\"a\" x",5);check("\"a",2);check("",0);
 assert(yes&&no);printf("R56A CSV PASS: 200000 rows valid=%u invalid=%u\n",yes,no);return 0;
}
