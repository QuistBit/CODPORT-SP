#ifndef BG_COD1_TURRET_SHOT_H
#define BG_COD1_TURRET_SHOT_H
/* Immutable staged native bullet request. Owned CRT stage uses >>16, not
 * the unrelated R132 random stream. No shipping original-DLL dependency. */
#include "bg_cod1_turret_bullet.h"
#include "bg_cod1_turret_shot_packet.h"
/* Native4912B..4918E: AI-owned bullet passes through same-team sentient.
 * Player attacks (no native actor) do not use this guard. */
static int C1TS_FriendlyPass(int attackerHasActor,int attackerTeam,int victimHasSentient,int victimTeam){return attackerHasActor&&victimHasSentient&&attackerTeam>=1&&attackerTeam<=3&&victimTeam==attackerTeam;}
typedef struct {uint32_t crt,qrand;} c1tsRandom_t;
typedef struct {float end[3],forward[3];uint32_t nativeFlags;c1tsRandom_t random;int tracked,hitBranch,corrected;} c1tsPlan_t;
static int C1TS_Finite(float x){return x>=-1000000&&x<=1000000;}
static int C1TS_Basis(const float b[3][3]){unsigned i,j,k;for(i=0;i<3;i++){double n=0;for(k=0;k<3;k++){if(!C1TS_Finite(b[i][k]))return 0;n+=(double)b[i][k]*b[i][k];}if(n<.999||n>1.001)return 0;for(j=0;j<i;j++){double d=0;for(k=0;k<3;k++)d+=(double)b[i][k]*b[j][k];if(d<-.001||d>.001)return 0;}}return 1;}
static int C1TS_PlanInner(const float basis[3][3],const float muzzle[3],const float target[3],float accuracy,float halfWidth,uint32_t flags,const c1tsRandom_t*random,c1tsPlan_t*out){c1tsPlan_t p;c1tbTrackedDirection_t t;unsigned i;double dot;
 if(!basis||!muzzle||!target||!random||!out||!C1TS_Basis(basis)||!C1TS_Finite(accuracy)||!C1TS_Finite(halfWidth)||accuracy<0||accuracy>1||halfWidth<0||halfWidth>1000||C1TB_Overlap(out,sizeof(*out),basis,36)||C1TB_Overlap(out,sizeof(*out),muzzle,12)||C1TB_Overlap(out,sizeof(*out),target,12)||C1TB_Overlap(out,sizeof(*out),random,sizeof(*random)))return 0;
 for(i=0;i<3;i++)if(!C1TS_Finite(muzzle[i])||!C1TS_Finite(target[i]))return 0;memset(&p,0,sizeof(p));p.nativeFlags=flags;p.random=*random;memcpy(p.forward,basis[0],12);
 if(flags&0x40u){float d[3];for(i=0;i<3;i++)d[i]=target[i]-muzzle[i];dot=(double)d[2]*basis[0][2]+(double)d[1]*basis[0][1]+(double)d[0]*basis[0][0];if(dot<=0)p.nativeFlags|=0x80u;else p.nativeFlags&=~0x80u;
  if(!C1TB_TrackedForward(basis[0],basis[2],muzzle,target,accuracy,halfWidth,&p.random.crt,&p.random.qrand,&t))return 0;p.tracked=1;p.hitBranch=t.hit_branch;p.corrected=t.applied;memcpy(p.forward,t.forward,12);
 }
 if(!C1TB_EndPoint(p.forward,basis[1],basis[2],muzzle,0,&p.random.crt,p.end))return 0;*out=p;return 1;
}
static int C1TS_Plan(const float basis[3][3],const float muzzle[3],const float target[3],float accuracy,float halfWidth,uint32_t flags,const c1tsRandom_t*random,c1tsPlan_t*out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1TS_PlanInner(basis,muzzle,target,accuracy,halfWidth,flags,random,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
#endif
