#ifndef BG_COD1_TURRET_CONSTRUCTOR14_H
#define BG_COD1_TURRET_CONSTRUCTOR14_H
/* GAME 233D0 constructor values. This is a value component, not a spawned
 * entity or an armed turret. Allocation, precache/script registration, sounds,
 * DObj, linking, controller frames and lifetime are separate owner duties.
 * Only finite WeaponDef defaults with no spawn-var overrides are admitted.
 * Evidence: Campaign-415/turret-constructor-native-439A.json. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>

enum { C1TS14_CAPACITY=32 };
typedef struct {
 int32_t inuse,state,flags,unknown0c;
 uint32_t targetEntity,previousTarget;float targetPoint[3];
 int32_t targetChanged,unknown28,unknown2c,unknown30;
 float pitchMin,yawMin,pitchMax,yawMax,initialYawMin,initialYawMax,restPitch;
 int32_t convergenceMs;
 float rangeSquared;
 int32_t unknown58,stance,private60,soundMsec;
 float accuracy;
 int32_t unknown6c,unknown70,unknown74,private78,team,unknown80[6];float pitchLimit;
 uint8_t loopSound,stopSound;uint16_t reserved9e;
} C1TS14Record;
typedef char C1TS14RecordSize[(sizeof(C1TS14Record)==160)?1:-1];
typedef char C1TS14RecordSoundOffset[(offsetof(C1TS14Record,loopSound)==0x9c)?1:-1];
typedef struct {
 float leftArc,rightArc,topArc,bottomArc,convergenceSeconds,maxRange,accuracy;
 int32_t damage,stance;
 uint8_t weaponId,loopSound,stopSound,reserved;
} C1TS14Weapon;
typedef struct {
 uint32_t flags;int32_t health,time;
 float origin[3],angles[3];
} C1TS14EntityInput;
typedef struct {
 uint32_t flags;int32_t health,damage,nextThink,handler,contents,svFlags;
 int32_t posType,angleType,field13c,field1c8,field1b8,field1c4;
 float origin[3],angles[3],mins[3],maxs[3];
 uint8_t type,weaponId,field129;uint8_t reserved;
} C1TS14EntityProjection;
static int C1TS14_Finite(float x){uint32_t bits;memcpy(&bits,&x,4);return (bits&0x7fffffffu)<=0x49742400u;}
static int C1TS14_Overlap(const void*a,size_t na,const void*b,size_t nb){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<y?y-x<na:x-y<nb;}
static int C1TS14_FirstFree(const C1TS14Record *pool,unsigned count){unsigned i;if(!pool||count!=C1TS14_CAPACITY)return -1;for(i=0;i<count;i++)if(!pool[i].inuse)return (int)i;return -1;}
static int C1TS14_Construct(const C1TS14Weapon *weapon,const C1TS14EntityInput *input,C1TS14Record *record,C1TS14EntityProjection *entity){
 C1TS14Weapon w;C1TS14EntityInput in;C1TS14Record r;C1TS14EntityProjection e;
 unsigned i;unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;double value;
 if(!weapon||!input||!record||!entity||C1TS14_Overlap(record,sizeof(*record),entity,sizeof(*entity))||C1TS14_Overlap(record,sizeof(*record),weapon,sizeof(*weapon))||C1TS14_Overlap(record,sizeof(*record),input,sizeof(*input))||C1TS14_Overlap(entity,sizeof(*entity),weapon,sizeof(*weapon))||C1TS14_Overlap(entity,sizeof(*entity),input,sizeof(*input)))return 0;
 w=*weapon;in=*input;
 if(!w.weaponId||w.reserved||in.time<0||in.time>2147483547||!C1TS14_Finite(w.leftArc)||!C1TS14_Finite(w.rightArc)||!C1TS14_Finite(w.topArc)||!C1TS14_Finite(w.bottomArc)||!C1TS14_Finite(w.convergenceSeconds)||!C1TS14_Finite(w.maxRange)||!C1TS14_Finite(w.accuracy))return 0;
 for(i=0;i<3;i++)if(!C1TS14_Finite(in.origin[i])||!C1TS14_Finite(in.angles[i]))return 0;
 memset(&r,0,sizeof(r));memset(&e,0,sizeof(e));
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 r.inuse=1;r.flags=0x1003;
 r.yawMin=-w.rightArc;if(r.yawMin>0)r.yawMin=0;
 r.yawMax=w.leftArc;if(r.yawMax<0)r.yawMax=0;
 r.pitchMin=-w.topArc;if(r.pitchMin>0)r.pitchMin=0;
 r.pitchMax=w.bottomArc;if(r.pitchMax<0)r.pitchMax=0;
 r.initialYawMin=r.yawMin;r.initialYawMax=r.yawMax;r.restPitch=-90;
 value=w.convergenceSeconds;if(value<0)value=0;value=value*1000.0+0.5;r.convergenceMs=(int32_t)value;
 r.rangeSquared=w.maxRange>0?(float)((double)w.maxRange*(double)w.maxRange):FLT_MAX;
 r.stance=w.stance;r.private60=-1;r.accuracy=w.accuracy;if(r.accuracy<0)r.accuracy=0;else if(r.accuracy>1)r.accuracy=1;
 r.private78=-1;r.team=3;r.loopSound=w.loopSound;r.stopSound=w.stopSound;
 __asm ldmxcsr mx
 __asm frstor env
 e.flags=in.flags|0x8000;e.health=in.health?in.health:100;e.damage=w.damage<0?0:w.damage;e.nextThink=in.time+100;e.handler=13;e.contents=0x200004;e.svFlags=0x80;
 e.posType=0;e.angleType=3;e.field13c=1;e.field1c8=1;e.field1b8=4;e.field1c4=2;
 memcpy(e.origin,in.origin,sizeof(e.origin));memcpy(e.angles,in.angles,sizeof(e.angles));e.mins[0]=e.mins[1]=-32;e.maxs[0]=e.maxs[1]=32;e.maxs[2]=56;e.type=10;e.weaponId=w.weaponId;e.field129=1;
 *record=r;*entity=e;return 1;
}
#endif
