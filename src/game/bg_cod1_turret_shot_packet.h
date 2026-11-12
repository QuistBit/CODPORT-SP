#ifndef BG_COD1_TURRET_SHOT_PACKET_H
#define BG_COD1_TURRET_SHOT_PACKET_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#define C1TS_WIRE_MAGIC 0x54530000u
#define C1TS_EVENT_SHOT 1u
#define C1TS_EVENT_STOP 2u
#define C1TS_WEAPON_CLASS 254
#define C1TS_TRACE_MASK UINT32_C(0x0280e031)
typedef struct {uint64_t epoch;uint32_t generation;int owner,tick,serial,kind;float point[3],basis[3][3];} c1tsPacket_t;
#if defined(C1TS_PACKET_ENTITY_STATE)
static int C1TSP_Decode(const entityState_t*s,int now,c1tsPacket_t*out){c1tsPacket_t p;unsigned k,i;double n,d;if(!s||!out||s->eType!=ET_EVENTS+EV_COD1_TURRET_SHOT||((uint32_t)s->time2&0xffff0000u)!=C1TS_WIRE_MAGIC||s->otherEntityNum<MAX_CLIENTS||s->otherEntityNum>=ENTITYNUM_MAX_NORMAL||s->time<0||s->time>now||s->effect1Time<=0)return 0;memset(&p,0,sizeof(p));p.epoch=(uint64_t)(uint32_t)s->cod1TurretEpochLo|((uint64_t)(uint32_t)s->cod1TurretEpochHi<<32);p.generation=(uint32_t)s->cod1TurretGeneration;p.owner=s->otherEntityNum;p.tick=s->time;p.serial=s->effect1Time;p.kind=(uint32_t)s->time2&0xffffu;if(!p.epoch||!p.generation||(p.kind!=C1TS_EVENT_SHOT&&p.kind!=C1TS_EVENT_STOP))return 0;
 for(k=0;k<3;k++){if(!(s->origin[k]>=-1000000&&s->origin[k]<=1000000))return 0;p.point[k]=s->origin[k];p.basis[0][k]=s->origin2[k];p.basis[2][k]=s->angles2[k];}
 if(p.kind==C1TS_EVENT_SHOT){for(i=0;i<3;i+=2){n=0;for(k=0;k<3;k++){if(!(p.basis[i][k]>=-1.001f&&p.basis[i][k]<=1.001f))return 0;n+=(double)p.basis[i][k]*p.basis[i][k];}if(n<.998||n>1.002)return 0;}d=(double)p.basis[0][0]*p.basis[2][0]+(double)p.basis[0][1]*p.basis[2][1]+(double)p.basis[0][2]*p.basis[2][2];if(d<-.001||d>.001)return 0;
 /* axis1 = up cross forward: full captured roll, right-handed FX frame. */
 p.basis[1][0]=p.basis[2][1]*p.basis[0][2]-p.basis[2][2]*p.basis[0][1];p.basis[1][1]=p.basis[2][2]*p.basis[0][0]-p.basis[2][0]*p.basis[0][2];p.basis[1][2]=p.basis[2][0]*p.basis[0][1]-p.basis[2][1]*p.basis[0][0];}
 *out=p;return 1;
}
#endif /* C1TS_PACKET_ENTITY_STATE */
#endif
