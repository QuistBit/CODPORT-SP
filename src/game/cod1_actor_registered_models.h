/* R162 private registered attachment transaction. Existing import1017 and
 * its version1 records keep their exact layout. Native GAME DLLs use1018. */
#ifndef COD1_ACTOR_REGISTERED_MODELS_H
#define COD1_ACTOR_REGISTERED_MODELS_H
#include "cod1_actor_model_apply.h"
#define COD1_ACTOR_REGISTERED_MODELS_VERSION 1u
typedef struct {
    uint32_t modelIndex, reserved;
    cod1ModelResourceReference_t resource;
    char nativeName[MAX_QPATH];
} cod1ActorRegisteredModelInput_t;
typedef struct {
    uint32_t version, bytes, relink, attachmentCount;
    cod1ActorModelApply_t model;
    cod1ActorRegisteredModelInput_t attachments[COD1_DOBJ_MODELS_MAX-1];
} cod1ActorRegisteredModelsApply_t;
/* attachmentCount0..7. With a base the count equals build.modelCount-1;
 * without a base build.modelCount is0, while saved registrations still get
 * validated. Every attachment uses its actual decoded bank reference. Tails
 * are zero. relink1 is setModel's ordinary area relink; relink0 only rebuilds
 * topology and advances the existing lane revision. It requires the exact
 * previous base registration/path, first-model bounds and unchanged pose/
 * hull/native flags. No ordinary unlink/link or linkcount/PVS write occurs.
 * Refusal preserves output and any previous topology/area registration.
 * Internal constructor API only: no GSC tag-ID/lookup/ScriptError adapter,
 * empty-model native holes, current animation or actor publication. ABI59. */
#endif
