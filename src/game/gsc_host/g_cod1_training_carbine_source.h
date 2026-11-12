#ifndef G_COD1_TRAINING_CARBINE_SOURCE_H
#define G_COD1_TRAINING_CARBINE_SOURCE_H
#include "g_cod1_training_grenade_source.h"
static int G_Cod1TrainingCarbineSourceSelect(const char *path,char *data,size_t size){
    size_t i,j;int keep;const unsigned char *hash;const cod1TrainingGrenadeSpan_t *spans;size_t count,wanted;
    static const unsigned char hash0[32]={0xbb,0x86,0xa2,0xa2,0x1f,0x3f,0xa3,0x9c,0x8e,0x53,0xec,0xc5,0x9d,0x47,0xc0,0x99,0x09,0x5a,0x71,0x9a,0xa1,0x1e,0x59,0xc5,0x6d,0x7f,0x87,0xeb,0x24,0x17,0x03,0x15};
    static const cod1TrainingGrenadeSpan_t spans0[]={{14619u,16247u,0},{50312u,50555u,0},{50561u,50846u,0},{61122u,61686u,0},{58969u,60525u,0}};
    static const unsigned char hash1[32]={0xce,0x48,0x16,0xbf,0xbc,0xfe,0x2b,0xf9,0xd3,0x4e,0x49,0x94,0xb2,0x08,0x97,0x16,0x24,0x22,0x1e,0x9c,0xe7,0xbd,0x69,0x6e,0x8a,0x58,0x2e,0x8f,0x3a,0xe3,0x7e,0x27};
    static const cod1TrainingGrenadeSpan_t spans1[]={{3425u,3512u,0},{3259u,3421u,0},{3850u,4313u,0}};
    static const unsigned char hash2[32]={0x40,0xda,0xa9,0x32,0x0e,0x91,0x1b,0x4b,0xca,0x22,0xf9,0xe4,0x56,0x8c,0x33,0xd9,0x07,0xbf,0x10,0x17,0x3c,0xba,0xae,0xf4,0x1e,0xcd,0x7d,0x86,0x5c,0x72,0x50,0xde};
    static const cod1TrainingGrenadeSpan_t spans2[]={{0u,10518u,0},{12192u,13606u,0}};
    if(!strcmp(path,"maps/training.gsc")){hash=hash0;spans=spans0;count=sizeof(spans0)/sizeof(spans0[0]);wanted=61690;}
    else if(!strcmp(path,"maps/_utility.gsc")){hash=hash1;spans=spans1;count=sizeof(spans1)/sizeof(spans1[0]);wanted=25279;}
    else if(!strcmp(path,"maps/_autosave.gsc")){hash=hash2;spans=spans2;count=sizeof(spans2)/sizeof(spans2[0]);wanted=13608;}
    else return 0;
    if(!data||size!=wanted||!C1TGS_Matches((const unsigned char *)data,size,hash))return -1;
    for(i=0;i<size;i++){keep=0;for(j=0;j<count;j++)if(i>=spans[j].first&&i<spans[j].end){keep=1;break;}if(!keep&&data[i]!=13&&data[i]!=10)data[i]=32;}
    return 1;
}
#endif
