#ifndef COD1_CAMPAIGN_FEATURES47_H
#define COD1_CAMPAIGN_FEATURES47_H
/* Known original entry requirements, not an activation certificate. The
 * native descriptors will be selected only by a later map-owned admission.
 * Legacy callers still perform exactly one live cvar read per invocation. */
#include <stddef.h>
#define C1CP47_FEATURES(X) \
 X(ENABLE,g_cod1GscTest,1,1) \
 X(MODE,g_cod1GscEntityTest,43,43) \
 X(SOUND,g_cod1GscScriptSound,1,1) \
 X(OPENING,g_cod1GscTrainingOpening,1,0) \
 X(FINALE,g_cod1GscTrainingFinale,1,0) \
 X(VOICE,g_cod1GscStreamedVoice,1,1) \
 X(SOUND_NOTIFY,g_cod1GscSoundNotify,1,1) \
 X(GRENADES,g_cod1GscTrainingGrenades,1,0) \
 X(AI_ARRAYS,g_cod1GscAiArrays,1,0) \
 X(ACTOR_PREFIX,g_cod1GscActorPrefix,1,0) \
 X(DELETE,g_cod1GscDeleteTransactions,1,0) \
 X(CARBINE,g_cod1GscTrainingCarbine,1,0) \
 X(TURRET,g_cod1GscR232Turret,1,0) \
 X(NATIVE_THREADS,g_cod1GscNativeThreads,1,0) \
 X(PLACED_INIT,g_cod1GscPlacedInit,1,0) \
 X(PLACED_STATE,g_cod1GscPlacedCurrentState,1,0) \
 X(CAPTIONS,g_cod1GscLoadedCaptions,1,1) \
 X(MOODY,g_cod1GscMoodyCharacter,1,0) \
 X(RIGS,g_cod1GscTrainingRigs,1,0) \
 X(CLIENT_STARTUP,g_cod1GscClientStartup,1,1) \
 X(GRAPH_STARTUP,g_cod1GscGraphStartup,1,0) \
 X(BSP_CONSTRUCT,g_cod1GscBspConstruct,1,0) \
 X(PLACED_COHORT,g_cod1GscPlacedCohort,1,0)
typedef enum {
#define C1CP47_ENUM(id,name,training,intro) C1CP47_F_##id,
 C1CP47_FEATURES(C1CP47_ENUM)
#undef C1CP47_ENUM
 C1CP47_F_COUNT
} c1cp47Feature;
typedef enum { C1CP47_PROFILE_TRAINING=1,C1CP47_PROFILE_US_INTRO=2 } c1cp47Profile;
typedef struct {const char *name;int training,intro;} c1cp47FeatureSpec;
static const c1cp47FeatureSpec c1cp47Features[C1CP47_F_COUNT]={
#define C1CP47_ROW(id,name,training,intro) {#name,training,intro},
 C1CP47_FEATURES(C1CP47_ROW)
#undef C1CP47_ROW
};
#undef C1CP47_FEATURES
static inline const char *C1CP47_FeatureName(c1cp47Feature feature){
 return (unsigned)feature<C1CP47_F_COUNT?c1cp47Features[feature].name:NULL;
}
static inline int C1CP47_ReadLegacy(c1cp47Feature feature,int(*read)(const char*)){
 const char *name=C1CP47_FeatureName(feature);
 return name&&read?read(name):0;
}
/* Values mirror the accepted Training CFG and its explicit us_intro
 * transition, including retained captions. No Pathfinder profile: it is
 * not yet admitted as normal campaign gameplay. */
static inline int C1CP47_ExpectedFeature(c1cp47Profile profile,c1cp47Feature feature,int *out){
 if(!out||(unsigned)feature>=C1CP47_F_COUNT)return 0;
 if(profile==C1CP47_PROFILE_TRAINING)*out=c1cp47Features[feature].training;
 else if(profile==C1CP47_PROFILE_US_INTRO)*out=c1cp47Features[feature].intro;
 else return 0;
 return 1;
}
#endif
