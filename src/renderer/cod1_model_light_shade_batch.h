/* Prepared immutable light records for one synchronous vertex batch. Only
 * input-invariant validation and unconditional arithmetic are hoisted. Vertex
 * transforms, attenuation, cone branch, exponent loop and rounding retain the
 * scalar operation order. Caller replays the scalar path on any refusal. */
#ifndef COD1_MODEL_LIGHT_SHADE_BATCH_H
#define COD1_MODEL_LIGHT_SHADE_BATCH_H
#include "cod1_model_light_shade.h"
#include "../qcommon/cod1_jobs.h"
typedef struct {
 const c1mLight *lights;const float *weights;int count,roundMode;
 float rgb[3],cutoff[16];
} c1mShadeBatch;
static int C1M_ShadeBatchPrepare(const c1mWorldCoefficients *world,float remainingSky,
 const c1mLight *lights,const float *weights,int count,int roundMode,
 const float sine[1024],c1mShadeBatch *out){
 int i,k,index;float phase;
 if(!world||!sine||!out||count<0||count>16||(count&&(!lights||!weights))||!C1M_Finite(remainingSky)||roundMode<0||roundMode>3)return 0;
 out->lights=lights;out->weights=weights;out->count=count;out->roundMode=roundMode;
 for(k=0;k<3;k++){
  if(!C1M_Finite(world->ambient[k])||!C1M_Finite(world->sky[k]))return 0;
  out->rgb[k]=C1M_ShadeStore((long double)world->sky[k]*remainingSky+world->ambient[k]);
 }
 for(i=0;i<count;i++){
  const c1mLight *light=lights+i;
  if(!C1M_Finite(weights[i])||!C1M_Finite(light->positionW))return 0;
  for(k=0;k<3;k++)if(!C1M_Finite(light->position[k])||!C1M_Finite(light->ambient[k])||!C1M_Finite(light->diffuse[k]))return 0;
  if(light->positionW!=0){
   for(k=0;k<3;k++)if(!C1M_Finite(light->attenuation[k]))return 0;
   if(!C1M_Finite(light->spotCutoff))return 0;
   if(light->spotCutoff!=180.0f){
    for(k=0;k<3;k++)if(!C1M_Finite(light->spotDirection[k]))return 0;
    phase=C1M_ShadeStore(((long double)light->spotCutoff+90.0L)*(long double)2.844444513320923f);
    if(!C1M_ShadeRound((long double)phase+0.000000000931322574615478515625L,roundMode,&index))return 0;
    index&=1023;if(!C1M_Finite(sine[index]))return 0;out->cutoff[i]=sine[index];
    /* Exponent arithmetic stays in the actual cone branch: ceil may set FP
     * flags even when this particular vertex would be outside the cone. */
    if(!C1M_Finite(light->spotExponent)||light->spotExponent>128)return 0;
   }
  }
 }
 return 1;
}
static int C1M_ShadeBatchVertex(const c1mShadeBatch *batch,const float vertex[3],const float normal[3],unsigned char outRGBA[4]){
 float rgb[3],dir[3],dot,distance,spot;long double factor,denominator;
 unsigned char result[4];int i,k,exponent;
 if(!batch||!vertex||!normal||!outRGBA)return 0;
 for(k=0;k<3;k++){
  if(!C1M_Finite(vertex[k])||!C1M_Finite(normal[k]))return 0;
  rgb[k]=batch->rgb[k];
 }
 for(i=0;i<batch->count;i++){
  const c1mLight *light=batch->lights+i;factor=(long double)batch->weights[i];
  if(light->positionW==0){
   dot=C1M_ShadeStore((long double)light->position[2]*normal[2]+(long double)light->position[0]*normal[0]+(long double)light->position[1]*normal[1]);
   if(dot<0)dot=0;
  }else{
   for(k=0;k<3;k++)dir[k]=C1M_ShadeStore((long double)light->position[k]-vertex[k]);
   distance=C1M_ShadeNormalize(dir);if(distance<0)return 0;
   dot=C1M_ShadeStore((long double)dir[2]*normal[2]+(long double)dir[1]*normal[1]+(long double)dir[0]*normal[0]);
   if(dot<0)dot=0;
   denominator=((long double)distance*light->attenuation[2]+light->attenuation[1])*distance+light->attenuation[0];
   if(denominator==0)return 0;factor=(long double)batch->weights[i]/denominator;
   if(light->spotCutoff!=180.0f){
    factor=C1M_ShadeStore(factor);
    spot=C1M_ShadeStore((long double)dir[2]*light->spotDirection[2]+(long double)dir[1]*light->spotDirection[1]+(long double)dir[0]*light->spotDirection[0]);
    if(spot<=batch->cutoff[i])factor=0;
    else{exponent=light->spotExponent>0?(int)ceill(light->spotExponent):0;for(k=0;k<exponent;k++)factor*=spot;}
   }
  }
  for(k=0;k<3;k++){
   rgb[k]=C1M_ShadeStore(((long double)dot*light->diffuse[k]+light->ambient[k])*factor+rgb[k]);
   if(!C1M_Finite(rgb[k]))return 0;
  }
 }
 for(k=0;k<3;k++)if(!C1M_ShadeByte(rgb[k],batch->roundMode,&result[k]))return 0;
 result[3]=255;memcpy(outRGBA,result,4);return 1;
}
#endif
