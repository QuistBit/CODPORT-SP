#ifndef CG_COD1_POSE_MATH_H
#define CG_COD1_POSE_MATH_H
/* Ordinary position accumulator: native CoD1 cgame 0x30029690.
 * All developer/cvar additive modifiers are explicit inputs, not guessed defaults.
 */
typedef struct {
 float move[3][3],offset[2][3],minSpeed[3],rate,proneRate;
} CodPoseDef;
typedef struct {float minSpeed,move[3],offset[3],rate;} CodPoseModifiers;
static float CP_Add(float a,float b){volatile float x=a+b;return x;}
static float CP_Mul(float a,float b){volatile float x=a*b;return x;}
static float CP_Div(float a,float b){volatile float x=a/b;return x;}
static void COD1_PoseAccumulate(float state[3],float output[3],const CodPoseDef*d,const CodPoseModifiers*m,
 int flags,int desiredHeight,float actualHeight,int duckHeight,int proneHeight,
 float speed,int baseSpeed,int weaponState,int dt,float ads,int angular){
 float threshold,target[3]={0,0,0},fraction=0,rate,change,minimum,scale;
 int i,stance=(flags&64)?2:(flags&32)?1:0;
 threshold=CP_Add(m->minSpeed,d->minSpeed[stance]);
 if(speed>threshold && weaponState!=5){
  fraction=CP_Div(CP_Add(speed,-threshold),CP_Add((float)baseSpeed,-threshold));
  if(fraction<0)fraction=0;if(fraction>1)fraction=1;
  for(i=0;i<3;i++)target[i]=CP_Add(CP_Mul(d->move[stance][i],fraction),CP_Mul(m->move[i],fraction));
 }
 stance=desiredHeight==duckHeight?1:desiredHeight==proneHeight?2:0;
 if(!angular && stance)for(i=0;i<3;i++)target[i]=CP_Add(CP_Add(target[i],d->offset[stance-1][i]),m->offset[i]);
 if(angular && ads!=0)for(i=0;i<3;i++)target[i]=CP_Mul(target[i],CP_Add(1,-ads));
 rate=CP_Add(m->rate,actualHeight==(float)proneHeight?d->proneRate:d->rate);
 for(i=0;i<3;i++)if(state[i]!=target[i]){
  change=CP_Mul(CP_Mul(CP_Mul(rate,CP_Add(target[i],-state[i])),(float)dt),.001f);
  if(state[i]<target[i]){
   minimum=CP_Mul((float)dt,.0001f);if(change<minimum)change=minimum;
   state[i]=CP_Add(state[i],change);if(state[i]>target[i])state[i]=target[i];
  }else{
   minimum=CP_Mul((float)dt,-.0001f);if(change>minimum)change=minimum;
   state[i]=CP_Add(state[i],change);if(state[i]<target[i])state[i]=target[i];
  }
 }
 scale=ads==0?1:ads<=.5f?CP_Add(1,-CP_Add(ads,ads)):0;
 for(i=0;i<3;i++)output[i]=CP_Add(output[i],CP_Mul(state[i],scale));
}
static void COD1_PosePosition(float*s,float*out,const CodPoseDef*d,const CodPoseModifiers*m,
 int flags,int desired,float actual,int duck,int prone,float speed,int base,int ws,int dt,float ads){
 COD1_PoseAccumulate(s,out,d,m,flags,desired,actual,duck,prone,speed,base,ws,dt,ads,0);
}
/* Native 0x30029ba0: same filter, separate thresholds/rates/rotational fields.
 * Offset fields are unused. ADS attenuates target AND later output. */
static void COD1_PoseRotation(float*s,float*out,const CodPoseDef*d,const CodPoseModifiers*m,
 int flags,int desired,float actual,int duck,int prone,float speed,int base,int ws,int dt,float ads){
 COD1_PoseAccumulate(s,out,d,m,flags,desired,actual,duck,prone,speed,base,ws,dt,ads,1);
}

#endif
