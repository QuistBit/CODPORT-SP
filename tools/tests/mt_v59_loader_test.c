/* Actual IBSP59 loader integration, using a concurrent POSIX dispatcher.
 * See mt_v59_loader_test.py for build/run commands. No original game assets.
 * The shipped Windows worker runtime is validated separately. */
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <fenv.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/qcommon/v59/v59_internal.h"

/* Allocation balance covers every explicit CRT allocation in the real loader,
 * curve builder and query modules, including failed/partially finished jobs.
 * Worker-only failure injection cannot disturb fixture or pthread setup. */
static atomic_long live_allocations;
static atomic_int worker_fail_after=ATOMIC_VAR_INIT(-1);
static _Thread_local int worker_thread;
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);
static int fail_worker_allocation(void){
 int left;if(!worker_thread)return 0;left=atomic_load(&worker_fail_after);
 while(left>=0){if(atomic_compare_exchange_weak(&worker_fail_after,&left,left-1))return left==0;}
 return 0;
}
void *__wrap_malloc(size_t n){void *p;if(fail_worker_allocation())return NULL;p=__real_malloc(n);if(p)atomic_fetch_add(&live_allocations,1);return p;}
void *__wrap_calloc(size_t n,size_t size){void *p;if(fail_worker_allocation())return NULL;p=__real_calloc(n,size);if(p)atomic_fetch_add(&live_allocations,1);return p;}
void __wrap_free(void *p){if(p)atomic_fetch_sub(&live_allocations,1);__real_free(p);}

#define TEST_SURFACES 160
#define TEST_VERTICES (TEST_SURFACES * 64)
#define TEST_INDICES (TEST_SURFACES * 300)
typedef struct {
 unsigned char *bytes;size_t size;
 unsigned surface_at,vertex_at,index_at;
 unsigned vertex_first[TEST_SURFACES],index_first[TEST_SURFACES];
 unsigned vertex_count[TEST_SURFACES],index_count[TEST_SURFACES];
 int surfaces;
} fixture;
static void put16(unsigned char *p,unsigned x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);}
static void put32(unsigned char *p,uint32_t x){unsigned i;for(i=0;i<4;i++)p[i]=(unsigned char)(x>>(i*8));}
static void putfloat(unsigned char *p,float x){uint32_t bits;memcpy(&bits,&x,4);put32(p,bits);}
static unsigned lump_bytes(fixture *f,int lump,const void *bytes,unsigned size){
 unsigned at=(unsigned)f->size;assert(f->size+size<1024*1024);
 put32(f->bytes+8+lump*8,size);put32(f->bytes+12+lump*8,at);
 if(size)memcpy(f->bytes+at,bytes,size);f->size+=size;return at;
}
static fixture make_fixture(int count,int curves){
 fixture f;unsigned char *vertices=calloc(TEST_VERTICES,12),*indexes=calloc(TEST_INDICES,2);
 unsigned char materials[72]={0},plane[16]={0},node_bytes[36]={0},leaves[72]={0},model_bytes[48]={0};
 unsigned char records[TEST_SURFACES*16]={0},refs[TEST_SURFACES*4]={0};
 unsigned nv=0,ni=0;int i,j;
 assert(vertices&&indexes&&count<=TEST_SURFACES);memset(&f,0,sizeof(f));
 f.bytes=calloc(1,1024*1024);assert(f.bytes);f.size=272;f.surfaces=count;
 memcpy(f.bytes,"IBSP",4);put32(f.bytes+4,59);
 put32(materials+64,0x00100000);put32(materials+68,1);lump_bytes(&f,0,materials,sizeof(materials));
 putfloat(plane,1);lump_bytes(&f,2,plane,sizeof(plane));
 for(i=0;i<count;i++){
  unsigned char *r=records+i*16;int curve=curves&&(i==63||i==65||i==130);
  unsigned first=nv,start=ni,n=0,m=0;float x=(float)(i%16)*20,y=(float)(i/16)*20;
  f.vertex_first[i]=first;f.index_first[i]=start;r[2]=(unsigned char)(curve?0:1);put32(refs+i*4,(unsigned)i);
  if(curve){
   int row,col;for(row=0;row<3;row++)for(col=0;col<3;col++){
    unsigned char *v=vertices+(nv++)*12;putfloat(v,x+col*2.0f);putfloat(v+4,y+row*2.0f);
    putfloat(v+8,(row==1&&col==1)?0.5f:0.0f);
   }
   put16(r+4,3);put16(r+6,3);put32(r+8,16);put32(r+12,first);n=9;
  }else if(i%17!=0){
   /* A mixture of triangle, duplicate-edge and grid soups. Grid construction
    * exercises the quadratic edge lookup work that workers now perform. */
   if(i%5==0){
    int row,col;for(row=0;row<8;row++)for(col=0;col<8;col++){
     unsigned char *v=vertices+(nv++)*12;putfloat(v,x+col);putfloat(v+4,y+row);putfloat(v+8,0.125f*col);
    }
    n=64;
    for(row=0;row<7;row++)for(col=0;col<7;col++){
     unsigned a=(unsigned)(row*8+col),t[6]={a,a+1,a+9,a,a+9,a+8};
     for(j=0;j<6;j++){put16(indexes+(ni++)*2,t[j]);m++;}
    }
   }else{
    static const float points[4][3]={{0,0,0},{4,0,0},{4,4,1},{0,4,1}};
    unsigned t[6]={0,1,2,0,2,3};n=4;m=i%3?6:3;
    if(i%11==0){t[3]=0;t[4]=1;t[5]=2;m=6;}
    for(j=0;j<4;j++){
     unsigned char *v=vertices+(nv++)*12;putfloat(v,x+points[j][0]);putfloat(v+4,y+points[j][1]);putfloat(v+8,points[j][2]);
    }
    for(j=0;j<(int)m;j++)put16(indexes+(ni++)*2,t[j]);
   }
  }
  if(!curve){put16(r+4,n);put16(r+6,m);put32(r+8,first);put32(r+12,start);}
  f.vertex_count[i]=n;f.index_count[i]=m;
 }
 f.vertex_at=lump_bytes(&f,25,vertices,nv*12);f.index_at=lump_bytes(&f,26,indexes,ni*2);
 f.surface_at=lump_bytes(&f,24,records,(unsigned)count*16);lump_bytes(&f,23,refs,(unsigned)count*4);
 for(i=0;i<2;i++){put32(leaves+i*36+8,0);put32(leaves+i*36+12,(unsigned)count);}
 lump_bytes(&f,21,leaves,sizeof(leaves));put32(node_bytes+4,UINT32_MAX);put32(node_bytes+8,UINT32_MAX-1);
 lump_bytes(&f,20,node_bytes,sizeof(node_bytes));
 for(i=0;i<3;i++){putfloat(model_bytes+i*4,-1000);putfloat(model_bytes+12+i*4,1000);}
 put32(model_bytes+24+3*4,(unsigned)count);lump_bytes(&f,27,model_bytes,sizeof(model_bytes));
 free(vertices);free(indexes);return f;
}

typedef struct {
 unsigned count,lanes,lane;int partial;v59_job_fn fn;void *context;
 pthread_barrier_t *barrier;fenv_t fp;
} dispatch_lane;
static unsigned dispatch_calls,max_batch;
static atomic_uint worker_callbacks,active_workers,peak_workers;
static void *run_lane(void *opaque){
 dispatch_lane *l=opaque;unsigned i,active,peak;
 fesetenv(&l->fp);worker_thread=1;
 active=atomic_fetch_add(&active_workers,1)+1;peak=atomic_load(&peak_workers);
 while(peak<active&&!atomic_compare_exchange_weak(&peak_workers,&peak,active)){}
 pthread_barrier_wait(l->barrier);
 for(i=l->lane;i<l->count;i+=l->lanes){
  unsigned index=l->count-1-i; /* Completion deliberately opposes source order. */
  if(l->partial&&!(index&1))continue;
  assert(l->fn(l->context,index,index+1));atomic_fetch_add(&worker_callbacks,1);
 }
 atomic_fetch_sub(&active_workers,1);worker_thread=0;return NULL;
}
static int dispatch_impl(unsigned count,unsigned grain,v59_job_fn fn,void *context,int partial){
 pthread_t threads[4];dispatch_lane lanes[4];pthread_barrier_t barrier;
 unsigned n=count<4?count:4,i;assert(count>1&&count<=64&&grain==1);
 dispatch_calls++;if(count>max_batch)max_batch=count;assert(!atomic_load(&active_workers));
 assert(!pthread_barrier_init(&barrier,NULL,n));
 for(i=0;i<n;i++){
  lanes[i]=(dispatch_lane){count,n,i,partial,fn,context,&barrier,{0}};fegetenv(&lanes[i].fp);
  assert(!pthread_create(&threads[i],NULL,run_lane,lanes+i));
 }
 for(i=0;i<n;i++)assert(!pthread_join(threads[i],NULL));
 assert(!pthread_barrier_destroy(&barrier));return partial?0:1;
}
static int threaded(unsigned n,unsigned g,v59_job_fn fn,void *c){return dispatch_impl(n,g,fn,c,0);}
static int partial(unsigned n,unsigned g,v59_job_fn fn,void *c){return dispatch_impl(n,g,fn,c,1);}
static int declined(unsigned n,unsigned g,v59_job_fn fn,void *c){(void)n;(void)g;(void)fn;(void)c;dispatch_calls++;return 0;}
/* Linker wrapping checks the real legacy curve builder never runs on workers
 * and never overlaps mesh jobs; the actual curve implementation still runs. */
int __real_v59_curve_build(int,int,int,const float*,curve*,float*,char*,size_t);
int __wrap_v59_curve_build(int w,int h,int t,const float *p,curve *c,float *b,char *e,size_t n){
 assert(!worker_thread&&!atomic_load(&active_workers));
 return __real_v59_curve_build(w,h,t,p,c,b,e,n);
}
static void equal_bytes(const void *a,const void *b,size_t n){assert(!n||!memcmp(a,b,n));}
static int allocation_count(const v59_world *w){const allocation *a;int n=0;for(a=w->allocs;a;a=a->next)n++;return n;}
static void equal_worlds(const v59_world *a,const v59_world *b){
 int i;assert(a&&b);
 assert(a->nm==b->nm&&a->nb==b->nb&&a->ns==b->ns&&a->np==b->np&&a->nn==b->nn&&a->nl==b->nl);
 assert(a->nbr==b->nbr&&a->nsr==b->nsr&&a->nmodels==b->nmodels);
 assert(a->maxedges==b->maxedges&&a->maxvertices==b->maxvertices&&a->maxdepth==b->maxdepth);
 assert(allocation_count(a)==allocation_count(b));
 equal_bytes(a->materials,b->materials,a->nm*sizeof(*a->materials));
 equal_bytes(a->planes,b->planes,a->np*sizeof(*a->planes));equal_bytes(a->nodes,b->nodes,a->nn*sizeof(*a->nodes));
 equal_bytes(a->leaves,b->leaves,a->nl*sizeof(*a->leaves));equal_bytes(a->models,b->models,a->nmodels*sizeof(*a->models));
 equal_bytes(a->surfacerefs,b->surfacerefs,a->nsr*sizeof(*a->surfacerefs));
 for(i=0;i<a->ns;i++){
  const surface *s=a->surfaces+i,*t=b->surfaces+i;
  assert(s->type==t->type&&s->material==t->material&&s->contents==t->contents);
  equal_bytes(s->bounds,t->bounds,sizeof(s->bounds));
  assert(s->m.nt==t->m.nt&&s->m.ne==t->m.ne&&s->m.nv==t->m.nv&&s->m.down==t->m.down);
  equal_bytes(s->m.tri,t->m.tri,s->m.nt*sizeof(*s->m.tri));equal_bytes(s->m.edges,t->m.edges,s->m.ne*sizeof(*s->m.edges));
  equal_bytes(s->m.vertices,t->m.vertices,s->m.nv*sizeof(*s->m.vertices));
  assert(s->c.np==t->c.np&&s->c.nf==t->c.nf);
  equal_bytes(s->c.planes,t->c.planes,s->c.np*sizeof(*s->c.planes));
  equal_bytes(s->c.facets,t->c.facets,s->c.nf*sizeof(*s->c.facets));
 }
}
static void equal_queries(const v59_world *a,const v59_world *b){
 v59_context *x=v59_context_create(a),*y=v59_context_create(b);int i;assert(x&&y);
 for(i=0;i<40;i++){
  float start[3]={(i%16)*20.0f+1,(i/16)*20.0f+1,32},end[3]={start[0],start[1],-32};
  const float mins[3]={-.25f,-.25f,-.5f},maxs[3]={.25f,.25f,.5f};
  assert(v59_query(x,start,end,mins,maxs,1,0,1)==v59_query(y,start,end,mins,maxs,1,0,1));
  equal_bytes(v59_prepared(x),v59_prepared(y),280);equal_bytes(v59_result(x),v59_result(y),280);
  assert(v59_event_count(x)==v59_event_count(y)&&v59_leaf_count(x)==v59_leaf_count(y));
  equal_bytes(v59_events(x),v59_events(y),v59_event_count(x)*2*sizeof(int));
  equal_bytes(v59_leaves(x),v59_leaves(y),v59_leaf_count(x)*sizeof(int));
 }
 v59_context_free(x);v59_context_free(y);
}
static void check_valid(int count,int curves){
 fixture f=make_fixture(count,curves);char error[256];v59_world *reference;
 v59_parallel_fn modes[4]={NULL,threaded,partial,declined};int m;
 reference=v59_load(f.bytes,f.size,error,sizeof(error));if(!reference)fprintf(stderr,"fixture: %s\n",error);assert(reference);
 for(m=0;m<4;m++){
  v59_world *w=v59_load_parallel(f.bytes,f.size,error,sizeof(error),modes[m]);
  assert(w&&!error[0]);equal_worlds(reference,w);equal_queries(reference,w);v59_free(w);
 }
 v59_free(reference);free(f.bytes);
}
static void check_bad(int kind){
 fixture f=make_fixture(141,1);unsigned char *record=f.bytes+f.surface_at+7*16;
 unsigned char *indexes=f.bytes+f.index_at+f.index_first[7]*2;
 unsigned char *vertices=f.bytes+f.vertex_at+f.vertex_first[7]*12;
 char expected[256],actual[256];v59_parallel_fn modes[3]={threaded,partial,declined};int m,j;
 /* A later malformed descriptor must not hide an earlier geometry failure. */
 put16(f.bytes+f.surface_at+10*16,50);
 switch(kind){
 case 0:put16(record+6,1);break;
 case 1:put16(indexes,50);break;
 case 2:put16(indexes+2,0);break;
 case 3:for(j=0;j<3;j++){putfloat(vertices+j*12,(float)j);putfloat(vertices+j*12+4,0);putfloat(vertices+j*12+8,0);}break;
 case 4:put32(record+8,UINT32_MAX);break;
 case 5:put16(record,50);break;
 case 6: /* Finite source numbers whose triangle cross-product overflows. */
  for(j=0;j<4;j++){putfloat(vertices+j*12,(j==1||j==2)?1e30f:0);putfloat(vertices+j*12+4,j>1?1e30f:0);putfloat(vertices+j*12+8,0);}break;
 case 7:put16(f.bytes+f.surface_at+10*16,0);put32(f.bytes+f.surface_at+63*16+8,0);put16(f.bytes+f.surface_at+66*16,50);break;
 case 8:record[2]=9;break;
 default:assert(0);
 }
 assert(!v59_load(f.bytes,f.size,expected,sizeof(expected))&&expected[0]);
 for(m=0;m<3;m++){
  assert(!v59_load_parallel(f.bytes,f.size,actual,sizeof(actual),modes[m]));
  if(strcmp(expected,actual))fprintf(stderr,"bad %d: %s != %s\n",kind,expected,actual);
  assert(!strcmp(expected,actual));
 }
 free(f.bytes);
}
static void check_worker_allocation_failure(void){
 fixture f=make_fixture(72,0);long before=atomic_load(&live_allocations);int i;char error[256];
 for(i=0;i<13;i++){
  atomic_store(&worker_fail_after,i);
  assert(!v59_load_parallel(f.bytes,f.size,error,sizeof(error),threaded));
  assert(!strcmp(error,"allocation failed"));assert(atomic_load(&worker_fail_after)==-1);
  assert(atomic_load(&live_allocations)==before);
 }
 free(f.bytes);
}
int main(void){
 int i;check_valid(0,0);check_valid(1,0);check_valid(2,0);check_valid(150,0);check_valid(141,1);
 for(i=0;i<9;i++)check_bad(i);
 fesetround(FE_DOWNWARD);check_valid(72,0);fesetround(FE_UPWARD);check_valid(72,0);fesetround(FE_TONEAREST);
 check_worker_allocation_failure();assert(!atomic_load(&live_allocations));
 assert(dispatch_calls>0&&max_batch==64&&atomic_load(&worker_callbacks)>0&&atomic_load(&peak_workers)>=2);
 printf("IBSP59 loader: serial/concurrent/partial/declined dispatch parity; curve boundaries; ordered malformed failures; query bytes; rounding modes; worker OOM cleanup and zero live allocations PASS (%u dispatches, %u worker callbacks, peak %u workers)\n",dispatch_calls,atomic_load(&worker_callbacks),atomic_load(&peak_workers));
 return 0;
}
