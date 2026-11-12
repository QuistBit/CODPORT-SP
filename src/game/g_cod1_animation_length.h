#ifndef G_COD1_ANIMATION_LENGTH_H
#define G_COD1_ANIMATION_LENGTH_H
#include "g_cod1_entity_host.h"
#include "cod1_gsc/gsc_fixture.h"
#include "../renderer/cod1_actor_xanim14.h"
/* Explicit resource input, not an FS/Session loader. Caller supplies the
 * genuine name->XAnim bytes authority. Full v14 validation precedes selection;
 * only copied leaf names/processed frame counts/FPS survive. No poses/events,
 * native tree indices or model/Actor requirement are introduced. */
#define COD1_ANIMATION_LENGTH_MAX 256
typedef struct { const char *name;const void *bytes;size_t size; } cod1AnimationClipInput_t;
typedef struct { char name[64];GscAnimationLengthReply metadata; } cod1AnimationLengthRow_t;
static struct { uint64_t epoch,runtime;size_t count;cod1AnimationLengthRow_t rows[COD1_ANIMATION_LENGTH_MAX]; } cod1AnimationLengths,cod1AnimationLengthsStage;
static int cod1AnimationLengthsBusy;
static int Cod1AnimationLengthOverlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!a||!b||!an||!bn)return 0;
    return x<=y?y-x<an:x-y<bn;
}
static int Cod1AnimationLengthNameEqual(const char *a,const char *b) {
    for(;;){unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}
}
static int Cod1AnimationLengthQuery(void *user,const GscAnimationLengthRequest *input,GscAnimationLengthReply *out,char *error,size_t size,int (*current)(void *),void *context) {
    GscAnimationLengthRequest request;size_t i;uint64_t epoch=cod1AnimationLengths.epoch;(void)user;
    if(!input||!out||!current||cod1AnimationLengthsBusy||!epoch||epoch!=G_Cod1EntityMapEpoch())return 0;
    request=*input;
    if(request.runtime_identity!=cod1AnimationLengths.runtime||
       Cod1AnimationLengthOverlap(out,sizeof(*out),&cod1AnimationLengths,sizeof(cod1AnimationLengths))||
       Cod1AnimationLengthOverlap(error,size,&cod1AnimationLengths,sizeof(cod1AnimationLengths))||
       Cod1AnimationLengthOverlap(out,sizeof(*out),error,size))return 0;
    if(!memchr(request.tree,0,sizeof(request.tree))||!memchr(request.name,0,sizeof(request.name))||strcmp(request.tree,"generic_human")||(request.flags&GSC_ANIMATION_NODE_BRANCH))return 0;
    cod1AnimationLengthsBusy=1;
    if(current(context)!=1||epoch!=G_Cod1EntityMapEpoch()){cod1AnimationLengthsBusy=0;return 0;}
    for(i=0;i<cod1AnimationLengths.count;++i)if(Cod1AnimationLengthNameEqual(request.name,cod1AnimationLengths.rows[i].name)){
        GscAnimationLengthReply reply=cod1AnimationLengths.rows[i].metadata;
        if(current(context)!=1||epoch!=G_Cod1EntityMapEpoch()){cod1AnimationLengthsBusy=0;return 0;}
        *out=reply;cod1AnimationLengthsBusy=0;return 1;
    }
    cod1AnimationLengthsBusy=0;if(error&&size)snprintf(error,size,"animation clip unavailable: %s",request.name);return 0;
}
static int G_Cod1SelectAnimationLengths(GscRuntime *runtime,const cod1AnimationClipInput_t *input,size_t count,char *error,size_t size) {
    GscAnimationLengthHost host;uint64_t epoch;size_t i,j,n;int ok=0;
    if(!runtime||!input||!count||count>COD1_ANIMATION_LENGTH_MAX||
       !gsc_runtime_storage_separate(runtime,error,size)||!gsc_runtime_storage_separate(runtime,input,count*sizeof(*input))||
       Cod1AnimationLengthOverlap(input,count*sizeof(*input),error,size)||
       Cod1AnimationLengthOverlap(error,size,&cod1AnimationLengths,sizeof(cod1AnimationLengths))||
       Cod1AnimationLengthOverlap(error,size,&cod1AnimationLengthsStage,sizeof(cod1AnimationLengthsStage))||
       Cod1AnimationLengthOverlap(input,count*sizeof(*input),&cod1AnimationLengthsStage,sizeof(cod1AnimationLengthsStage)))return 0;
    /* Admit all source/output ranges before diagnostics or staging writes. */
    for(i=0;i<count;++i){
        if(!input[i].name||!input[i].bytes||!input[i].size||
           !gsc_runtime_storage_separate(runtime,input[i].name,1)||!gsc_runtime_storage_separate(runtime,input[i].bytes,input[i].size)||
           Cod1AnimationLengthOverlap(input[i].bytes,input[i].size,error,size)||
           Cod1AnimationLengthOverlap(input[i].bytes,input[i].size,&cod1AnimationLengthsStage,sizeof(cod1AnimationLengthsStage)))return 0;
        for(n=0;n<64&&input[i].name[n];++n){}
        if(!n||n==64||Cod1AnimationLengthOverlap(input[i].name,n+1,error,size)||!gsc_runtime_storage_separate(runtime,input[i].name,n+1)||
           Cod1AnimationLengthOverlap(input[i].name,n+1,&cod1AnimationLengthsStage,sizeof(cod1AnimationLengthsStage)))return 0;
    }
    epoch=G_Cod1EntityMapEpoch();
    if(cod1AnimationLengthsBusy||!epoch||cod1AnimationLengths.epoch==epoch)return 0;
    cod1AnimationLengthsBusy=1;memset(&cod1AnimationLengthsStage,0,sizeof(cod1AnimationLengthsStage));
    for(i=0;i<count;++i){ca14ActorClip clip;cod1AnimationLengthRow_t *row=&cod1AnimationLengthsStage.rows[i];memset(&clip,0,sizeof(clip));
        for(n=0;input[i].name[n];++n){unsigned char c=(unsigned char)input[i].name[n];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))goto done;}
        for(j=0;j<i;++j)if(Cod1AnimationLengthNameEqual(input[i].name,cod1AnimationLengthsStage.rows[j].name))goto done;
        if(!CA14_ActorClipLoad(input[i].bytes,input[i].size,&clip))goto done;
        strcpy(row->name,input[i].name);
        /* Original loader 478350..478413: last-frame count is fileFrames-1
         * plus loop bit; FPS is the signed16 file cell converted to exact positive float. */
        row->metadata.frames=(uint32_t)(clip.frames-1+(clip.originalFlags&1));row->metadata.fps=(uint32_t)clip.fps;CA14_ActorClipFree(&clip);
    }
    cod1AnimationLengthsStage.epoch=epoch;cod1AnimationLengthsStage.runtime=gsc_runtime_instance_identity(runtime);cod1AnimationLengthsStage.count=count;
    memset(&host,0,sizeof(host));host.query=Cod1AnimationLengthQuery;
    if(epoch!=G_Cod1EntityMapEpoch()||!gsc_runtime_select_animation_length(runtime,&host)||epoch!=G_Cod1EntityMapEpoch())goto done;
    cod1AnimationLengths=cod1AnimationLengthsStage;ok=1;
 done:
    memset(&cod1AnimationLengthsStage,0,sizeof(cod1AnimationLengthsStage));cod1AnimationLengthsBusy=0;
    if(!ok&&error&&size)snprintf(error,size,"animation clip table admission refused");
    return ok;
}
#endif
