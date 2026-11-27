#include <assert.h>
#include <stdio.h>
#include "g_cod1_campaign_lane47.h"
static unsigned calls;
static int rawInt(const char *name){assert(name);++calls;return 77;}
static void rawString(const char *name,char *out,int cap){assert(name&&cap>0);++calls;out[0]='x';if(cap>1)out[1]=0;else out[0]=0;}
int main(void){
 const char *maps[]={"training","us_intro"};unsigned m,i,j,cases=0;
 for(m=0;m<2;m++){
  c1cp47Owner o={0};assert(C1CP47_OwnerArm(&o,7,0));
  assert(C1CP47_OwnerAdmit(&o,7,maps[m],maps[m],1,1,0,0,0,0,0));
  assert(C1CP47_Lane(&o,7,1,1,0)==-1);
  assert(!C1CP47_OwnerActivate(&o,7,maps[m],1,1,0,0,0));
  assert(!C1CP47_OwnerActivate(&o,8,maps[m],1,1,0,0,1));
  assert(!C1CP47_OwnerActivate(&o,7,"dam",1,1,0,0,1));
  assert(!C1CP47_OwnerActivate(&o,7,maps[m],1,1,0,1,1));
  assert(C1CP47_OwnerActivate(&o,7,maps[m],1,1,0,0,1));
  assert(!C1CP47_OwnerActivate(&o,7,maps[m],1,1,0,0,1));
  assert(C1CP47_Lane(&o,7,1,1,0)==1&&C1CP47_OwnerCurrent(&o,7,maps[m],1,1,0,0));
  assert(!C1CP47_Lane(&o,8,1,1,0)&&!C1CP47_Lane(&o,7,0,1,0)&&!C1CP47_Lane(&o,7,1,0,0)&&!C1CP47_Lane(&o,7,1,1,1));
  for(i=0;i<C1CP47_F_COUNT;i++){
   int expect;assert(C1CP47_ExpectedFeature(o.profile,(c1cp47Feature)i,&expect));
   calls=0;assert(C1CP47_LaneInteger(-1,o.profile,(c1cp47Feature)i,rawInt)==77&&calls==1);
   calls=0;assert(C1CP47_LaneInteger(1,o.profile,(c1cp47Feature)i,rawInt)==expect&&!calls);
   assert(C1CP47_LaneInteger(0,o.profile,(c1cp47Feature)i,rawInt)==0&&!calls);++cases;
  }
  for(i=0;i<C1CP47_K_COUNT;i++)for(j=1;j<=32;j++){
   char a[40],b[40];const char *expected;int n;size_t len;
   memset(a,0xa5,sizeof(a));memset(b,0xa5,sizeof(b));calls=0;
   C1CP47_LaneString(1,o.profile,i,a,(int)j,rawString);
   if(i<C1CP47_F_COUNT){assert(C1CP47_ExpectedFeature(o.profile,(c1cp47Feature)i,&n));expected=n==43?"43":n==1?"1":"0";}
   else if(i==C1CP47_K_ROOT)expected=m?"maps/us_intro.gsc":"maps/training.gsc";
   else expected="x";
   len=strlen(expected);if(len>=j)len=j-1;memcpy(b,expected,len);b[len]=0;
   assert(calls==(i>C1CP47_K_ROOT)&&!memcmp(a,b,sizeof(a)));
   memset(a,0xa5,sizeof(a));calls=0;C1CP47_LaneString(0,o.profile,i,a,(int)j,rawString);
   assert(calls==(i>C1CP47_K_ROOT));assert(a[0]==((i>C1CP47_K_ROOT&&j>1)?'x':0));
   calls=0;C1CP47_LaneString(-1,o.profile,i,a,(int)j,rawString);assert(calls==1);++cases;
  }
  C1CP47_OwnerRetire(&o);assert(o.native&&!C1CP47_Lane(&o,7,1,1,0));
  assert(!C1CP47_OwnerArm(&o,7,0)&&!C1CP47_Lane(&o,7,1,1,0));
  assert(C1CP47_OwnerArm(&o,8,0)&&C1CP47_Lane(&o,8,1,1,0)==-1);
  assert(!C1CP47_OwnerActivate(&o,7,maps[m],1,1,0,0,1));
 }
 printf("R47E lane: %u cases PASS\n",cases);return 0;
}
