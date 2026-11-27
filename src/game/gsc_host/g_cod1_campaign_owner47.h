#ifndef COD1_CAMPAIGN_OWNER47_H
#define COD1_CAMPAIGN_OWNER47_H
#include "g_cod1_campaign_entry47.h"
#include "g_cod1_campaign_config47.h"
/* One prospective map profile, never a Session or a capability grant.
 * The epoch remains as a high-water mark on refusal and retirement. */
enum { C1CP47_O_EMPTY, C1CP47_O_ARMED, C1CP47_O_CANDIDATE,
 C1CP47_O_REFUSED, C1CP47_O_RETIRED };
typedef struct {uint64_t epoch;unsigned phase;c1cp47Profile profile;} c1cp47Owner;
static inline void C1CP47_OwnerRetire(c1cp47Owner *o){
 if(o){o->phase=C1CP47_O_RETIRED;o->profile=(c1cp47Profile)0;}
}
static inline int C1CP47_OwnerArm(c1cp47Owner *o,uint64_t epoch,int restart){
 if(!o||!epoch||epoch==UINT64_MAX||epoch<=o->epoch)return 0;
 o->epoch=epoch;o->profile=(c1cp47Profile)0;
 o->phase=restart?C1CP47_O_REFUSED:C1CP47_O_ARMED;
 return o->phase==C1CP47_O_ARMED;
}
static inline int C1CP47_OwnerPending(const c1cp47Owner *o,uint64_t epoch){
 return o&&epoch&&o->epoch==epoch&&o->phase==C1CP47_O_ARMED;
}
static inline const char *C1CP47_ProfileMap(c1cp47Profile profile){
 switch(profile){case C1CP47_PROFILE_TRAINING:return "training";
 case C1CP47_PROFILE_US_INTRO:return "us_intro";default:return NULL;}
}
/* Strict default-only admission for the prospective normal lane. Preserve
 * explicit legacy configurations even if a noncanonical value parses as 0. */
static inline int C1CP47_DefaultFeatureString(const char *s){return s&&(!*s||!strcmp(s,"0"));}
static inline int C1CP47_DefaultRootString(const char *s){
 return s&&(!*s||!strcmp(s,"scripts/gsc_tests/r76_fog.gsc"));
}
static inline int C1CP47_OwnerAdmit(c1cp47Owner *o,uint64_t epoch,
 const char *probed,const char *live,int singlePlayer,int bsp59,int restarted,
 int saveLoading,int dedicated,int legacy,int blocked){
 const c1cp47Route *route;
 if(!C1CP47_OwnerPending(o,epoch))return 0;
 o->phase=C1CP47_O_REFUSED;
 if(!singlePlayer||!bsp59||restarted||saveLoading||dedicated||legacy||blocked||
    !probed||!live||strcmp(probed,live)||(route=C1CP47_FindMap(probed))==NULL)return 0;
 if(route->id==C1CP47_TRAINING)o->profile=C1CP47_PROFILE_TRAINING;
 else if(route->id==C1CP47_US_INTRO)o->profile=C1CP47_PROFILE_US_INTRO;
 else return 0;
 o->phase=C1CP47_O_CANDIDATE;return 1;
}
static inline int C1CP47_OwnerCurrent(const c1cp47Owner *o,uint64_t epoch,
 const char *live,int singlePlayer,int bsp59,int restarted,int saveLoading){
 const char *map;
 return o&&epoch&&o->epoch==epoch&&o->phase==C1CP47_O_CANDIDATE&&
  singlePlayer&&bsp59&&!restarted&&!saveLoading&&live&&
  (map=C1CP47_ProfileMap(o->profile))!=NULL&&!strcmp(map,live);
}
#endif
