#ifndef G_COD1_ACTOR_SCRIPTED_RECORD_H
#define G_COD1_ACTOR_SCRIPTED_RECORD_H
#include "g_cod1_actor_scripted_prefix.h"
#include "../qcommon/cod1_actor_scripted_record14.h"
/* Explicit fresh normal/unattached owner. Requires real selected scripted
 * frame/resource owner and pending prefix. No default profile activation. */
const GscActorScriptedStartHost*G_Cod1ActorScriptedStartHost(void);
/* Selected positive-goal slot provenance only; unknown/cleared slot refuses.
 * Returns record presence AFTER this native-prefix invocation. Pose writes
 * are current-origin/angles only, no trajectory/link/publication. */
int G_Cod1ActorScriptedRecordUpdate(const GscEntityRef*,int,int*,char*,size_t,int(*)(void*),void*);
#endif
