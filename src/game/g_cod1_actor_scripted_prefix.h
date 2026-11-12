/* R243 exclusive portable scripted-record/root-link owner. Constructed with
 * the actual Actor allocation, not inferred from NativeStatic or raw pointers.
 * Future record/link producers must transition this same owner. No restore or
 * pre-Actor root-link producer is admitted; such ownership must enter UNKNOWN.
 * DObj model/head/gear attachments are a different owner and remain allowed. */
#ifndef G_COD1_ACTOR_SCRIPTED_PREFIX_H
#define G_COD1_ACTOR_SCRIPTED_PREFIX_H
#include "cod1_gsc/gsc_fixture.h"
typedef enum {FRESH_AUX_UNKNOWN=0,FRESH_AUX_EMPTY=1,FRESH_AUX_OWNED=2} FreshRecordKind;
typedef enum {FRESH_LINK_UNKNOWN=0,FRESH_LINK_UNATTACHED=1,FRESH_LINK_ATTACHED=2} FreshLinkKind;
typedef enum {FRESH_PREFIX_UNUSED=0,FRESH_PREFIX_QUEUE_COMMITTED=1,FRESH_PREFIX_INVALID=2} FreshPrefixKind;
typedef struct {uint64_t actorSerial,revision;FreshRecordKind record;FreshLinkKind link;
 FreshPrefixKind prefix;int admitted;} cod1ActorScriptedAux_t;
const GscActorScriptedPrefixHost *G_Cod1ActorScriptedPrefixHost(void);
#endif
