#ifndef G_COD1_CAMPAIGN_API47_H
#define G_COD1_CAMPAIGN_API47_H
/* Current campaign interface, independent of historical GSC header overlays
 * used by the Windows compile groups. No game/protocol structures here. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "gsc_host/g_cod1_campaign_features47.h"
int G_Cod1GscFeature47(c1cp47Feature feature);
void G_Cod1GscCampaignBegin47(int restart);
void G_Cod1GscCampaignObserve47(const char *probedMap);
void G_Cod1GscCampaignRetire47(void);
#else
#define G_Cod1GscCampaignBegin47(restart) ((void)0)
#define G_Cod1GscCampaignObserve47(map) ((void)0)
#define G_Cod1GscCampaignRetire47() ((void)0)
#endif
#endif
