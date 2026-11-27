#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../src/game/gsc_host/g_cod1_campaign_features47.h"
/* Independent accepted CFG/transition reference, ordered by feature ID. */
static const char *const names[]={
 "g_cod1GscTest","g_cod1GscEntityTest","g_cod1GscScriptSound",
 "g_cod1GscTrainingOpening","g_cod1GscTrainingFinale","g_cod1GscStreamedVoice",
 "g_cod1GscSoundNotify","g_cod1GscTrainingGrenades","g_cod1GscAiArrays",
 "g_cod1GscActorPrefix","g_cod1GscDeleteTransactions","g_cod1GscTrainingCarbine",
 "g_cod1GscR232Turret","g_cod1GscNativeThreads","g_cod1GscPlacedInit",
 "g_cod1GscPlacedCurrentState","g_cod1GscLoadedCaptions","g_cod1GscMoodyCharacter",
 "g_cod1GscTrainingRigs","g_cod1GscClientStartup","g_cod1GscGraphStartup",
 "g_cod1GscBspConstruct","g_cod1GscPlacedCohort"
};
static const int intro[]={1,43,1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0};
static const int live[]={-7,0,1,2,43,47,99};
static const char *expected;static int value,calls;
static int readLive(const char *name){assert(!strcmp(name,expected));calls++;return value;}
int main(void){
 unsigned i,j,cases=0;int out;
 assert(sizeof(names)/sizeof(names[0])==C1CP47_F_COUNT);
 assert(sizeof(intro)/sizeof(intro[0])==C1CP47_F_COUNT);
 for(i=0;i<C1CP47_F_COUNT;i++){
  c1cp47Feature feature=(c1cp47Feature)i;
  assert(!strcmp(C1CP47_FeatureName(feature),names[i]));
  for(j=0;j<i;j++)assert(strcmp(names[j],names[i]));
  assert(C1CP47_ExpectedFeature(C1CP47_PROFILE_TRAINING,feature,&out)&&out==(i==1?43:1));
  assert(C1CP47_ExpectedFeature(C1CP47_PROFILE_US_INTRO,feature,&out)&&out==intro[i]);
  expected=names[i];
  for(j=0;j<sizeof(live)/sizeof(live[0]);j++){
   value=live[j];calls=0;assert(C1CP47_ReadLegacy(feature,readLive)==value&&calls==1);++cases;
  }
  calls=0;assert(!C1CP47_ReadLegacy(feature,NULL)&&!calls);
  out=123;assert(!C1CP47_ExpectedFeature((c1cp47Profile)0,feature,&out)&&out==123);
 }
 calls=0;out=123;
 assert(!C1CP47_FeatureName((c1cp47Feature)-1));
 assert(!C1CP47_ReadLegacy(C1CP47_F_COUNT,readLive)&&!calls);
 assert(!C1CP47_ExpectedFeature(C1CP47_PROFILE_TRAINING,C1CP47_F_COUNT,&out)&&out==123);
 assert(!C1CP47_ExpectedFeature(C1CP47_PROFILE_TRAINING,C1CP47_F_ENABLE,NULL));
 printf("R47B feature gateway: %u live reads, %u profile values PASS\n",cases,2u*C1CP47_F_COUNT);
 return 0;
}
