#ifndef COD1_CAMPAIGN_LANE47_H
#define COD1_CAMPAIGN_LANE47_H
#include "g_cod1_campaign_owner47.h"
/* An absent/empty optional selector means normal campaign startup. Only
 * explicit legacy/invalid values opt out; world and legacy fences still apply. */
static inline int C1CP47_NativeRequested(const char *s){
 return s&&(!*s||!strcmp(s,"1"));
}
/* The lane is selected once, before BSP construction. Revocation cannot
 * convert a native Session to legacy configuration. Only a newer map arm
 * clears the native latch. The live world/receipt gates stay separate. */
static inline int C1CP47_OwnerActivate(c1cp47Owner *o,uint64_t epoch,
 const char *live,int sp,int bsp59,int restarted,int save,int requested){
 if(requested!=1||!o||o->native||o->phase!=C1CP47_O_CANDIDATE||
    !C1CP47_OwnerCurrent(o,epoch,live,sp,bsp59,restarted,save))return 0;
 o->native=1;o->phase=C1CP47_O_ACTIVE;return 1;
}
/* -1 legacy; 0 native revoked or stale; 1 active native. No engine reads. */
static inline int C1CP47_Lane(const c1cp47Owner *o,uint64_t epoch,int sp,int bsp59,int restarted){
 if(!o||!o->native)return -1;
 return epoch&&o->epoch==epoch&&o->phase==C1CP47_O_ACTIVE&&sp&&bsp59&&!restarted&&C1CP47_ProfileMap(o->profile)!=NULL;
}
static inline int C1CP47_LaneInteger(int lane,c1cp47Profile profile,c1cp47Feature feature,int(*read)(const char*)){
 int value=0;
 if(lane<0)return C1CP47_ReadLegacy(feature,read);
 if(lane==1)(void)C1CP47_ExpectedFeature(profile,feature,&value);
 return value;
}
static inline void C1CP47_LaneString(int lane,c1cp47Profile profile,unsigned key,char *out,int cap,void(*read)(const char*,char*,int)){
 const char *value="";int number;size_t n;
 if(!out||cap<=0)return;
 if(lane<0||!C1CP47_MapOwnedKey(key)){C1CP47_ReadLegacyString(key,out,cap,read);return;}
 if(lane==1){
  if(key==C1CP47_K_ROOT){
   const c1cp47Route *route=C1CP47_FindMap(C1CP47_ProfileMap(profile));
   if(route)value=route->root;
  }else if(C1CP47_ExpectedFeature(profile,(c1cp47Feature)key,&number)){
   value=number==43?"43":number==1?"1":"0";
  }
 }
 n=strlen(value);if(n>=(size_t)cap)n=(size_t)cap-1;
 memcpy(out,value,n);out[n]=0;
}
#endif
