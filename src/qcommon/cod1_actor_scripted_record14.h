#ifndef COD1_ACTOR_SCRIPTED_RECORD14_H
#define COD1_ACTOR_SCRIPTED_RECORD14_H
#include <stddef.h>
#include "cod1_actor_scripted_math14.h"
#include "cod1_actor_scripted_correction14.h"
/* PURE normal/unattached record numeric steps. These functions do not own an
 * entity, tree, weight mutation, VM Value, named note, phase or lifetime. Live
 * GAME binding must supply already-authenticated samples and exact owner. */
typedef struct {float matrix[9],origin[3],positionCorrection[3],angleCorrection[3];uint32_t clip,root,firstUpdate,mode;float plant[4];} c1asr14Record_t;
typedef struct {float origin[3],angles[3];} c1asr14Pose_t;
typedef char c1asr14RecordSizeAssert[(sizeof(c1asr14Record_t)==104)?1:-1];
typedef char c1asr14RecordOffsetsAssert[(offsetof(c1asr14Record_t,clip)==0x48&&offsetof(c1asr14Record_t,root)==0x4c&&offsetof(c1asr14Record_t,firstUpdate)==0x50&&offsetof(c1asr14Record_t,mode)==0x54&&offsetof(c1asr14Record_t,plant)==0x58)?1:-1];
static uint32_t C1ASR14_Bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static uint32_t C1ASR14_Word(const float*f){uint32_t u;memcpy(&u,f,4);return u;}
static int C1ASR14_Vector(const float*v,unsigned count,uint32_t bound){unsigned i;if(!v)return 0;for(i=0;i<count;i++)if((C1ASR14_Word(v+i)&0x7fffffffu)>bound)return 0;return 1;}
static int C1ASR14_Overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n&&m&&(x<=y?y-x<n:x-y<m);}
static int C1ASR14_Transform(const c1asr14Record_t*r){return r&&r->mode==0&&C1ASR14_Vector(r->matrix,9,0x3f8147aeu)&&C1ASR14_Vector(r->origin,3,0x49742400u);}
static int C1ASR14_Record(const c1asr14Record_t*r){return C1ASR14_Transform(r)&&C1ASR14_Vector(r->positionCorrection,3,0x49f42400u)&&C1ASR14_Vector(r->angleCorrection,3,0x45610000u);}
static int C1ASR14_Sample(const float*q,const float*t){return C1ASR14_Vector(q,2,0x43000000u)&&((C1ASR14_Word(q)|C1ASR14_Word(q+1))&0x7fffffffu)&&C1ASR14_Vector(t,3,0x49742400u);}
#if defined(_MSC_VER)&&defined(_M_IX86)
#define C1ASR14_FP_BEGIN unsigned char caller[108];uint32_t mx,nmx=0x1f80;unsigned short cw=0x027f; __asm {fnsave caller} __asm {stmxcsr mx} __asm {fldcw cw} __asm {ldmxcsr nmx}
#define C1ASR14_FP_END __asm {ldmxcsr mx} __asm {frstor caller}
#else
#define C1ASR14_FP_BEGIN
#define C1ASR14_FP_END
#endif
static int C1ASR14_NormalBasis(c1asr14Record_t*record,uint32_t clip,uint32_t root,const float origin[3],const float angles[3]){
    c1asr14Record_t value;
    if(!record||!(clip&65535u)||!C1ASR14_Vector(origin,3,0x49742400u)||!C1ASR14_Vector(angles,3,0x45610000u)||C1ASR14_Overlap(record,104,origin,12)||C1ASR14_Overlap(record,104,angles,12))return 0;
    memcpy(&value,record,104);value.clip=clip;value.root=root;value.firstUpdate=0;value.mode=0;memcpy(value.origin,origin,12);
    {C1ASR14_FP_BEGIN C1ASM14_Matrix(angles,value.matrix);C1ASR14_FP_END}
    memcpy(record,&value,104);return 1;
}
static int C1ASR14_StartCorrection(c1asr14Record_t*record,const float q[2],const float translation[3],const float currentOrigin[3],const float currentAngles[3]){
    c1asr14Record_t value;c1asr14Pose_t pose;unsigned i;int valid;
    if(!C1ASR14_Transform(record)||!C1ASR14_Sample(q,translation)||!C1ASR14_Vector(currentOrigin,3,0x49742400u)||!C1ASR14_Vector(currentAngles,3,0x45610000u)||C1ASR14_Overlap(record,104,q,8)||C1ASR14_Overlap(record,104,translation,12)||C1ASR14_Overlap(record,104,currentOrigin,12)||C1ASR14_Overlap(record,104,currentAngles,12))return 0;
    memcpy(&value,record,104);
    {C1ASR14_FP_BEGIN
     valid=(double)q[0]*q[0]+(double)q[1]*q[1]>1.e-20;
     if(valid){float transform[12];memcpy(transform,value.matrix,36);memcpy(transform+9,value.origin,12);C1ASM14_Pose(transform,q,translation,1,pose.origin,pose.angles);for(i=0;i<3;i++){value.positionCorrection[i]=(float)((double)currentOrigin[i]-pose.origin[i]);value.angleCorrection[i]=C1ASM14_Difference(currentAngles[i],pose.angles[i]);}}
     C1ASR14_FP_END}
    if(!valid||!C1ASR14_Record(&value))return 0;memcpy(record,&value,104);return 1;
}
static int C1ASR14_UpdatePose(c1asr14Record_t*record,const float q[2],const float translation[3],float rate,c1asr14Pose_t*out){
    c1asr14Record_t value;c1asr14Pose_t pose;uint32_t rb=C1ASR14_Bits(rate);int valid;
    if(!out||!C1ASR14_Record(record)||!C1ASR14_Sample(q,translation)||(rb&0x7fffffffu)>0x461c4000u||((rb&0x80000000u)&&(rb&0x7fffffffu))||C1ASR14_Overlap(record,104,out,sizeof(*out))||C1ASR14_Overlap(record,104,q,8)||C1ASR14_Overlap(record,104,translation,12)||C1ASR14_Overlap(out,sizeof(*out),q,8)||C1ASR14_Overlap(out,sizeof(*out),translation,12))return 0;
    memcpy(&value,record,104);
    {C1ASR14_FP_BEGIN
     valid=(double)q[0]*q[0]+(double)q[1]*q[1]>1.e-20;
     if(valid){float transform[12];memcpy(transform,value.matrix,36);memcpy(transform+9,value.origin,12);C1ASM14_Pose(transform,q,translation,0,pose.origin,pose.angles);C1ASC14_Position(value.positionCorrection,pose.origin,.05f);C1ASC14_Angle(value.angleCorrection,pose.angles,.05f,rate);}
     C1ASR14_FP_END}
    if(!valid||!C1ASR14_Record(&value)||!C1ASR14_Vector(pose.origin,3,0x49742400u)||!C1ASR14_Vector(pose.angles,3,0x45610000u))return 0;
    memcpy(record,&value,104);memcpy(out,&pose,sizeof(pose));return 1;
}
/* A producer must distinguish known absent runtime node from unregistered
 * resource; slotPresent=-1 is unknown/refusal, not successful completion. */
static int C1ASR14_Finished(int slotPresent,float phase,float oldPhase,uint16_t cycle,uint16_t oldCycle){
    int16_t currentSigned,oldSigned;int result;
    if(slotPresent==0)return 1;if(slotPresent!=1||!C1ASR14_Vector(&phase,1,0x3f800000u)||!C1ASR14_Vector(&oldPhase,1,0x3f800000u)||((C1ASR14_Bits(phase)&0x80000000u)&&(C1ASR14_Bits(phase)&0x7fffffffu))||((C1ASR14_Bits(oldPhase)&0x80000000u)&&(C1ASR14_Bits(oldPhase)&0x7fffffffu)))return -1;
    memcpy(&currentSigned,&cycle,2);memcpy(&oldSigned,&oldCycle,2);
    {C1ASR14_FP_BEGIN result=phase<oldPhase||C1ASR14_Bits(phase)==0x3f800000u||currentSigned>oldSigned;C1ASR14_FP_END}
    return result;
}
#undef C1ASR14_FP_BEGIN
#undef C1ASR14_FP_END
#endif
