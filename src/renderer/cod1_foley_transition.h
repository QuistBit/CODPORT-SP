/* R197 selected Foley scene: pure time sampling, no shared playback cursor.
 * Both clips contain the same named local-rotation / bind-translation-offset
 * channels. Completion against each component bind remains in pose capture. */
#ifndef COD1_FOLEY_TRANSITION_H
#define COD1_FOLEY_TRANSITION_H
#include "cod1_actor_xanim14.h"

#define CF197_START_MS 1800
#define CF197_GESTURE_MS 2700
#define CF197_BLEND_MS 900
#define CF197_PLAY_MS (CF197_START_MS + CF197_BLEND_MS)
#define CF197_RETURN_MS (CF197_PLAY_MS + CF197_GESTURE_MS)
#define CF197_END_MS (CF197_RETURN_MS + CF197_BLEND_MS)
#define CF197_CYCLE_MS 9000 /* multiple of the 900ms breathing cycle */

static float CF197_Smooth(float t) { return t*t*(3.f-2.f*t); }

/* Shortest-arc spherical quaternion interpolation. Stage the whole pose so
 * a missing/duplicate track cannot leave a partially blended submission. */
static int CF197_Blend(const cv14Pose *a,const cv14Pose *b,float weight,
                        cv14Pose *out) {
    cv14Pose result;
    int i,j,k;
    float dot,length,sign,angle,sine,wa,wb;
    if(!a||!b||!out||a->n<1||a->n>CV14_BONES||a->n!=b->n||
       !(weight>=0.f&&weight<=1.f))return 0;
    result=*a;
    for(i=0;i<a->n;i++){
        if(CV14_Find(a,a->name[i])!=i)return 0;
        j=CV14_Find(b,a->name[i]);if(j<0)return 0;
        for(k=j+1;k<b->n;k++)if(!strcmp(b->name[k],a->name[i]))return 0;
        dot=length=0.f;
        for(k=0;k<4;k++)dot+=a->t[i].q[k]*b->t[j].q[k];
        sign=dot<0.f?-1.f:1.f;
        dot*=sign;if(dot>1.f)dot=1.f;
        wa=1.f-weight;wb=weight;
        if(dot<.9995f){
            angle=acosf(dot);sine=sinf(angle);
            wa=sinf((1.f-weight)*angle)/sine;wb=sinf(weight*angle)/sine;
        }
        for(k=0;k<3;k++)result.t[i].p[k]=a->t[i].p[k]*(1.f-weight)+b->t[j].p[k]*weight;
        for(k=0;k<4;k++){
            result.t[i].q[k]=a->t[i].q[k]*wa+sign*b->t[j].q[k]*wb;
            length+=result.t[i].q[k]*result.t[i].q[k];
        }
        if(!(length>1.e-12f&&length<4.01f))return 0;
        length=sqrtf(length);
        for(k=0;k<4;k++)result.t[i].q[k]/=length;
    }
    *out=result;
    return 1;
}

static int CF197_Sample(const ca14ActorClip *breath,const ca14ActorClip *gesture,
                         int elapsedMs,cv14Pose *out) {
    cv14Pose a,b;
    int phase,gestureMs;
    float weight;
    if(!breath||!gesture||!out||elapsedMs<0||
       breath->frames!=27||breath->fps!=30||breath->originalFlags!=3||
       gesture->frames!=82||gesture->fps!=30||gesture->originalFlags!=2||
       breath->rootRotationKeys||breath->rootTranslationKeys||
       gesture->rootRotationKeys||gesture->rootTranslationKeys)return 0;
    phase=elapsedMs%CF197_CYCLE_MS;
    if(!CA14_ActorClipSample(breath,elapsedMs,&a))return 0;
    if(phase<=CF197_START_MS||phase>=CF197_END_MS){*out=a;return 1;}
    /* Fixed fade endpoints avoid a shortest-arc branch change as a moving
     * target crosses 180 degrees. Both breathing endpoints are cycle phase0.
     * The nonlooping gesture starts only after the entry fade finishes. */
    gestureMs=phase-CF197_PLAY_MS;
    if(gestureMs<0)gestureMs=0;
    if(gestureMs>CF197_GESTURE_MS)gestureMs=CF197_GESTURE_MS;
    if(!CA14_ActorClipSample(gesture,gestureMs,&b))return 0;
    weight=1.f;
    if(phase<CF197_PLAY_MS){
        if(!CA14_ActorClipSample(breath,0,&a))return 0;
        weight=CF197_Smooth((float)(phase-CF197_START_MS)/CF197_BLEND_MS);
    }else if(phase>CF197_RETURN_MS){
        if(!CA14_ActorClipSample(breath,0,&a))return 0;
        weight=1.f-CF197_Smooth((float)(phase-CF197_RETURN_MS)/CF197_BLEND_MS);
    }
    return CF197_Blend(&a,&b,weight,out);
}
#endif
