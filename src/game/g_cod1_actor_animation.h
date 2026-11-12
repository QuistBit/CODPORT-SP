#ifndef G_COD1_ACTOR_ANIMATION_H
#define G_COD1_ACTOR_ANIMATION_H
#include "g_cod1_actor.h"
#include "g_cod1_animation_length.h"
#include "../renderer/cod1_native_anim_eval.h"
/* Explicit genuine-resource owner, not Session/render/network publication.
 * Both clips must have exact half-second native duration; root channels are
 * validated then omitted as in the existing actor sampler. Three proved
 * notetrack/end payloads use the existing VM service. The original selector
 * retains125ms steps; the separate cadence selector admits the proved GAME
 * 50ms caller through600ms. Other clocks/clips/retargeting/Session are held. */
typedef struct {
    const char *tree,*model_name,*parts_name;
    const void *atr,*model_bytes,*parts_bytes;size_t atr_size,model_size,parts_size;
    const cod1AnimationClipInput_t *clips;size_t clip_count;
} cod1ActorAnimationResources_t;
typedef struct {
    cod1ActorLease_t lease;uint64_t topology_serial;int32_t now_ms;uint32_t body_active;
    uint32_t weight_bits[2],goal_bits[2],remaining_bits[2],time_bits[2];
    char names[2][64],pending_notify[128];uint32_t precision_bits,rounding,delivered_notes,pose_present;char pose[128];
} cod1ActorAnimationSnapshot_t;
typedef struct { uint32_t bone_count;float position[128][3],rotation[128][4]; } cod1ActorAnimationPose_t;
static int G_Cod1SelectActorAnimation(GscRuntime *,const cod1ActorAnimationResources_t *,char *,size_t);
static int G_Cod1SelectActorAnimationCadence(GscRuntime *,const cod1ActorAnimationResources_t *,char *,size_t);
static int G_Cod1ActorAnimationBind(const cod1ActorLease_t *,char *,size_t);
static int G_Cod1ActorAnimationRead(const cod1ActorLease_t *,cod1ActorAnimationSnapshot_t *,char *,size_t);
static int G_Cod1ActorAnimationEvaluate(GscRuntime *,const cod1ActorLease_t *,int32_t,cod1ActorAnimationPose_t *,cod1ActorAnimationSnapshot_t *,char *,size_t);
#include "g_cod1_animation_timeline_math.h"
typedef struct { uint32_t frame;char text[128]; } cod1ActorAnimationNote_t;
typedef struct {
    uint64_t epoch,runtime;char model[128],parts[128],tree[64];uint32_t body,ordinals[2],frame_cadence;
    cv14Skeleton skeleton;cnDefinition_t definition;ca14ActorClip clips[2];char names[2][64];
    unsigned note_count[2];cod1ActorAnimationNote_t notes[2][32];
} cod1ActorAnimationBank_t;
typedef struct { cod1ActorAnimationSnapshot_t state;cod1ActorAssignedModel_t model;int32_t offset,origin;int bound,started; } cod1ActorAnimationRecord_t;
static cod1ActorAnimationBank_t cod1ActorAnimationBank,cod1ActorAnimationStage;
static cod1ActorAnimationRecord_t cod1ActorAnimations[COD1_ACTOR_CAPACITY];
static int cod1ActorAnimationBusy;
static int Cod1ActorAnimationInternal(const void *p,size_t n) {
    int i;
    if(Cod1AnimationLengthOverlap(p,n,&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||
       Cod1AnimationLengthOverlap(p,n,&cod1ActorAnimationStage,sizeof(cod1ActorAnimationStage))||
       Cod1AnimationLengthOverlap(p,n,cod1ActorAnimations,sizeof(cod1ActorAnimations)))return 1;
    for(i=0;i<2;++i)if(Cod1AnimationLengthOverlap(p,n,cod1ActorAnimationBank.clips[i].boneData,cod1ActorAnimationBank.clips[i].boneSize)||
       Cod1AnimationLengthOverlap(p,n,cod1ActorAnimationStage.clips[i].boneData,cod1ActorAnimationStage.clips[i].boneSize))return 1;
    return 0;
}
static int Cod1ActorAnimationInput(const GscRuntime *r,const void *p,size_t n,const void *error,size_t size) {
    return p&&n&&!Cod1ActorAnimationInternal(p,n)&&gsc_runtime_storage_separate(r,p,n)&&!Cod1AnimationLengthOverlap(p,n,error,size);
}
static size_t Cod1ActorAnimationText(const char *p,size_t limit){size_t n;if(!p)return 0;for(n=0;n<limit&&p[n];++n){}return n;}
static void Cod1ActorAnimationError(char *error,size_t size,const char *text){if(error&&size)snprintf(error,size,"%s",text);}
static int Cod1ActorAnimationNotes(const ca14ActorClip *clip,unsigned *out_count,cod1ActorAnimationNote_t out[32]) {
    xm14Reader r;int frames,bones,n,i,count,ids[4096],simple;char name[128];
    r.p=clip->boneData;r.n=clip->boneSize;r.at=0;r.ok=1;XM14_U16(&r);frames=XM14_U16(&r);bones=XM14_U16(&r);n=XM14_U8(&r)&1;XM14_U16(&r);frames+=n;n=(bones+7)/8;
    if(!XM14_Take(&r,(size_t)(2*n)))return 0;
    for(i=0;i<bones;++i)if(!XM14_String(&r,name))return 0;
    for(i=0;i<bones;++i){simple=clip->boneData[9+n+i/8]&(1<<(i%8));count=XM14_U16(&r);if(!CV14_Indices(&r,count,frames,ids)||!XM14_Take(&r,(size_t)count*(simple?2:6)))return 0;count=XM14_U16(&r);if(!CV14_Indices(&r,count,frames,ids)||!XM14_Take(&r,(size_t)count*12))return 0;}
    count=XM14_U8(&r);if(count>32)return 0;
    for(i=0;i<count;++i){if(!XM14_String(&r,out[i].text))return 0;out[i].frame=XM14_U16(&r);if(!out[i].frame||(i&&out[i].frame<out[i-1].frame))return 0;}
    if(!r.ok||r.at!=r.n)return 0;
    *out_count=(unsigned)count;return 1;
}
static void Cod1ActorAnimationBankFree(cod1ActorAnimationBank_t *bank){int i;for(i=0;i<2;++i)CA14_ActorClipFree(&bank->clips[i]);memset(bank,0,sizeof(*bank));}
static int Cod1ActorAnimationCurrent(const cod1ActorAnimationRecord_t *record,char *error,size_t size) {
    cod1ActorLeaseSnapshot_t lease;cod1ActorAssignedModel_t assigned;cod1DObjQueryResult_t query;
    if(!record||!record->bound||cod1ActorAnimationBank.epoch!=G_Cod1EntityMapEpoch()||
       !G_Cod1ActorReadLease(&record->state.lease,&lease,error,size)||!lease.nativeDataInitialized||lease.initialized||
       !G_Cod1ActorReadAssignedModel(&record->state.lease,&assigned,error,size)||
       memcmp(&assigned,&record->model,sizeof(assigned))||
       !G_Cod1ActorModelRegistrationCurrent(&record->model.registration,error,size)||
       !G_Cod1ActorQueryDObj(&record->state.lease,record->state.topology_serial,COD1_DOBJ_QUERY_MODEL,"",&query,error,size)||
       strcmp(query.model,cod1ActorAnimationBank.model)||!G_Cod1ActorReadLease(&record->state.lease,&lease,error,size))return 0;
    return 1;
}
static int Cod1ActorAnimationBindRaw(const cod1ActorLease_t *input,int32_t vm_now,char *error,size_t size) {
    cod1ActorAnimationRecord_t record;cod1ActorLeaseSnapshot_t current;cod1ActorDObjLease_t topology;unsigned index;
    if(!input||input->actorIndex>=COD1_ACTOR_CAPACITY||vm_now<0||level.time<0||
       cod1ActorAnimationBank.epoch!=G_Cod1EntityMapEpoch())return 0;
    index=input->actorIndex;
    if(cod1ActorAnimations[index].bound&&cod1ActorAnimations[index].state.lease.serial==input->serial)return !memcmp(input,&cod1ActorAnimations[index].state.lease,sizeof(*input))&&Cod1ActorAnimationCurrent(&cod1ActorAnimations[index],error,size);
    memset(&record,0,sizeof(record));record.state.lease=*input;record.offset=level.time-vm_now;record.origin=level.time;record.state.now_ms=level.time;
    if(!G_Cod1ActorReadLease(input,&current,error,size)||!current.nativeDataInitialized||current.initialized||
       !G_Cod1ActorCaptureDObj(&input->entity,&topology,error,size)||memcmp(&topology.lease,input,sizeof(*input))||
       !G_Cod1ActorReadAssignedModel(input,&record.model,error,size)||strcmp(record.model.registration.nativeName,cod1ActorAnimationBank.model))return 0;
    record.state.topology_serial=topology.topologySerial;record.bound=1;
    strcpy(record.state.names[0],cod1ActorAnimationBank.names[0]);strcpy(record.state.names[1],cod1ActorAnimationBank.names[1]);
    if(!Cod1ActorAnimationCurrent(&record,error,size))return 0;
    cod1ActorAnimations[index]=record;return 1;
}
static int G_Cod1ActorAnimationBind(const cod1ActorLease_t *input,char *error,size_t size) {
    int result;if(cod1ActorAnimationBusy||!input||Cod1AnimationLengthOverlap(input,sizeof(*input),error,size)||Cod1ActorAnimationInternal(input,sizeof(*input))||Cod1ActorAnimationInternal(error,size))return 0;
    cod1ActorAnimationBusy=1;result=Cod1ActorAnimationBindRaw(input,0,error,size);cod1ActorAnimationBusy=0;return result;
}
static int Cod1ActorAnimationValidate(void *user,const GscEntityRef *input,uint64_t runtime,int32_t now,char *error,size_t size,int (*current)(void *),void *context) {
    cod1ActorDObjLease_t topology;cod1ActorAnimationRecord_t *record;int result=0;(void)user;
    if(!input||!current||cod1ActorAnimationBusy||runtime!=cod1ActorAnimationBank.runtime||cod1ActorAnimationBank.epoch!=G_Cod1EntityMapEpoch())return 0;
    cod1ActorAnimationBusy=1;
    if(current(context)!=1||!G_Cod1ActorCaptureDObj(input,&topology,error,size)||current(context)!=1||
       !Cod1ActorAnimationBindRaw(&topology.lease,now,error,size))goto done;
    record=&cod1ActorAnimations[topology.lease.actorIndex];
    if((int64_t)record->offset+now!=level.time||current(context)!=1||!Cod1ActorAnimationCurrent(record,error,size)||current(context)!=1)goto done;
    result=1;
 done:cod1ActorAnimationBusy=0;return result;
}
static int Cod1ActorAnimationCommand(void *user,const GscAnimationMethodRequest *input,char *error,size_t size,int (*current)(void *),void *context) {
    GscAnimationMethodRequest request;cod1ActorAnimationRecord_t record,*stored=NULL;cod1AnimationTimelinePolicy_t policy;int slot=-1,i,result=0;(void)user;
    if(!input||!current||cod1ActorAnimationBusy)return 0;
    request=*input;
    if(request.animation.runtime_identity!=cod1ActorAnimationBank.runtime||strcmp(request.animation.tree,cod1ActorAnimationBank.tree)||
       (request.precision_bits!=53&&request.precision_bits!=64)||request.rounding>3||request.weight_bits!=UINT32_C(0x3f800000)||request.rate_bits!=UINT32_C(0x3f800000))return 0;
    for(i=0;i<COD1_ACTOR_CAPACITY;++i)if(cod1ActorAnimations[i].bound&&(cod1ActorAnimations[i].state.lease.entity.epoch==request.entity.epoch&&cod1ActorAnimations[i].state.lease.entity.slot==request.entity.slot&&cod1ActorAnimations[i].state.lease.entity.generation==request.entity.generation)){stored=&cod1ActorAnimations[i];break;}
    if(!stored)return 0;
    record=*stored;policy.precision_bits=request.precision_bits;policy.rounding=request.rounding;
    if((int64_t)record.offset+request.now_ms!=level.time||level.time!=record.origin||record.started||
       (record.state.body_active&&(record.state.precision_bits!=request.precision_bits||record.state.rounding!=request.rounding)))return 0;
    cod1ActorAnimationBusy=1;if(current(context)!=1||!Cod1ActorAnimationCurrent(&record,error,size)||current(context)!=1)goto done;
    if(request.method==GSC_ANIMATION_SET_KNOB){
        if(strcmp(request.animation.name,"body")||request.animation.ordinal!=cod1ActorAnimationBank.body||!(request.animation.flags&GSC_ANIMATION_NODE_BRANCH)||request.blend_bits)goto done;
        record.state.body_active=1;
    } else {
        if(!record.state.body_active||strcmp(request.root.name,"body")||request.root.ordinal!=cod1ActorAnimationBank.body||request.root.runtime_identity!=cod1ActorAnimationBank.runtime||strcmp(request.root.tree,request.animation.tree))goto done;
        for(i=0;i<2;++i)if(request.animation.ordinal==cod1ActorAnimationBank.ordinals[i]&&!strcmp(request.animation.name,cod1ActorAnimationBank.names[i]))slot=i;
        if(request.method==GSC_ANIMATION_SET_KNOB_ALL){
            if(slot!=0||request.blend_bits||request.notify[0])goto done;
            record.state.weight_bits[0]=record.state.goal_bits[0]=UINT32_C(0x3f800000);record.state.weight_bits[1]=record.state.goal_bits[1]=0;
        } else if(request.method==GSC_ANIMATION_SET_FLAGGED_KNOB_ALL){
            if(slot!=1||request.blend_bits!=UINT32_C(0x3e800000)||!request.notify[0]||!memchr(request.notify,0,sizeof(request.notify))||record.state.weight_bits[0]!=UINT32_C(0x3f800000))goto done;
            if(!Cod1AnimTimelineGoal(record.state.weight_bits[0],0,request.blend_bits,&policy,&record.state.weight_bits[0],&record.state.remaining_bits[0])||
               !Cod1AnimTimelineGoal(record.state.weight_bits[1],UINT32_C(0x3f800000),request.blend_bits,&policy,&record.state.weight_bits[1],&record.state.remaining_bits[1]))goto done;
            record.state.goal_bits[0]=0;record.state.goal_bits[1]=UINT32_C(0x3f800000);strcpy(record.state.pending_notify,request.notify);record.started=1;
        } else goto done;
    }
    record.state.precision_bits=request.precision_bits;record.state.rounding=request.rounding;
    if(current(context)!=1||!Cod1ActorAnimationCurrent(&record,error,size)||current(context)!=1)goto done;
    *stored=record;result=1;
 done:cod1ActorAnimationBusy=0;if(!result)Cod1ActorAnimationError(error,size,"animation command outside the selected current body/sibling transition domain");return result;
}
static int Cod1SelectActorAnimation(GscRuntime *runtime,const cod1ActorAnimationResources_t *input,int frame_cadence,char *error,size_t size) {
    cod1ActorAnimationResources_t resources;GscAnimationMethodHost host;GscAnimationCatalog *catalog=NULL;GscAnimationCatalogEntry entry,children[2];xm14Model *model=NULL;cv14Pose *sample=NULL;uint64_t epoch;size_t i,j,k;int result=0;
    if(!runtime||!input||!gsc_runtime_storage_separate(runtime,error,size)||Cod1ActorAnimationInternal(error,size)||!Cod1ActorAnimationInput(runtime,input,sizeof(*input),error,size)||cod1ActorAnimationBusy)return 0;
    if(frame_cadence&&!gsc_runtime_frame_animation_cadence_selected(runtime))return 0;
    resources=*input;if(!resources.tree||!resources.model_name||!resources.parts_name||!resources.atr||!resources.model_bytes||!resources.parts_bytes||!resources.clips||resources.clip_count!=2||!resources.atr_size||!resources.model_size||!resources.parts_size)return 0;
    if(!Cod1ActorAnimationInput(runtime,resources.clips,2*sizeof(*resources.clips),error,size)||
       !Cod1ActorAnimationInput(runtime,resources.atr,resources.atr_size,error,size)||
       !Cod1ActorAnimationInput(runtime,resources.model_bytes,resources.model_size,error,size)||
       !Cod1ActorAnimationInput(runtime,resources.parts_bytes,resources.parts_size,error,size))return 0;
    {const char *names[3]={resources.tree,resources.model_name,resources.parts_name};size_t limits[3]={64,128,128};
     for(i=0;i<3;++i){size_t n;if(!gsc_runtime_storage_separate(runtime,names[i],1))return 0;n=Cod1ActorAnimationText(names[i],limits[i]);if(!n||n==limits[i]||!Cod1ActorAnimationInput(runtime,names[i],n+1,error,size))return 0;}}
    if(strcmp(resources.tree,"generic_human"))return 0;
    for(i=0;i<2;++i){size_t n;const cod1AnimationClipInput_t *clip=&resources.clips[i];
        if(!clip->name||!gsc_runtime_storage_separate(runtime,clip->name,1))return 0;
        n=Cod1ActorAnimationText(clip->name,64);
        if(!n||n==64||!Cod1ActorAnimationInput(runtime,clip->name,n+1,error,size)||!Cod1ActorAnimationInput(runtime,clip->bytes,clip->size,error,size))return 0;
    }
    epoch=G_Cod1EntityMapEpoch();if(!epoch||cod1ActorAnimationBank.epoch==epoch)return 0;
    cod1ActorAnimationBusy=1;memset(&cod1ActorAnimationStage,0,sizeof(cod1ActorAnimationStage));
    catalog=gsc_animation_catalog_parse(resources.tree,resources.atr,resources.atr_size,error,size);model=calloc(1,sizeof(*model));sample=calloc(1,sizeof(*sample));
    if(!catalog||!model||!sample||!XM14_Model(resources.model_bytes,resources.model_size,model)||strcmp(model->lod[0],resources.parts_name)||!CV14_Parts(resources.parts_bytes,resources.parts_size,&cod1ActorAnimationStage.skeleton))goto done;
    for(i=0;i<gsc_animation_catalog_count(catalog);++i){if(!gsc_animation_catalog_entry(catalog,i,&entry))goto done;if(!strcmp(entry.name,"body"))cod1ActorAnimationStage.body=entry.ordinal;}
    if(!cod1ActorAnimationStage.body)goto done;
    for(i=0;i<2;++i){const cod1AnimationClipInput_t *clip=&resources.clips[i];int found=0;
        if(!clip->name||!clip->bytes||!clip->size||strlen(clip->name)>=64||!gsc_runtime_storage_separate(runtime,clip->bytes,clip->size)||Cod1AnimationLengthOverlap(clip->bytes,clip->size,error,size))goto done;
        for(j=0;j<gsc_animation_catalog_count(catalog);++j){if(!gsc_animation_catalog_entry(catalog,j,&entry))goto done;if(Cod1AnimationLengthNameEqual(entry.name,clip->name)){children[i]=entry;found=1;break;}}
        if(!found||(children[i].flags&GSC_ANIMATION_NODE_BRANCH)||!CA14_ActorClipLoad(clip->bytes,clip->size,&cod1ActorAnimationStage.clips[i]))goto done;
        if(cod1ActorAnimationStage.clips[i].frames!=16||cod1ActorAnimationStage.clips[i].fps!=30||(cod1ActorAnimationStage.clips[i].originalFlags&1)||
           !Cod1ActorAnimationNotes(&cod1ActorAnimationStage.clips[i],&cod1ActorAnimationStage.note_count[i],cod1ActorAnimationStage.notes[i]))goto done;
        strcpy(cod1ActorAnimationStage.names[i],children[i].name);cod1ActorAnimationStage.ordinals[i]=children[i].ordinal;
    }
    if(children[0].parent!=children[1].parent||children[0].ordinal==children[1].ordinal||!cod1ActorAnimationStage.note_count[1]||cod1ActorAnimationStage.notes[1][0].frame!=6||
       (cod1ActorAnimationStage.note_count[1]!=2)||cod1ActorAnimationStage.notes[1][1].frame!=11)goto done;
    {uint32_t at=children[0].parent;int body=0;for(i=0;i<64&&at;++i){if(at==cod1ActorAnimationStage.body){body=1;break;}if(!gsc_animation_catalog_entry(catalog,at-1,&entry))goto done;at=entry.parent;}if(!body)goto done;}
    cod1ActorAnimationStage.definition.bones=cod1ActorAnimationStage.skeleton.n;
    for(i=0;i<(size_t)cod1ActorAnimationStage.skeleton.n;++i){cod1ActorAnimationStage.definition.parent[i]=cod1ActorAnimationStage.skeleton.parent[i];cod1ActorAnimationStage.definition.bindLocal[i]=cod1ActorAnimationStage.skeleton.local[i];strcpy(cod1ActorAnimationStage.definition.boneName[i],cod1ActorAnimationStage.skeleton.name[i]);}
    for(i=0;i<2;++i){cnLeaf_t *leaf=&cod1ActorAnimationStage.definition.leaf[i+1];ca14ActorClip *clip=&cod1ActorAnimationStage.clips[i];
        if(!CV14_SamplePose(clip->boneData,clip->boneSize,0,sample))goto done;
        leaf->data=clip->boneData;leaf->size=clip->boneSize;leaf->parts=sample->n;leaf->lastFrame=15;memset(leaf->remap,0x7f,sizeof(leaf->remap));
        for(j=0;j<(size_t)sample->n;++j){int matched=0;for(k=0;k<(size_t)cod1ActorAnimationStage.skeleton.n;++k)if(!strcmp(sample->name[j],cod1ActorAnimationStage.skeleton.name[k])){leaf->remap[j]=(unsigned char)k;matched=1;break;}if(!matched)goto done;}
    }
    strcpy(cod1ActorAnimationStage.model,resources.model_name);strcpy(cod1ActorAnimationStage.parts,resources.parts_name);strcpy(cod1ActorAnimationStage.tree,resources.tree);cod1ActorAnimationStage.epoch=epoch;cod1ActorAnimationStage.runtime=gsc_runtime_instance_identity(runtime);cod1ActorAnimationStage.frame_cadence=(uint32_t)frame_cadence;
    memset(&host,0,sizeof(host));host.validate=Cod1ActorAnimationValidate;host.invoke=Cod1ActorAnimationCommand;
    if(epoch!=G_Cod1EntityMapEpoch()||!gsc_runtime_select_animation_methods(runtime,&host)||epoch!=G_Cod1EntityMapEpoch())goto done;
    Cod1ActorAnimationBankFree(&cod1ActorAnimationBank);cod1ActorAnimationBank=cod1ActorAnimationStage;memset(&cod1ActorAnimationStage,0,sizeof(cod1ActorAnimationStage));memset(cod1ActorAnimations,0,sizeof(cod1ActorAnimations));result=1;
 done:if(catalog)gsc_animation_catalog_free(catalog);free(model);free(sample);if(!result)Cod1ActorAnimationBankFree(&cod1ActorAnimationStage);cod1ActorAnimationBusy=0;if(!result)Cod1ActorAnimationError(error,size,"genuine actor animation resource admission refused");return result;
}
static int G_Cod1SelectActorAnimation(GscRuntime *runtime,const cod1ActorAnimationResources_t *input,char *error,size_t size) {
    return Cod1SelectActorAnimation(runtime,input,0,error,size);
}
static int G_Cod1SelectActorAnimationCadence(GscRuntime *runtime,const cod1ActorAnimationResources_t *input,char *error,size_t size) {
    return Cod1SelectActorAnimation(runtime,input,1,error,size);
}
static int G_Cod1ActorAnimationRead(const cod1ActorLease_t *lease,cod1ActorAnimationSnapshot_t *out,char *error,size_t size) {
    cod1ActorAnimationRecord_t *record;int result=0;
    if(!lease||!out||lease->actorIndex>=COD1_ACTOR_CAPACITY||cod1ActorAnimationBusy||Cod1ActorAnimationInternal(lease,sizeof(*lease))||Cod1ActorAnimationInternal(out,sizeof(*out))||Cod1ActorAnimationInternal(error,size)||Cod1AnimationLengthOverlap(lease,sizeof(*lease),out,sizeof(*out))||Cod1AnimationLengthOverlap(lease,sizeof(*lease),error,size)||Cod1AnimationLengthOverlap(out,sizeof(*out),error,size)||
       Cod1AnimationLengthOverlap(out,sizeof(*out),&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||Cod1AnimationLengthOverlap(out,sizeof(*out),cod1ActorAnimations,sizeof(cod1ActorAnimations))||
       Cod1AnimationLengthOverlap(error,size,&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||Cod1AnimationLengthOverlap(error,size,cod1ActorAnimations,sizeof(cod1ActorAnimations)))return 0;
    record=&cod1ActorAnimations[lease->actorIndex];if(!record->bound||memcmp(lease,&record->state.lease,sizeof(*lease)))return 0;
    cod1ActorAnimationBusy=1;if(Cod1ActorAnimationCurrent(record,error,size)){*out=record->state;result=1;}cod1ActorAnimationBusy=0;return result;
}
static int G_Cod1ActorAnimationEvaluate(GscRuntime *runtime,const cod1ActorLease_t *lease,int32_t now,cod1ActorAnimationPose_t *out,cod1ActorAnimationSnapshot_t *state,char *error,size_t size) {
    cod1ActorAnimationRecord_t record,*stored;cod1ActorAnimationPose_t *pose=NULL;cv14Transform *final=NULL;cod1NativeAnimState_t native;cod1AnimationTimelinePolicy_t policy;int32_t elapsed,delta,logical;uint32_t dt,timing,oldtime;int i,k,result=0,dispatch=0,dispatch_note=0,awakened=0,guarded=0;GscEventValue event;GscAnimationTickScope scope;cod1ActorLease_t owned_lease;
    if(!runtime||!lease||!out||!state||lease->actorIndex>=COD1_ACTOR_CAPACITY||cod1ActorAnimationBusy||now<0||now!=level.time||
       !gsc_runtime_storage_separate(runtime,lease,sizeof(*lease))||Cod1ActorAnimationInternal(state,sizeof(*state))||!gsc_runtime_storage_separate(runtime,out,sizeof(*out))||!gsc_runtime_storage_separate(runtime,state,sizeof(*state))||!gsc_runtime_storage_separate(runtime,error,size)||
       Cod1ActorAnimationInternal(lease,sizeof(*lease))||Cod1ActorAnimationInternal(out,sizeof(*out))||Cod1ActorAnimationInternal(error,size)||Cod1AnimationLengthOverlap(lease,sizeof(*lease),out,sizeof(*out))||Cod1AnimationLengthOverlap(lease,sizeof(*lease),state,sizeof(*state))||Cod1AnimationLengthOverlap(lease,sizeof(*lease),error,size)||
       Cod1AnimationLengthOverlap(out,sizeof(*out),cod1ActorAnimations,sizeof(cod1ActorAnimations))||Cod1AnimationLengthOverlap(state,sizeof(*state),cod1ActorAnimations,sizeof(cod1ActorAnimations))||Cod1AnimationLengthOverlap(error,size,cod1ActorAnimations,sizeof(cod1ActorAnimations))||
       Cod1AnimationLengthOverlap(out,sizeof(*out),&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||Cod1AnimationLengthOverlap(state,sizeof(*state),&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||Cod1AnimationLengthOverlap(error,size,&cod1ActorAnimationBank,sizeof(cod1ActorAnimationBank))||
       Cod1AnimationLengthOverlap(out,sizeof(*out),state,sizeof(*state))||Cod1AnimationLengthOverlap(out,sizeof(*out),error,size)||Cod1AnimationLengthOverlap(state,sizeof(*state),error,size))return 0;
    owned_lease=*lease;stored=&cod1ActorAnimations[owned_lease.actorIndex];if(!stored->bound||memcmp(&stored->state.lease,&owned_lease,sizeof(owned_lease)))return 0;record=*stored;
    if((int64_t)now-record.offset<0||(int64_t)now-record.offset>INT32_MAX)return 0;
    logical=now-record.offset;
    if(gsc_runtime_instance_identity(runtime)!=cod1ActorAnimationBank.runtime||!gsc_runtime_animation_delivery_available(runtime,cod1ActorAnimationBank.runtime,logical))return 0;
    elapsed=now-record.origin;delta=now-record.state.now_ms;
    if(cod1ActorAnimationBank.frame_cadence){if(elapsed<0||elapsed>600||elapsed%50||delta<0||(delta!=0&&delta!=50))return 0;}
    else if(elapsed<0||elapsed>500||elapsed%125||delta<0||(delta!=0&&delta!=125))return 0;
    if(!gsc_runtime_animation_tick_begin(runtime,cod1ActorAnimationBank.runtime,logical,&scope))return 0;
    guarded=1;
    cod1ActorAnimationBusy=1;if(!record.started||!Cod1ActorAnimationCurrent(&record,error,size)||gsc_runtime_error(runtime)[0])goto done;
    policy.precision_bits=record.state.precision_bits;policy.rounding=record.state.rounding;
    dt=delta?(cod1ActorAnimationBank.frame_cadence?UINT32_C(0x3d4ccccd):UINT32_C(0x3e000000)):0;oldtime=record.state.time_bits[1];
    for(i=0;i<2;++i){
        if(cod1ActorAnimationBank.frame_cadence){
            if(delta&&(!Cod1AnimTimelineWeightCadence(record.state.weight_bits[i],record.state.goal_bits[i],record.state.remaining_bits[i],dt,&policy,&record.state.weight_bits[i],&record.state.remaining_bits[i])||
               !Cod1AnimTimelineTimeCadence(record.state.time_bits[i],record.state.weight_bits[i]!=0,dt,&policy,&record.state.time_bits[i])))goto done;
        } else {
        if(delta&&!Cod1AnimTimelineWeight(record.state.weight_bits[i],record.state.goal_bits[i],record.state.remaining_bits[i],dt,&policy,&record.state.weight_bits[i],&record.state.remaining_bits[i]))goto done;
        if(delta&&record.state.weight_bits[i]){cod1TimelineDyadic_t step=Cod1TimelineBits(dt),sum;step.exponent+=1;
            if(!Cod1TimelineAdd(Cod1TimelineBits(record.state.time_bits[i]),step,policy.rounding,&sum)||!Cod1TimelineStore(sum,policy.rounding,&timing))goto done;
            record.state.time_bits[i]=timing;
        }
        }
    }
    record.state.now_ms=now;
    if(cod1ActorAnimationBank.frame_cadence){
        unsigned note;uint32_t threshold;
        for(note=0;delta&&note<2;++note){
            {uint32_t frame=cod1ActorAnimationBank.notes[1][note].frame;
                uint32_t framebits=frame==6?UINT32_C(0x40c00000):frame==11?UINT32_C(0x41300000):0;
                if(!framebits||!Cod1TimelineDivideFinite(framebits,UINT32_C(0x41700000),&policy,&threshold))goto done;
            }
            if(oldtime<=threshold&&record.state.time_bits[1]>threshold){++dispatch;dispatch_note=(int)note;}
        }
        /* Native nonloop completion emits again on each positive tick while
         * its flagged notification remains installed; same-time reads do not. */
        if(delta&&record.state.time_bits[1]==UINT32_C(0x3f800000)){++dispatch;dispatch_note=2;}
        if(dispatch>1)goto done;
    } else if(elapsed>=250){unsigned expected=(unsigned)(elapsed/125-1);if(expected>record.state.delivered_notes){if(expected!=record.state.delivered_notes+1)goto done;dispatch=1;}}
    memset(&native,0,sizeof(native));native.version=COD1_NATIVE_STATE_VERSION;native.size=sizeof(native);
    for(i=0;i<2;++i){memcpy(&native.weight[i+1],&record.state.weight_bits[i],4);memcpy(&native.time[i+1],&record.state.time_bits[i],4);}
    final=calloc(CV14_BONES,sizeof(*final));pose=calloc(1,sizeof(*pose));if(!final||!pose||!CN_Evaluate(&cod1ActorAnimationBank.definition,&native,final))goto done;
    pose->bone_count=cod1ActorAnimationBank.definition.bones;for(i=0;i<(int)pose->bone_count;++i){for(k=0;k<3;++k)pose->position[i][k]=final[i].p[k];for(k=0;k<4;++k)pose->rotation[i][k]=final[i].q[k];}
    if(!Cod1ActorAnimationCurrent(&record,error,size)||gsc_runtime_error(runtime)[0])goto done;
    /* The coarse tick clock and native time/state precede notify without
     * running tasks. Accepted clock/state/delivery are retained prefixes:
     * a later callback refusal never rolls back another owner/VM. */
    if(!gsc_runtime_animation_tick_commit(runtime,&scope))goto done;
    *stored=record;
    if(dispatch){memset(&event,0,sizeof(event));event.kind=GSC_EVENT_STRING;
        if(!cod1ActorAnimationBank.frame_cadence)dispatch_note=(int)record.state.delivered_notes;
        if(dispatch_note<2)strcpy(event.string,cod1ActorAnimationBank.notes[1][dispatch_note].text);else strcpy(event.string,"end");
        ++record.state.delivered_notes;*stored=record;
        if(!gsc_runtime_animation_tick_notify(runtime,&scope,&owned_lease.entity,record.state.pending_notify,&event,1,&awakened)||!Cod1ActorAnimationCurrent(&record,error,size)||gsc_runtime_error(runtime)[0])goto done;}
    if(!Cod1ActorAnimationCurrent(&record,error,size)||gsc_runtime_error(runtime)[0])goto done;
    *out=*pose;*state=record.state;result=1;
 done:free(final);free(pose);cod1ActorAnimationBusy=0;if(guarded)gsc_runtime_animation_tick_end(runtime,&scope);if(!result)Cod1ActorAnimationError(error,size,"actor animation tick refused; accepted timeline/notify prefixes are retained");return result;
}
/* Native actor WORD string cell +178, selected separately from the old
 * constructor host. This owns text/presence, never an invented string ID.
 * The unset WORD0 getter remains held. No geometry/prone activation occurs. */
static cod1ActorAnimationRecord_t *Cod1ActorAnimationFieldRecord(const GscEntityRef *input,cod1ActorLeaseSnapshot_t *snapshot) {
    int i;cod1ActorAssignedModel_t assigned;
    if(!input||cod1ActorAnimationBank.epoch!=G_Cod1EntityMapEpoch())return NULL;
    for(i=0;i<COD1_ACTOR_CAPACITY;++i){cod1ActorAnimationRecord_t *record=&cod1ActorAnimations[i];const GscEntityRef *owner=&record->state.lease.entity;
        if(record->bound&&owner->epoch==input->epoch&&owner->slot==input->slot&&owner->generation==input->generation){
            if(!G_Cod1ActorReadLease(&record->state.lease,snapshot,NULL,0)||!snapshot->nativeDataInitialized||snapshot->initialized||
               !snapshot->dobj.present||snapshot->dobj.summary.topologySerial!=record->state.topology_serial||
               !G_Cod1ActorReadAssignedModel(&record->state.lease,&assigned,NULL,0)||memcmp(&assigned,&record->model,sizeof(assigned)))return NULL;
            return record;
        }
    }
    return NULL;
}
static int Cod1ActorAnimationFieldRead(void *user,const GscEntityRef *input,const char *name,GscStringCastValue *out,char *error,size_t size) {
    const GscEntityFieldHost *base=G_Cod1ActorConstructorFieldHost();cod1ActorAnimationRecord_t *record;cod1ActorLeaseSnapshot_t snapshot;GscStringCastValue value;(void)user;
    if(!name)return 0;
    if(Q_stricmp(name,"anim_pose"))return base->read(base->user,input,name,out,error,size);
    if(!input||!out||cod1ActorAnimationBusy||Cod1ActorAnimationInternal(input,sizeof(*input))||Cod1ActorAnimationInternal(out,sizeof(*out))||Cod1ActorAnimationInternal(error,size)||
       Cod1AnimationLengthOverlap(input,sizeof(*input),out,sizeof(*out))||Cod1AnimationLengthOverlap(input,sizeof(*input),error,size)||Cod1AnimationLengthOverlap(out,sizeof(*out),error,size)||Cod1AnimationLengthOverlap(name,strlen(name)+1,error,size))return 0;
    record=Cod1ActorAnimationFieldRecord(input,&snapshot);
    if(!record||!record->state.pose_present){Cod1ActorAnimationError(error,size,"native anim_pose getter requires an assigned current owned string");return 0;}
    memset(&value,0,sizeof(value));value.type=1;strcpy(value.string,record->state.pose);*out=value;return 1;
}
static int Cod1ActorAnimationFieldWrite(void *user,const GscEntityRef *input,const char *name,const GscStringCastValue *source,char *error,size_t size) {
    const GscEntityFieldHost *base=G_Cod1ActorConstructorFieldHost();cod1ActorAnimationRecord_t *record;cod1ActorLeaseSnapshot_t snapshot;GscStringCastValue value;int active;int32_t phase;(void)user;
    if(!name)return 0;
    if(Q_stricmp(name,"anim_pose"))return base->write(base->user,input,name,source,error,size);
    if(!input||!source||cod1ActorAnimationBusy||Cod1ActorAnimationInternal(input,sizeof(*input))||Cod1ActorAnimationInternal(source,sizeof(*source))||Cod1ActorAnimationInternal(error,size)||
       Cod1AnimationLengthOverlap(input,sizeof(*input),error,size)||Cod1AnimationLengthOverlap(source,sizeof(*source),error,size)||Cod1AnimationLengthOverlap(name,strlen(name)+1,error,size))return 0;
    value=*source;record=Cod1ActorAnimationFieldRecord(input,&snapshot);
    if(!record||value.type!=1||!memchr(value.string,0,sizeof(value.string))){Cod1ActorAnimationError(error,size,"native anim_pose requires a current owned plain string");return 0;}
    memcpy(&phase,&snapshot.nativeData.actor.zeroCells[5],sizeof(phase));
    active=snapshot.nativeData.actor.zeroCells[4]!=0&&snapshot.nativeData.actor.zeroCells[3]==0&&phase>=0;
    if((!strcmp(value.string,"prone"))!=active){Cod1ActorAnimationError(error,size,"native anim_pose conflicts with the current prone state");return 0;}
    memset(record->state.pose,0,sizeof(record->state.pose));strcpy(record->state.pose,value.string);record->state.pose_present=1;return 1;
}
static int Cod1ActorAnimationFieldClassify(void *user,const GscEntityRef *owner,const char *name,char *error,size_t size) {
    const GscEntityFieldHost *base=G_Cod1ActorConstructorFieldHost();(void)user;return base->classify(base->user,owner,name,error,size);
}
static const GscEntityFieldHost *G_Cod1ActorAnimationFieldHost(void) {
    static const GscEntityFieldHost host={NULL,Cod1ActorAnimationFieldClassify,Cod1ActorAnimationFieldRead,Cod1ActorAnimationFieldWrite};return &host;
}
#endif
