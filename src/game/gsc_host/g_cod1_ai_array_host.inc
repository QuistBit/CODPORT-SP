/* R202 actual actor-pool query plus constructor-route coverage. Included by
 * g_spawn.c after the real typed spawner registry. Retail getaiarray at
 * 0x20030b90 uses the32 actor slots in pool order and the sentient team mask;
 * occupied does not imply alive/initialized. Coverage is a separate port
 * authority: skipped BSP actor constructors may never become an empty world.
 * This revision records those misses; it does not create/publish actor AI. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "../g_cod1_actor.h"
#include <stdint.h>
#include <string.h>

typedef struct {
    GscEntityRef entity;
    uint64_t intake;
    uint32_t ordinal;
} Cod1AiDomainSpawner;
static cod1AiDomainSnapshot_t cod1AiDomain;
static Cod1AiDomainSpawner cod1AiDomainSpawners[MAX_GENTITIES];
static uint64_t cod1AiDomainSerial;
static uint32_t cod1AiDomainAccounted;
static int (*cod1AiCoverage)(uint64_t);
static int cod1AiArrayBusy,cod1AiArrayCancelled;

static int Cod1AiOverlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int Cod1AiPrivate(const void *p,size_t n) {
    return Cod1AiOverlap(p,n,&cod1AiDomain,sizeof(cod1AiDomain))||
        Cod1AiOverlap(p,n,cod1AiDomainSpawners,sizeof(cod1AiDomainSpawners))||
        Cod1AiOverlap(p,n,&cod1AiDomainSerial,sizeof(cod1AiDomainSerial))||
        Cod1AiOverlap(p,n,&cod1AiArrayBusy,sizeof(cod1AiArrayBusy))||
        Cod1AiOverlap(p,n,&cod1AiArrayCancelled,sizeof(cod1AiArrayCancelled))||
        Cod1AiOverlap(p,n,g_entities,sizeof(g_entities))||
        Cod1AiOverlap(p,n,&level,sizeof(level));
}
static int Cod1AiError(char *error,size_t size,const char *text) {
    if(error&&size){size_t n=strlen(text);if(n>=size)n=size-1;memcpy(error,text,n);error[n]=0;}return 0;
}
static int Cod1AiRefEqual(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int Cod1AiOwner(void) {
    return cod1AiDomain.epoch&&cod1AiDomain.serial&&
        cod1AiDomain.epoch==G_Cod1EntityMapEpoch()&&G_Cod1MapEntities()&&
        g_gametype.integer==GT_SINGLE_PLAYER;
}
/* No public authority setter exists. These hooks are called only by the
 * actual BSP parser/actor_ dispatch; Finish cannot erase a failed route. */
static void Cod1AiDomainBegin(void) {
    if(cod1AiArrayBusy)cod1AiArrayCancelled=1;
    memset(&cod1AiDomain,0,sizeof(cod1AiDomain));
    memset(cod1AiDomainSpawners,0,sizeof(cod1AiDomainSpawners));
    cod1AiDomainAccounted=0;cod1AiCoverage=NULL;
    if(cod1AiDomainSerial==UINT64_MAX)return;
    cod1AiDomain.serial=++cod1AiDomainSerial;
    cod1AiDomain.epoch=G_Cod1EntityMapEpoch();
    if(!Cod1AiOwner())cod1AiDomain.invalid=1;
}
static void Cod1AiDomainObserve(const gentity_t *ent,int spawnerAccepted) {
    GscEntityRef ref;uint64_t intake;uint32_t i;
    if(cod1AiArrayBusy)cod1AiArrayCancelled=1;
    if(!Cod1AiOwner()||cod1AiDomain.sealed||!level.spawning||
       !ent||!ent->classname||!memchr(ent->classname,0,128)||
       strncmp(ent->classname,"actor_",6)||cod1AiDomain.constructorRows==UINT32_MAX) {
        cod1AiDomain.invalid=1;return;
    }
    ++cod1AiDomain.constructorRows;
    if(!spawnerAccepted||!(ent->spawnflags&1)||
       !G_Cod1SpawnerCapture(ent,&ref)||!(intake=G_Cod1SpawnerIntakeIdentity(&ref))) {
        ++cod1AiDomain.unaccountedRows;return;
    }
    if(cod1AiDomain.spawnerRows>=MAX_GENTITIES){cod1AiDomain.invalid=1;return;}
    for(i=0;i<cod1AiDomain.spawnerRows;++i)
        if(Cod1AiRefEqual(&ref,&cod1AiDomainSpawners[i].entity)) {
            cod1AiDomain.invalid=1;return;
        }
    cod1AiDomainSpawners[cod1AiDomain.spawnerRows].entity=ref;
    cod1AiDomainSpawners[cod1AiDomain.spawnerRows].intake=intake;
    cod1AiDomainSpawners[cod1AiDomain.spawnerRows].ordinal=cod1AiDomain.constructorRows-1;
    ++cod1AiDomain.spawnerRows;
}
static void Cod1AiDomainFinish(void) {
    if(cod1AiArrayBusy)cod1AiArrayCancelled=1;
    if(!Cod1AiOwner()||cod1AiDomain.sealed||level.spawning)
        cod1AiDomain.invalid=1;
    cod1AiDomain.sealed=1;
    cod1AiDomain.complete=Cod1AiOwner()&&!cod1AiDomain.invalid&&
        !cod1AiDomain.unaccountedRows&&
        cod1AiDomain.constructorRows==cod1AiDomain.spawnerRows;
    if(Cod1AiOwner())G_Printf("CoD1 actor domain census: %u constructor routes, %u typed spawners, %u unaccounted actors; complete=%d\n",
        cod1AiDomain.constructorRows,cod1AiDomain.spawnerRows,
        cod1AiDomain.unaccountedRows,cod1AiDomain.complete);
}
int G_Cod1AiDomainRead(cod1AiDomainSnapshot_t *out,char *error,size_t size) {
    if(!out||Cod1AiPrivate(out,sizeof(*out))||Cod1AiPrivate(error,size)||
       Cod1AiOverlap(out,sizeof(*out),error,size))return 0;
    if(!Cod1AiOwner())return Cod1AiError(error,size,"actor census has no current CoD SP map authority");
    *out=cod1AiDomain;return 1;
}
static int Cod1AiDomainCurrent(const cod1AiDomainSnapshot_t *captured,int hostTime) {
    GscEntityRef spawners[MAX_GENTITIES];size_t count=0,i,j;
    if(!Cod1AiOwner()||!cod1AiDomain.sealed||!cod1AiDomain.complete||
       cod1AiDomain.invalid||cod1AiDomain.unaccountedRows||
       memcmp(captured,&cod1AiDomain,sizeof(*captured))||
       level.time!=hostTime||level.spawning||level.restarted||
       !G_Cod1EntityCollectionReady(captured->epoch,level.num_entities)||
       !G_Cod1SpawnerCollectAll(spawners,MAX_GENTITIES,&count,NULL,0)||
       count!=cod1AiDomain.spawnerRows)return 0;
    if(cod1AiDomainAccounted&&(!cod1AiCoverage||cod1AiCoverage(captured->epoch)!=1))return 0;
    for(i=0;i<count;++i) {
        for(j=0;j<count;++j)if(Cod1AiRefEqual(&spawners[i],&cod1AiDomainSpawners[j].entity))break;
        if(j==count||G_Cod1SpawnerIntakeIdentity(&spawners[i])!=cod1AiDomainSpawners[j].intake)return 0;
    }
    return 1;
}
static int Cod1AiLease(const cod1AiDomainSnapshot_t *domain,int hostTime,
    int (*current)(void *),void *lease) {
    return !cod1AiArrayCancelled&&current&&current(lease)==1&&
        !cod1AiArrayCancelled&&Cod1AiDomainCurrent(domain,hostTime);
}
static int Cod1AiCollect(uint32_t mask,cod1ActorReservationView_t views[COD1_ACTOR_CAPACITY],
    size_t *used,GscAiArraySnapshot *snapshot) {
    size_t i;GscAiArraySnapshot copy;memset(&copy,0,sizeof(copy));
    if(!G_Cod1ActorCollectReservations(views,COD1_ACTOR_CAPACITY,used,NULL,0))return 0;
    for(i=0;i<*used;++i) {
        cod1ActorReservationView_t *row=&views[i];GscAiArrayMember *member;
        if(row->lease.kind!=COD1_ACTOR_LEASE_ACTOR||row->lease.actorIndex>=COD1_ACTOR_CAPACITY||
           (i&&views[i-1].lease.actorIndex>=row->lease.actorIndex)||row->team>3||
           !G_Cod1EntitySameAllocation(&row->lease.entity))return 0;
        if(!(mask&(UINT32_C(1)<<row->team)))continue;
        member=&copy.members[copy.count++];member->actor_index=row->lease.actorIndex;
        member->entity=row->lease.entity;
    }
    *snapshot=copy;return 1;
}
static int Cod1AiOperation(const GscAiArrayRequest *input,const GscAiArraySnapshot *expected,
    GscAiArraySnapshot *output,char *error,size_t size,int (*current)(void *),void *lease) {
    GscAiArrayRequest request;GscAiArraySnapshot first,last,expectedCopy;
    cod1ActorReservationView_t views[COD1_ACTOR_CAPACITY],after[COD1_ACTOR_CAPACITY];
    cod1AiDomainSnapshot_t domain;size_t used=0,afterUsed=0;int hostTime;
    if(cod1AiArrayBusy){cod1AiArrayCancelled=1;return 0;}
    if(!input||(!expected&&!output)||!current||
       Cod1AiPrivate(input,sizeof(*input))||Cod1AiPrivate(expected,expected?sizeof(*expected):0)||
       Cod1AiPrivate(output,output?sizeof(*output):0)||Cod1AiPrivate(error,size)||
       Cod1AiOverlap(input,sizeof(*input),error,size)||
       Cod1AiOverlap(expected,expected?sizeof(*expected):0,error,size)||
       Cod1AiOverlap(output,output?sizeof(*output):0,error,size)||
       Cod1AiOverlap(input,sizeof(*input),output,output?sizeof(*output):0))return 0;
    request=*input;if(expected)expectedCopy=*expected;
    /* now_ms is Session-relative logical time, checked by current(). It need
     * not equal map level.time; the latter has its own immutable host fence. */
    if(request.now_ms<0||!request.team_mask||(request.team_mask&~UINT32_C(14)))goto refuse;
    domain=cod1AiDomain;hostTime=level.time;
    cod1AiArrayBusy=1;cod1AiArrayCancelled=0;
    if(!Cod1AiLease(&domain,hostTime,current,lease)||memcmp(input,&request,sizeof(request))||
       (expected&&memcmp(expected,&expectedCopy,sizeof(expectedCopy)))||
       !Cod1AiCollect(request.team_mask,views,&used,&first))goto busyRefuse;
    if(!Cod1AiLease(&domain,hostTime,current,lease)||memcmp(input,&request,sizeof(request))||
       (expected&&memcmp(expected,&expectedCopy,sizeof(expectedCopy)))||
       !Cod1AiCollect(request.team_mask,after,&afterUsed,&last)||used!=afterUsed||
       memcmp(views,after,used*sizeof(*views))||memcmp(&first,&last,sizeof(first))||
       (expected&&memcmp(&first,&expectedCopy,sizeof(first))))goto busyRefuse;
    /* Last lease check has completed. Collection/ref/census checks below are
     * pure; no external callback can mutate the borrowed request or pool before
     * copying the staged owned array. */
    if(!Cod1AiDomainCurrent(&domain,hostTime))goto busyRefuse;
    if(output)*output=first;
    cod1AiArrayBusy=0;return 1;
busyRefuse:
    cod1AiArrayBusy=0;
refuse:
    return Cod1AiError(error,size,"getaiarray requires a complete current actor constructor census and unchanged actor-pool ownership; skipped BSP actors remain unsupported");
}
static int Cod1AiArrayQuery(void *user,const GscAiArrayRequest *request,
    GscAiArraySnapshot *output,char *error,size_t size,int (*current)(void *),void *lease) {
    (void)user;return Cod1AiOperation(request,NULL,output,error,size,current,lease);
}
static int Cod1AiArrayValidate(void *user,const GscAiArrayRequest *request,
    const GscAiArraySnapshot *snapshot,char *error,size_t size,int (*current)(void *),void *lease) {
    (void)user;if(!snapshot)return 0;
    return Cod1AiOperation(request,snapshot,NULL,error,size,current,lease);
}
const GscAiArrayHost *G_Cod1AiArrayHost(void) {
    static const GscAiArrayHost host={NULL,Cod1AiArrayQuery,Cod1AiArrayValidate};return &host;
}
#endif
