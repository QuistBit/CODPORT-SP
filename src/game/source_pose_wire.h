#ifndef COD1_SOURCE_POSE_WIRE_H
#define COD1_SOURCE_POSE_WIRE_H
#include "bg_cod1_turret_actor_wire.h"
#include "../qcommon/cod1_actor_source_pose_payload14.h"
#define COD1_SOURCE_POSE_WIRE_VERSION 29
static int C1SourcePoseWireRead(const entityState_t*s,c1asp14Packet_t*out){
 float payload[C1ASP14_WORDS];
 if(!s||!out||C1AB14_Overlap(s,sizeof(*s),out,sizeof(*out)))return 0;
 memcpy(payload,s->cod1TurretActorGenericCurrent,98*4);memcpy(payload+98,s->cod1TurretActorPhases,42*4);memcpy(payload+140,s->cod1TurretActorCurrent,50*4);
 return C1ASP14_DecodePayload(payload,out);
}
static int C1SourcePoseWireWrite(entityState_t*s,const c1asp14Packet_t*p){
 float payload[C1ASP14_WORDS];
 if(!s||!p||C1AB14_Overlap(s,sizeof(*s),p,sizeof(*p))||!C1ASP14_EncodePayload(p,payload))return 0;
 memcpy(s->cod1TurretActorGenericCurrent,payload,98*4);memcpy(s->cod1TurretActorPhases,payload+98,42*4);memcpy(s->cod1TurretActorCurrent,payload+140,50*4);return 1;
}
static int C1SourcePoseWireValid(const entityState_t*s,int now){
 unsigned i;c1asp14Packet_t packet;
 if(!s||s->cod1TurretActorVersion!=COD1_SOURCE_POSE_WIRE_VERSION||s->number<0||s->number>=MAX_GENTITIES||s->eType!=ET_GENERAL||s->pos.trType!=TR_STATIONARY||s->apos.trType!=TR_STATIONARY||!(s->cod1TurretEpochLo|s->cod1TurretEpochHi)||!s->cod1TurretActorGeneration||s->cod1TurretActorGun!=-1||s->cod1TurretActorGunGeneration||s->cod1TurretActorTick<0||s->cod1TurretActorTick>now||s->cod1TurretActorConfig<CS_COD1_TURRET_ACTORS||(s->cod1TurretActorConfig>=CS_COD1_TURRET_ACTORS+MAX_COD1_TURRET_ACTORS&&s->cod1TurretActorConfig!=CS_COD1_JUMPER)||s->cod1TurretActorFlags!=1||s->frame)return 0;
 for(i=0;i<3;i++)if(!(s->pos.trBase[i]>=-1000000&&s->pos.trBase[i]<=1000000)||!(s->apos.trBase[i]>=-3600&&s->apos.trBase[i]<=3600))return 0;
 if(!C1SourcePoseWireRead(s,&packet))return 0;
 {unsigned jump=0,body=0;
  for(i=1;i<packet.count;i++){
   unsigned n=packet.source[i];
   if(n==892)jump++;
   if(s->cod1TurretActorConfig==CS_COD1_JUMPER&&!c1asp14Definition[n].children){
    unsigned ancestor=n,run=0;while(ancestor&&ancestor!=5){if(ancestor==164)run=1;ancestor=c1asp14Definition[ancestor].parent;}
    if(ancestor==5){if(n!=892&&!run)return 0;body++;}
   }
  }
  /* Bound jumper config: jump, its genuine fade into combatrun, or run.
   * Other source-pose actor configs still cannot borrow resource892. */
  return s->cod1TurretActorConfig==CS_COD1_JUMPER?body>0:jump==0;
 }
}
#endif
