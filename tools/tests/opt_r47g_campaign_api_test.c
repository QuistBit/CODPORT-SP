/* Compile-only consumer of the public interface; no provider stubs needed.
 * Run against both current and 609A-overlay g_cod1_gsc_test.h. */
#ifdef R47G_API_FIRST
#include "g_cod1_campaign_api47.h"
#include "g_cod1_gsc_test.h"
#else
#include "g_cod1_gsc_test.h"
#include "g_cod1_campaign_api47.h"
#endif
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
int R47G_Consumer(void){
 int (*read)(c1cp47Feature)=G_Cod1GscFeature47;
 G_Cod1GscCampaignBegin47(0);
 G_Cod1GscCampaignObserve47("training");
 G_Cod1GscCampaignRetire47();
 return read(C1CP47_F_TURRET)+read(C1CP47_F_MODE)+read(C1CP47_F_ENABLE)
       +read(C1CP47_F_GRENADES)+read(C1CP47_F_OPENING);
}
#else
int R47G_Consumer(void){
 G_Cod1GscCampaignBegin47(0);
 G_Cod1GscCampaignObserve47("training");
 G_Cod1GscCampaignRetire47();
 return 0;
}
#endif
