/* OPT R27: opt-in wall-clock profiling for synchronous map-load scopes.
 * Stack-only state; no state survives ERR_DROP or a nested load.
 * Include after qcommon declarations. No new engine/module ABI. */
#ifndef COD1_LOAD_PROFILE_H
#define COD1_LOAD_PROFILE_H

#define COD1_LOAD_PROFILE_MAX_STAGES 16
typedef struct {
	qboolean enabled;
	unsigned start;
	unsigned last;
	unsigned count;
	const char *stage[COD1_LOAD_PROFILE_MAX_STAGES];
	unsigned elapsed[COD1_LOAD_PROFILE_MAX_STAGES];
} cod1LoadProfile_t;

static void Cod1LoadProfileBegin( cod1LoadProfile_t *p ) {
	p->enabled = Cvar_Get( "com_cod1LoadProfile", "0", CVAR_TEMP )->integer != 0;
	p->count = 0;
	if ( p->enabled ) {
		p->start = p->last = (unsigned)Sys_Milliseconds();
	}
}

static void Cod1LoadProfileMark( cod1LoadProfile_t *p, const char *stage ) {
	unsigned now;
	if ( !p->enabled || p->count >= COD1_LOAD_PROFILE_MAX_STAGES ) {
		return;
	}
	now = (unsigned)Sys_Milliseconds();
	p->stage[p->count] = stage;
	p->elapsed[p->count++] = now - p->last;
	p->last = now;
}

static void Cod1LoadProfileReport( const cod1LoadProfile_t *p, const char *scope, const char *map ) {
	unsigned i;
	if ( !p->enabled ) {
		return;
	}
	for ( i = 0; i < p->count; i++ ) {
		Com_Printf( "[OPT_LOAD] scope=%s map=%s stage=%s ms=%u\n",
			scope, map, p->stage[i], p->elapsed[i] );
	}
	Com_Printf( "[OPT_LOAD] scope=%s map=%s total_ms=%u\n",
		scope, map, p->last - p->start );
}

#endif
