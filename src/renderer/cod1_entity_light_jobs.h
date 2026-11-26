/* Renderer-private vertex math with a caller-owned immutable settings
 * snapshot. The including unit provides the existing renderer vertex/entity
 * types and, when enabled, the same four-vertex SIMD helper as before. */
#ifndef COD1_ENTITY_LIGHT_JOBS_H
#define COD1_ENTITY_LIGHT_JOBS_H
#include "cod1_model_light_shade_batch.h"
#include "../qcommon/cod1_jobs.h"
typedef struct {
    const float *sine;
    int valid,fullbright,roundMode;
    byte identity;
} cod1EntityLightInput_t;
static qboolean R_Cod1ShadeEntityVerticesInput(const trRefEntity_t *ent,int count,
        const vec4_t *xyz,const vec4_t *normal,byte *rgba,const cod1EntityLightInput_t *input) {
    float world[3],n[3],cross[3][3],det;int i,j,v,roundMode,fullbright;
    const c1mModelLightFrame *frame;c1mShadeBatch batch={0};c1jFp fp;int useBatch=0;
#ifdef C1Q347_ENABLED
    int quadSeparate=0;
#endif
    if(count==0)return qtrue;
    if(count<0 || !ent || !xyz || !normal || !rgba ||
       !ent->cod1ModelLighting.valid || !input->valid)return qfalse;
    frame=&ent->cod1ModelLighting;
    CrossProduct(ent->e.axis[1],ent->e.axis[2],cross[0]);
    CrossProduct(ent->e.axis[2],ent->e.axis[0],cross[1]);
    CrossProduct(ent->e.axis[0],ent->e.axis[1],cross[2]);
    det=DotProduct(ent->e.axis[0],cross[0]);if(det==0 || !C1M_Finite(det))return qfalse;
    fullbright=input->fullbright;
    roundMode=input->roundMode;
    if(!fullbright){
        C1J_FpGet(&fp);
        /* Hoisting is valid with masked exceptions. Otherwise preserve the
         * scalar trap order, including branches whose light inputs go unread. */
        if((fp.control&63)==63&&(fp.mxcsr&0x1f80)==0x1f80){
            if(!C1M_ShadeBatchPrepare(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,frame->prepared.weights,frame->prepared.count,roundMode,input->sine,&batch))return qfalse;
            useBatch=1;
        }
    }
#ifdef C1Q347_ENABLED
    if(useBatch&&count<=SHADER_MAX_VERTEXES)
        quadSeparate=C1Q347_Separate(rgba,(size_t)count*4,xyz,(size_t)count*sizeof(*xyz))&&
                     C1Q347_Separate(rgba,(size_t)count*4,normal,(size_t)count*sizeof(*normal))&&
                     C1Q347_Separate(rgba,(size_t)count*4,ent,sizeof(*ent));
#endif
    for(v=0;v<count;v++) {
#ifdef C1Q347_ENABLED
        if(quadSeparate&&count-v>=4){
            c1jFp prior;C1J_FpGet(&prior);
            if(C1Q347_Entity(&batch,ent->e.origin,ent->e.axis,cross,det,xyz+v,normal+v,rgba+(size_t)v*4)){v+=3;continue;}
            C1J_FpRestore(&prior,0,0);
        }
#endif
        byte *color=rgba+(size_t)v*4;
        for(i=0;i<3;i++) {
            world[i]=ent->e.origin[i];n[i]=0;
            for(j=0;j<3;j++){world[i]+=xyz[v][j]*ent->e.axis[j][i];n[i]+=normal[v][j]*cross[j][i]/det;}
        }
        if(C1M_ShadeNormalize(n)<0)return qfalse;
        if(fullbright) {
            color[0]=color[1]=color[2]=input->identity;color[3]=255;
        } else if(useBatch){
            if(!C1M_ShadeBatchVertex(&batch,world,n,color))return qfalse;
        } else if(!C1M_Shade(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,
            frame->prepared.weights,frame->prepared.count,world,n,roundMode,
            input->sine,color))return qfalse;
    }
    return qtrue;
}
typedef struct {
    const trRefEntity_t *ent;
    const vec4_t *xyz,*normal;
    byte *rgba;
    cod1EntityLightInput_t input;
} cod1EntityLightJob_t;
static int R_Cod1EntityLightRange(void *context,unsigned first,unsigned end) {
    const cod1EntityLightJob_t *job=(const cod1EntityLightJob_t *)context;
    return R_Cod1ShadeEntityVerticesInput(job->ent,(int)(end-first),
        job->xyz+first,job->normal+first,job->rgba+(size_t)first*4,&job->input);
}
static qboolean C1EntityLight_Shade(const trRefEntity_t *ent,int count,
        const vec4_t *xyz,const vec4_t *normal,byte *rgba,const cod1EntityLightInput_t *input) {
    cod1EntityLightJob_t job;
    byte scratch[SHADER_MAX_VERTEXES*4];
    c1jFp before;
    if(count==0)return qtrue;
    if(count<0 || !ent || !xyz || !normal || !rgba || !input)return qfalse;
    job.ent=ent;job.xyz=xyz;job.normal=normal;job.rgba=scratch;job.input=*input;
    if(count>=512 && count<=SHADER_MAX_VERTEXES && C1J_Enabled()) {
        C1J_FpGet(&before);
        if(C1J_ParallelFor((unsigned)count,256,R_Cod1EntityLightRange,&job)) {
            memcpy(rgba,scratch,(size_t)count*4);return qtrue;
        }
        C1J_FpRestore(&before,0,0);
    }
    return R_Cod1ShadeEntityVerticesInput(ent,count,xyz,normal,rgba,&job.input);
}
#endif
