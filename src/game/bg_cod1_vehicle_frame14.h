#ifndef BG_COD1_VEHICLE_FRAME14_H
#define BG_COD1_VEHICLE_FRAME14_H
#include "bg_cod1_vehicle_controller14.h"
/* GAME 3F530/3F700 arithmetic, qualified C47 branch with no driver/turret,
 * health0 and no touch contacts. GAME must verify the world/topology domain.
 * Firstthink's two equal bone indices sample the same current matrix, so its
 * finite difference length is exactly zero. No bind pose is substituted for
 * a current animated pose. Firstthink does not advance the path. */
static int C1VF14_Frame(const C1V14PathNode*nodes,unsigned count,const C1V14Definition*d,
 const C1V14Record*input,unsigned mode,int first,float speed,C1V14Record*out,C1VC14Result*result){
 C1V14Record r,advanced;C1VC14Result v;unsigned i;
 if(!input||!out||!result||!d||d->type!=2||mode>1||(first!=0&&first!=1)||!isfinite(speed)||speed<0||
 C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),result,sizeof(*result))||
 C1V14_Overlap(d,sizeof(*d),out,sizeof(*out))||C1V14_Overlap(d,sizeof(*d),result,sizeof(*result))||
 C1V14_Overlap(nodes,count*sizeof(*nodes),out,sizeof(*out))||C1V14_Overlap(nodes,count*sizeof(*nodes),result,sizeof(*result))||C1V14_Overlap(out,sizeof(*out),result,sizeof(*result)))return 0;
 r=*input;memset(&v,0,sizeof(v));v.speed=speed;
 if(d->steerWheels||d->textureScroll||d->turretWeapon[0]||r.targetEntity!=2047||r.unknown188||r.values160[2])return 0;
 for(i=0;i<12;i++)if(r.values190[i])return 0;
 for(i=0;i<17;i++)if(r.values1d4[i])return 0;
 for(i=0;i<3;i++)if(!isfinite(r.origin[i])||!isfinite(r.angles[i]))return 0;
 memcpy(r.oldOrigin,r.origin,12);memcpy(r.oldAngles,r.angles,12);
 if(first)r.values160[4]=0;
 else{
  if(mode){if(!C1VC14_PathFrame(nodes,count,d,&r,speed,&advanced,&v))return 0;r=advanced;}
  r.values160[5]=0;
 }
 *out=r;*result=v;return 1;
}
#endif
