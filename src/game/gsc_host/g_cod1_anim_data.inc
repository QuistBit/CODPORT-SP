#include "../g_cod1_anim_data.h"
static uint64_t animDataSerial,animDataEpoch,animDataReceipt,animDataPending,animDataRuntime;
static int animDataBusy;
static void AnimDataReset(void){animDataEpoch=animDataReceipt=animDataPending=animDataRuntime=0;}
static int AnimDataCurrent(uint64_t receipt){
    return receipt&&ActorMapAvailable()&&cod1EntityReady&&!level.spawning&&!level.restarted&&
        animDataEpoch==cod1EntityEpoch&&(receipt==animDataReceipt||receipt==animDataPending);
}
int G_Cod1SelectAnimData(GscRuntime *runtime,char *error,size_t size){
    GscMapExecutionCertificate certificate;uint64_t epoch;int selected;
    if(!runtime||!gsc_runtime_storage_separate(runtime,error,size))return 0;
    if(animDataBusy||animDataReceipt||!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||animDataSerial==UINT64_MAX){EntityError(error,size,"anim data requires an unbound ready map");return 0;}
    epoch=cod1EntityEpoch;animDataEpoch=epoch;animDataPending=++animDataSerial;
    certificate.receipt=animDataPending;certificate.current=AnimDataCurrent;animDataBusy=1;
    selected=gsc_runtime_select_anim_data(runtime,&certificate);animDataBusy=0;
    if(!selected||epoch!=cod1EntityEpoch||!AnimDataCurrent(certificate.receipt)){animDataPending=0;EntityError(error,size,"anim data runtime admission refused");return 0;}
    animDataRuntime=gsc_runtime_instance_identity(runtime);animDataReceipt=animDataPending;animDataPending=0;return 1;
}
