/* Compare production shell headers with synthetic snapshot types; no GAME VM. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint64_t words[513];} cod1ActorRecord_t;
typedef struct {uint64_t words[129];} gentity_t;
typedef struct {cod1ActorRecord_t *actor;gentity_t *entity;uint64_t fields[71];} C1FaceFrameCall;
static unsigned allocs,frees,live,failNext;
static void *trackedMalloc(size_t n){void *p;++allocs;if(failNext){failNext=0;return NULL;}p=malloc(n);assert(p);++live;memset(p,0xab,n);return p;}
static void trackedFree(void*p){if(p){assert(live);--live;++frees;free(p);}}
static int ActorOverlap(const void*p,size_t n,const void*q,size_t m){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
 if(!n||!m)return 0;
 if(!p||!q||n>UINTPTR_MAX-a||m>UINTPTR_MAX-b)return 1;
 return a<b+m&&b<a+n;
}
#define malloc trackedMalloc
#define free trackedFree
#define C1FaceFrameAllocation ReferenceAllocation
#define ActorFaceFrameAllocate ReferenceAllocate
#define ActorFaceFrameFree ReferenceFree
#include "g_cod1_face_frame_storage35.h"
#undef C1FaceFrameAllocation
#undef ActorFaceFrameAllocate
#undef ActorFaceFrameFree
#include "g_cod1_face_frame_workspace45.h"
#undef malloc
#undef free
static void checkHeader(C1FaceFrameCall*c){C1FaceFrameCall expected;memset(&expected,0,sizeof(expected));expected.actor=c->actor;expected.entity=c->entity;assert(c->actor&&c->entity&&!memcmp(c,&expected,sizeof(expected)));}
static void compare(C1FaceFrameCall*a,C1FaceFrameCall*b,unsigned seed){
 unsigned i;checkHeader(a);checkHeader(b);
 for(i=0;i<513;i++)a->actor->words[i]=b->actor->words[i]=seed+i;
 for(i=0;i<129;i++)a->entity->words[i]=b->entity->words[i]=seed^i;
 for(i=0;i<71;i++)a->fields[i]=b->fields[i]=seed*71u+i;
 assert(!memcmp(a->actor,b->actor,sizeof(*a->actor))&&!memcmp(a->entity,b->entity,sizeof(*a->entity))&&!memcmp(a->fields,b->fields,sizeof(a->fields)));
}
int main(void){
 C1FaceFrameCall *r,*a,*b,*c;unsigned i,before,base;uint64_t outside=0;
 assert(!ActorFaceFrameWorkspacePrivate(&outside,sizeof(outside)));
 failNext=1;assert(!ActorFaceFrameAllocate()&&!live&&!actorFaceIdle45&&!actorFaceActive45);
 before=allocs;
 for(i=0;i<1000;i++){r=ReferenceAllocate();assert(r);compare(r,r,i);ReferenceFree(r);}
 assert(allocs-before==1000&&!live);
 before=allocs;
 for(i=0;i<1000;i++){a=ActorFaceFrameAllocate();assert(a);checkHeader(a);memset(a->actor,(int)(i&255),sizeof(*a->actor));memset(a->entity,(int)(i&255),sizeof(*a->entity));a->fields[70]=i+1;ActorFaceFrameFree(a);assert(live==1&&!actorFaceActive45&&actorFaceIdle45);}
 assert(allocs-before==1);
 /* Compare defined snapshots and zero header across repeated dirty reuse. */
 for(i=0;i<1000;i++){r=ReferenceAllocate();a=ActorFaceFrameAllocate();assert(r&&a);compare(r,a,i);ReferenceFree(r);ActorFaceFrameFree(a);}
 assert(live==1&&actorFaceIdle45);
 assert(ActorFaceFrameWorkspacePrivate(actorFaceIdle45,sizeof(*actorFaceIdle45)));
 assert(ActorFaceFrameWorkspacePrivate(&actorFaceIdle45,sizeof(actorFaceIdle45)));
 assert(ActorFaceFrameWorkspacePrivate(&actorFaceActive45,sizeof(actorFaceActive45)));
 assert(ActorFaceFrameWorkspacePrivate((char*)actorFaceIdle45+sizeof(*actorFaceIdle45)-1,1));
 assert(!ActorFaceFrameWorkspacePrivate(&outside,sizeof(outside)));
 before=allocs;failNext=1;a=ActorFaceFrameAllocate();assert(a&&allocs==before);checkHeader(a);
 /* Active shell is already private before the published operation exists. */
 assert(ActorFaceFrameWorkspacePrivate(a,sizeof(*a))&&ActorFaceFrameWorkspacePrivate(a->actor,sizeof(*a->actor))&&ActorFaceFrameWorkspacePrivate(a->entity,sizeof(*a->entity)));
 assert(!ActorFaceFrameAllocate()&&live==1);assert(actorFaceActive45&& &actorFaceActive45->call==a);
 b=ActorFaceFrameAllocate();c=ActorFaceFrameAllocate();assert(b&&c&&a!=b&&b!=c&&a!=c&&live==3);
 assert(a->actor!=b->actor&&b->actor!=c->actor&&a->entity!=c->entity);
 a->actor->words[0]=17;b->actor->words[0]=22;c->actor->words[0]=33;
 a->entity->words[128]=41;b->entity->words[128]=42;c->entity->words[128]=43;
 ActorFaceFrameFree(b);assert(actorFaceIdle45&&live==3); /* middle unlink */
 ActorFaceFrameWorkspaceClear();assert(!actorFaceIdle45&&live==2);
 assert(a->actor->words[0]==17&&c->actor->words[0]==33&&a->entity->words[128]==41&&c->entity->words[128]==43);
 assert(ActorFaceFrameWorkspacePrivate(a->actor,sizeof(*a->actor))&&ActorFaceFrameWorkspacePrivate(c->entity,sizeof(*c->entity)));
 ActorFaceFrameWorkspaceClear();assert(live==2);
 ActorFaceFrameFree(a);assert(live==2&&actorFaceIdle45);ActorFaceFrameFree(c);assert(live==1&&!actorFaceActive45);
 base=allocs;a=ActorFaceFrameAllocate();assert(a&&allocs==base);checkHeader(a);ActorFaceFrameFree(a);
 ActorFaceFrameWorkspaceClear();ActorFaceFrameWorkspaceClear();assert(!live&&!actorFaceIdle45&&!actorFaceActive45);
 assert(allocs==frees+2); /* two injected allocation failures */
 puts("R45B shell: 1000 baseline mallocs vs 1 cached; 1000 snapshot comparisons; nested admission, alias ranges, reset, failures: PASS");return 0;
}
