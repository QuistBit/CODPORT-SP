#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "g_cod1_campaign_config47.h"
static const char *const names[]={
 "g_cod1GscTest","g_cod1GscEntityTest","g_cod1GscScriptSound","g_cod1GscTrainingOpening","g_cod1GscTrainingFinale","g_cod1GscStreamedVoice","g_cod1GscSoundNotify","g_cod1GscTrainingGrenades","g_cod1GscAiArrays","g_cod1GscActorPrefix","g_cod1GscDeleteTransactions","g_cod1GscTrainingCarbine","g_cod1GscR232Turret","g_cod1GscNativeThreads","g_cod1GscPlacedInit","g_cod1GscPlacedCurrentState","g_cod1GscLoadedCaptions","g_cod1GscMoodyCharacter","g_cod1GscTrainingRigs","g_cod1GscClientStartup","g_cod1GscGraphStartup","g_cod1GscBspConstruct","g_cod1GscPlacedCohort","g_cod1GscTestScript","mapname","savegame_loading","developer_script"
};
static const char *expected,*value;static unsigned calls;
static void readLive(const char *name,char *out,int cap){size_t n=strlen(value);assert(!strcmp(name,expected)&&cap>0);calls++;if(n>=(size_t)cap)n=(size_t)cap-1;memcpy(out,value,n);out[n]=0;}
int main(void){
 static const char *const values[]={"","0","01","-1","43","1.0","maps/training.gsc","maps/us_intro.gsc"};
 unsigned i,j,k,cases=0;char out[64],ref[64];
 assert(sizeof(names)/sizeof(names[0])==C1CP47_K_COUNT);
 for(i=0;i<C1CP47_K_COUNT;i++){
  assert(!strcmp(C1CP47_KeyName(i),names[i]));assert(C1CP47_MapOwnedKey(i)==(i<=C1CP47_K_ROOT));
  for(j=0;j<sizeof(values)/sizeof(values[0]);j++)for(k=1;k<=sizeof(out);k+=7){
   expected=names[i];value=values[j];memset(out,0xa5,sizeof(out));memset(ref,0xa5,sizeof(ref));
   readLive(expected,ref,(int)k);calls=0;C1CP47_ReadLegacyString(i,out,(int)k,readLive);
   assert(calls==1&&!memcmp(out,ref,sizeof(out)));++cases;
  }
 }
 calls=0;out[0]='x';C1CP47_ReadLegacyString(C1CP47_K_COUNT,out,sizeof(out),readLive);assert(!calls&&!out[0]);
 out[0]='x';C1CP47_ReadLegacyString(C1CP47_K_ROOT,out,0,readLive);assert(!calls&&out[0]=='x');
 C1CP47_ReadLegacyString(C1CP47_K_ROOT,NULL,1,readLive);assert(!calls);
 assert(!C1CP47_KeyName((unsigned)-1)&&!C1CP47_MapOwnedKey((unsigned)-1));
 printf("R47C configuration string gateway: %u cases PASS\n",cases);return 0;
}
