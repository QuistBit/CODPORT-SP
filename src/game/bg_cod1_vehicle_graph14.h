#ifndef BG_COD1_VEHICLE_GRAPH14_H
#define BG_COD1_VEHICLE_GRAPH14_H
#include "bg_cod1_vehicle14.h"
/* GAME47F20,46FB0,47150,472F0. The caller supplies constructor records,
 * stable interned string identities and separate staging/output storage.
 * Preparation is deliberately ordered: later nodes see earlier results. */
static int C1VG14_DefaultAngles(const float*a){return a[0]==3.1415927410125732f&&a[1]==3.1415927410125732f&&a[2]==3.1415927410125732f;}
static double C1VG14_Atan2(double y,double x){double r;
 __asm fld y
 __asm fld x
 __asm fpatan
 __asm fstp r
 return r;
}
static float C1VG14_Normalize(float*v){
 float sum=(float)((double)v[0]*v[0]+(double)v[1]*v[1]+(double)v[2]*v[2]);
 float length=(float)sqrt((double)sum);unsigned i;
 if(length!=0){float inv=(float)(1.0/(double)length);for(i=0;i<3;i++)v[i]=(float)((double)v[i]*inv);}return length;
}
static void C1VG14_VectorAngles(const float*v,float*a){
 float yaw,pitch,horizontal;
 if(v[0]==0&&v[1]==0){yaw=0;pitch=v[2]>0?270.0f:90.0f;}
 else{
  yaw=(float)(C1VG14_Atan2(v[1],v[0])*180.0/3.14159265358979323846);if(yaw<0)yaw=(float)((double)yaw+360.0);
  horizontal=(float)sqrt((double)v[0]*v[0]+(double)v[1]*v[1]);
  pitch=(float)(C1VG14_Atan2(v[2],horizontal)*-180.0/3.14159265358979323846);if(pitch<0)pitch=(float)((double)pitch+360.0);
 }a[0]=pitch;a[1]=yaw;a[2]=0;
}
static float C1VG14_Value(const C1V14PathNode*n,int look){return look?n->lookahead:n->speed;}
static float C1VG14_InheritValue(C1V14PathNode*n,unsigned count,unsigned id,int look){
 double beforeDistance=0,afterDistance=0,total;float before=-1,after=-1,storedBefore;unsigned steps;int at=n[id].previous;
 float value=C1VG14_Value(n+id,look);if(value>=0)return value;
 for(steps=0;at>=0&&steps<count;steps++){
  beforeDistance+=n[at].distance;value=C1VG14_Value(n+at,look);
  if(look?value>0:value>=0){before=value;break;}at=n[at].previous;if(at==(int)id)break;
 }
 storedBefore=(float)beforeDistance;at=(int)id;
 for(steps=0;steps<count;steps++){
  value=C1VG14_Value(n+at,look);if(look?value>0:value>=0){after=value;break;}
  {int next=n[at].next;if(next<0||next==(int)id)break;afterDistance+=n[at].distance;at=next;}
 }
 if(before<0&&after<0)return 0;if(before<0)return after;if(after<0)return before;
 total=beforeDistance+afterDistance;if(total<=0)return 0;
 return (float)(((double)after-before)*((double)storedBefore/total)+before);
}
static void C1VG14_InheritAngles(C1V14PathNode*n,unsigned count,unsigned id){
 float before[3],after[3],beforeDistance=0,afterDistance=0,sum,t;int hasBefore=0,hasAfter=0,at;unsigned steps,k;
 if(!C1VG14_DefaultAngles(n[id].angles))return;at=n[id].previous;
 for(steps=0;at>=0&&steps<count;steps++){
  beforeDistance=(float)((double)beforeDistance+n[at].distance);
  if(!C1VG14_DefaultAngles(n[at].angles)){memcpy(before,n[at].angles,12);hasBefore=1;break;}
  at=n[at].previous;if(at==(int)id)break;
 }
 at=(int)id;
 for(steps=0;steps<count;steps++){
  if(!C1VG14_DefaultAngles(n[at].angles)){memcpy(after,n[at].angles,12);hasAfter=1;break;}
  {int next=n[at].next;if(next<0||next==(int)id)break;afterDistance=(float)((double)afterDistance+n[at].distance);at=next;}
 }
 if(!hasBefore&&!hasAfter){memset(n[id].angles,0,12);return;}
 if(!hasBefore){memcpy(n[id].angles,after,12);return;}if(!hasAfter){memcpy(n[id].angles,before,12);return;}
 sum=(float)((double)beforeDistance+afterDistance);if(sum<=0){memset(n[id].angles,0,12);return;}t=(float)((double)beforeDistance/sum);
 for(k=0;k<3;k++){
  float adjusted=after[k];if((double)adjusted-before[k]>180)adjusted=(float)((double)adjusted-360);
  if((double)adjusted-before[k]<-180)adjusted=(float)((double)adjusted+360);
  n[id].angles[k]=(float)(((double)adjusted-before[k])*t+before[k]);
 }
}
static int C1VG14_Wrap(float input,float*out){double scaled=(double)input*182.0444488525390625;float value;
 if(!isfinite(scaled)||scaled<=-2147483648.0||scaled>=2147483648.0)return 0;
 value=(float)((double)((int)scaled&65535)*0.0054931640625);if(value>180)value=(float)((double)value-360);*out=value;return 1;
}
__declspec(noinline) static int C1VG14_PrepareBody(C1V14PathNode*n,unsigned count){
 unsigned i,j,k;for(i=0;i<count;i++){
  n[i].next=n[i].previous=-1;
  if(n[i].target)for(j=0;j<count;j++)if(n[j].targetname==n[i].target){n[i].next=(int16_t)j;break;}
  for(j=0;j<count;j++)if(i!=j&&n[j].target==n[i].targetname){n[i].previous=(int16_t)j;break;}
  if(n[i].next==(int)i)n[i].next=-1;
 }
 for(i=0;i<count;i++)if(n[i].next>=0){
  for(k=0;k<3;k++)n[i].direction[k]=(float)((double)n[n[i].next].origin[k]-n[i].origin[k]);
  n[i].distance=C1VG14_Normalize(n[i].direction);if(!isfinite(n[i].distance))return 0;
  if(!n[i].rotate)C1VG14_VectorAngles(n[i].direction,n[i].angles);
 }
 for(i=0;i<count;i++){
  n[i].speed=C1VG14_InheritValue(n,count,i,0);n[i].lookahead=C1VG14_InheritValue(n,count,i,1);
  if(!isfinite(n[i].speed)||n[i].speed<0||!isfinite(n[i].lookahead))return 0;
  if(n[i].rotate)C1VG14_InheritAngles(n,count,i);
  for(k=0;k<3;k++)if(!C1VG14_Wrap(n[i].angles[k],n[i].angles+k))return 0;
  if(n[i].speed<=0||n[i].lookahead<=0)n[i].next=-1;
  if(n[i].next<0){if(n[i].speed<=0)n[i].speed=1;if(n[i].lookahead<=0)n[i].lookahead=1;}
 }return 1;
}
/* No callbacks inside arithmetic; preserve all caller x87/SSE state. Output
 * remains untouched on malformed input or an unrepresentable finite result. */
static int C1VG14_Prepare(const C1V14PathNode*input,unsigned count,C1V14PathNode*scratch,C1V14PathNode*out){
 unsigned i,k;int ok;size_t bytes=(size_t)count*sizeof(*input);unsigned char env[108];uint32_t mx,nearMx=0x1f80;unsigned short cw=0x027f;
 if(count>1500||(!input&&count)||(!scratch&&count)||(!out&&count)||C1V14_Overlap(input,bytes,scratch,bytes)||C1V14_Overlap(input,bytes,out,bytes)||C1V14_Overlap(scratch,bytes,out,bytes))return 0;
 for(i=0;i<count;i++){
  if(!input[i].targetname||input[i].id!=(int)i||!isfinite(input[i].speed)||!isfinite(input[i].lookahead))return 0;
  for(k=0;k<3;k++)if(!isfinite(input[i].origin[k])||!isfinite(input[i].angles[k]))return 0;
 }
 if(!count)return 1;memcpy(scratch,input,bytes);
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1VG14_PrepareBody(scratch,count);
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)memcpy(out,scratch,bytes);return ok;
}
#endif
