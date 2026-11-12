/* R72 proposal: model-center authored-light candidates. Native static evidence:
 * CoDSP SP 4aacb0..4ab48f; leaf translation 4d1662..4d16e0.
 * This is a visibility/list producer, not RGB shading or per-vertex sampling.
 * Original executable and reconstructed native image must not be executed. */
#ifndef COD1_MODEL_LIGHT_CANDIDATES_H
#define COD1_MODEL_LIGHT_CANDIDATES_H
#include "cod1_light_visibility.h"

typedef struct {
    int count, hadSky, outsideLeaf, unavailable;
    unsigned int indices[15];
    float weights[15], remainingSky, coverage;
    unsigned int diskSamples, runtimeSamples, rejectedSamples;
} c1ModelLightCandidates;

/* Boolean success. Zero means unsupported/unavailable input; output remains
 * zero except unavailable. Explicit rejected samples are excluded and retain
 * native partial-coverage behavior. Missing collision service is not rejection.
 * Valid no-light leaves are successful zero-count lists. */
static int C1M_Candidates(const c1LightCache *cache,const float *point,
                         unsigned int numLights,int sunIndex,int quality,
                         const float *sunDir,c1VisibilityRuntime *rt,
                         c1VisibilityTrace trace,c1VisibilityContents contents,
                         void *context,c1ModelLightCandidates *output) {
    c1ModelLightCandidates result;
    const unsigned char *leaf;
    float fractions[3][2], skyFactor;
    int l, first, count, i, j, lo[3], sunBit=-1;
    if(!output)return 0;
    memset(output,0,sizeof(*output));
    memset(&result,0,sizeof(result));
    if(!cache || !point || !cache->planes || !cache->nodes ||
       !cache->leaves || !cache->refs || !cache->slots ||
       quality<1 || quality>5)return 0;
    l=C1_Leaf(cache,point);
    if(l<0 || l>=cache->nl)return 0;
    leaf=cache->leaves+l*36;
    /* Native negative-cluster special case: a known world sun supplies both
     * candidate weight and sky scalar one; otherwise return no candidates. */
    if(C1_I32(leaf)<0){
        result.outsideLeaf=1;
        if(sunIndex>=0 && (unsigned int)sunIndex<numLights){
            result.count=1;result.indices[0]=(unsigned int)sunIndex;
            result.weights[0]=result.remainingSky=1.0f;
        }
        *output=result;return 1;
    }
    first=C1_I32(leaf+28);count=C1_I32(leaf+32);
    if(first<0 || count<0 || first>cache->nr || count>cache->nr-first)return 0;
    if(count && (cache->refs[first*2+1]&128u)){
        result.hadSky=1;first++;count--;
    }
    /* Native leaf loader explicitly rejects a count above 15 after removing
     * the signed negative sky marker. Bit 15 belongs to sky ray visibility. */
    if(count>15)return 0;
    result.count=count;
    for(i=0;i<count;i++){
        unsigned int index=cache->refs[(first+i)*2] |
                          ((unsigned int)cache->refs[(first+i)*2+1]<<8);
        if(index>=numLights)return 0;
        result.indices[i]=index;
        if(sunIndex>=0 && index==(unsigned int)sunIndex)sunBit=i;
    }
    if(!count && !result.hadSky){*output=result;return 1;}
    for(i=0;i<3;i++){
        float shifted=C1V_Store((double)point[i]+131072.0);
        double spacing=i==2?0.015625:0.03125;
        lo[i]=((int)floor((double)shifted))>>(i==2?6:5);
        fractions[i][1]=C1V_Store((double)shifted*spacing-lo[i]);
        fractions[i][0]=C1V_Store(1.0-(double)fractions[i][1]);
    }
    skyFactor=C1V_Store(0.5/(double)(quality*quality));
    for(i=0;i<8;i++){
        unsigned int grid[3], mask, skyCount;
        const unsigned char *disk;
        const c1VisibilityRecord *runtime;
        double product;
        float weight;
        for(j=0;j<3;j++)grid[j]=(unsigned int)(lo[j]+((i>>j)&1));
        product=(double)fractions[1][(i>>1)&1] *
                fractions[2][(i>>2)&1] * fractions[0][i&1];
        weight=C1V_Store(product);
        /* Native visits all eight corners, including zero-weight corners. */
        disk=C1_Lookup(cache,grid[0],grid[1],grid[2],C1_U32(leaf));
        if(disk){
            if(disk[4]!=1){result.rejectedSamples++;continue;}
            mask=(unsigned int)disk[10]|((unsigned int)disk[11]<<8);
            skyCount=disk[4+quality];result.diskSamples++;
        }else{
            if(!rt || !trace || !contents || !sunDir || rt->quality!=quality){
                output->unavailable=1;return 0;
            }
            runtime=C1V_Record(rt,cache,grid,C1_U32(leaf),point,sunDir,
                              sunBit,trace,contents,context);
            if(!runtime){output->unavailable=1;return 0;}
            if(runtime->state!=1){result.rejectedSamples++;continue;}
            mask=runtime->mask;skyCount=runtime->sky;result.runtimeSamples++;
        }
        result.coverage=C1V_Store((double)result.coverage+product);
        result.remainingSky=C1V_Store((double)result.remainingSky+
                      (double)skyCount*weight*skyFactor);
        for(j=0;j<count;j++)if(mask&(1u<<j))
            result.weights[j]=C1V_Store((double)result.weights[j]+weight);
    }
    /* Original threshold is float bits 0x3f7ae148 = .9800000190734863.
     * Near-complete coverage is deliberately left unnormalized. */
    if(result.coverage<0.9800000190734863f && result.coverage>0.0f){
        float inverse=C1V_Store(1.0/(double)result.coverage);
        if(result.remainingSky!=0.0f)
            result.remainingSky=C1V_Store((double)result.remainingSky*inverse);
        for(i=0;i<count;i++)if(result.weights[i]!=0.0f){
            if(sunIndex>=0 && result.indices[i]==(unsigned int)sunIndex)
                result.weights[i]=C1V_Store(1.0-(double)result.coverage+
                                          result.weights[i]);
            else result.weights[i]=C1V_Store((double)result.weights[i]*inverse);
        }
    }
    /* Native authored sky-marker rule, 4ab45c..4ab47d; this is separate from
     * world ambient and must not be applied to an unavailable service. */
    if(result.hadSky && result.remainingSky<0.25f)result.remainingSky=0.25f;
    *output=result;return 1;
}
#endif
