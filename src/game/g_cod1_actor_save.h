#ifndef G_COD1_ACTOR_SAVE_H
#define G_COD1_ACTOR_SAVE_H

/* Include only after g_local.h and bg_cod1_actor_runtime.h: g_local.h has
 * no include guard in this codebase. */

/* A developer-spawned visual probe has no stable CS_MODELS index across
 * save reloads. G_LoadGame clears entities omitted from a save. */
static qboolean G_Cod1ActorSkipSave( const gentity_t *ent ) {
	return ent && ent->classname &&
	       !Q_stricmp( ent->classname, COD1_ACTOR_NET_CLASSNAME );
}

#endif
