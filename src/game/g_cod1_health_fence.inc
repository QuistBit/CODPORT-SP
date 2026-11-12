/* Sole owner g_items.c; selected health stages only. Other item lifecycles
 * retain their existing behavior. Resolve foreign calls to locals first. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_path_entity_host.h"
#define C1_HEALTH_BEFORE do{if(healthOwned)healthBefore=*ent;}while(0)
#define C1_HEALTH_AFTER do{if(healthOwned&&(!G_Cod1EntitySameAllocation(&healthRef)||memcmp(ent,&healthBefore,sizeof(healthBefore))))return;}while(0)
static int Cod1HealthLinkFence(gentity_t*e,const GscEntityRef*ref,const gentity_t*before){
 gentity_t current;
 if(!G_Cod1EntitySameAllocation(ref))return 0;current=*e;
 /* SV_LinkEntity owns only these entity stores; no pose/model/callback
  * mutation through its external call may mint a completed health owner. */
 current.r.linked=before->r.linked;current.r.linkcount=before->r.linkcount;
 VectorCopy(before->r.absmin,current.r.absmin);VectorCopy(before->r.absmax,current.r.absmax);
 current.s.solid=before->s.solid;
 return !memcmp(&current,before,sizeof(current));
}
#else
#define C1_HEALTH_BEFORE ((void)0)
#define C1_HEALTH_AFTER ((void)0)
#endif
