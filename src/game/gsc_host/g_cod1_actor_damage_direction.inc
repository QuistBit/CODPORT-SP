#include "../../qcommon/cod1_actor_damage_yaw14.h"
/* Original readonly field row8988C: vector at Actor+224.
 * C490 clears the actual allocation before D710 defaults. E860 copies the
 * supplied direction before the pain dispatch; E950 leaves the old direction
 * untouched if the current native state is already6. No normalization here.
 * This owns data only: it does not enable damage or enter the death AI state. */
static int ActorPoseLease(const GscEntityRef*,cod1ActorLease_t*);
static int ActorDamageDirectionRead(const cod1ActorRecord_t*a,GscStringCastValue*out){
 GscStringCastValue v;float direction[3];int i;
 if(!a||!out||a->damageStateUnavailable)return 0;
 memcpy(direction,a->damageDirection224,12);
 for(i=0;i<3;i++)if(!isfinite(direction[i]))return 0;
 memset(&v,0,sizeof(v));v.type=3;memcpy(v.vector,direction,12);*out=v;return 1;
}
/* Readonly row89878: signed native int at Actor+21C, born zero in C490.
 * Both admitted damage fields belong to the same committed hit. */
static int ActorDamageTakenRead(const cod1ActorRecord_t*a,GscStringCastValue*out){
 GscStringCastValue v;
 if(!a||!out||a->damageStateUnavailable)return 0;
 memset(&v,0,sizeof(v));v.type=5;v.integer=a->damageTaken21c;*out=v;return 1;
}
/* Readonly row898A0, signed int Actor+220. Snapshot at the hit, not a
 * getter-time recomputation from the actor's subsequent orientation. */
static int ActorDamageYawRead(const cod1ActorRecord_t*a,GscStringCastValue*out){
 GscStringCastValue v;if(!a||!out||a->damageStateUnavailable)return 0;
 memset(&v,0,sizeof(v));v.type=5;v.integer=a->damageYaw220;*out=v;return 1;
}
int G_Cod1ActorRecordDamageState(const GscEntityRef*input,const float*direction,int32_t taken,int killed){
 GscEntityRef owner;cod1ActorLease_t lease;cod1ActorRecord_t*a;cod1ActorTurretMount_t*m;uint32_t bits[3];float supplied[3];int32_t yaw;int i,state;
 if(!input||!EntityValidate(NULL,input))return 0;owner=*input;
 if(!cod1ActorByEntity[owner.slot])return 1; /* Ordinary non-actor combat. */
 if(!ActorPoseLease(&owner,&lease))return 0;
 a=cod1Actors+lease.actorIndex;m=&a->turretMount;
 if(!m->initialized||m->serial!=lease.serial||m->currentTop<0||m->currentTop>=6){a->damageStateUnavailable=1;return 0;}
 state=m->currentStack[m->currentTop];
 if(state<0||state>11||(killed!=0&&killed!=1)){a->damageStateUnavailable=1;return 0;}
 if(killed&&state==6)return 1;
 if(!direction){a->damageStateUnavailable=1;return 0;}
 memcpy(bits,direction,12);
 for(i=0;i<3;i++){float v;memcpy(&v,bits+i,4);if(!isfinite(v)){a->damageStateUnavailable=1;return 0;}}
 memcpy(supplied,bits,12);
 if(!C1ADY14_Compute(supplied,g_entities[owner.slot].r.currentAngles[1],&yaw)){a->damageStateUnavailable=1;return 0;}
 /* Pure, current allocation-owned commit. No callback between acquisition
  * and write, no borrowed direction pointer retained, no look/velocity input. */
 a->damageTaken21c=taken;a->damageYaw220=yaw;memcpy(a->damageDirection224,bits,12);a->damageStateUnavailable=0;return 1;
}
