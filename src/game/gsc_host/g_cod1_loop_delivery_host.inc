/* GAME transport for committed singleton streamed-auto loop descriptors.
 * The client owns distance/refresh/fresh RNG ordering. Retirement drains
 * independently of the VM; receipts never generate VM events. */
#include "../bg_cod1_loop_receipt.h"
typedef struct {C1PDelivery delivery;GscEntityRef recipient;GscTestQuakeScene scene;uint64_t revision,ownerRevision;int retire,resend,terminal;} GscLoopDelivery;
static GscLoopDelivery gscLoopDeliveries[MAX_GENTITIES];
static uint64_t gscLoopDeliverySequence,gscLoopDeliveryRevision;
static int gscLoopDeliveryBusy,gscLoopDeliveryCancelled;
static int gscLoopDeliveryFrameBusy;
/* Transport ownership can outlive a destroyed VM. Only tombstones may use
 * this map/namespace identity; never dereference its retired Session pointer. */
static int GscLoopDeliveryScene(const GscTestQuakeScene*s){
 return s&&s->owned&&s->marker_ready&&s->epoch&&s->epoch!=UINT64_MAX&&s->incarnation&&s->incarnation!=UINT64_MAX&&
  gscTestQuake.owned&&gscTestQuake.marker_ready&&s->epoch==gscTestQuake.epoch&&s->incarnation==gscTestQuake.incarnation&&
  s->map_epoch&&s->map_epoch==gscTestQuake.map_epoch&&s->map_epoch==G_Cod1EntityMapEpoch()&&s->id==gscTestQuake.id&&s->session==gscTestQuake.session;
}
static int GscLoopDeliveryScope(const GscLoopDelivery*d){char marker[COD1_SCRIPT_QUAKE_METADATA_CAPACITY];uint64_t epoch=0;int time=level.time;
 if(!d||!GscLoopDeliveryScene(&d->scene)||(d->delivery.sent.live&&!GscTestQuakeIdentity(&d->scene))||!GscTestQuakePlayerCurrent(&d->recipient))return 0;
 memset(marker,0,sizeof(marker));trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,marker,sizeof(marker));
 return GscLoopDeliveryScene(&d->scene)&&(!d->delivery.sent.live||GscTestQuakeIdentity(&d->scene))&&GscTestQuakePlayerCurrent(&d->recipient)&&level.time==time&&CQ_DecodeMetadata(marker,&epoch)&&epoch==d->scene.epoch;
}
static int GscLoopDeliveryResolved(const C1LPacket*p){GscLoopSoundRow*r;Cod1SSVariant*v;
 if(!p||!p->live||!C1L_Valid(p))return 0;r=gscLoopSounds+p->sound.entity;
 if(!r->present||!r->plan||r->plan->count!=1||r->owner.slot!=p->sound.entity||!GscTestQuakeCurrent(&r->scene)||!G_Cod1EntitySameAllocation(&r->owner)||p->sound.epoch!=r->scene.epoch||p->entity_epoch!=r->owner.epoch||p->generation!=r->owner.generation||!r->state.alias_index||strcmp(p->alias,gscSoundIndices[r->state.alias_index]))return 0;
 v=r->plan->variants;
 if(p->native_parameters){C1LRange range;range.volume_min=v->volume_min;range.volume_max=v->volume_max;range.pitch_min=v->pitch_min;range.pitch_max=v->pitch_max;
  return v->streamed&&v->probability>=0&&!strcmp(v->channel,"auto")&&!strcmp(p->sound.path,v->path)&&!memcmp(p->sound.origin,g_entities[p->sound.entity].r.currentOrigin,12)&&!memcmp(&p->range,&range,sizeof(range))&&p->sound.dist_min==v->dist_min&&p->sound.dist_max==v->dist_max;
 }
 return v->streamed&&v->probability>0&&!strcmp(p->sound.path,v->path)&&!memcmp(p->sound.origin,g_entities[p->sound.entity].r.currentOrigin,12)&&p->sound.gain>=(double)v->volume_min*(double).8f-0.000001&&p->sound.gain<=(double)v->volume_max*(double).8f+0.000001&&CQ_FloatBits(p->refresh_gain)==CQ_FloatBits((float)((double)v->volume_min*(double).8f))&&p->sound.pitch>=v->pitch_min&&p->sound.pitch<=v->pitch_max&&p->sound.dist_min==v->dist_min&&p->sound.dist_max==v->dist_max;
}
/* The adapter checks ownership/resource/ranges; it does not certify who drew
 * random values. The caller must use the proven native parameter operation. */
static int GscLoopDeliverySend(const C1LPacket*input){
 C1LPacket p;GscLoopDelivery next,*row;char command[C1L_SOUND_CAPACITY];uint64_t rev;int time=level.time,ok=0;
 if(gscLoopDeliveryBusy){gscLoopDeliveryCancelled=1;return 0;}
 if(!input||gscLoopDeliverySequence==UINT64_MAX||gscLoopDeliveryRevision==UINT64_MAX)return 0;p=*input;if(!C1L_Valid(&p))return 0;row=gscLoopDeliveries+p.sound.entity;
 if(p.live){
  if(!GscLoopDeliveryResolved(&p))return 0;
  /* A new allocation must not overwrite an unacknowledged old tombstone.
   * Failed client replacement preserves its old stream, so latest-sequence
   * alone is not proof that the previous allocation has retired. */
  if(row->delivery.sent.sound.sequence&&
     (p.sound.epoch!=row->delivery.sent.sound.epoch||p.entity_epoch!=row->delivery.sent.entity_epoch||p.generation!=row->delivery.sent.generation)&&
     (row->delivery.sent.live||!row->delivery.accepted))return 0;
 }
 else if(!row->delivery.sent.sound.sequence||p.sound.epoch!=row->delivery.sent.sound.epoch||p.entity_epoch!=row->delivery.sent.entity_epoch||p.generation!=row->delivery.sent.generation)return 0;
 memset(&next,0,sizeof(next));next.scene=p.live?gscTestQuake:row->scene;next.delivery.sent.live=p.live;
 if(!p.live){next.retire=1;next.terminal=row->terminal;next.ownerRevision=row->ownerRevision;}
 else next.ownerRevision=gscLoopSounds[p.sound.entity].revision;
 gscLoopDeliveryBusy=1;gscLoopDeliveryCancelled=0;
 if(!G_Cod1EntityCapture(g_entities,&next.recipient)||!GscLoopDeliveryScope(&next)||gscLoopDeliveryCancelled)goto done;
 p.sound.sequence=++gscLoopDeliverySequence;next.revision=rev=++gscLoopDeliveryRevision;
 if(!C1L_Encode(command,sizeof(command),&p)||!C1P_Begin(&next.delivery,rev,&p)||level.time!=time||!GscLoopDeliveryScope(&next)||gscLoopDeliveryCancelled|| (p.live&&!GscLoopDeliveryResolved(&p)))goto done;
 *row=next;trap_SendServerCommand(0,command);
 if(row->revision==rev&&GscLoopDeliveryScope(&next)&&!gscLoopDeliveryCancelled&&level.time==time&&(p.live?GscLoopDeliveryResolved(&p):1)){ok=1;}
 else if(row->revision==rev){row->delivery.pending=row->delivery.accepted=0;row->delivery.failed=1;}
 done:gscLoopDeliveryBusy=0;return ok;
}
static int GscLoopDeliveryRetire(const GscEntityRef*ref){GscLoopDelivery*d;
 if(gscLoopDeliveryBusy){gscLoopDeliveryCancelled=1;return 0;}
 if(!ref||ref->slot>=MAX_GENTITIES)return 0;d=gscLoopDeliveries+ref->slot;
 if(d->delivery.sent.sound.sequence&&d->delivery.sent.entity_epoch==ref->epoch&&d->delivery.sent.generation==ref->generation)d->retire=1;
 return 1;
}
static int GscLoopDeliveryRetireAll(void){int i;
 if(gscLoopDeliveryBusy){gscLoopDeliveryCancelled=1;return 0;}
 for(i=0;i<MAX_GENTITIES;i++)if(gscLoopDeliveries[i].delivery.sent.sound.sequence)gscLoopDeliveries[i].retire=1;
 return 1;
}
static void GscLoopDeliveryReset(void){
 /* Sequence/revision never rewind. A callback reset invalidates its caller. */
 if(gscLoopDeliveryBusy)gscLoopDeliveryCancelled=1;
 memset(gscLoopDeliveries,0,sizeof(gscLoopDeliveries));
}
static int GscLoopDeliveryNative(unsigned slot,C1LPacket*p){GscLoopSoundRow*r=gscLoopSounds+slot;Cod1SSVariant*v;
 if(!r->present||!r->state.alias_index||!r->plan||r->plan->count!=1||!GscTestQuakeCurrent(&r->scene)||!G_Cod1EntitySameAllocation(&r->owner))return 0;
 v=r->plan->variants;if(!v->streamed||strcmp(v->channel,"auto"))return 0;
 memset(p,0,sizeof(*p));p->live=1;p->native_parameters=1;p->sound.epoch=r->scene.epoch;p->sound.sequence=1;p->sound.entity=slot;p->entity_epoch=r->owner.epoch;p->generation=r->owner.generation;
 strcpy(p->alias,gscSoundIndices[r->state.alias_index]);strcpy(p->sound.path,v->path);memcpy(p->sound.origin,g_entities[slot].r.currentOrigin,12);
 p->sound.dist_min=v->dist_min;p->sound.dist_max=v->dist_max;p->range.volume_min=v->volume_min;p->range.volume_max=v->volume_max;p->range.pitch_min=v->pitch_min;p->range.pitch_max=v->pitch_max;p->refresh_gain=(float)((double)v->volume_min*(double).8f);
 return GscLoopDeliveryResolved(p);
}
/* Commit revisions, not task counts, distinguish new authoritative commands
 * from a failed/deferred delivery. Fresh random draws belong to the client. */
void G_Cod1GscLoopSoundFrame(void){int i,time=level.time;
 if(gscLoopSoundStage.busy||(gscLoopSoundStage.present&&!gscLoopSoundStage.committed))return;
 if(gscLoopDeliveryBusy){gscLoopDeliveryCancelled=1;return;}
 if(gscLoopDeliveryFrameBusy)return;gscLoopDeliveryFrameBusy=1;
 for(i=0;i<MAX_GENTITIES;i++){
  GscLoopDelivery*d=gscLoopDeliveries+i;C1LPacket p;int need;
  GscLoopSoundRow*source=gscLoopSounds+i;
  if(source->present&&source->state.alias_index&&source->revision!=d->ownerRevision){C1LPacket fresh;
   int newOwner=d->delivery.sent.sound.sequence&&(source->owner.epoch!=d->delivery.sent.entity_epoch||source->owner.generation!=d->delivery.sent.generation);
   if((!newOwner||(!d->delivery.sent.live&&d->delivery.accepted))&&GscLoopDeliveryNative((unsigned)i,&fresh)){
    if(!GscLoopDeliverySend(&fresh)||level.time!=time)break;continue;
   }
  }
  if(!d->delivery.sent.sound.sequence)continue;p=d->delivery.sent;
  if(!GscLoopDeliveryScene(&d->scene)){memset(d,0,sizeof(*d));continue;}
  if(p.live){
   GscLoopSoundRow*r=gscLoopSounds+i;
   if(!r->present||!r->state.alias_index||!GscTestQuakeCurrent(&r->scene)||
      r->owner.epoch!=p.entity_epoch||r->owner.generation!=p.generation||!G_Cod1EntitySameAllocation(&r->owner))d->retire=1;
   else {memcpy(p.sound.origin,g_entities[i].r.currentOrigin,12);if(!GscLoopDeliveryResolved(&p)){d->retire=1;d->terminal=1;}}
  }
  if(d->delivery.failed){d->retire=1;d->terminal=1;}
  if(!GscTestQuakePlayerCurrent(&d->recipient))d->resend=1;
  if(d->retire){
   if(!p.live&&!d->resend)continue; /* Wait for real ack; retain failed retirement. */
   {C1LPacket tomb;memset(&tomb,0,sizeof(tomb));tomb.sound.epoch=p.sound.epoch;tomb.sound.entity=p.sound.entity;tomb.sound.sequence=1;tomb.entity_epoch=p.entity_epoch;tomb.generation=p.generation;p=tomb;}
   need=1;
  }else{
   if(!p.live||d->terminal)continue;
   need=d->resend||(!d->delivery.pending&&!d->delivery.suspended&&memcmp(&p,&d->delivery.sent,sizeof(p)));
  }
  if(need){if(!GscLoopDeliverySend(&p)||level.time!=time)break;}
 }
 gscLoopDeliveryFrameBusy=0;
}
/* A client asks for a full resend after successfully resetting its local loop
 * engine. This is not a playback-ready receipt. Repeated requests only set a
 * bit; sequence allocation waits for the frame service. A same-tick module
 * reset must not lose its request to a timestamp-based duplicate filter. */
void G_Cod1GscLoopSoundResync(int clientNum){char version[8],value[33];uint64_t epoch;GscLoopDelivery lease;int i,time=level.time;
 if(clientNum!=0||gscLoopDeliveryBusy)return;gscLoopDeliveryBusy=1;gscLoopDeliveryCancelled=0;
 if(trap_Argc()!=3)goto done;memset(version,0,sizeof(version));memset(value,0,sizeof(value));trap_Argv(1,version,sizeof(version));trap_Argv(2,value,sizeof(value));
 if(strcmp(version,"1")||!CQ_ParseHex(value,16,&epoch)||epoch!=gscTestQuake.epoch)goto done;
 memset(&lease,0,sizeof(lease));lease.scene=gscTestQuake;
 if(!G_Cod1EntityCapture(g_entities,&lease.recipient)||!GscLoopDeliveryScope(&lease)||gscLoopDeliveryCancelled||level.time!=time)goto done;
 for(i=0;i<MAX_GENTITIES;i++)if(gscLoopDeliveries[i].delivery.sent.sound.epoch==epoch)gscLoopDeliveries[i].resend=1;
 done:gscLoopDeliveryBusy=0;
}
void G_Cod1GscLoopSoundReply(int clientNum){
 char copied[C1P_ARGC][33];const char*a[C1P_ARGC];C1PReceipt receipt;GscLoopDelivery*row,expected;int i,time=level.time;
 if(clientNum!=0||gscLoopDeliveryBusy)return;gscLoopDeliveryBusy=1;gscLoopDeliveryCancelled=0;if(trap_Argc()!=C1P_ARGC)goto done;
 for(i=0;i<C1P_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));a[i]=copied[i];}
 if(!C1P_Decode(C1P_ARGC,a,&receipt))goto done;row=gscLoopDeliveries+receipt.entity;expected=*row;
 if(!C1P_Matches(&receipt,&expected.delivery.sent)||!GscLoopDeliveryScope(&expected)||gscLoopDeliveryCancelled||level.time!=time||memcmp(row,&expected,sizeof(expected)))goto done;
 (void)C1P_Report(&row->delivery,expected.revision,&receipt);
 done:gscLoopDeliveryBusy=0;
}
