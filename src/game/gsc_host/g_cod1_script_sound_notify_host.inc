/* R194 explicit mode40 sound diagnostic. Native event/flag values stay in
 * semantic sidecars. The paired command calls the existing alias mixer. */
#include "../g_cod1_script_sound_core.h"
#include "../bg_cod1_sound_notify_packet.h"
#include "../bg_cod1_voice_packet.h"
typedef struct {
 GscTestQuakeScene scene;GscEntityRef owner,recipient;char alias[128],map[128];int argc,unregisteredVoice;GscSoundNotifyRequest request;
 Cod1SSPlan plan;Cod1SSDiagnostic diag;
} GscSoundCall;
typedef struct {GscEntityRef owner;Cod1ScriptSoundState state;C1NPending pending;uint64_t session;C1NPacket voicePacket;int voiceActive,voiced,loadedCaption,voiceDrained,voiceFailed,voiceHasNotify,pendingSeparate,foleySampleClock;} GscSoundOwner;
static GscSoundOwner gscSoundOwners[MAX_GENTITIES];
static char gscSoundIndices[256][128];
static uint64_t gscSoundEpoch;
static uint32_t gscSoundRandom=1;
static int gscSoundStreamedSelected;
/* GAME owner-thread scratch; reset with sound/session or map lifetime. */
static Cod1SSAbsentCache56 gscSoundAbsence56;
#include "bg_cod1_sound_read56.inc"
static Cod1SSReadBuffer56 gscSoundRead56;
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
 fileHandle_t f=0;int len,got;char *p;(void)user;
 len=trap_FS_FOpenFile(path,&f,FS_READ);
 if(!f||len<=0||len>(int)COD1_SS_TABLE_BYTES){if(f)trap_FS_FCloseFile(f);return 0;}
 p=Cod1SS_ReadAcquire56(&gscSoundRead56,(size_t)len);if(!p){trap_FS_FCloseFile(f);return 0;}
 got=trap_FS_ReadCount(p,len,f);trap_FS_FCloseFile(f);
 if(got!=len){Cod1SS_ReadRelease56(&gscSoundRead56,p);return 0;}
 *out=p;*size=(size_t)len;return 1;
}
static void GscSoundRelease(void *user,char *data){(void)user;Cod1SS_ReadRelease56(&gscSoundRead56,data);}
static int GscSoundExists(void *user,const char *path) {
 fileHandle_t f=0;int len;(void)user;len=trap_FS_FOpenFile(path,&f,FS_READ);if(f)trap_FS_FCloseFile(f);return len>0;
}
static int GscSoundAlias(void *user,char *out,size_t cap) {
 GscSoundCall *c=user;size_t n=strlen(c->alias);if(!GscSoundCurrent(c)||n>=cap)return 0;memcpy(out,c->alias,n+1);return 1;
}
/* Native event AC plays the next voice without replacing the older AD
 * notification. Its deadline still belongs to the sample reported for AD,
 * even after the audio channel switches to the new voice. */
static int GscSoundIndependentVoice(const GscSoundCall *c){
 const GscSoundOwner *side=gscSoundOwners+c->owner.slot;
 unsigned i;
 if(c->argc!=1 || (!Cod1SS_Equal(c->map,"training")&&!(gscSoundStreamedSelected==COD1_SS_CAMPAIGN_VOICE_PROFILE&&Cod1SS_Equal(c->map,"us_intro"))) || !c->plan.count || !side->pending.pending ||
    !side->voiced || side->loadedCaption || side->voiceFailed ||
    side->pending.identity.epoch!=c->scene.epoch || side->pending.identity.session!=gscTestId ||
    side->pending.identity.entity!=c->owner.slot || side->pending.identity.generation!=c->owner.generation ||
    (!side->pendingSeparate && side->voicePacket.sound.sequence!=side->pending.identity.sequence))return 0;
 for(i=0;i<c->plan.count;i++)if((!c->plan.variants[i].streamed&&gscSoundStreamedSelected!=COD1_SS_CAMPAIGN_VOICE_PROFILE) || !Cod1SS_Equal(c->plan.variants[i].channel,"voice"))return 0;
 return 1;
}
/* A plain loaded body sound has no completion slot and uses a separate mixer channel. */
static int GscSoundPlainBody(const GscSoundCall *c){
 unsigned i;
 if(c->argc!=1 || !c->plan.count)return 0;
 for(i=0;i<c->plan.count;i++){
  const Cod1SSVariant *v=&c->plan.variants[i];
  if(v->streamed || v->subtitle[0] || (!Cod1SS_Equal(v->channel,"body")&&!Cod1SS_Equal(v->channel,"local")))return 0;
 }
 return 1;
}
static int GscSoundResolve(void *user,const char *alias) {
 GscSoundCall *c=user;Cod1SSFiles files;int status;
 memset(&files,0,sizeof(files));files.user=c;files.current=GscSoundCurrent;files.list=GscSoundList;
 files.read=GscSoundRead;files.release=GscSoundRelease;files.exists=GscSoundExists;
 int profile=gscSoundStreamedSelected;
 /* face::PlayFaceThread supplies the original third argument (true).
  * C1N_Begin owns its replacement policy and the mixer reports duration. */
 if(profile==2 && (c->argc==2 || (c->argc==3 && c->request.mode_valid && c->request.mode==1)) &&
    c->request.notify_valid &&
    G_Cod1GscFeature47(C1CP47_F_CAPTIONS)==1)
  profile=COD1_SS_LOADED_CAPTION_PROFILE;
 status=Cod1ScriptSoundResolveModeCached56(&files,alias,c->map,&c->plan,&c->diag,profile,0,&gscSoundAbsence56);
 if(status==COD1_SS_RESOURCE_READY){unsigned i;if(c->argc==1&&gscSoundOwners[c->owner.slot].pending.pending&&(gscSoundOwners[c->owner.slot].voiced||gscSoundOwners[c->owner.slot].loadedCaption||c->plan.variants[0].streamed)&&!GscSoundIndependentVoice(c)&&!GscSoundPlainBody(c)){c->diag.reason=COD1_SS_REASON_UNSUPPORTED;return -1;}for(i=1;i<c->plan.count;i++)if(c->plan.variants[i].streamed!=c->plan.variants[0].streamed || !!c->plan.variants[i].subtitle[0]!=!!c->plan.variants[0].subtitle[0]){c->diag.reason=COD1_SS_REASON_UNSUPPORTED;return -1;}}
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
typedef struct {C1NIdentity identity;char notify[128];GscEntityRef owner;} GscSoundEvent;
static GscSoundEvent gscSoundEvents[128];static unsigned gscSoundEventCount;static int gscSoundNativeCall,gscSoundNotifySelected;
static int GscSoundNotifyCurrent(void *user,const C1NIdentity *id){
 GscSoundOwner *side;(void)user;
 if(!id||id->entity>=MAX_GENTITIES||!gscTestSession||gscTestStopRequested||gscTestCompleted||
    !gscTestId||gscTestId!=id->session||!GscTestQuakeCurrent(&gscTestQuake)||gscTestQuake.epoch!=id->epoch)return 0;
 side=&gscSoundOwners[id->entity];
 return side->session==id->session&&side->owner.generation==id->generation&&
    G_Cod1EntitySameAllocation(&side->owner)&&
    !(g_entities[id->entity].takedamage&&g_entities[id->entity].health<=0);
}
static int GscSoundNotifyQueue(void *user,const C1NIdentity *id,const char *name){
 GscSoundEvent *event;int awakened;char error[384];(void)user;
 if(!GscSoundNotifyCurrent(NULL,id)||!C1N_Text(name)||gscSoundEventCount>=128)return 0;
 /* Replacement notifies only registrations present now, before the new
  * script wait can register. The selected native event marks tasks but does
  * not reenter the scheduler. Completion from frame uses the normal queue. */
 if(gscSoundNativeCall){awakened=0;return gsc_session_sound_notify_native(gscTestSession,id->session,
    &gscSoundOwners[id->entity].owner,name,&awakened,error,sizeof(error));}
 event=&gscSoundEvents[gscSoundEventCount++];memset(event,0,sizeof(*event));event->identity=*id;
 event->owner=gscSoundOwners[id->entity].owner;strcpy(event->notify,name);return 1;
}
static C1NOps GscSoundNotifyOps(void){C1NOps ops;memset(&ops,0,sizeof(ops));ops.current=GscSoundNotifyCurrent;ops.notify=GscSoundNotifyQueue;return ops;}
static void GscSoundVoiceStop(GscSoundOwner *side){C1DReport report;char command[C1D_CAPACITY];C1NPacket packet;if(!side->voiceActive)return;packet=side->voicePacket;side->voiceActive=0;memset(&report,0,sizeof(report));report.identity=C1N_PacketIdentity(&packet);if(C1V_ControlEncode(command,sizeof(command),&report,'x'))trap_SendServerCommand(0,command);}
static void GscSoundNotifyReset(void){unsigned i;Cod1SS_ReadReset56(&gscSoundRead56);Cod1SS_AbsentReset56(&gscSoundAbsence56);gscSoundNotifySelected=0;gscSoundStreamedSelected=0;for(i=0;i<MAX_GENTITIES;i++)GscSoundVoiceStop(&gscSoundOwners[i]);memset(gscSoundOwners,0,sizeof(gscSoundOwners));memset(gscSoundEvents,0,sizeof(gscSoundEvents));gscSoundEventCount=0;}
static int GscSoundNotifyFlush(int now,char *error,size_t cap){
 unsigned i,count;int awakened;C1NOps ops=GscSoundNotifyOps();GscSoundEvent event;
 for(i=0;i<MAX_GENTITIES;i++){
  GscSoundOwner *side=&gscSoundOwners[i];C1NPending *pending=&side->pending;
  if(side->voiceActive){C1NIdentity id=C1N_PacketIdentity(&side->voicePacket);if(!GscSoundNotifyCurrent(NULL,&id)){GscSoundVoiceStop(side);C1N_Reset(pending);continue;}}
  if(pending->pending&&!GscSoundNotifyCurrent(NULL,&pending->identity)){GscSoundVoiceStop(side);C1N_Reset(pending);continue;}
  if(side->voiceFailed){GscTestError(error,cap,"voice client failed/cancelled; no dialog_done generated");return 0;}
  if(pending->pending&&side->voiced&&((!side->pendingSeparate&&!side->foleySampleClock&&!side->voiceDrained)||pending->duration<=0)){
   if(now<pending->last||now-pending->start>C1N_MAX_DURATION_MS+2000){GscSoundVoiceStop(side);C1N_Reset(pending);GscTestError(error,cap,"streamed voice duration/drain timed out; no dialog_done generated");return 0;}continue;
  }
  if(pending->pending && side->loadedCaption && pending->duration<=0){
   if(now<pending->last || now-pending->start>C1N_FALLBACK_MS){
    GscSoundVoiceStop(side);C1N_Reset(pending);
    GscTestError(error,cap,"loaded caption received no positive mixer duration");return 0;
   }
   continue;
  }
  if(pending->pending&&!C1N_Frame(pending,now,&ops)){GscTestError(error,cap,"sound completion frame refused");return 0;}
 }
 count=gscSoundEventCount;
 for(i=0;i<count;i++){
  event=gscSoundEvents[i];
  if(!GscSoundNotifyCurrent(NULL,&event.identity)||!G_Cod1EntitySameAllocation(&event.owner))continue;
  awakened=0;if(!gsc_session_notify_entity(gscTestSession,event.identity.session,&event.owner,event.notify,&awakened,error,cap))return 0;
  G_Printf("[sound completion] owner=%u tick=%d seq=%llu event=%s woken=%d\n",event.owner.slot,now,(unsigned long long)event.identity.sequence,event.notify,awakened);
 }
 memset(gscSoundEvents,0,sizeof(gscSoundEvents));gscSoundEventCount=0;return 1;
}
void G_Cod1GscSoundDurationReply(int clientNum){
 char copied[C1D_ARGC][32];const char *argv[C1D_ARGC];C1DReport report;C1NOps ops;int i;
 if(clientNum!=0||gscTestBusy||!gscTestSession||!g_entities[0].client||g_entities[0].client->pers.connected!=CON_CONNECTED||trap_Argc()!=C1D_ARGC)return;
 for(i=0;i<C1D_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));argv[i]=copied[i];}
 if(!C1D_Decode(C1D_ARGC,argv,&report)||!GscSoundNotifyCurrent(NULL,&report.identity))return;
 ops=GscSoundNotifyOps();(void)C1N_Report(&gscSoundOwners[report.identity.entity].pending,&report.identity,report.duration,level.time,&ops);
}
void G_Cod1GscVoiceReply(int clientNum){
 char copied[C1D_ARGC][32];const char *argv[C1D_ARGC];C1DReport report;C1NIdentity expected;GscSoundOwner *side;int i;
 if(clientNum!=0||gscTestBusy||!gscTestSession||!gscSoundStreamedSelected||!g_entities[0].client||g_entities[0].client->pers.connected!=CON_CONNECTED||trap_Argc()!=C1D_ARGC)return;
 for(i=0;i<C1D_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));argv[i]=copied[i];}
 if(!C1V_ControlDecode(C1D_ARGC,argv,&report,'f')||!GscSoundNotifyCurrent(NULL,&report.identity))return;
 side=&gscSoundOwners[report.identity.entity];expected=C1N_PacketIdentity(&side->voicePacket);if(!side->voiceActive||memcmp(&expected,&report.identity,sizeof(expected)))return;
 /* A loaded one-shot has one start result and never reports a stream drain. */
 if(side->loadedCaption && (report.duration || !side->pending.pending || side->pending.duration>=0))return;
 if(!report.duration){
  side->voiceActive=0;
  /* Ordinary playsound has no completion dependency (native event 0xAC).
   * Release this stream; never consume a pending notification from this owner. */
  if(!side->voiceHasNotify && (!side->pending.pending||side->pendingSeparate)){
   G_Printf("[R227] optional voice cancelled owner=%u sequence=%llu, no notify requested\n",
       (unsigned)expected.entity,(unsigned long long)expected.sequence);
   return;
  }
  side->voiceFailed=1;C1N_Reset(&side->pending);return;
 }
 if(!side->voiceHasNotify && side->pendingSeparate){side->voiceActive=0;return;}
 if(side->pending.pending&&(!side->voiced||side->pending.duration!=report.duration))return;
 side->voiceActive=0;side->voiceDrained=1;
}
static int GscSoundMode(void *user,int32_t *out){GscSoundCall *c=user;if(!GscSoundCurrent(c)||!c->request.mode_valid)return 0;*out=c->request.mode;return 1;}
static int GscSoundNotifyName(void *user,uint32_t *out){GscSoundCall *c=user;if(!GscSoundCurrent(c)||!c->request.notify_valid||!C1N_Text(c->request.notify))return 0;*out=1;return 1;}
static int GscSoundPending(void *user,unsigned index,uint32_t notify,int32_t mode){
 GscSoundCall *c=user;C1NIdentity id;C1NOps ops=GscSoundNotifyOps();(void)index;(void)notify;
 if(!GscSoundCurrent(c)||!gscTestId||gscTestQuake.sequence==UINT64_MAX)return 0;
 memset(&id,0,sizeof(id));id.epoch=c->scene.epoch;id.session=gscTestId;id.entity=c->owner.slot;id.generation=c->owner.generation;id.sequence=gscTestQuake.sequence+1;
 {GscSoundOwner *side=&gscSoundOwners[c->owner.slot];int conflict=0;
 int ok=C1N_BeginResult(&side->pending,&id,c->request.notify,mode,level.time,&ops,&conflict);
 if(conflict && side->voiced && c->plan.count && c->plan.variants[0].streamed &&
    Cod1SS_Equal(c->plan.variants[0].channel,"voice") && Cod1SS_Equal(c->map,"training")){
  /* GAME 466A0 signalled the former waiter and declined the new name.
   * The AD event was already emitted, and the script method returns. */
  c->unregisteredVoice=1;
  side->pendingSeparate=0;
  G_Printf("[Training voice] second notification declined owner=%u old=%s new=%s\n",
    id.entity,side->pending.notify,c->request.notify);
  return 1;
 }
 if(ok){side->voiced=c->plan.count&&c->plan.variants[0].streamed;
 /* For Foley's ordinary named line original GAME 1EF10 checks the
  * reported sample duration on the server tick. Mixer drain can follow
  * later; the packet still owns the device until it drains or is replaced. */
 side->foleySampleClock=side->voiced && c->argc==2 && Cod1SS_Equal(c->map,"training") &&
    g_entities[id.entity].targetname && !strcmp(g_entities[id.entity].targetname,"foley") &&
    Cod1SS_Equal(c->plan.variants[0].channel,"voice");
 side->loadedCaption=c->plan.count&&!side->voiced&&c->plan.variants[0].subtitle[0];side->voiceDrained=0;side->voiceFailed=0;side->pendingSeparate=0;}return ok;}
}
static int GscSoundInvokeSelected(void *user,const GscEntityRef *owner,const char *alias,int argc,const GscSoundNotifyRequest *request,char *error,size_t cap) {
 GscSoundCall c;GscSoundOwner *side;Cod1ScriptSoundOps ops;Cod1ScriptSoundResult result;
 Cod1SoundPacket packet;Cod1SSVariant *v;char command[C1V_CAPACITY],actual[MAX_STRING_CHARS],marker[32];
 float total=0,draw;unsigned i,pick=0;uint64_t next;int channel;
 (void)user;memset(&c,0,sizeof(c));c.scene=gscTestQuake;
 if(!owner||!alias||!Cod1ScriptSoundText(alias)||!Cod1SS_Token(alias,128)||
    !GscTestQuakeCurrent(&c.scene)||!c.scene.marker_ready||!G_Cod1EntitySameAllocation(owner)||owner->slot>=MAX_GENTITIES)goto refuse;
 c.owner=*owner;strcpy(c.alias,alias);c.argc=argc;if(request)c.request=*request;
 if(!G_Cod1EntityCapture(&g_entities[0],&c.recipient)||!GscTestQuakePlayerCurrent(&c.recipient))goto refuse;
 if(gscSoundEpoch!=owner->epoch){Cod1SS_ReadReset56(&gscSoundRead56);Cod1SS_AbsentReset56(&gscSoundAbsence56);gscSoundEpoch=owner->epoch;memset(gscSoundIndices,0,sizeof(gscSoundIndices));memset(gscSoundOwners,0,sizeof(gscSoundOwners));gscSoundRandom=1;}
 trap_Cvar_VariableStringBuffer("mapname",c.map,sizeof(c.map));if(!GscSoundCurrent(&c))goto refuse;
 side=&gscSoundOwners[owner->slot];
 if(side->owner.epoch!=owner->epoch||side->owner.generation!=owner->generation){GscSoundVoiceStop(side);memset(side,0,sizeof(*side));side->owner=*owner;}
 if(side->session!=gscTestId){GscSoundVoiceStop(side);C1N_Reset(&side->pending);side->session=gscTestId;}
 side->state.has_client=g_entities[owner->slot].client!=NULL;
 memset(&ops,0,sizeof(ops));ops.user=&c;ops.read_alias=GscSoundAlias;ops.alias_exists=GscSoundResolve;
 ops.get_index=GscSoundIndexGet;ops.set_index=GscSoundIndexSet;ops.read_count=GscSoundCount;
 if(request){ops.read_mode=GscSoundMode;ops.read_notify=GscSoundNotifyName;ops.notify=GscSoundPending;}
 result=Cod1ScriptSoundInvoke(&side->state,1,(uint32_t)level.time,&ops);
 if(result!=COD1_SS_OK||(!request&&argc!=1)||!GscSoundCurrent(&c)) {
  snprintf(error,cap,"sound core=%d resource=%d line=%u; selected sound invocation refused",(int)result,(int)c.diag.reason,c.diag.line);return 0;
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
 channel=Cod1SS_Equal(v->channel,"body")?1:Cod1SS_Equal(v->channel,"weapon")?2:Cod1SS_Equal(v->channel,"voice")?3:Cod1SS_Equal(v->channel,"item")?4:Cod1SS_Equal(v->channel,"local")?5:0;
 packet.channel=(uint32_t)channel;strcpy(packet.path,v->path);
 memcpy(packet.origin,g_entities[owner->slot].r.currentOrigin,sizeof(packet.origin));
 packet.gain=C1S_ALIAS_GAIN*(v->volume_min+(v->volume_max-v->volume_min)*GscSoundUnit());
 packet.pitch=v->pitch_min+(v->pitch_max-v->pitch_min)*GscSoundUnit();packet.dist_min=v->dist_min;packet.dist_max=v->dist_max;
 if(v->streamed){C1VPacket p;memset(&p,0,sizeof(p));p.sound.sound=packet;p.sound.session=gscTestId;p.sound.generation=owner->generation;strcpy(p.subtitle,v->subtitle);p.mix_mode=v->mix_mode;p.slave_cap=v->slave_cap;
  if(!gscSoundStreamedSelected||!C1V_EncodePacket(command,sizeof(command),&p))goto refuse;
  if(GscSoundIndependentVoice(&c)){
   side->pendingSeparate=1;
   if(g_debugMove.integer)G_Printf("[Training voice overlap] owner=%u old=%llu new=%llu start=%d duration=%d\n",
     owner->slot,(unsigned long long)side->pending.identity.sequence,(unsigned long long)packet.sequence,
     (int)side->pending.start,side->pending.duration);
  }
  side->voicePacket=p.sound;side->voiceHasNotify=argc>=2&&!c.unregisteredVoice;side->loadedCaption=0;side->voiceActive=1;side->voiceDrained=0;side->voiceFailed=0;
 }else if(v->subtitle[0]){
  C1VPacket caption={0};
  caption.sound.sound=packet;caption.sound.session=gscTestId;caption.sound.generation=owner->generation;
  strcpy(caption.subtitle,v->subtitle);
  if(!request || !side->loadedCaption || !C1V_EncodeLoadedCaption(command,sizeof(command),&caption))goto refuse;
  GscSoundVoiceStop(side);
  side->voicePacket=caption.sound;side->voiceHasNotify=1;side->voiceActive=1;
 }else{
  /* Event AC replaces the audible channel, retaining an older AD deadline.
   * Its completion still uses that original sample's positive duration. */
  if(GscSoundIndependentVoice(&c))side->pendingSeparate=1;
  if(!GscSoundPlainBody(&c))GscSoundVoiceStop(side);if(request){C1NPacket p;memset(&p,0,sizeof(p));p.sound=packet;p.session=gscTestId;p.generation=owner->generation;
  if(!C1N_EncodePacket(command,sizeof(command),&p))goto refuse;
 }else if(!C1S_Encode(command,sizeof(command),&packet))goto refuse;}
 if(!CQ_EncodeMetadata(marker,sizeof(marker),c.scene.epoch)||
    !GscSoundCurrent(&c)||!GscTestQuakeAdmission(&c.scene))goto refuse;
 trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
 if(!GscSoundCurrent(&c)||strcmp(actual,marker)||gscTestQuake.sequence!=next)goto refuse;
 gscTestQuake.sequence=packet.sequence;trap_SendServerCommand(0,command);
 if(!GscSoundCurrent(&c)||gscTestQuake.sequence!=packet.sequence)goto refuse;
 if(side->voiceActive && side->pending.pending && GscSoundPlainBody(&c))
  G_Printf("[voice/body] owner=%u generation=%u tick=%d voice=%llu body=%s\n",owner->slot,owner->generation,level.time,(unsigned long long)side->pending.identity.sequence,alias);
 if(v->streamed && request)
  G_Printf("[voice start] owner=%u tick=%d seq=%llu alias=%s\n",owner->slot,level.time,(unsigned long long)packet.sequence,alias);
 G_DPrintf("[R194] SOUND_SENT alias=%s path=%s owner=%u\n",alias,packet.path,owner->slot);
 return GscSoundCurrent(&c);
refuse:
 GscTestError(error,cap,"playsound owner/resource/mixer or map lease refused");return 0;
}

static int GscSoundInvoke(void *user,const GscEntityRef *owner,const char *alias,int argc,char *error,size_t cap){
 return GscSoundInvokeSelected(user,owner,alias,argc,NULL,error,cap);
}
static int GscSoundNotifyInvoke(void *user,const GscSoundNotifyRequest *r,char *error,size_t cap){
 if(!r){GscTestError(error,cap,"missing sound request");return 0;}
 {int ok;if(gscSoundNativeCall){GscTestError(error,cap,"sound native callback reentered");return 0;}
  gscSoundNativeCall=1;ok=GscSoundInvokeSelected(user,&r->owner,r->alias,r->argc,r,error,cap);gscSoundNativeCall=0;return ok;}
}
