#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gsc_session_clock_r50.h"
enum {GSC_SESSION_DISABLED,GSC_SESSION_EMPTY,GSC_SESSION_PREPARED,GSC_SESSION_RUNNING,GSC_SESSION_COMPLETED,GSC_SESSION_FAILED,GSC_SESSION_COMPILED,GT_SINGLE_PLAYER=2};
typedef struct {int state,busy,has_program,has_runtime,active_threads;uint64_t session_id;int logical_ms;char error[32];int native,fog;} GscSessionSnapshot;
typedef struct {int tasks;} GscRuntime;
typedef struct GscSession {GscSessionSnapshot view;int busy;void *program;GscRuntime *runtime,*bsp_runtime;} GscSession;
static unsigned scans,stats;
static int gsc_active_threads(const GscRuntime *r){if(!r)return 0;scans+=(unsigned)r->tasks;return r->tasks;}
static int gsc_runtime_native_stats(const GscRuntime *r,int *out){++stats;*out=r->tasks;return 1;}
static int gsc_runtime_fog_native_stats(const GscRuntime *r,int *out){++stats;*out=r->tasks;return 1;}
#include "snapshot.inc"
#include "clock.inc"
typedef struct {GscSession *session;uint64_t id,operation_id;uint32_t registry_revision,fingerprint;int mode,failed;unsigned char registered[8],complete[8];char published[32];} GscTestItemScene;
typedef struct {GscTestItemScene scene;uint64_t serial;int logical_ms,server_time;} WeaponRegistrationLease;
typedef struct {int active,cancelled;uint64_t serial;} Guard;
typedef struct {uint32_t descriptorFingerprint,registryRevision;} Registry;
static GscTestItemScene gscTestItem;
static GscSession *gscTestSession;
static int gscTestItemInitialization,gscTestItemBusy,gscTestEntityMode,gscTestCompleted,gscTestStopRequested;
static struct {int restarted,time;} level;
static struct {int integer;} g_gametype;
static Guard weaponRegistrationGuard;
static uint64_t weaponRegistrationSerial;
static Registry weaponRegistrationRegistry;
static uint32_t revision;
static int map_current,registry_current,identity_current;
static int G_Cod1MapEntities(void){return map_current;}
static uint32_t CW_RegistryRevision(void){return revision;}
static int CAR_Current(const Registry *r){return registry_current&&r->registryRevision==revision;}
static int GscTestItemIdentityChecked(const GscTestItemScene *lease,int validated,uint32_t fingerprint){return identity_current&&lease&&lease->session&&lease->session==gscTestItem.session&&lease->session==gscTestSession&&lease->id==gscTestItem.id&&lease->fingerprint==gscTestItem.fingerprint&&(!validated||fingerprint==lease->fingerprint);}
#define GscTestItemCurrentChecked old_current_checked
#define GscTestItemCurrent old_current
#define GscTestItemTime old_time
#define WeaponRegistrationScope old_scope
#define WeaponRegistrationBegin old_begin
#include "item.old"
#include "scope.old"
#include "begin.old"
#undef GscTestItemCurrentChecked
#undef GscTestItemCurrent
#undef GscTestItemTime
#undef WeaponRegistrationScope
#undef WeaponRegistrationBegin
#include "item.new"
#include "scope.new"
#include "begin.new"
static GscSession session,other;
static GscRuntime runtime={64},bsp={16};
static unsigned cases,accepted;
static void init(int state,int actual){
 memset(&session,0,sizeof(session));memset(&other,0,sizeof(other));memset(&gscTestItem,0,sizeof(gscTestItem));
 session.view.state=state;session.view.logical_ms=actual;session.view.session_id=7;session.runtime=&runtime;session.bsp_runtime=&bsp;
 gscTestSession=&session;gscTestItem.session=&session;gscTestItem.id=7;gscTestItem.registry_revision=3;gscTestItem.fingerprint=9;gscTestItem.operation_id=11;gscTestItem.mode=43;gscTestItem.registered[2]=1;gscTestItem.complete[2]=1;strcpy(gscTestItem.published,"marker");
 gscTestItemInitialization=map_current=registry_current=identity_current=1;gscTestItemBusy=gscTestCompleted=gscTestStopRequested=0;gscTestEntityMode=43;level.restarted=0;level.time=900;g_gametype.integer=GT_SINGLE_PLAYER;revision=3;weaponRegistrationRegistry=(Registry){9,3};weaponRegistrationGuard=(Guard){1,0,12};weaponRegistrationSerial=12;
}
static void check(WeaponRegistrationLease *lease,int logical){
 Guard guard;uint64_t serial;WeaponRegistrationLease x,y;GscSessionClockR50 value={-1,-1,-1};int a,b;
 assert(old_current(&lease->scene)==GscTestItemCurrent(&lease->scene));
 assert(old_current_checked(&lease->scene,1,9)==GscTestItemCurrentChecked(&lease->scene,1,9));
 assert(old_time(&lease->scene,logical)==GscTestItemTime(&lease->scene,logical));
 assert(old_scope(lease)==WeaponRegistrationScope(lease));
 a=old_current(&lease->scene);b=GscTestItemCurrentClockChecked(&lease->scene,0,0,&value);assert(a==b);
 if(a){assert(value.valid==1&&value.state==session.view.state&&value.logical_ms==session.view.logical_ms);}else assert(value.valid==-1&&value.state==-1&&value.logical_ms==-1);
 guard=weaponRegistrationGuard;serial=weaponRegistrationSerial;memset(&x,0xa5,sizeof(x));y=x;
 a=old_begin(&x);Guard old_guard=weaponRegistrationGuard;uint64_t old_serial=weaponRegistrationSerial;
 weaponRegistrationGuard=guard;weaponRegistrationSerial=serial;
 b=WeaponRegistrationBegin(&y);assert(a==b&&old_serial==weaponRegistrationSerial&&old_guard.active==weaponRegistrationGuard.active&&old_guard.cancelled==weaponRegistrationGuard.cancelled&&old_guard.serial==weaponRegistrationGuard.serial&&!memcmp(&x,&y,sizeof(x)));
 if(b)++accepted;
 ++cases;
}
int main(void){
 int times[]={-1,0,100,INT32_MAX};
 for(int state=0;state<=GSC_SESSION_COMPILED;state++)for(unsigned t=0;t<sizeof(times)/sizeof(times[0]);t++)for(unsigned m=0;m<1024;m++){
  init(state,times[t]);WeaponRegistrationLease lease={gscTestItem,12,times[t],900};
  if(m&1)session.view.session_id=8;
  if(m&2)session.view.error[0]='e';
  if(m&4)gscTestItem.failed=1;
  if(m&8)gscTestCompleted=1;
  if(m&16)level.restarted=1;
  if(m&32)identity_current=0;
  if(m&64)revision=4;
  if(m&128)weaponRegistrationGuard.cancelled=1;
  if(m&256)lease.scene.operation_id++;
  if(m&512)weaponRegistrationGuard.active=0;
  check(&lease,times[t]);
 }
 for(int k=0;k<19;k++){
  init(GSC_SESSION_RUNNING,100);WeaponRegistrationLease lease={gscTestItem,12,100,900};
  switch(k){case 0:gscTestSession=&other;break;case 1:lease.scene.session=NULL;break;case 2:lease.scene.id++;break;case 3:lease.scene.fingerprint++;break;case 4:lease.scene.mode=27;break;case 5:gscTestEntityMode=42;break;case 6:gscTestStopRequested=1;break;case 7:map_current=0;break;case 8:g_gametype.integer=0;break;case 9:gscTestItemBusy=1;break;case 10:registry_current=0;break;case 11:lease.scene.registry_revision++;break;case 12:lease.scene.registered[2]=0;break;case 13:lease.scene.complete[2]=0;break;case 14:lease.scene.published[0]='X';break;case 15:lease.server_time++;break;case 16:lease.logical_ms++;break;case 17:gscTestItemInitialization=0;break;case 18:weaponRegistrationSerial=UINT64_MAX;weaponRegistrationGuard.active=0;break;}
  check(&lease,100);
 }
 init(GSC_SESSION_RUNNING,100);WeaponRegistrationLease lease={gscTestItem,12,100,900};
 scans=stats=0;assert(old_scope(&lease));assert(scans==160&&stats==4);unsigned old_scans=scans,old_stats=stats;
 scans=stats=0;assert(WeaponRegistrationScope(&lease));assert(!scans&&!stats);
 printf("%u cases PASS (%u admitted Begin); successful Scope task inspections %u -> %u; diagnostics %u -> %u\n",cases,accepted,old_scans,scans,old_stats,stats);
 GscSessionClockR50 empty=gsc_session_clock_r50(NULL,7);assert(!empty.valid&&!empty.state&&!empty.logical_ms);
 return 0;
}
