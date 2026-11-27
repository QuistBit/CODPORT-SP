#ifndef COD1_CAMPAIGN_CONFIG47_H
#define COD1_CAMPAIGN_CONFIG47_H
/* Feature IDs occupy the original integer gateway's numeric range. The
 * remaining keys are live engine state or the separately selected root. */
#include "g_cod1_campaign_features47.h"
#include <stdint.h>
#include <string.h>
typedef enum {
 C1CP47_K_ROOT=C1CP47_F_COUNT,
 C1CP47_K_MAP,C1CP47_K_SAVE,C1CP47_K_DEVELOPER,C1CP47_K_COUNT
} c1cp47Key;
static inline const char *C1CP47_KeyName(unsigned key){
 if(key<C1CP47_F_COUNT)return C1CP47_FeatureName((c1cp47Feature)key);
 switch(key){
 case C1CP47_K_ROOT:return "g_cod1GscTestScript";
 case C1CP47_K_MAP:return "mapname";
 case C1CP47_K_SAVE:return "savegame_loading";
 case C1CP47_K_DEVELOPER:return "developer_script";
 default:return NULL;
 }
}
static inline void C1CP47_ReadLegacyString(unsigned key,char *out,int cap,void(*read)(const char*,char*,int)){
 const char *name=C1CP47_KeyName(key);
 if(!out||cap<=0)return;
 if(name&&read)read(name,out,cap);else out[0]=0;
}
/* No engine state or feature grant is implied by a key classification.
 * Only the 23 features and root are candidates for immutable ownership. */
static inline int C1CP47_MapOwnedKey(unsigned key){return key<=C1CP47_K_ROOT;}
#endif
