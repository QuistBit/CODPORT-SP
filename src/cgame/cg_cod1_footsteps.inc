/* R195 player movement audio. Exact original alias resources; independent of
 * GSC/test mode. No RTCW sample fallback or silent truncation to four variants. */
#include <stdlib.h>
#include "../game/bg_cod1_footstep.h"
#include "../game/bg_cod1_script_sound.h"
typedef struct {
 int state;Cod1SSPlan plan;sfxHandle_t sounds[COD1_SS_VARIANTS];
} C1FootstepCache;
typedef struct {uint64_t epoch;char map[128];} C1FootstepLease;
static C1FootstepCache cgFootsteps[C1F_MODES][C1F_MATERIALS];
static uint64_t cgFootstepEpoch=1;
static uint32_t cgFootstepRng=1;
static int cgFootstepLoading;
void CG_Cod1FootstepsReset(void) {
 if(cgFootstepEpoch<UINT64_MAX)++cgFootstepEpoch;
 memset(cgFootsteps,0,sizeof(cgFootsteps));cgFootstepRng=1;cgFootstepLoading=0;
}
static int C1FootstepCurrent(void *user) {
 C1FootstepLease *l=user;return l&&l->epoch==cgFootstepEpoch&&l->epoch!=UINT64_MAX&&!strcmp(l->map,cgs.mapname);
}
static int C1FootstepList(void *user,char *out,size_t size) {
 int n;(void)user;n=trap_Cod1FS_GetFileList("soundaliases",".csv",out,(int)size);
 return n>=COD1_SS_TABLES?-1:n;
}
static int C1FootstepRead(void *user,const char *path,char **out,size_t *size) {
 fileHandle_t f=0;int n;char *p;(void)user;n=trap_FS_FOpenFile(path,&f,FS_READ);
 if(n<=0||n>(int)COD1_SS_TABLE_BYTES){if(f)trap_FS_FCloseFile(f);return 0;}
 p=malloc((size_t)n);if(!p){trap_FS_FCloseFile(f);return 0;}
 trap_FS_Read(p,n,f);trap_FS_FCloseFile(f);*out=p;*size=(size_t)n;return 1;
}
static void C1FootstepRelease(void *user,char *data){(void)user;free(data);}
static int C1FootstepExists(void *user,const char *path) {
 fileHandle_t f=0;int n;(void)user;n=trap_FS_FOpenFile(path,&f,FS_READ);if(f)trap_FS_FCloseFile(f);return n>0;
}
static int C1FootstepLoad(C1FootstepLease *lease,C1FootstepCache *cache,const char *alias) {
 Cod1SSFiles files;Cod1SSPlan plan;Cod1SSDiagnostic diag;sfxHandle_t sounds[COD1_SS_VARIANTS];
 unsigned i;int result;float total=0;char map[128];
 if(cgFootstepLoading||!C1FootstepCurrent(lease))return 0;
 cgFootstepLoading=1;memset(&files,0,sizeof(files));files.user=lease;
 files.current=C1FootstepCurrent;files.list=C1FootstepList;files.read=C1FootstepRead;
 files.release=C1FootstepRelease;files.exists=C1FootstepExists;
 Q_strncpyz(map,cgs.mapname,sizeof(map));
 result=Cod1ScriptSoundResolve(&files,alias,map,&plan,&diag);
 if(!C1FootstepCurrent(lease))return 0;
 if(result!=COD1_SS_RESOURCE_READY)goto held;
 for(i=0;i<plan.count;++i){Cod1SSVariant *v=&plan.variants[i];
  if(!Cod1SS_Equal(v->channel,"body")||v->volume_max>1||v->pitch_max>4||
     v->dist_max<v->dist_min||!CQ_Finite(total+v->probability))goto held;
  total+=v->probability;
 }
 if(!(total>0))goto held;
 for(i=0;i<plan.count;++i){sounds[i]=trap_S_RegisterSound(plan.variants[i].path);
  if(!C1FootstepCurrent(lease))return 0;if(!sounds[i])goto held;}
 cache->plan=plan;memcpy(cache->sounds,sounds,plan.count*sizeof(sounds[0]));
 cache->state=1;cgFootstepLoading=0;return 1;
held:
 cache->state=-1;cgFootstepLoading=0;
 CG_Printf("[R195] FOOTSTEP_HELD alias=%s resource=%d reason=%d line=%u\n",alias,result,(int)diag.reason,diag.line);
 return 0;
}
static float C1FootstepUnit(void) {
 cgFootstepRng=cgFootstepRng*1664525u+1013904223u;
 return (cgFootstepRng>>8)*(1.0f/16777216.0f);
}
static void CG_Cod1FootstepEvent(const entityState_t *state) {
 C1FootstepLease lease;C1FootstepCache *cache;const Cod1SSVariant *v;
 char alias[128];unsigned i,pick=0;float total=0,draw,gain,pitch;int parameter=state->eventParm;
 if(!cg_footsteps.integer||!C1F_Alias(parameter,alias,sizeof(alias))||state->number<0||state->number>=MAX_GENTITIES)return;
 lease.epoch=cgFootstepEpoch;Q_strncpyz(lease.map,cgs.mapname,sizeof(lease.map));cache=&cgFootsteps[(parameter>>5)&3][parameter&31];
 if(!cache->state&&!C1FootstepLoad(&lease,cache,alias))return;
 if(!C1FootstepCurrent(&lease)||cache->state!=1)return;
 for(i=0;i<cache->plan.count;++i)total+=cache->plan.variants[i].probability;
 draw=C1FootstepUnit()*total;
 for(i=0;i<cache->plan.count;++i){if(cache->plan.variants[i].probability>0)pick=i;draw-=cache->plan.variants[i].probability;if(draw<0)break;}
 v=&cache->plan.variants[pick];gain=.8f*(v->volume_min+(v->volume_max-v->volume_min)*C1FootstepUnit());
 pitch=v->pitch_min+(v->pitch_max-v->pitch_min)*C1FootstepUnit();
 trap_Cod1StartAliasSound(NULL,state->number,CHAN_BODY,cache->sounds[pick],gain,pitch,v->dist_min,v->dist_max);
 if(!C1FootstepCurrent(&lease))return;
 if(cg_debugEvents.integer)CG_Printf("[R195] FOOTSTEP_PLAY alias=%s path=%s entity=%d\n",alias,v->path,state->number);
}
