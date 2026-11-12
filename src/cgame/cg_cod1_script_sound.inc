/* R194 command arrival is the declared SP clock/placement policy. */
#include "../game/bg_cod1_script_sound_packet.h"
static uint64_t cgSoundEpoch,cgSoundSequence;
static void CG_Cod1ScriptSoundCommand(void) {
 char copied[C1S_ARGC][COD1_SS_PATH_BYTES];const char *argv[C1S_ARGC];
 Cod1SoundPacket packet;uint64_t incarnation,epoch;int i,channels[5]={CHAN_AUTO,CHAN_BODY,CHAN_WEAPON,CHAN_VOICE,CHAN_ITEM};sfxHandle_t sound;
 epoch=cgCod1ScriptQuakeEpoch;incarnation=cgCod1ScriptQuakeIncarnation;
 if(!epoch||trap_Argc()!=C1S_ARGC||incarnation!=cgCod1ScriptQuakeIncarnation)return;
 for(i=0;i<C1S_ARGC;++i){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));argv[i]=copied[i];
  if(incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch)return;}
 if(!C1S_Decode(C1S_ARGC,argv,&packet)||packet.epoch!=epoch)return;
 if(cgSoundEpoch!=epoch){cgSoundEpoch=epoch;cgSoundSequence=0;}
 if(packet.sequence<=cgSoundSequence)return;
 /* Consume before external registration, so rejected/reentered messages never replay. */
 cgSoundSequence=packet.sequence;
 sound=trap_S_RegisterSound(packet.path);
 if(!sound||incarnation!=cgCod1ScriptQuakeIncarnation||epoch!=cgCod1ScriptQuakeEpoch||cgSoundSequence!=packet.sequence)return;
 trap_Cod1StartAliasSound(packet.origin,(int)packet.entity,channels[packet.channel],sound,packet.gain,packet.pitch,packet.dist_min,packet.dist_max);
 CG_Printf("[R194] SOUND_PLAY path=%s\n",packet.path);
}
