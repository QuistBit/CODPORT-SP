#include "../g_cod1_animation_refs.h"
static uint64_t animationRefsSerial,animationRefsEpoch,animationRefsReceipt,animationRefsPending,animationRefsRuntime;
static int animationRefsBusy;
static void AnimationRefsReset(void){animationRefsEpoch=animationRefsReceipt=animationRefsPending=animationRefsRuntime=0;}
static int AnimationRefsCurrent(uint64_t receipt){return receipt&&animationRefsEpoch==cod1EntityEpoch&&AnimDataCurrent(animDataReceipt)&&animationRefsRuntime==animDataRuntime&&(receipt==animationRefsReceipt||receipt==animationRefsPending);}
int G_Cod1SelectAnimationReferences(GscRuntime *runtime,const char *tree,const void *bytes,size_t length,char *error,size_t size){
 GscAnimationCatalog *catalog;GscMapExecutionCertificate certificate;uint64_t epoch;int selected;
 if(!runtime||!tree||!bytes||!gsc_runtime_storage_separate(runtime,error,size)||!gsc_runtime_storage_separate(runtime,tree,64)||!gsc_runtime_storage_separate(runtime,bytes,length))return 0;
 if(animationRefsBusy||animationRefsReceipt||!animDataReceipt||!AnimDataCurrent(animDataReceipt)||animDataRuntime!=gsc_runtime_instance_identity(runtime)||animationRefsSerial==UINT64_MAX){EntityError(error,size,"animation references require the bound anim VM and an unbound ready map");return 0;}
 catalog=gsc_animation_catalog_parse(tree,bytes,length,error,size);if(!catalog)return 0;
 epoch=cod1EntityEpoch;animationRefsEpoch=epoch;animationRefsRuntime=animDataRuntime;animationRefsPending=++animationRefsSerial;certificate.receipt=animationRefsPending;certificate.current=AnimationRefsCurrent;
 animationRefsBusy=1;selected=gsc_runtime_select_animation_references(runtime,catalog,&certificate);animationRefsBusy=0;gsc_animation_catalog_free(catalog);
 if(!selected||epoch!=cod1EntityEpoch||!AnimationRefsCurrent(certificate.receipt)){animationRefsPending=0;EntityError(error,size,"animation reference resource admission refused");return 0;}
 animationRefsReceipt=animationRefsPending;animationRefsPending=0;return 1;
}
