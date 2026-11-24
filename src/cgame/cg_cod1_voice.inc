/* R230 reliable typed voice/subtitle consumer. Epoch/incarnation and full
 * entity allocation identity fence requests and completion. Platform audio
 * duration is supplemented by real stream-drain acknowledgement, not a timer
 * pretending that a failed/missing stream played. */
#include "../game/bg_cod1_voice_packet.h"
typedef struct {int active,loadedCaption,start,duration;cod1VoiceRequest request;char subtitle[COD1_SS_TEXT_BYTES];} CGVTrack;
static CGVTrack cgvTracks[1024];static uint64_t cgvEpoch,cgvIncarnation,cgvSequence;static int cgvLastTime,cgvCaption=-1;
static void CGV_Report(const C1NPacket *p,int duration,char command){C1DReport report;char text[C1D_CAPACITY];memset(&report,0,sizeof(report));report.identity=C1N_PacketIdentity(p);report.duration=duration;if(C1V_ControlEncode(text,sizeof(text),&report,command))trap_SendClientCommand(text);}
static void CGV_Stop(int i,int report){CGVTrack old=cgvTracks[i];cod1VoiceRequest stop;if(!old.active&&!old.loadedCaption)return;memset(&cgvTracks[i],0,sizeof(cgvTracks[i]));if(cgvCaption==i)cgvCaption=-1;if(old.loadedCaption)return;stop=old.request;stop.operation=C1V_STREAM_STOP;trap_Cod1VoiceStream(&stop);if(report&&old.request.incarnation==cgCod1ScriptQuakeIncarnation&&old.request.packet.sound.epoch==cgCod1ScriptQuakeEpoch)CGV_Report(&old.request.packet,0,'f');}
static int CGV_Current(uint64_t epoch,uint64_t incarnation){return epoch&&epoch==cgCod1ScriptQuakeEpoch&&incarnation==cgCod1ScriptQuakeIncarnation;}
/* The authored opening launches todayobstacle and start1 together. Retain the
 * accepted reliable event as a presentation cue, independent of sound drain.
 * This does not complete animscripted or emit a VM notification. */
#include "../game/bg_cod1_unmounted_actor_wire.h"
static struct {C1NIdentity id; uint64_t incarnation,actorEpoch; int start;} cgBriefing;
static uint64_t CGV_ActorEpoch(const entityState_t *s) {
    return ((uint64_t)(uint32_t)s->cod1TurretEpochHi<<32)|(uint32_t)s->cod1TurretEpochLo;
}
static void CGV_BriefingStart(const C1NPacket *packet,uint64_t incarnation){
    /* Reliable commands execute before installing nextSnap, or on snap for
     * the initial snapshot. Bind both independent lifetimes at that point. */
    const snapshot_t *basis=cg.nextSnap?cg.nextSnap:cg.snap;
    const entityState_t *owner=NULL;
    int i;
    memset(&cgBriefing,0,sizeof(cgBriefing));
    if(!basis || basis->serverTime<0 || basis->numEntities<0 || basis->numEntities>MAX_ENTITIES_IN_SNAPSHOT)return;
    for(i=0;i<basis->numEntities;i++){
        const entityState_t *s=&basis->entities[i];
        if(s->number!=(int)packet->sound.entity)continue;
        if(owner || (s->eFlags&EF_NODRAW) || !C1UAW_Valid(s,basis->serverTime) ||
           (uint32_t)s->cod1TurretActorGeneration!=packet->generation)return;
        owner=s;
    }
    if(!owner || !CGV_Current(packet->sound.epoch,incarnation))return;
    cgBriefing.id=C1N_PacketIdentity(packet);
    cgBriefing.incarnation=incarnation;cgBriefing.actorEpoch=CGV_ActorEpoch(owner);
    cgBriefing.start=basis->serverTime;
}
int CG_Cod1BriefingElapsed(const entityState_t *s) {
    uint64_t epoch=CGV_ActorEpoch(s);
    if (!cgBriefing.id.sequence || !CGV_Current(cgBriefing.id.epoch,cgBriefing.incarnation) ||
        cgBriefing.actorEpoch!=epoch || cgBriefing.id.entity!=s->number ||
        cgBriefing.id.generation!=(uint32_t)s->cod1TurretActorGeneration ||
        cg.time<cgBriefing.start) return -1;
    return cg.time-cgBriefing.start;
}
#include "cg_cod1_subtitle_r240.inc"
static int CGV_Scope(void){cod1VoiceRequest reset;uint64_t epoch=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation;int i;if(cgvEpoch==epoch&&cgvIncarnation==inc)return epoch!=0;for(i=0;i<1024;i++)CGV_Stop(i,0);memset(cgvTracks,0,sizeof(cgvTracks));memset(&cgBriefing,0,sizeof(cgBriefing));cgvEpoch=0;cgvSequence=0;cgvCaption=-1;cgvIncarnation=inc;cgvLastTime=cg.time;if(!epoch)return 0;memset(&reset,0,sizeof(reset));reset.operation=C1V_STREAM_RESET;reset.incarnation=inc;reset.packet.sound.epoch=epoch;if(trap_Cod1VoiceStream(&reset)!=0||!CGV_Current(epoch,inc))return 0;cgvEpoch=epoch;return 1;}
static void CG_Cod1VoiceCommand(void){
 char copied[C1V_ARGC][COD1_SS_PATH_BYTES];const char *a[C1V_ARGC];C1VPacket packet;cod1VoiceRequest request;CGVTrack track;uint64_t epoch=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation;int i,duration;const char *translated;
 if(!epoch||trap_Argc()!=C1V_ARGC)return;for(i=0;i<C1V_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));a[i]=copied[i];if(!CGV_Current(epoch,inc))return;}
 if(!C1V_DecodePacket(C1V_ARGC,a,&packet)||packet.sound.sound.epoch!=epoch||!CGV_Scope()||!CGV_Current(epoch,inc)||packet.sound.sound.sequence<=cgvSequence)return;cgvSequence=packet.sound.sound.sequence;
 if(!Q_stricmp(cgs.mapname,"maps/training.bsp") && cg.time>=0 &&
    !strcmp(packet.subtitle,"SUBTITLE_TRAINING_FOLEY_START1") &&
    !Q_stricmp(packet.sound.sound.path,"sound/voiceovers/us/training/Training_Foley_01.wav")) {
    CGV_BriefingStart(&packet.sound,inc);
 }
 translated=packet.subtitle[0]?CG_Cod1LocaleLookupSubtitle(packet.subtitle):"";if((packet.subtitle[0]&&(!translated||!*translated))||cg.time<0||cg.time>INT_MAX-C1N_MAX_DURATION_MS){CGV_Report(&packet.sound,0,'f');return;}if(!CGV_Current(epoch,inc))return;
 memset(&request,0,sizeof(request));request.operation=C1V_STREAM_PLAY;request.incarnation=inc;request.packet=packet.sound;request.mix_mode=packet.mix_mode;request.slave_cap=packet.slave_cap;duration=trap_Cod1VoiceStream(&request);if(!CGV_Current(epoch,inc))return;if(duration<=0||duration>C1N_MAX_DURATION_MS){CGV_Report(&packet.sound,0,'f');return;}
 /* Successful engine replacement retires the older allocation/channel. Do
  * not stop it by entity number after publication of its successor. */
 memset(&track,0,sizeof(track));track.active=1;track.start=cg.time;track.duration=duration;track.request=request;strcpy(track.subtitle,packet.subtitle);cgvTracks[packet.sound.sound.entity]=track;cgvCaption=packet.subtitle[0]?(int)packet.sound.sound.entity:-1;
 {C1DReport report;char text[C1D_CAPACITY];memset(&report,0,sizeof(report));report.identity=C1N_PacketIdentity(&packet.sound);report.duration=duration;if(C1D_Encode(text,sizeof(text),&report))trap_SendClientCommand(text);}
}
static void CG_Cod1VoiceStopCommand(void){char copied[C1D_ARGC][32];const char *a[C1D_ARGC];C1DReport report;C1NIdentity id;int i;if(trap_Argc()!=C1D_ARGC)return;for(i=0;i<C1D_ARGC;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));a[i]=copied[i];}if(!C1V_ControlDecode(C1D_ARGC,a,&report,'x')||report.duration||report.identity.entity>=1024)return;id=C1N_PacketIdentity(&cgvTracks[report.identity.entity].request.packet);if(!memcmp(&id,&report.identity,sizeof(id)))CGV_Stop(report.identity.entity,0);}
void CG_Cod1VoiceFrame(void){int i,status;uint64_t epoch=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation;if(!CGV_Scope())return;if(cg.time<cgvLastTime){memset(&cgBriefing,0,sizeof(cgBriefing));for(i=0;i<1024;i++)CGV_Stop(i,1);cgvLastTime=cg.time;return;}cgvLastTime=cg.time;
 for(i=0;i<1024;i++)if(cgvTracks[i].active){CGVTrack old=cgvTracks[i];cod1VoiceRequest query=old.request;query.operation=C1V_STREAM_STATUS;status=trap_Cod1VoiceStream(&query);if(!CGV_Current(epoch,inc))return;if(!status){memset(&cgvTracks[i],0,sizeof(cgvTracks[i]));if(cgvCaption==i)cgvCaption=-1;CGV_Report(&old.request.packet,old.duration,'f');}else if(status<0||cg.time-old.start>old.duration+2000)CGV_Stop(i,1);if(!CGV_Current(epoch,inc))return;}
}
void CG_Cod1VoiceSubtitleDraw(void){const char*text;CGVTrack*t;
 if(cgvCaption<0||cgvCaption>=1024)return;t=&cgvTracks[cgvCaption];
 if((!t->active&&!t->loadedCaption)||!CGV_Current(t->request.packet.sound.epoch,t->request.incarnation)||cg.time<t->start||cg.time-t->start>=t->duration)return;
 text=CG_Cod1LocaleLookupSubtitle(t->subtitle);if(text)CS240_SubtitleDraw(text);
}
