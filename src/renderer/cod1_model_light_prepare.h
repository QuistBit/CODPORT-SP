/* R72 research proposal: finite native model prepared-light selector.
 * Input records use the shared recovered runtime layout. Candidate visibility,
 * scene-list ownership, decoding and final GL-equivalent shading are separate.
 * No native executable is executed; no x87/GPU pixel parity is claimed. */
#ifndef COD1_MODEL_LIGHT_PREPARE_H
#define COD1_MODEL_LIGHT_PREPARE_H
#include "cod1_model_light_records.h"
#include <limits.h>

static void C1M_DefaultPrepareSettings(c1mPrepareSettings *s) {
    s->minIntensity=0.02f;s->tailCutoff=0.2f;s->maxLights=8;
    s->diffuseSunQuality=2;s->hardwareMaxLights=8;s->reduceTail=1;
}
static int C1M_PrepareBudget(const c1mPrepareSettings *s) {
    int quality=s->diffuseSunQuality,cap=s->hardwareMaxLights,n=s->maxLights;
    if(quality<0)quality=0;if(quality>2)quality=2;
    if(cap<=0)cap=8;if(cap>16)cap=16;
    if(n<quality+1)n=quality+1;if(n>cap)n=cap;return n;
}
static int C1M_PrepareNormalize(float v[3]) {
    double length=sqrt((double)v[2]*v[2]+(double)v[1]*v[1]+(double)v[0]*v[0]);int k;
    if(length==0)return 1;
    for(k=0;k<3;k++){v[k]=C1M_Store((double)v[k]/length);if(!C1M_Finite(v[k]))return 0;}
    return 1;
}
/* Native scene append at4ab503..558 includes the radius boundary. Visibility
 * weight is1. Its scene producer/list lifetime must be supplied by the caller. */
static int C1M_DynamicEligible(const c1mLight *light,const float center[3]) {
    double d[3],distanceSquared,radiusSquared;int k;
    if(!light || !center || !C1M_Finite(light->radius))return 0;
    for(k=0;k<3;k++){
        if(!C1M_Finite(center[k]) || !C1M_Finite(light->position[k]))return 0;
        d[k]=C1M_Store((double)center[k]-light->position[k]);
    }
    distanceSquared=d[2]*d[2]+d[1]*d[1]+d[0]*d[0];
    radiusSquared=(double)light->radius*light->radius*4.0;
    return radiusSquared>=distanceSquared;
}
static int C1M_PrepareAttenuation(const c1mLight *light,const float center[3],
                                 int overbrightBits,double *value) {
    double denominator=1,d[3],d2=0,scale;int k;
    if(overbrightBits<0 || overbrightBits>30 || !C1M_Finite(light->scalar) ||
       !C1M_Finite(light->positionW))return 0;
    scale=(double)(1U<<overbrightBits);
    if(light->positionW!=0){
        for(k=0;k<3;k++)if(!C1M_Finite(light->attenuation[k]))return 0;
        denominator=light->attenuation[0];
        if(light->attenuation[1]!=0 || light->attenuation[2]!=0){
            for(k=0;k<3;k++){
                if(!C1M_Finite(light->position[k]))return 0;
                d[k]=(double)light->position[k]-center[k];
            }
            d2=d[2]*d[2]+d[1]*d[1]+d[0]*d[0];
            denominator+=d2*light->attenuation[2];
            if(light->attenuation[1]!=0)denominator+=sqrt(d2)*light->attenuation[1];
        }
        if(denominator==0)return 0;
    }
    *value=(double)light->scalar*scale/denominator;
    return *value>=-DBL_MAX && *value<=DBL_MAX;
}
static int C1M_PrepareInsert(c1mPrepared *out,const c1mLight *light,float weight,
                            int sourceIndex,const float center[3],float remainingSky,
                            float skyScalar,int overbrightBits,float threshold) {
    static const float priority=9.999999980506448e18f;
    double attenuation;float score;int position=0,j;
    if(!C1M_Finite(weight) || !C1M_Finite(light->scalar))return 0;
    if(weight==0)return 1;
    if(light->scalar<0)score=priority;
    else{
        if(!C1M_PrepareAttenuation(light,center,overbrightBits,&attenuation))return 0;
        score=C1M_Store(attenuation*weight);
        if(light->type==1 || light->type==8){
            score=C1M_Store((double)(1U<<overbrightBits)*skyScalar*remainingSky+score);
        }
        if(!C1M_Finite(score))return 0;
        if(score<threshold)return 1;
    }
    while(position<out->count && out->scores[position]>score)position++;
    if(position>=C1M_PREPARED_LIMIT)return 1;
    if(out->count==C1M_PREPARED_LIMIT)out->count--;
    for(j=out->count;j>position;j--){
        out->lights[j]=out->lights[j-1];out->weights[j]=out->weights[j-1];
        out->scores[j]=out->scores[j-1];out->sourceIndices[j]=out->sourceIndices[j-1];
    }
    out->lights[position]=*light;out->weights[position]=weight;
    out->scores[position]=score;out->sourceIndices[position]=sourceIndex;out->count++;
    return 1;
}
/* Synthetic type8 records copy world sky RGB into runtime+04 fields literally;
 * unlike authored decoded records, that group is not luminance-normalized. */
static int C1M_PrepareSkyLight(c1mLight *light,const float sky[3],float scalar,
                             int profile) {
    float ambientFactor=profile==0?0.5f:profile==1?0.75f:0;
    float diffuseFactor=profile==0?0.5f:profile==1?0.25f:-0.25f;int k;
    memset(light,0,sizeof(*light));light->type=8;light->spotCutoff=180;
    light->position[2]=profile==2?-1.0f:1.0f;
    light->scalar=profile==2?C1M_Store((double)scalar*(-0.25)):scalar;
    for(k=0;k<3;k++){
        light->normalizedRGB[k]=sky[k];
        light->ambient[k]=C1M_Store((double)sky[k]*ambientFactor);
        light->diffuse[k]=C1M_Store((double)sky[k]*diffuseFactor);
        if(!C1M_Finite(light->ambient[k]) || !C1M_Finite(light->diffuse[k]))return 0;
    }
    return C1M_Finite(light->scalar);
}
static int C1M_PrepareReduce(c1mPrepared *out,const float center[3],float skyScalar,
                            float identityLight,int overbrightBits,float cutoffRatio) {
    static const float priority=9.999999980506448e18f;
    c1mLight aggregate;float directions[8][3],direction[3]={0,0,0},cutoff;
    int prefix,i,k,originalCount=out->count;
    if(out->count<=out->budget)return 1;
    prefix=out->scores[0]==priority?1:0;
    cutoff=C1M_Store((double)out->scores[prefix]*cutoffRatio);
    if(!C1M_Finite(cutoff))return 0;prefix++;
    while(prefix<out->budget-1 && out->scores[prefix]>cutoff)prefix++;
    memset(&aggregate,0,sizeof(aggregate)); /* type0,scalar0,w0: recovered init */
    aggregate.attenuation[0]=1;aggregate.spotCutoff=180;
    for(i=prefix;i<originalCount;i++){
        const c1mLight *light=&out->lights[i];
        for(k=0;k<3;k++){
            if(!C1M_Finite(light->position[k]))return 0;
            directions[i][k]=light->positionW==0?light->position[k]:
                             C1M_Store((double)light->position[k]-center[k]);
        }
        if(light->positionW!=0){if(!C1M_PrepareNormalize(directions[i]))return 0;}
        else if(light->type==1 || light->type==8){
            out->scores[i]=C1M_Store((double)out->scores[i]-
                                  (double)(1U<<overbrightBits)*skyScalar*out->remainingSky);
        }
        for(k=0;k<3;k++){
            direction[k]=C1M_Store((double)out->scores[i]*directions[i][k]+direction[k]);
            if(!C1M_Finite(direction[k]))return 0;
        }
    }
    if(!C1M_PrepareNormalize(direction))return 0;
    for(i=prefix;i<originalCount;i++){
        const c1mLight *light=&out->lights[i];
        double d=(double)direction[0]*directions[i][0]+
                 (double)direction[2]*directions[i][2]+
                 (double)direction[1]*directions[i][1];
        double diffuseCoefficient,ambientCoefficient;
        if(d<=0){diffuseCoefficient=0;ambientCoefficient=0.5;}
        else{
            /*4aab91 tests candidate1;4aab96 aggregate8, fresh aggregate0. */
            diffuseCoefficient=d*(light->type==1?1.0:0.800000011920929);
            ambientCoefficient=(1.0-diffuseCoefficient)*0.5;
        }
        for(k=0;k<3;k++){
            double energy=(double)identityLight*out->scores[i]*light->normalizedRGB[k];
            aggregate.diffuse[k]=C1M_Store(energy*diffuseCoefficient+aggregate.diffuse[k]);
            aggregate.ambient[k]=C1M_Store(energy*ambientCoefficient+aggregate.ambient[k]);
            if(!C1M_Finite(aggregate.diffuse[k]) || !C1M_Finite(aggregate.ambient[k]))return 0;
        }
    }
    memcpy(aggregate.position,direction,sizeof(direction));
    out->lights[prefix]=aggregate;out->weights[prefix]=1;
    out->scores[prefix]=0;out->sourceIndices[prefix]=-1;
    out->aggregatedTailCount=originalCount-prefix;out->count=prefix+1;
    /* The native prefix rule can exceed budgets1/2 in edge settings. Preserve
     * that rule; do not silently truncate. Count stays within selected8. */
    return 1;
}
static int C1M_Prepare(const c1mLight *const *candidates,const float *weights,int count,
                       const float center[3],float remainingSky,
                       const c1mWorldCoefficients *world,float identityLight,
                       int overbrightBits,const c1mPrepareSettings *settings,
                       c1mPrepared *published) {
    c1mPrepared out;c1mLight skyLight;c1mPrepareSettings defaults;
    int quality,skyCount=0,i,k,profile;
    if(!published || !center || !world || count<0 || count>INT_MAX-2 ||
       (count && (!candidates || !weights)) || !C1M_Finite(remainingSky) ||
       !C1M_Finite(identityLight) || identityLight<=0 || !C1M_Finite(world->skyLuminance) ||
       overbrightBits<0 || overbrightBits>30)return 0;
    if(!settings){C1M_DefaultPrepareSettings(&defaults);settings=&defaults;}
    if(!C1M_Finite(settings->minIntensity) || !C1M_Finite(settings->tailCutoff))return 0;
    for(k=0;k<3;k++)if(!C1M_Finite(center[k]) || !C1M_Finite(world->sky[k]))return 0;
    memset(&out,0,sizeof(out));out.budget=C1M_PrepareBudget(settings);
    quality=settings->diffuseSunQuality;if(quality<0)quality=0;if(quality>2)quality=2;
    if(remainingSky!=0 && world->skyLuminance!=0 && quality!=0){
        skyCount=(quality==2 && count<=out.budget)?2:1;out.remainingSky=0;
    }else out.remainingSky=remainingSky;
    out.syntheticSkyCount=skyCount;
    for(i=0;i<count;i++){
        if(!candidates[i] || !C1M_PrepareInsert(&out,candidates[i],weights[i],i,center,
             out.remainingSky,world->skyLuminance,overbrightBits,settings->minIntensity))return 0;
    }
    for(i=0;i<skyCount;i++){
        profile=skyCount==1?0:i+1;
        if(!C1M_PrepareSkyLight(&skyLight,world->sky,world->skyLuminance,profile) ||
           !C1M_PrepareInsert(&out,&skyLight,remainingSky,count+i,center,0,
                              world->skyLuminance,overbrightBits,settings->minIntensity))return 0;
    }
    if(settings->reduceTail && !C1M_PrepareReduce(&out,center,world->skyLuminance,
                           identityLight,overbrightBits,settings->tailCutoff))return 0;
    *published=out;return 1;
}
#endif
