
/* Completion uses native client-reported duration and GAME time. A missing
 * resource produces no invented acknowledgement; GAME retains its native
 * unknown-duration fallback. No notify names cross the command boundary. */
#include "../game/bg_cod1_sound_notify_packet.h"
static uint64_t cgNotifySoundIncarnation;
static void CG_Cod1NotifySoundCommand(void) {
 char copied[C1N_ARGC][COD1_SS_PATH_BYTES],reply[C1D_CAPACITY];const char *argv[C1N_ARGC];
 C1NPacket packet;C1DReport report;uint64_t incarnation,epoch;int i,duration;
 int channels[6]={CHAN_AUTO,CHAN_BODY,CHAN_WEAPON,CHAN_VOICE,CHAN_ITEM,CHAN_LOCAL};sfxHandle_t sound;
 epoch=cgCod1ScriptQuakeEpoch;incarnation=cgCod1ScriptQuakeIncarnation;
 if(!epoch||trap_Argc()!=C1N_ARGC)return;
 for(i=0;i<C1N_ARGC;i++){
  memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));argv[i]=copied[i];
  if(incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch)return;
 }
 if(!C1N_DecodePacket(C1N_ARGC,argv,&packet)||packet.sound.epoch!=epoch)return;
 if(cgSoundEpoch!=epoch||(cgNotifySoundIncarnation&&cgNotifySoundIncarnation!=incarnation)){cgSoundEpoch=epoch;cgSoundSequence=0;}
 cgNotifySoundIncarnation=incarnation;
 if(packet.sound.sequence<=cgSoundSequence)return;
 cgSoundSequence=packet.sound.sequence;
 sound=trap_S_RegisterSound(packet.sound.path);
 if(!sound||incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch||cgSoundSequence!=packet.sound.sequence)return;
 trap_Cod1StartAliasSound(packet.sound.channel==5?NULL:packet.sound.origin,packet.sound.channel==5?cg.clientNum:(int)packet.sound.entity,channels[packet.sound.channel],sound,
    packet.sound.gain,packet.sound.pitch,packet.sound.dist_min,packet.sound.dist_max);
 if(incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch||cgSoundSequence!=packet.sound.sequence)return;
 duration=trap_Cod1SoundDuration(sound,packet.sound.pitch);
 if(duration<0||duration>C1N_MAX_DURATION_MS||incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch||cgSoundSequence!=packet.sound.sequence)return;
 memset(&report,0,sizeof(report));report.identity=C1N_PacketIdentity(&packet);report.duration=duration;
 if(C1D_Encode(reply,sizeof(reply),&report))trap_SendClientCommand(reply);
}
