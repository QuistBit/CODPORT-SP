#ifndef BG_COD1_VEHICLE_METHODS14_H
#define BG_COD1_VEHICLE_METHODS14_H
/* Original GAME 3FCA0/3FE40/3FF60/3FFE0/40050/40100 copied state.
 * C47 plane branch; GAME owns publication, wheel queries and node leases.
 * This component never publishes a world pose or delivers a notification. */
#include "bg_cod1_vehicle_path14.h"
enum {C1VM14_ATTACH=1,C1VM14_START,C1VM14_WAIT_NODE,C1VM14_WAIT_SPEED,C1VM14_SPEED,C1VM14_RESUME};
static int C1VM14_Apply(const C1V14Record*input,unsigned mode,unsigned kind,
 const C1V14PathNode*nodes,unsigned count,int index,float first,float second,
 C1V14Record*out,unsigned*outMode){
 C1V14Record r;C1VP14State path,prior;unsigned resultMode=mode;int ok=1;
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;unsigned short cw=0x027f;
 if(!input||!out||!outMode||mode>1||kind<1||kind>6||
 C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),outMode,sizeof(*outMode))||
 C1V14_Overlap(out,sizeof(*out),outMode,sizeof(*outMode))||count>1500||
 C1V14_Overlap(nodes,count*sizeof(*nodes),out,sizeof(*out))||C1V14_Overlap(nodes,count*sizeof(*nodes),outMode,sizeof(*outMode)))return 0;
 r=*input;
 if(kind==C1VM14_ATTACH){
  memcpy(&prior,&r,sizeof(prior));if(!C1VP14_Init(nodes,count,index,&prior,&path))return 0;
  memcpy(&r,&path,sizeof(path));memcpy(r.origin,path.origin,12);memcpy(r.angles,path.angles,12);
  memcpy(r.oldOrigin,r.origin,12);memcpy(r.oldAngles,r.angles,12);
 }else if(kind==C1VM14_START){if(r.nodeIndex<0||r.nodeIndex>=(int)count)return 0;resultMode=1;}
 else if(kind==C1VM14_WAIT_NODE){if(index<0||index>=(int)count)return 0;r.waitNode=(int16_t)index;}
 else {
  if(!isfinite(first)||first<0||(kind==C1VM14_SPEED&&(!isfinite(second)||second<0)))return 0;
  __asm fnsave env
  __asm stmxcsr mx
  __asm fldcw cw
  __asm ldmxcsr nearMx
  first=(float)((double)first*17.600000381469727);
  if(kind==C1VM14_WAIT_SPEED)r.waitSpeed=first;
  else if(kind==C1VM14_SPEED){r.speedMode=1;r.targetSpeed=first;r.speedAccel=(float)((double)second*17.600000381469727);ok=isfinite(r.speedAccel);}
  else{r.speedMode=2;r.speedAccel=first;}
  ok=ok&&isfinite(first);
  __asm ldmxcsr mx
  __asm frstor env
  if(!ok)return 0;
 }
 *out=r;*outMode=resultMode;return 1;
}
#endif
