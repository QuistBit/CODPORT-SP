#ifndef G_COD1_ACTOR_THREAD_AUTHORITY_H
#define G_COD1_ACTOR_THREAD_AUTHORITY_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
/* Pure existing allocation/Actor+Sentient lease identity. Neither reserves nor
 * initializes an actor and makes no alive/controller/readiness promise. */
const GscThreadEntityAuthorityHost *G_Cod1ActorThreadAuthorityHost(void);
void G_Cod1BindReadOnlyActor(GscReadOnlyContext *context);
/* Selected SP script receivers: completed current human-player identity, or
 * the unchanged Actor/spawner authority. No path/pose/effect grant. */
const GscThreadEntityAuthorityHost *G_Cod1ScriptThreadAuthorityHost(void);
#endif
#endif
