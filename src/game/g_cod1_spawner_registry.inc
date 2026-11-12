/* Included only by g_spawn.c after g_local.h and g_cod1_mapspawn.h.
 * Retail gamex86.dll SHA27930aa3..., actor_ case-sensitive prefix dispatcher,
 * spawner init 0x2006b180, getspawnerarray 0x20030c10, named setter 0x200385b0.
 * Native entity byte type12 is represented by this registry, not s.eType=12.
 * Full actor/sentient/physics/path/controller/script initialization is absent. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_actor.h"

typedef struct {
    cod1SpawnerSnapshot_t snapshot;
    uint64_t intakeIdentity;
    unsigned short keyOffset[MAX_SPAWN_VARS];
    unsigned short valueOffset[MAX_SPAWN_VARS];
    char raw[MAX_SPAWN_VARS_CHARS];
} cod1SpawnerRecord_t;

static cod1SpawnerRecord_t cod1Spawners[MAX_GENTITIES];
static uint64_t cod1SpawnerEpoch;
static uint64_t cod1SpawnerIntakeSerial;

static int Cod1SpawnerOverlap( const void *a, size_t as,
    const void *b, size_t bs ) {
    uintptr_t aa, bb;
    if ( !a || !b || !as || !bs ) return 0;
    aa = (uintptr_t)a; bb = (uintptr_t)b;
    return aa <= bb ? bb - aa < as : aa - bb < bs;
}

static int Cod1SpawnerError( char *error, size_t size, const char *text ) {
    if ( error && size ) Q_strncpyz( error, text, size );
    return 0;
}

void G_Cod1SpawnerMapBegin( void ) {
    memset( cod1Spawners, 0, sizeof( cod1Spawners ) );
    cod1SpawnerEpoch = G_Cod1EntityMapEpoch();
}

static int Cod1SpawnerMapCurrent( void ) {
    return cod1SpawnerEpoch && cod1SpawnerEpoch == G_Cod1EntityMapEpoch() &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER;
}

static const cod1SpawnerRecord_t *Cod1SpawnerFind( const GscEntityRef *ref ) {
    const cod1SpawnerRecord_t *record;
    const gentity_t *ent;
    if ( !Cod1SpawnerMapCurrent() || !ref || ref->slot < MAX_CLIENTS ||
        ref->slot >= ENTITYNUM_MAX_NORMAL ||
        !G_Cod1EntitySameAllocation( ref ) ) return NULL;
    record = &cod1Spawners[ref->slot];
    if ( record->snapshot.entity.epoch != ref->epoch ||
        record->snapshot.entity.generation != ref->generation ||
        record->snapshot.entity.slot != ref->slot ) return NULL;
    ent = &g_entities[ref->slot];
    if ( ent->client || ent->r.linked || ent->r.bmodel || ent->r.contents ||
        ent->s.eType != ET_GENERAL || ent->s.modelindex ||
        !(ent->r.svFlags & SVF_NOCLIENT) ) return NULL;
    return record;
}

/* The normal engine parser puts every token inside this arena. Check that
 * exact bounded ownership before copying; never strlen a foreign pointer. */
static int Cod1SpawnerTokenLength( const char *token, size_t *length ) {
    uintptr_t base = (uintptr_t)level.spawnVarChars, p = (uintptr_t)token;
    size_t offset, n;
    if ( !token || !length || level.numSpawnVarChars < 0 ||
        level.numSpawnVarChars > MAX_SPAWN_VARS_CHARS || p < base ) return 0;
    offset = (size_t)(p - base);
    if ( offset >= (size_t)level.numSpawnVarChars ) return 0;
    for ( n = 0; offset + n < (size_t)level.numSpawnVarChars; ++n ) {
        if ( !token[n] ) { *length = n + 1; return 1; }
    }
    return 0;
}

/* R175: deliberately narrow initial-BSP authority for cells missing from
 * gentity. The native actor_ dispatcher parses keys, then nativeData clears
 * model BYTE+12d BEFORE the bit0 spawner branch. That branch leaves maxhealth
 * and anglelerprate unchanged (cleared entity defaults 0 and 540f).
 * This does not repeat model registration or the complete retail key parser.
 * Already parsed ordinary cells remain live GAME fields. Unsupported text
 * escapes/float conversions keep extras unavailable without losing R101
 * membership. Never reconstruct these initial defaults after later mutation. */
static int Cod1SpawnerInitialInt32( const char *text, int32_t *out ) {
    uint32_t value = 0, limit;
    int negative = 0;
    const unsigned char *p = (const unsigned char *)text;
    if ( *p == '-' ) { negative = 1; ++p; }
    if ( !*p || (*p == '0' && p[1]) ) return 0;
    limit = negative ? UINT32_C(2147483648) : UINT32_C(2147483647);
    for ( ; *p; ++p ) {
        uint32_t digit;
        if ( *p < '0' || *p > '9' ) return 0;
        digit = (uint32_t)(*p - '0');
        if ( value > (limit - digit) / 10 ) return 0;
        value = value * 10 + digit;
    }
    if ( negative && value == UINT32_C(2147483648) ) *out = INT32_MIN;
    else *out = negative ? -(int32_t)value : (int32_t)value;
    return 1;
}

static int Cod1SpawnerInitialText( cod1NativeStaticText_t *out,
    const char *text ) {
    size_t n;
    for ( n = 0; text[n]; ++n ) {
        unsigned char c = (unsigned char)text[n];
        if ( n >= sizeof( out->text ) - 1 || c < 32 || c > 126 || c == '\\' ) return 0;
    }
    memset( out, 0, sizeof( *out ) );
    out->present = 1;
    memcpy( out->text, text, n + 1 );
    return 1;
}

static void Cod1SpawnerInitialNativeExtras( const cod1SpawnerRecord_t *record ) {
    cod1EntityNativeStaticExtras_t extras;
    char error[128];
    int i;
    if ( !G_Cod1EntityNativeStaticInitialAuthorityAvailable( &record->snapshot.entity ) ) return;
    memset( &extras, 0, sizeof( extras ) );
    extras.modelByteAuthoritative = 1;
    extras.anglelerprateBits = UINT32_C(0x44070000);
    for ( i = 0; i < record->snapshot.rawFieldCount; ++i ) {
        const char *key = record->raw + record->keyOffset[i];
        const char *value = record->raw + record->valueOffset[i];
        cod1NativeStaticText_t *text = NULL;
        if ( !Q_stricmp( key, "teamname" ) ) text = &extras.teamname;
        else if ( !Q_stricmp( key, "groupname" ) ) text = &extras.groupname;
        else if ( !Q_stricmp( key, "export" ) ) text = &extras.exportName;
        else if ( !Q_stricmp( key, "script_noteworthy" ) ) text = &extras.scriptNoteworthy;
        else if ( !Q_stricmp( key, "maxhealth" ) ) {
            if ( !Cod1SpawnerInitialInt32( value, &extras.maxhealth ) ) return;
        } else if ( !Q_stricmp( key, "anglelerprate" ) ) return;
        if ( text && !Cod1SpawnerInitialText( text, value ) ) return;
    }
    (void)G_Cod1EntityPublishNativeStaticExtras( &record->snapshot.entity,
        &extras, error, sizeof( error ) );
}

int G_Cod1SpawnerIntake( struct gentity_s *ent ) {
    cod1SpawnerRecord_t copy;
    GscEntityRef ref;
    size_t keyBytes, valueBytes, used = 0;
    int i, countPresent = 0;
    if ( !Cod1SpawnerMapCurrent() || !level.spawning ||
        level.numSpawnVars < 0 || level.numSpawnVars > MAX_SPAWN_VARS ||
        !ent || !ent->classname || strncmp( ent->classname, "actor_", 6 ) ||
        !(ent->spawnflags & 1) || ent->client || ent->r.linked || ent->r.bmodel ||
        !G_Cod1EntityCapture( ent, &ref ) || ref.slot < MAX_CLIENTS ||
        ref.slot >= ENTITYNUM_MAX_NORMAL ) return 0;
    memset( &copy, 0, sizeof( copy ) );
    copy.snapshot.entity = ref;
    for ( i = 0; i < 3; ++i ) {
        if ( !(ent->s.origin[i] >= -FLT_MAX && ent->s.origin[i] <= FLT_MAX) ||
            !(ent->s.angles[i] >= -FLT_MAX && ent->s.angles[i] <= FLT_MAX) ) return 0;
        copy.snapshot.origin[i] = ent->s.origin[i];
        copy.snapshot.angles[i] = ent->s.angles[i];
    }
    for ( i = 0; i < level.numSpawnVars; ++i ) {
        const char *key = level.spawnVars[i][0], *value = level.spawnVars[i][1];
        if ( !Cod1SpawnerTokenLength( key, &keyBytes ) ||
            !Cod1SpawnerTokenLength( value, &valueBytes ) ||
            keyBytes + valueBytes > sizeof( copy.raw ) - used ) return 0;
        copy.keyOffset[i] = (unsigned short)used;
        memcpy( copy.raw + used, key, keyBytes ); used += keyBytes;
        copy.valueOffset[i] = (unsigned short)used;
        memcpy( copy.raw + used, value, valueBytes ); used += valueBytes;
        if ( !Q_stricmp( key, "count" ) ) countPresent = 1;
    }
    copy.snapshot.rawFieldCount = level.numSpawnVars;
    copy.snapshot.rawBytes = (int)used;
    copy.snapshot.count = ent->count || countPresent ? ent->count : 1;
    copy.snapshot.team = 0;
    copy.snapshot.lastSpawnTime = -1;
    /* No external call between captured identity and commit. The original
     * branch does not link a spawner; do not turn it into a visible model. */
    ent->count = copy.snapshot.count;
    ent->s.eType = ET_GENERAL;
    ent->s.modelindex = 0;
    ent->s.groundEntityNum = ENTITYNUM_NONE;
    ent->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_NOCLIENT;
    ent->r.contents = 0;
    ent->takedamage = qfalse;
    /* Original spawner helper2006b1b1 clears native key+250 after parsing. */
    ent->key = 0;
    G_SetOrigin( ent, ent->s.origin );
    G_SetAngle( ent, ent->s.angles );
    if(cod1SpawnerIntakeSerial!=UINT64_MAX)copy.intakeIdentity=++cod1SpawnerIntakeSerial;
    cod1Spawners[ref.slot] = copy;
    Cod1SpawnerInitialNativeExtras( &cod1Spawners[ref.slot] );
    return 1;
}

int G_Cod1SpawnerCapture( const struct gentity_s *ent, GscEntityRef *out ) {
    GscEntityRef ref;
    if ( !out || !G_Cod1EntityCapture( ent, &ref ) || !Cod1SpawnerFind( &ref ) ) return 0;
    *out = ref;
    return 1;
}

int G_Cod1SpawnerRead( const GscEntityRef *entity,
    cod1SpawnerSnapshot_t *out, char *error, size_t errorSize ) {
    GscEntityRef ref;
    const cod1SpawnerRecord_t *record;
    if ( Cod1SpawnerOverlap( out, sizeof( *out ), error, errorSize ) ||
        Cod1SpawnerOverlap( entity, sizeof( *entity ), error, errorSize ) ) return 0;
    if ( !entity || !out ) return Cod1SpawnerError( error, errorSize, "spawner read requires identity and output" );
    ref = *entity;
    record = Cod1SpawnerFind( &ref );
    if ( !record ) return Cod1SpawnerError( error, errorSize, "spawner read has no current typed allocation" );
    *out = record->snapshot;
    return 1;
}

int G_Cod1SpawnerRawField( const GscEntityRef *entity, size_t index,
    char *key, size_t keyCapacity, char *value, size_t valueCapacity,
    char *error, size_t errorSize ) {
    GscEntityRef ref;
    const cod1SpawnerRecord_t *record;
    const char *savedKey, *savedValue;
    size_t keyBytes, valueBytes;
    if ( Cod1SpawnerOverlap( key, keyCapacity, value, valueCapacity ) ||
        Cod1SpawnerOverlap( key, keyCapacity, error, errorSize ) ||
        Cod1SpawnerOverlap( value, valueCapacity, error, errorSize ) ||
        Cod1SpawnerOverlap( entity, sizeof( *entity ), error, errorSize ) ) return 0;
    if ( !entity || !key || !value ) return Cod1SpawnerError( error, errorSize, "spawner field requires copied outputs" );
    ref = *entity;
    record = Cod1SpawnerFind( &ref );
    if ( !record || index >= (size_t)record->snapshot.rawFieldCount )
        return Cod1SpawnerError( error, errorSize, "spawner field identity or index is unavailable" );
    savedKey = record->raw + record->keyOffset[index];
    savedValue = record->raw + record->valueOffset[index];
    keyBytes = strlen( savedKey ) + 1; valueBytes = strlen( savedValue ) + 1;
    if ( keyBytes > keyCapacity || valueBytes > valueCapacity )
        return Cod1SpawnerError( error, errorSize, "spawner field output is too small; no truncation" );
    memcpy( key, savedKey, keyBytes ); memcpy( value, savedValue, valueBytes );
    return 1;
}

static int Cod1SpawnerCollect( unsigned int mask, int teams,
    GscEntityRef *out, size_t capacity, size_t *count,
    char *error, size_t errorSize ) {
    size_t needed = 0;
    int i;
    if ( capacity > MAX_GENTITIES ||
        Cod1SpawnerOverlap( out, capacity <= MAX_GENTITIES ? capacity * sizeof( *out ) : 0,
            count, sizeof( *count ) ) ||
        Cod1SpawnerOverlap( out, capacity <= MAX_GENTITIES ? capacity * sizeof( *out ) : 0,
            error, errorSize ) ||
        Cod1SpawnerOverlap( count, sizeof( *count ), error, errorSize ) ) return 0;
    if ( !count || (capacity && !out) || !Cod1SpawnerMapCurrent() ||
        (teams && (!mask || (mask & ~14u))) )
        return Cod1SpawnerError( error, errorSize, "spawner collection requires current map and valid nonzero team mask" );
    for ( i = MAX_CLIENTS; i < ENTITYNUM_MAX_NORMAL; ++i ) {
        const cod1SpawnerRecord_t *record = &cod1Spawners[i];
        if ( !Cod1SpawnerFind( &record->snapshot.entity ) ) continue;
        if ( teams && !(mask & (1u << record->snapshot.team)) ) continue;
        ++needed;
    }
    if ( needed > capacity ) return Cod1SpawnerError( error, errorSize, "spawner collection output is too small; no truncation" );
    needed = 0;
    for ( i = MAX_CLIENTS; i < ENTITYNUM_MAX_NORMAL; ++i ) {
        const cod1SpawnerRecord_t *record = &cod1Spawners[i];
        if ( !Cod1SpawnerFind( &record->snapshot.entity ) ) continue;
        if ( teams && !(mask & (1u << record->snapshot.team)) ) continue;
        out[needed++] = record->snapshot.entity;
    }
    *count = needed;
    return 1;
}

int G_Cod1SpawnerCollectAll( GscEntityRef *out, size_t capacity,
    size_t *count, char *error, size_t errorSize ) {
    return Cod1SpawnerCollect( 0, 0, out, capacity, count, error, errorSize );
}

int G_Cod1SpawnerCollectTeams( unsigned int teamMask,
    GscEntityRef *out, size_t capacity, size_t *count,
    char *error, size_t errorSize ) {
    return Cod1SpawnerCollect( teamMask, 1, out, capacity, count, error, errorSize );
}

int G_Cod1SpawnerSetTeam( const GscEntityRef *entity, const char *team,
    char *error, size_t errorSize ) {
    GscEntityRef ref;
    const cod1SpawnerRecord_t *record;
    int value;
    if ( Cod1SpawnerOverlap( entity, sizeof( *entity ), error, errorSize ) ) return 0;
    if ( !entity ) return Cod1SpawnerError( error, errorSize, "spawner team requires a current identity" );
    ref = *entity;
    record = Cod1SpawnerFind( &ref );
    if ( !record ) return Cod1SpawnerError( error, errorSize, "spawner team has no current typed allocation" );
    if ( !team ) return Cod1SpawnerError( error, errorSize, "spawner team requires axis, allies or neutral" );
    if ( !Q_stricmp( team, "axis" ) ) value = COD1_SPAWNER_TEAM_AXIS;
    else if ( !Q_stricmp( team, "allies" ) ) value = COD1_SPAWNER_TEAM_ALLIES;
    else if ( !Q_stricmp( team, "neutral" ) ) value = COD1_SPAWNER_TEAM_NEUTRAL;
    else return Cod1SpawnerError( error, errorSize, "spawner team requires axis, allies or neutral" );
    cod1Spawners[ref.slot].snapshot.team = value;
    return 1;
}
uint64_t G_Cod1SpawnerIntakeIdentity(const GscEntityRef *entity) {
    const cod1SpawnerRecord_t *record=Cod1SpawnerFind(entity);
    return record?record->intakeIdentity:0;
}
#include "g_cod1_spawner_count.inc"
int G_Cod1SpawnerSetScriptTeam(const GscEntityRef *entity,const char *team,char *error,size_t size) {
    GscEntityRef ref;const cod1SpawnerRecord_t *record;int value;
    if(Cod1SpawnerOverlap(entity,entity?sizeof(*entity):0,error,size)||
       (team&&Cod1SpawnerOverlap(team,strlen(team)+1,error,size)))return 0;
    if(!entity)return Cod1SpawnerError(error,size,"spawner team requires a current identity");
    ref=*entity;record=Cod1SpawnerFind(&ref);
    if(!record||!record->intakeIdentity)return Cod1SpawnerError(error,size,"spawner team has no current typed intake");
    if(!team)return Cod1SpawnerError(error,size,"spawner team requires axis, allies or neutral");
    if(!Q_stricmp(team,"axis"))value=1;else if(!Q_stricmp(team,"allies"))value=2;
    else if(!Q_stricmp(team,"neutral"))value=3;else return Cod1SpawnerError(error,size,"spawner team requires axis, allies or neutral");
    cod1Spawners[ref.slot].snapshot.team=value;g_entities[ref.slot].key=value;return 1;
}
#endif
