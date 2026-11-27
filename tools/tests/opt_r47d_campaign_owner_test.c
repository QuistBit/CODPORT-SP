#include <assert.h>
#include <stdio.h>
#include "g_cod1_campaign_owner47.h"
int main(void){
 const char *maps[]={NULL,"","training","us_intro","pathfinder","Training","dam","maps/training","../training"};
 unsigned a,b,mask,cases=0;
 for(a=0;a<sizeof(maps)/sizeof(maps[0]);a++)for(b=0;b<sizeof(maps)/sizeof(maps[0]);b++)for(mask=0;mask<128;mask++){
  c1cp47Owner o={0};int admitted,expected;
  int restart=(mask>>0)&1,sp=(mask>>1)&1,bsp=(mask>>2)&1,save=(mask>>3)&1,dedicated=(mask>>4)&1,legacy=(mask>>5)&1,blocked=(mask>>6)&1;
  assert(C1CP47_OwnerArm(&o,7,restart)==!restart);
  admitted=C1CP47_OwnerAdmit(&o,7,maps[a],maps[b],sp,bsp,0,save,dedicated,legacy,blocked);
  expected=!restart&&sp&&bsp&&!save&&!dedicated&&!legacy&&!blocked&&a==b&&(a==2||a==3);
  assert(admitted==expected);
  assert(o.epoch==7&&o.phase==(expected?C1CP47_O_CANDIDATE:C1CP47_O_REFUSED));
  assert(C1CP47_OwnerCurrent(&o,7,maps[b],sp,bsp,0,save)==expected);
  if(expected){
   int v;assert(o.profile==(a==2?C1CP47_PROFILE_TRAINING:C1CP47_PROFILE_US_INTRO));
   assert(C1CP47_ExpectedFeature(o.profile,C1CP47_F_MODE,&v)&&v==43);
   assert(!C1CP47_OwnerCurrent(&o,8,maps[b],1,1,0,0));
   assert(!C1CP47_OwnerCurrent(&o,7,"pathfinder",1,1,0,0));
   assert(!C1CP47_OwnerCurrent(&o,7,maps[b],1,1,1,0));
   assert(!C1CP47_OwnerCurrent(&o,7,maps[b],1,1,0,1));
   assert(!C1CP47_OwnerCurrent(&o,7,maps[b],0,1,0,0));
   assert(!C1CP47_OwnerCurrent(&o,7,maps[b],1,0,0,0));
  }
  assert(!C1CP47_OwnerAdmit(&o,7,"training","training",1,1,0,0,0,0,0));
  assert(!C1CP47_OwnerArm(&o,7,0));C1CP47_OwnerRetire(&o);
  assert(!C1CP47_OwnerCurrent(&o,7,maps[b],1,1,0,0));assert(!C1CP47_OwnerArm(&o,7,0));
  assert(C1CP47_OwnerArm(&o,8,0));
  assert(!C1CP47_OwnerAdmit(&o,7,"training","training",1,1,0,0,0,0,0));
  assert(C1CP47_OwnerPending(&o,8));C1CP47_OwnerRetire(&o);
  assert(!C1CP47_OwnerAdmit(&o,8,"training","training",1,1,0,0,0,0,0));
  assert(!C1CP47_OwnerArm(&o,0,0)&&!C1CP47_OwnerArm(&o,UINT64_MAX,0));++cases;
 }
 assert(C1CP47_DefaultFeatureString("")&&C1CP47_DefaultFeatureString("0"));
 assert(!C1CP47_DefaultFeatureString(NULL)&&!C1CP47_DefaultFeatureString("01")&&!C1CP47_DefaultFeatureString("0.0")&&!C1CP47_DefaultFeatureString("43")&&!C1CP47_DefaultFeatureString("abc"));
 assert(C1CP47_DefaultRootString("")&&C1CP47_DefaultRootString("scripts/gsc_tests/r76_fog.gsc"));
 assert(!C1CP47_DefaultRootString(NULL)&&!C1CP47_DefaultRootString("maps/training.gsc")&&!C1CP47_DefaultRootString("scripts/gsc_tests/other.gsc"));
 puts("R47D map owner: 10368 cases PASS");assert(cases==10368);return 0;
}
