/* OPT R27: opt-in wall-clock profiling for synchronous map-load scopes.
 * Stack-only state; no state survives ERR_DROP or a nested load.
 * Include after qcommon declarations. No new engine/module ABI. */
#ifndef COD1_LOAD_PROFILE_H
#define COD1_LOAD_PROFILE_H

/* Allow a module to use the same clock/report logic through its existing
 * syscall adapters. Ordinary engine translation units keep R27 defaults. */
#ifndef C1LP_ENABLED
#define C1LP_ENABLED() (Cvar_Get( "com_cod1LoadProfile", "0", CVAR_TEMP )->integer != 0)
#define C1LP_NOW() Sys_Milliseconds()
#define C1LP_PRINT Com_Printf
#define C1LP_DEFAULT_PROVIDER
#endif

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
	p->enabled = C1LP_ENABLED();
	p->count = 0;
	if ( p->enabled ) {
		p->start = p->last = (unsigned)C1LP_NOW();
	}
}

static void Cod1LoadProfileMark( cod1LoadProfile_t *p, const char *stage ) {
	unsigned now;
	if ( !p->enabled || p->count >= COD1_LOAD_PROFILE_MAX_STAGES ) {
		return;
	}
	now = (unsigned)C1LP_NOW();
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
		C1LP_PRINT( "[OPT_LOAD] scope=%s map=%s stage=%s ms=%u\n",
			scope, map, p->stage[i], p->elapsed[i] );
	}
	C1LP_PRINT( "[OPT_LOAD] scope=%s map=%s total_ms=%u\n",
		scope, map, p->last - p->start );
}

#ifdef C1LP_DEFAULT_PROVIDER
#undef C1LP_DEFAULT_PROVIDER
#undef C1LP_ENABLED
#undef C1LP_NOW
#undef C1LP_PRINT
#endif

#endif
