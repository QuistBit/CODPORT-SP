/* Persistent sound state: command receipt stages a value, frame processing
 * accepts it only after the engine owns a valid PCM stream. No completion
 * event is synthesized. Failed replacement retains the accepted old stream. */
#include "../game/bg_cod1_loop_sound.h"
#include "../game/bg_cod1_loop_receipt.h"
/* active retains a desired descriptor; audible alone confirms a PCM stream. */
typedef struct {int active,pending,audible;uint64_t sequence;C1LPacket accepted,next;} CGLRow;
static CGLRow cglRows[1024];
static uint64_t cglEpoch,cglIncarnation,cglFrame,cglRevision;
static int cglTime,cglBlocked,cglBusy,cglResync;
static int CGL_Current(uint64_t e,uint64_t inc,uint64_t rev){return e&&e==cgCod1ScriptQuakeEpoch&&inc==cgCod1ScriptQuakeIncarnation&&rev==cglRevision;}
static int CGL_SameOwner(const C1LPacket*a,const C1LPacket*b){return a->entity_epoch==b->entity_epoch&&a->generation==b->generation&&a->sound.entity==b->sound.entity&&a->sound.epoch==b->sound.epoch;}
static int CGL_Scope(void){cod1LoopStreamRequest r;uint64_t e=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation,rev;
 if(cglEpoch==e&&cglIncarnation==inc)return e&&!cglBlocked;
 memset(cglRows,0,sizeof(cglRows));cglEpoch=e;cglIncarnation=inc;cglFrame=0;cglTime=cg.time;cglBlocked=1;rev=++cglRevision;
 if(!e)return 0;memset(&r,0,sizeof(r));r.operation=C1L_RESET;r.incarnation=inc;r.packet.sound.epoch=e;
 if(!trap_Cod1LoopStream(&r)||!CGL_Current(e,inc,rev))return 0;cglBlocked=0;cglResync=1;return 1;
}
static void CG_Cod1LoopSoundCommand(void){
 char copied[C1L_SOUND_ARGC][COD1_SS_PATH_BYTES+1];const char*a[C1L_SOUND_ARGC];C1LPacket p;uint64_t e=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation,rev=cglRevision;int i;
 if(!e||trap_Argc()!=C1L_SOUND_ARGC)return;
 for(i=0;i<C1L_SOUND_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));a[i]=copied[i];if(!CGL_Current(e,inc,rev))return;}
 if(!C1L_Decode(C1L_SOUND_ARGC,a,&p)||p.sound.epoch!=e||!CGL_Scope()||e!=cgCod1ScriptQuakeEpoch||inc!=cgCod1ScriptQuakeIncarnation)return;
 if(p.sound.sequence<=cglRows[p.sound.entity].sequence)return;
 ++cglRevision;cglRows[p.sound.entity].sequence=p.sound.sequence;cglRows[p.sound.entity].next=p;cglRows[p.sound.entity].pending=1;
}
/* No active snapshot: retire PCM first, then report temporary unavailability.
 * Keep sequence floors so delayed commands cannot revive an older owner. */
void CG_Cod1LoopSoundSuspend(void){cod1LoopStreamRequest r;C1PReceipt reports[2048];unsigned count=0;int i;uint64_t e,inc,rev;
 if(cglBusy)return;cglBusy=1;if(!CGL_Scope())goto done;e=cglEpoch;inc=cglIncarnation;rev=cglRevision;
 if(cglFrame>=UINT64_MAX-1)goto done;
 memset(&r,0,sizeof(r));r.operation=C1L_END_FRAME;r.incarnation=inc;r.packet.sound.epoch=e;r.frame=++cglFrame;
 if(!trap_Cod1LoopStream(&r)||!CGL_Current(e,inc,rev))goto done;
 for(i=0;i<1024;i++){
  CGLRow*row=cglRows+i;
  if(row->active)reports[count++]=C1P_From(&row->accepted,C1P_SUSPENDED);
  if(row->pending)reports[count++]=C1P_From(&row->next,row->next.live?C1P_SUSPENDED:C1P_ACCEPTED);
  row->active=row->pending=row->audible=0;
 }
 cglResync=1;
 for(i=0;i<(int)count;i++){char wire[C1P_CAPACITY];if(C1P_Encode(wire,sizeof(wire),reports+i))trap_SendClientCommand(wire);if(!CGL_Current(e,inc,rev))break;}
 done:cglBusy=0;
}
void CG_Cod1LoopSoundFrame(void){
 cod1LoopStreamRequest r;uint64_t e,inc,rev;int i,ok;C1PReceipt reports[2048];unsigned reportCount=0;
 if(cglBusy)return;cglBusy=1;if(!CGL_Scope())goto done;
 e=cglEpoch;inc=cglIncarnation;rev=cglRevision;
 if(cg.time<cglTime||cglFrame>=UINT64_MAX-1){cglBlocked=1;for(i=0;i<1024;i++)if(cglRows[i].active){reports[reportCount++]=C1P_From(&cglRows[i].accepted,cglRows[i].audible?C1P_LOST:C1P_REJECTED);cglRows[i].active=cglRows[i].audible=0;}goto retire;}cglTime=cg.time;++cglFrame;
 for(i=0;i<1024;i++){
  CGLRow row=cglRows[i];int refreshed=0;memset(&r,0,sizeof(r));r.incarnation=inc;r.frame=cglFrame;
  if(row.pending){
   if(row.next.live){r.operation=C1L_UPDATE;r.packet=row.next;ok=trap_Cod1LoopStream(&r);
    if(!CGL_Current(e,inc,rev))goto abort;
    if(ok==C1L_PLAYING||ok==C1L_OUT_OF_RANGE){row.accepted=row.next;row.active=1;row.audible=ok==C1L_PLAYING;refreshed=1;}
    reports[reportCount++]=C1P_From(&row.next,ok==C1L_PLAYING?C1P_ACCEPTED:ok==C1L_OUT_OF_RANGE?C1P_DEFERRED:C1P_REJECTED);
   }else if(row.active&&CGL_SameOwner(&row.accepted,&row.next)){
    r.operation=C1L_STOP;r.packet=row.accepted;ok=trap_Cod1LoopStream(&r);
    if(!CGL_Current(e,inc,rev))goto abort;
    /* Even failed stop is not refreshed; end-frame owns final retirement. */
    row.active=row.audible=0;
   }
   if(!row.next.live)reports[reportCount++]=C1P_From(&row.next,C1P_ACCEPTED);
   row.pending=0;cglRows[i]=row;
  }
  if(row.active&&!refreshed){r.operation=C1L_UPDATE;r.packet=row.accepted;ok=trap_Cod1LoopStream(&r);
   if(!CGL_Current(e,inc,rev))goto abort;
   if(!ok){cglRows[i].active=cglRows[i].audible=0;reports[reportCount++]=C1P_From(&row.accepted,row.audible?C1P_LOST:C1P_REJECTED);}
   else if((ok==C1L_PLAYING)!=row.audible){cglRows[i].audible=ok==C1L_PLAYING;reports[reportCount++]=C1P_From(&row.accepted,ok==C1L_PLAYING?C1P_ACCEPTED:C1P_DEFERRED);}
  }
 }
 retire:memset(&r,0,sizeof(r));r.operation=C1L_END_FRAME;r.incarnation=inc;r.packet.sound.epoch=e;
 r.frame=cglBlocked?cglFrame+1:cglFrame;
 if(!trap_Cod1LoopStream(&r)||!CGL_Current(e,inc,rev))goto abort;
 /* Receipt follows successful end-frame retirement. Never acknowledge the
  * command merely because it parsed or was queued. A lost same-frame live
  * acceptance is reported as rejection, not a transient false success. */
 for(i=0;i<(int)reportCount;i++){
  char wire[C1P_CAPACITY];C1PReceipt*p=reports+i;CGLRow*row=cglRows+p->entity;
  if(p->live&&p->status==C1P_ACCEPTED&&(!row->active||!row->audible||!C1P_Matches(p,&row->accepted)))p->status=C1P_REJECTED;
  if(C1P_Encode(wire,sizeof(wire),p))trap_SendClientCommand(wire);
  if(!CGL_Current(e,inc,rev))goto abort;
 }
 if(cglResync&&!cglBlocked){char wire[24];memcpy(wire,"c1q 1 ",6);CQ_WriteHex(wire+6,16,e);wire[22]=0;cglResync=0;trap_SendClientCommand(wire);if(!CGL_Current(e,inc,rev))goto abort;}
 goto done;
 abort:cglBlocked=1;
 /* An interrupted same-scene frame must not leave a previously refreshed
  * handle alive forever. A later frame without refresh retires the lane. */
 if(e==cgCod1ScriptQuakeEpoch&&inc==cgCod1ScriptQuakeIncarnation&&cglFrame<UINT64_MAX-1){
  memset(&r,0,sizeof(r));r.operation=C1L_END_FRAME;r.incarnation=inc;r.packet.sound.epoch=e;r.frame=++cglFrame;
  if(trap_Cod1LoopStream(&r)&&e==cgCod1ScriptQuakeEpoch&&inc==cgCod1ScriptQuakeIncarnation){
   /* An earlier receipt may already have escaped through the command sink.
    * Report only verified retirement, using the previously accepted identity. */
   reportCount=0;for(i=0;i<1024;i++)if(cglRows[i].active){reports[reportCount++]=C1P_From(&cglRows[i].accepted,cglRows[i].audible?C1P_LOST:C1P_REJECTED);cglRows[i].active=cglRows[i].audible=0;}
   for(i=0;i<(int)reportCount;i++){char wire[C1P_CAPACITY];if(C1P_Encode(wire,sizeof(wire),reports+i))trap_SendClientCommand(wire);if(e!=cgCod1ScriptQuakeEpoch||inc!=cgCod1ScriptQuakeIncarnation)break;}
  }
 }
 done:cglBusy=0;
}
