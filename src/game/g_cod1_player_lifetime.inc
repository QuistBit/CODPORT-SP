/* Sole production owner: g_client.c. No public begin/complete API, provider
 * registration, cvar success lane, save struct or VM builtin is added. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include <stdint.h>
#include <limits.h>
#include <string.h>
typedef struct {
 cod1PlayerLifetime_t receipt;
 GscEntityRef born;
 gclient_t*client;
 uint64_t pending,epoch,activity;
 int expect_birth,in_birth,birth_done,bound,spawn_entered,spawn_returned;
 int expected_spawn;
 uint32_t previous_generation;
} C1PlayerLife;
static GscEntityRef cod1BeginEvent;
static cod1PlayerLifetime_t cod1BeginEventPlayer;
static C1PlayerLife cod1PlayerLife[MAX_CLIENTS];
static uint64_t cod1PlayerLifeTransaction,cod1PlayerLifeCompletion,cod1PlayerLifeActivity;
static int cod1PlayerLifeExhausted;
static int Cod1PlayerLifeOverlap(const void*a,size_t n,const void*b,size_t m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n&&m&&(x<=y?y-x<n:x-y<m);
}
int G_Cod1PlayerLifetimePrivate(const void*p,size_t n){
 return Cod1PlayerLifeOverlap(p,n,&cod1BeginEvent,sizeof(cod1BeginEvent))||
  Cod1PlayerLifeOverlap(p,n,&cod1BeginEventPlayer,sizeof(cod1BeginEventPlayer))||
  Cod1PlayerLifeOverlap(p,n,cod1PlayerLife,sizeof(cod1PlayerLife))||
  Cod1PlayerLifeOverlap(p,n,&cod1PlayerLifeTransaction,sizeof(cod1PlayerLifeTransaction))||
  Cod1PlayerLifeOverlap(p,n,&cod1PlayerLifeCompletion,sizeof(cod1PlayerLifeCompletion))||
  Cod1PlayerLifeOverlap(p,n,&cod1PlayerLifeActivity,sizeof(cod1PlayerLifeActivity))||
  Cod1PlayerLifeOverlap(p,n,&cod1PlayerLifeExhausted,sizeof(cod1PlayerLifeExhausted));
}
static int Cod1PlayerLifeSlot(const gentity_t*e){
 uintptr_t p=(uintptr_t)e,b=(uintptr_t)g_entities;size_t off;
 if(!e||p<b||(off=(size_t)(p-b))>=sizeof(gentity_t)*MAX_CLIENTS||off%sizeof(gentity_t))return -1;
 return (int)(off/sizeof(gentity_t));
}
static uint64_t Cod1PlayerLifeNext(uint64_t*counter){
 if(cod1PlayerLifeExhausted)return 0;
 if(*counter==UINT64_MAX){cod1PlayerLifeExhausted=1;memset(cod1PlayerLife,0,sizeof(cod1PlayerLife));return 0;}
 return ++*counter;
}
static int Cod1PlayerLifeRefEqual(const GscEntityRef*a,const GscEntityRef*b){
 return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int Cod1PlayerLifeDomain(int slot){
 return !cod1PlayerLifeExhausted&&slot>=0&&slot<MAX_CLIENTS&&level.clients==g_clients&&
  level.maxclients>0&&level.maxclients<=MAX_CLIENTS&&slot<level.maxclients&&
  G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&G_Cod1EntityMapEpoch()&&
  !level.spawning&&!level.restarted;
}
void G_Cod1PlayerLifetimeRetire(gentity_t*e){
 int slot=Cod1PlayerLifeSlot(e);if(slot>=0)memset(cod1PlayerLife+slot,0,sizeof(cod1PlayerLife[slot]));
}
void G_Cod1PlayerLifetimeMapReset(void){
 /* Counter history/exhaustion are GAME-module-local and NEVER reset here.
  * All receipt consumers are private to that same module incarnation. */
 memset(cod1PlayerLife,0,sizeof(cod1PlayerLife));
}
void G_Cod1PlayerLifetimeActivity(gentity_t*e){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;uint64_t serial;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!r->pending&&!r->receipt.completion)return;
 serial=Cod1PlayerLifeNext(&cod1PlayerLifeActivity);if(!serial)return;
 r->activity=serial;
}
/* An expected real G_InitGentity birth may retire the previous allocation
 * without erasing its private ClientBegin transaction. A second birth, any
 * ordinary retirement or a nested begin cancels that transaction forever. */
void G_Cod1PlayerLifetimeBeforeBirth(gentity_t*e,const GscEntityRef*previous){
 int slot=Cod1PlayerLifeSlot(e),captured;C1PlayerLife*r;GscEntityRef current;
 if(slot<0)return;r=cod1PlayerLife+slot;
 captured=G_Cod1EntityCapture(e,&current);
 if(!Cod1PlayerLifeDomain(slot)||!r->pending||!r->expect_birth||r->in_birth||r->birth_done||
    r->epoch!=G_Cod1EntityMapEpoch()||!previous||previous->epoch!=r->epoch||previous->slot!=(uint32_t)slot||
    previous->generation==UINT32_MAX||(captured?!Cod1PlayerLifeRefEqual(previous,&current):previous->generation!=0)){
  G_Cod1PlayerLifetimeRetire(e);return;
 }
 r->previous_generation=previous->generation;
 r->expect_birth=0;r->in_birth=1;G_Cod1PlayerLifetimeActivity(e);
}
void G_Cod1PlayerLifetimeAfterBirth(gentity_t*e){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;GscEntityRef ref;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!r->pending||!r->in_birth)return;
 if(!Cod1PlayerLifeDomain(slot)||r->epoch!=G_Cod1EntityMapEpoch()||!G_Cod1EntityCapture(e,&ref)||
    ref.epoch!=r->epoch||ref.slot!=(uint32_t)slot||ref.generation!=r->previous_generation+1u){G_Cod1PlayerLifetimeRetire(e);return;}
 r->born=ref;r->in_birth=0;r->birth_done=1;
}
static uint64_t Cod1PlayerLifeBegin(gentity_t*e){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;uint64_t token,activity;
 G_Cod1PlayerLifetimeRetire(e);
 if(!Cod1PlayerLifeDomain(slot))return 0;
 token=Cod1PlayerLifeNext(&cod1PlayerLifeTransaction);activity=Cod1PlayerLifeNext(&cod1PlayerLifeActivity);
 if(!token||!activity)return 0;r=cod1PlayerLife+slot;
 r->pending=token;r->epoch=G_Cod1EntityMapEpoch();r->client=level.clients+slot;r->activity=activity;
 return token;
}
static void Cod1PlayerLifeExpectBirth(gentity_t*e,uint64_t token){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!token||!Cod1PlayerLifeDomain(slot)||r->pending!=token||r->epoch!=G_Cod1EntityMapEpoch()||
    r->expect_birth||r->in_birth||r->birth_done){G_Cod1PlayerLifetimeRetire(e);return;}
 r->expect_birth=1;
}
static int Cod1PlayerLifeBound(gentity_t*e,C1PlayerLife*r,int slot){
 return Cod1PlayerLifeDomain(slot)&&r->pending&&r->bound&&r->birth_done&&!r->in_birth&&
  r->epoch==G_Cod1EntityMapEpoch()&&r->client==level.clients+slot&&e->client==r->client&&
  G_Cod1EntitySameAllocation(&r->born);
}
static void Cod1PlayerLifeBind(gentity_t*e,uint64_t token){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!token||!Cod1PlayerLifeDomain(slot)||r->pending!=token||!r->birth_done||r->in_birth||r->expect_birth||
    r->epoch!=G_Cod1EntityMapEpoch()||r->client!=level.clients+slot||e->client!=r->client||
    !G_Cod1EntitySameAllocation(&r->born)){G_Cod1PlayerLifetimeRetire(e);return;}
 r->bound=1;
}
static uint64_t Cod1PlayerLifeSpawnEntered(gentity_t*e){
 int slot=Cod1PlayerLifeSlot(e),count;C1PlayerLife*r;uint64_t token;
 if(slot<0)return 0;r=cod1PlayerLife+slot;
 if(!Cod1PlayerLifeBound(e,r,slot)||r->spawn_entered||r->spawn_returned){G_Cod1PlayerLifetimeRetire(e);return 0;}
 count=e->client->ps.persistant[PERS_SPAWN_COUNT];
 if(count<0||count==INT_MAX){G_Cod1PlayerLifetimeRetire(e);return 0;}
 token=r->pending;r->expected_spawn=count+1;r->spawn_entered=1;G_Cod1PlayerLifetimeActivity(e);
 return cod1PlayerLifeExhausted?0:token;
}
static void Cod1PlayerLifeSpawnReturned(gentity_t*e,uint64_t token){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!token||!Cod1PlayerLifeBound(e,r,slot)||r->pending!=token||!r->spawn_entered||r->spawn_returned||
    e->client->ps.persistant[PERS_SPAWN_COUNT]!=r->expected_spawn){G_Cod1PlayerLifetimeRetire(e);return;}
 r->spawn_returned=1;G_Cod1PlayerLifetimeActivity(e);
}
static int Cod1PlayerLifeSelection(const gentity_t*e,int slot){
 const gclient_t*c=e->client;
 return c==level.clients+slot&&c->pers.connected==CON_CONNECTED&&
  !(e->r.svFlags&(SVF_CASTAI|SVF_BOT))&&!e->aiCharacter&&!e->aiAttributes&&!e->AIScript_AlertEntity&&
  !e->aiInactive&&!c->ps.aiChar&&(!e->aiName||!strcmp(e->aiName,"player"));
}
static int Cod1PlayerLifeNormal(const gentity_t*e,int slot){
 return Cod1PlayerLifeSelection(e,slot)&&e->client->sess.sessionTeam!=TEAM_SPECTATOR&&
  e->client->ps.pm_type==PM_NORMAL&&e->health>0&&e->r.linked&&e->s.eType==ET_PLAYER;
}
static void Cod1PlayerLifeComplete(gentity_t*e,uint64_t token){
 int slot=Cod1PlayerLifeSlot(e);C1PlayerLife*r;uint64_t completed,activity;cod1PlayerLifetime_t result;
 if(slot<0)return;r=cod1PlayerLife+slot;
 if(!token||!Cod1PlayerLifeBound(e,r,slot)||r->pending!=token||!r->spawn_entered||!r->spawn_returned||
    !Cod1PlayerLifeNormal(e,slot)||e->client->ps.persistant[PERS_SPAWN_COUNT]!=r->expected_spawn){G_Cod1PlayerLifetimeRetire(e);return;}
 completed=Cod1PlayerLifeNext(&cod1PlayerLifeCompletion);activity=Cod1PlayerLifeNext(&cod1PlayerLifeActivity);
 if(!completed||!activity)return;
 memset(&result,0,sizeof(result));result.entity=r->born;result.completion=completed;result.activity=activity;
 result.spawn_count=r->expected_spawn;result.completed_time=level.time;
 r->pending=0;r->activity=activity;r->receipt=result;
}
static C1PlayerLife*Cod1PlayerLifeCompleted(const GscEntityRef*ref){
 C1PlayerLife*r;gentity_t*e;int slot;
 if(!ref||ref->slot>=MAX_CLIENTS)return NULL;slot=(int)ref->slot;r=cod1PlayerLife+slot;
 if(!r->receipt.completion||!Cod1PlayerLifeRefEqual(ref,&r->receipt.entity))return NULL;
 e=g_entities+slot;
 if(!Cod1PlayerLifeDomain(slot)||r->pending||!G_Cod1EntitySameAllocation(ref)||r->client!=level.clients+slot||
    e->client!=r->client||!Cod1PlayerLifeSelection(e,slot)||e->client->ps.persistant[PERS_SPAWN_COUNT]!=r->receipt.spawn_count){
  /* An observed broken lifetime cannot revive after bytes are restored. */
  G_Cod1PlayerLifetimeRetire(e);return NULL;
 }
 return r;
}
int G_Cod1PlayerLifetimeRead(const GscEntityRef*input,cod1PlayerLifetime_t*out){
 GscEntityRef ref;C1PlayerLife*r;
 if(!input||!out||G_Cod1EntityStorageProtected(out,sizeof(*out))||G_Cod1PlayerLifetimePrivate(out,sizeof(*out))||
    Cod1PlayerLifeOverlap(out,sizeof(*out),g_entities,sizeof(gentity_t)*MAX_GENTITIES)||
    Cod1PlayerLifeOverlap(out,sizeof(*out),g_clients,sizeof(gclient_t)*MAX_CLIENTS)||
    Cod1PlayerLifeOverlap(out,sizeof(*out),&level,sizeof(level)))return 0;
 ref=*input;r=Cod1PlayerLifeCompleted(&ref);if(!r)return 0;*out=r->receipt;return 1;
}
int G_Cod1PlayerLifetimeSameCompleted(const cod1PlayerLifetime_t*input){
 cod1PlayerLifetime_t q;C1PlayerLife*r;
 if(!input)return 0;q=*input;r=Cod1PlayerLifeCompleted(&q.entity);
 return r&&q.completion==r->receipt.completion&&q.activity==r->receipt.activity&&
  q.spawn_count==r->receipt.spawn_count&&q.completed_time==r->receipt.completed_time;
}
static void Cod1BeginEventComplete(gentity_t *player,gentity_t *event){
 GscEntityRef ref;cod1PlayerLifetime_t current;
 if(player!=g_entities)return;
 memset(&cod1BeginEvent,0,sizeof(cod1BeginEvent));
 if(!event||!G_Cod1EntityCapture(player,&ref)||!G_Cod1PlayerLifetimeRead(&ref,&current)||
    !G_Cod1EntityCapture(event,&ref)||event->s.eType!=ET_EVENTS+EV_PLAYER_TELEPORT_IN||
    event->eventTime!=level.time||event->s.clientNum!=0)return;
 cod1BeginEvent=ref;cod1BeginEventPlayer=current;
}
/* RTCW allocates a transient entity solely to deliver the entry event.
 * Only this exact allocation and its completed client may be omitted
 * from the original startup node scan. Other temp events are unrelated. */
int G_Cod1StartupPlayerEvent(const GscEntityRef *ref){
 cod1PlayerLifetime_t current;gentity_t *e;
 if(!ref||ref->epoch!=cod1BeginEvent.epoch||ref->slot!=cod1BeginEvent.slot||
    ref->generation!=cod1BeginEvent.generation||!G_Cod1EntitySameAllocation(ref)||
    !G_Cod1StartupPlayerPath(&cod1BeginEventPlayer.entity,&current)||
    current.completion!=cod1BeginEventPlayer.completion)return 0;
 e=g_entities+ref->slot;
 return e->s.eType==ET_EVENTS+EV_PLAYER_TELEPORT_IN&&e->s.clientNum==0&&e->freeAfterEvent&&
  e->eventTime==current.completed_time&&e->r.eventTime==e->eventTime&&!e->r.contents&&!e->client;
}
int G_Cod1PlayerLifetimeUnadvanced(const cod1PlayerLifetime_t*input){
 cod1PlayerLifetime_t q;
 if(!input)return 0;q=*input;
 return G_Cod1PlayerLifetimeSameCompleted(&q)&&cod1PlayerLife[q.entity.slot].activity==q.activity;
}
#endif
