#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define COD1_SS_TABLE_BYTES (4u*1024u*1024u)
#define FS_READ 0
typedef int fileHandle_t;
static unsigned allocations,frees,fail;
static void *test_malloc(size_t n){++allocations;return fail?NULL:malloc(n);}
static void *test_realloc(void *p,size_t n){++allocations;return fail?NULL:realloc(p,n);}
static void test_free(void*p){if(p)++frees;free(p);}
#define malloc test_malloc
#define realloc test_realloc
#define free test_free
#include "bg_cod1_sound_read56.inc"
#undef malloc
#undef realloc
#undef free
static Cod1SSReadBuffer56 gscSoundRead56;
static int mock_len,mock_handle,mock_got;static unsigned opens,closes,reads;
static int trap_FS_FOpenFile(const char *path,fileHandle_t *f,int mode){assert(path&&mode==FS_READ);++opens;*f=mock_handle;return mock_len;}
static int trap_FS_ReadCount(void *p,int n,fileHandle_t f){assert(f&&n==mock_len);++reads;if(mock_got>0&&mock_got<=n)memset(p,0x63,(size_t)mock_got);return mock_got;}
static void trap_FS_FCloseFile(fileHandle_t f){assert(f);++closes;}
#include "reader.inc"
static uint64_t rng=UINT64_C(0xe38a61278179b62f);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
int main(void){
 unsigned i,ok=0,refused=0;char *p,*nested;size_t n;unsigned a;
 for(i=0;i<100000;i++){
  unsigned o=opens,c=closes,r=reads;int expected;
  if(i%127==0)Cod1SS_ReadReset56(&gscSoundRead56);
  mock_len=(int)(rnd()%4096)+1;mock_handle=1;mock_got=mock_len;fail=0;
  switch(i%11){case 0:mock_handle=0;break;case 1:mock_len=-1;break;case 2:mock_len=0;break;case 3:mock_len=COD1_SS_TABLE_BYTES+1;break;case 4:mock_got=mock_len-1;break;case 5:mock_got=-1;break;case 6:mock_got=mock_len+1;break;case 7:fail=1;break;default:break;}
  expected=mock_handle&&mock_len>0&&mock_len<=(int)COD1_SS_TABLE_BYTES&&(!fail||(size_t)mock_len<=gscSoundRead56.capacity)&&mock_got==mock_len;
  p=NULL;n=0;assert(GscSoundRead(NULL,"soundaliases/a.csv",&p,&n)==expected);assert(opens==o+1&&closes==c+(mock_handle!=0));
  if(expected){assert(p&&n==(size_t)mock_len&&reads==r+1);for(size_t j=0;j<n;j++)assert(p[j]==0x63);++ok;GscSoundRelease(NULL,p);}else{assert(!p&&!n);++refused;}
  assert(!gscSoundRead56.busy&&gscSoundRead56.capacity<=COD1_SS56_READ_KEEP);
 }
 fail=0;Cod1SS_ReadReset56(&gscSoundRead56);a=allocations;mock_len=2048;mock_got=2048;mock_handle=1;
 for(i=0;i<10000;i++){assert(GscSoundRead(NULL,"soundaliases/a.csv",&p,&n));GscSoundRelease(NULL,p);}assert(allocations-a==1);
 p=Cod1SS_ReadAcquire56(&gscSoundRead56,64);assert(p);memset(p,'x',64);nested=Cod1SS_ReadAcquire56(&gscSoundRead56,128);assert(nested&&nested!=p);Cod1SS_ReadReset56(&gscSoundRead56);assert(gscSoundRead56.retire&&gscSoundRead56.busy&&p[63]=='x');Cod1SS_ReadRelease56(&gscSoundRead56,nested);Cod1SS_ReadRelease56(&gscSoundRead56,p);assert(!gscSoundRead56.bytes&&!gscSoundRead56.busy&&!gscSoundRead56.retire);
 p=Cod1SS_ReadAcquire56(&gscSoundRead56,COD1_SS56_READ_KEEP+1);assert(p&&!gscSoundRead56.bytes);Cod1SS_ReadRelease56(&gscSoundRead56,p);fail=1;assert(!Cod1SS_ReadAcquire56(&gscSoundRead56,64));assert(!gscSoundRead56.busy);fail=0;
 p=Cod1SS_ReadAcquire56(&gscSoundRead56,64);assert(p);Cod1SS_ReadRelease56(&gscSoundRead56,p);fail=1;assert(!Cod1SS_ReadAcquire56(&gscSoundRead56,128));assert(gscSoundRead56.capacity==64&&!gscSoundRead56.busy);fail=0;Cod1SS_ReadReset56(&gscSoundRead56);
 printf("R56A3A PASS: 100000 read cases success=%u refused=%u; 10000 repeated reads: 1 allocation; opens=%u closes=%u reads=%u\n",ok,refused,opens,closes,reads);return 0;
}
