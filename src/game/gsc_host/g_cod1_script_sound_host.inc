/* R194 explicit mode40 sound diagnostic. Native event/flag values stay in
 * semantic sidecars. The paired command calls the existing alias mixer. */
#include "../g_cod1_script_sound_core.h"
#include "../bg_cod1_script_sound_packet.h"
typedef struct {
 GscTestQuakeScene scene;GscEntityRef owner,recipient;char alias[128],map[128];int argc;
 Cod1SSPlan plan;Cod1SSDiagnostic diag;
} GscSoundCall;
typedef struct {GscEntityRef owner;Cod1ScriptSoundState state;} GscSoundOwner;
static GscSoundOwner gscSoundOwners[MAX_GENTITIES];
static char gscSoundIndices[256][128];
static uint64_t gscSoundEpoch;
static uint32_t gscSoundRandom=1;
static int GscSoundCurrent(void *user) {
 GscSoundCall *c=user;
 return c&&GscTestQuakeCurrent(&c->scene)&&G_Cod1EntitySameAllocation(&c->owner)&&
  c->owner.slot<MAX_GENTITIES&&GscTestQuakePlayerCurrent(&c->recipient)&&gscSoundEpoch==c->owner.epoch;
}
static int GscSoundList(void *user,char *out,size_t cap) {
 int count;(void)user;
 count=trap_FS_GetFileList("soundaliases",".csv",out,(int)cap);
 return count>=COD1_SS_TABLES?-1:count;
}
static int GscSoundRead(void *user,const char *path,char **out,size_t *size) {
 fileHandle_t f=0;int len;char *p;(void)user;
 len=trap_FS_FOpenFile(path,&f,FS_READ);
 if(len<=0||len>(int)COD1_SS_TABLE_BYTES){if(f)trap_FS_FCloseFile(f);return 0;}
 p=malloc((size_t)len);if(!p){trap_FS_FCloseFile(f);return 0;}
 trap_FS_Read(p,len,f);trap_FS_FCloseFile(f);*out=p;*size=(size_t)len;return 1;
}
static void GscSoundRelease(void *user,char *data){(void)user;free(data);}
static int GscSoundExists(void *user,const char *path) {
 fileHandle_t f=0;int len;(void)user;len=trap_FS_FOpenFile(path,&f,FS_READ);if(f)trap_FS_FCloseFile(f);return len>0;
}
static int GscSoundAlias(void *user,char *out,size_t cap) {
 GscSoundCall *c=user;size_t n=strlen(c->alias);if(!GscSoundCurrent(c)||n>=cap)return 0;memcpy(out,c->alias,n+1);return 1;
}
static int GscSoundResolve(void *user,const char *alias) {
 GscSoundCall *c=user;Cod1SSFiles files;int status;
 memset(&files,0,sizeof(files));files.user=c;files.current=GscSoundCurrent;files.list=GscSoundList;
 files.read=GscSoundRead;files.release=GscSoundRelease;files.exists=GscSoundExists;
 status=Cod1ScriptSoundResolve(&files,alias,c->map,&c->plan,&c->diag);
 return status==COD1_SS_RESOURCE_READY?1:status==COD1_SS_RESOURCE_NONE?0:-1;
}
static int GscSoundIndexGet(void *user,unsigned i,char *out,size_t cap) {
 if(!GscSoundCurrent(user)||i<1||i>255||cap<128)return 0;memcpy(out,gscSoundIndices[i],128);return 1;
}
static int GscSoundIndexSet(void *user,unsigned i,const char *alias) {
 if(!GscSoundCurrent(user)||i<1||i>255)return 0;strcpy(gscSoundIndices[i],alias);return 1;
}
static int GscSoundCount(void *user,uint32_t *out) {
 GscSoundCall *c=user;if(!GscSoundCurrent(c))return 0;*out=(uint32_t)c->argc;return 1;
}
static float GscSoundUnit(void) {
 gscSoundRandom=gscSoundRandom*1664525u+1013904223u;
 return (gscSoundRandom>>8)*(1.0f/16777216.0f);
}
static int GscSoundInvoke(void *user,const GscEntityRef *owner,const char *alias,int argc,char *error,size_t cap) {
 GscSoundCall c;GscSoundOwner *side;Cod1ScriptSoundOps ops;Cod1ScriptSoundResult result;
 Cod1SoundPacket packet;Cod1SSVariant *v;char command[C1S_CAPACITY],actual[MAX_STRING_CHARS],marker[32];
 float total=0,draw;unsigned i,pick=0;uint64_t next;int channel;
 (void)user;memset(&c,0,sizeof(c));c.scene=gscTestQuake;
 if(!owner||!alias||!Cod1ScriptSoundText(alias)||!Cod1SS_Token(alias,128)||
    !GscTestQuakeCurrent(&c.scene)||!c.scene.marker_ready||!G_Cod1EntitySameAllocation(owner)||owner->slot>=MAX_GENTITIES)goto refuse;
 c.owner=*owner;strcpy(c.alias,alias);c.argc=argc;
 if(!G_Cod1EntityCapture(&g_entities[0],&c.recipient)||!GscTestQuakePlayerCurrent(&c.recipient))goto refuse;
 if(gscSoundEpoch!=owner->epoch){gscSoundEpoch=owner->epoch;memset(gscSoundIndices,0,sizeof(gscSoundIndices));memset(gscSoundOwners,0,sizeof(gscSoundOwners));gscSoundRandom=1;}
 trap_Cvar_VariableStringBuffer("mapname",c.map,sizeof(c.map));if(!GscSoundCurrent(&c))goto refuse;
 side=&gscSoundOwners[owner->slot];
 if(side->owner.epoch!=owner->epoch||side->owner.generation!=owner->generation){memset(side,0,sizeof(*side));side->owner=*owner;}
 side->state.has_client=g_entities[owner->slot].client!=NULL;
 memset(&ops,0,sizeof(ops));ops.user=&c;ops.read_alias=GscSoundAlias;ops.alias_exists=GscSoundResolve;
 ops.get_index=GscSoundIndexGet;ops.set_index=GscSoundIndexSet;ops.read_count=GscSoundCount;
 result=Cod1ScriptSoundInvoke(&side->state,1,(uint32_t)level.time,&ops);
 if(result!=COD1_SS_OK||argc!=1||!GscSoundCurrent(&c)) {
  snprintf(error,cap,"sound core=%d resource=%d line=%u; only one argument is selected",(int)result,(int)c.diag.reason,c.diag.line);return 0;
 }
 /* Reject unsupported mixer ranges rather than clamp or substitute resources. */
 for(i=0;i<c.plan.count;++i){v=&c.plan.variants[i];
  if(v->volume_max>C1S_GAIN_MAX/C1S_ALIAS_GAIN||v->pitch_max>4||(v->dist_max!=0&&v->dist_max<v->dist_min)||!CQ_Finite(total+v->probability))goto refuse;
  total+=v->probability;
 }
 if(!(total>0))goto refuse;draw=GscSoundUnit()*total;
 for(i=0;i<c.plan.count;++i){if(c.plan.variants[i].probability>0)pick=i;draw-=c.plan.variants[i].probability;if(draw<0)break;}
 v=&c.plan.variants[pick];memset(&packet,0,sizeof(packet));
 packet.epoch=c.scene.epoch;next=gscTestQuake.sequence;
 if(next==UINT64_MAX)goto refuse;packet.sequence=next+1;packet.entity=owner->slot;
 channel=Cod1SS_Equal(v->channel,"body")?1:Cod1SS_Equal(v->channel,"weapon")?2:Cod1SS_Equal(v->channel,"voice")?3:Cod1SS_Equal(v->channel,"item")?4:0;
 packet.channel=(uint32_t)channel;strcpy(packet.path,v->path);
 memcpy(packet.origin,g_entities[owner->slot].r.currentOrigin,sizeof(packet.origin));
 packet.gain=C1S_ALIAS_GAIN*(v->volume_min+(v->volume_max-v->volume_min)*GscSoundUnit());
 packet.pitch=v->pitch_min+(v->pitch_max-v->pitch_min)*GscSoundUnit();packet.dist_min=v->dist_min;packet.dist_max=v->dist_max;
 if(!C1S_Encode(command,sizeof(command),&packet)||!CQ_EncodeMetadata(marker,sizeof(marker),c.scene.epoch)||
    !GscSoundCurrent(&c)||!GscTestQuakeAdmission(&c.scene))goto refuse;
 trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
 if(!GscSoundCurrent(&c)||strcmp(actual,marker)||gscTestQuake.sequence!=next)goto refuse;
 gscTestQuake.sequence=packet.sequence;trap_SendServerCommand(0,command);
 if(!GscSoundCurrent(&c)||gscTestQuake.sequence!=packet.sequence)goto refuse;
 G_Printf("[R194] SOUND_SENT alias=%s path=%s owner=%u\n",alias,packet.path,owner->slot);
 return GscSoundCurrent(&c);
refuse:
 GscTestError(error,cap,"playsound owner/resource/mixer or map lease refused");return 0;
}
