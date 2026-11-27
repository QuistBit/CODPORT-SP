/* Regression: no selector and no Training CFG must activate admitted maps. */
#include <assert.h>
#include <stdio.h>
#include "g_cod1_campaign_lane47.h"
static unsigned reads;
static int rawInt(const char *name){assert(name);++reads;return 0;}
static void rawString(const char *name,char *out,int cap){assert(name&&cap>0);++reads;out[0]=0;}
int main(void){
 const char *maps[]={"training","us_intro","pathfinder","dam"};
 const char *selectors[]={"","1","0","01","1.0","abc",NULL};
 unsigned m,s,mask,i,cases=0;
 for(m=0;m<4;m++)for(s=0;s<7;s++)for(mask=0;mask<128;mask++){
  c1cp47Owner o={0};char root[64];int requested,admitted,active,expected;
  int sp=!(mask&1),bsp=!(mask&2),restart=!!(mask&4),save=!!(mask&8);
  int dedicated=!!(mask&16),legacy=!!(mask&32),blocked=!!(mask&64);
  requested=C1CP47_NativeRequested(selectors[s]);assert(requested==(s<2));
  (void)C1CP47_OwnerArm(&o,1,restart);
  admitted=C1CP47_OwnerAdmit(&o,1,maps[m],maps[m],sp,bsp,restart,save,dedicated,legacy,blocked);
  active=admitted&&C1CP47_OwnerActivate(&o,1,maps[m],sp,bsp,restart,save,requested);
  expected=m<2&&s<2&&!mask;assert(active==expected);
  assert(C1CP47_Lane(&o,1,sp,bsp,restart)==(expected?1:-1));
  if(expected){
   reads=0;
   for(i=0;i<C1CP47_F_COUNT;i++){
    int v;assert(C1CP47_ExpectedFeature(o.profile,(c1cp47Feature)i,&v));
    assert(C1CP47_LaneInteger(1,o.profile,(c1cp47Feature)i,rawInt)==v);
   }
   C1CP47_LaneString(1,o.profile,C1CP47_K_ROOT,root,sizeof(root),rawString);
   assert(!strcmp(root,m?"maps/us_intro.gsc":"maps/training.gsc")&&!reads);
   C1CP47_OwnerRetire(&o);assert(C1CP47_Lane(&o,1,1,1,0)==0);
   assert(C1CP47_OwnerArm(&o,2,0));
   assert(C1CP47_OwnerAdmit(&o,2,"us_intro","us_intro",1,1,0,0,0,0,0));
   assert(C1CP47_OwnerActivate(&o,2,"us_intro",1,1,0,0,C1CP47_NativeRequested("")));
   C1CP47_LaneString(1,o.profile,C1CP47_K_ROOT,root,sizeof(root),rawString);
   assert(!strcmp(root,"maps/us_intro.gsc")&&!reads);
  }
  ++cases;
 }
 assert(cases==3584);printf("R47I automatic campaign: %u cases PASS\n",cases);return 0;
}
