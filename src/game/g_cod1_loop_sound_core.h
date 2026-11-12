#ifndef G_COD1_LOOP_SOUND_CORE_H
#define G_COD1_LOOP_SOUND_CORE_H
#include "g_cod1_script_sound_core.h"
/* Native GAME20032B10/20032B90. This models ONLY original flags at +a0 and
 * alias byte at +1, not RTCW flags/wire layout or allocation ownership.
 * Alias registry is shared with PlaySound, and precedes either state store.
 * No argument-count query, random variant selection, one-shot event or notify.
 * Providers must guard lifetime/currentness; never commit this value directly
 * to an entity after a callback without revalidating its allocation. */
typedef struct {uint32_t flags;uint8_t alias_index;} Cod1LoopSoundState;
static Cod1ScriptSoundResult Cod1LoopSoundPlay(Cod1LoopSoundState *s,int valid_receiver,
 const Cod1ScriptSoundOps *ops){
 char alias[COD1_SCRIPT_SOUND_TEXT];unsigned index;int exists;Cod1ScriptSoundResult result;
 if(!s||!ops)return COD1_SS_HELD;
 if(!valid_receiver)return COD1_SS_RECEIVER;
 memset(alias,0xa5,sizeof(alias));
 if(!ops->read_alias||ops->read_alias(ops->user,alias,sizeof(alias))!=1)return COD1_SS_ALIAS_GETTER;
 if(!Cod1ScriptSoundText(alias)||!ops->alias_exists)return COD1_SS_HELD;
 exists=ops->alias_exists(ops->user,alias);
 if(!exists)return COD1_SS_RESOURCE_MISSING;
 if(exists!=1)return COD1_SS_HELD;
 result=Cod1ScriptSoundIndex(ops,alias,&index);if(result!=COD1_SS_OK)return result;
 s->flags=(s->flags&~UINT32_C(1))|UINT32_C(0x20);
 s->alias_index=(uint8_t)index;return COD1_SS_OK;
}
static Cod1ScriptSoundResult Cod1LoopSoundStop(Cod1LoopSoundState *s,int valid_receiver){
 if(!s)return COD1_SS_HELD;
 if(!valid_receiver)return COD1_SS_RECEIVER;
 s->alias_index=0;return COD1_SS_OK;
}
#endif
