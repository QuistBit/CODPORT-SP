/* CoD1 BSP entity bootstrap.  Included by g_spawn.c.
 *
 * Retail gamex86.dll 0x200397b0/0x20039640 initializes script_brushmodel
 * with its inline brush model, stationary position/angle trajectories,
 * solid contents, and then links it.  The script VM controls later motion,
 * hiding and deletion; those operations are not implemented by this adapter.
 * Do not route this class through RTCW func_static: spawnflag 1 has different
 * semantics and would incorrectly hide the initial CoD brush model.
 */
#ifndef G_COD1_MAPSPAWN_H
#define G_COD1_MAPSPAWN_H

#include <float.h>
#include "bg_cod1_script_entity.h"

static int g_cod1StaticBrushCount;
static qboolean g_cod1MapEntities;
static int g_cod1MapWeaponAliases;
static int g_cod1ScriptOriginCount;
static int g_cod1ScriptModelCount;
static int g_cod1ScriptModelUnassigned;
static int g_cod1ScriptModelRejected;
static int g_cod1ScriptScaleRejected;
/* Model names are resource-owned for this map, rather than allocating a new
 * level string every time a script switches between already known models. */
static char g_cod1ScriptModelPaths[MAX_MODELS][MAX_QPATH];
static unsigned int g_cod1ScriptModelCacheEpoch;
static qboolean g_cod1ScriptModelCacheRetired;
#include "g_cod1_path_entity_registry.inc"

static void G_Cod1MapSpawnBegin( char mapname[MAX_QPATH] ) {
    char filename[MAX_QPATH + 16];
    unsigned char header[8];
    fileHandle_t file;
    int length;
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
    char priorFog[COD1_DIRECT_FOG_WIRE_BYTES];
#endif

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    Cod1PathEntitiesReset();
#endif

    g_cod1StaticBrushCount = 0;
    g_cod1MapWeaponAliases = 0;
    g_cod1ScriptOriginCount = 0;
    g_cod1ScriptModelCount = 0;
    g_cod1ScriptModelUnassigned = 0;
    g_cod1ScriptModelRejected = 0;
    g_cod1ScriptScaleRejected = 0;
    /* Never wrap a resolver invocation onto another map incarnation. The
     * checked entity host has its own identity; this guards shared path
     * storage if a configstring trap changes maps before the helper returns. */
    if ( g_cod1ScriptModelCacheEpoch == UINT_MAX ) {
        g_cod1ScriptModelCacheRetired = qtrue;
    } else {
        ++g_cod1ScriptModelCacheEpoch;
    }
    memset( g_cod1ScriptModelPaths, 0, sizeof( g_cod1ScriptModelPaths ) );
    g_cod1MapEntities = qfalse;
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
    /* A quick map_restart keeps server configstrings across GAME_INIT. The
     * prior script epoch's target must not become the new level's fog, or
     * block the initial RTCW fogswitch. This also clears an orphaned tag if
     * maps/<name>.bsp changed to v58. A subsequent save load restores its
     * own target after the player connects and G_LoadGame reads the save. */
    trap_GetConfigstring( CS_FOGVARS, priorFog, sizeof( priorFog ) );
    if ( Cod1DirectFogTagged( priorFog ) ) {
        trap_SetConfigstring( CS_FOGVARS, "" );
    }
#endif
    trap_Cvar_VariableStringBuffer( "mapname", mapname, MAX_QPATH );
    Com_sprintf( filename, sizeof(filename), "maps/%s.bsp", mapname );
    length = trap_FS_FOpenFile( filename, &file, FS_READ );
    if ( length < 0 ) {
        return;
    }
    if ( length >= sizeof(header) ) {
        trap_FS_Read( header, sizeof(header), file );
        g_cod1MapEntities = !memcmp(header, "IBSP", 4) && header[4] == 59 &&
            !header[5] && !header[6] && !header[7];
    }
    trap_FS_FCloseFile( file );
}

/* BSP classnames differ from several retained RTCW item names. Resolve only
 * the observed CoD aliases to existing inventory IDs, preserving classname
 * itself: original GSC getentarray() queries refer to that original name.
 * MP pickup names select the existing weapon family, not an MP ruleset.
 */
static gitem_t *G_Cod1MapWeaponItem( const char *classname ) {
    static const struct { const char *name; int weapon; } aliases[] = {
        { "weapon_m1garand", WP_GARAND },
        { "weapon_kar98k_scoped", WP_KAR98KSCOPE },
        { "weapon_mosinnagantsniper", WP_MOSINSCOPE },
        { "weapon_fraggrenade", WP_GRENADE_PINEAPPLE },
        { "mpweapon_fg42", WP_FG42 },
        { "mpweapon_panzerfaust", WP_PANZERFAUST }
    };
    int i;
    gitem_t *item;

    if ( !g_cod1MapEntities || !classname ) {
        return NULL;
    }
    for ( i = 0; i < sizeof(aliases)/sizeof(aliases[0]); ++i ) {
        if ( strcmp(classname, aliases[i].name) ) {
            continue;
        }
        for ( item = bg_itemlist + 1; item->classname; ++item ) {
            if ( item->giType == IT_WEAPON && item->giTag == aliases[i].weapon ) {
                ++g_cod1MapWeaponAliases;
                return item;
            }
        }
        break;
    }
    return NULL;
}

/* First script-entity bootstrap: preserve a live server entity even when its
 * model is assigned later by GSC. This uses the existing unanimated xmodel/
 * world renderer, not the first-person DObj or the actor pilot namespace.
 * Invalid paths and a full model table fail without discarding the entity or
 * reaching G_ModelIndex's fatal configstring-overflow path. */
int G_Cod1ScriptModelResolve( const char *name, const char **stablePath ) {
    char path[MAX_QPATH];
    char registered[MAX_STRING_CHARS];
    const char *part;
    size_t bytes;
    unsigned int epoch = g_cod1ScriptModelCacheEpoch;
    int i;

    if ( !stablePath ) {
        return 0;
    }
    *stablePath = NULL;
    if ( !name || !name[0] ) {
        return 0;
    }
    if ( g_cod1ScriptModelCacheRetired ||
         strlen( name ) >= MAX_QPATH || strncmp( name, "xmodel/", 7 ) ||
         !name[7] || name[7] == '/' || strstr( name, ".." ) ||
         strchr( name, '\\' ) || strchr( name, ':' ) ) {
        ++g_cod1ScriptModelRejected;
        return 0;
    }
    for ( part = name; *part; ++part ) {
        if ( (unsigned char)*part < 32 || *part == '"' ) {
            ++g_cod1ScriptModelRejected;
            return 0;
        }
    }
    /* Input may belong to an entity or VM Value whose owner is retired by a
     * trap. Keep an independent complete copy before the first external call. */
    bytes = strlen( name ) + 1;
    memcpy( path, name, bytes );

    for ( i = 1; i < MAX_MODELS; ++i ) {
        trap_GetConfigstring( CS_MODELS + i, registered, sizeof( registered ) );
        if ( g_cod1ScriptModelCacheRetired || epoch != g_cod1ScriptModelCacheEpoch ) {
            return 0;
        }
        if ( !registered[0] ) {
            trap_SetConfigstring( CS_MODELS + i, path );
            if ( g_cod1ScriptModelCacheRetired || epoch != g_cod1ScriptModelCacheEpoch ) {
                return 0;
            }
            memcpy( g_cod1ScriptModelPaths[i], path, bytes );
            *stablePath = g_cod1ScriptModelPaths[i];
            return i;
        }
        if ( !strcmp( registered, path ) ) {
            memcpy( g_cod1ScriptModelPaths[i], path, bytes );
            *stablePath = g_cod1ScriptModelPaths[i];
            return i;
        }
    }
    ++g_cod1ScriptModelRejected;
    return 0;
}

/* Keep the spawn-time empty/unassigned behavior and counters. The runtime
 * host owns entity validation and commits the returned stable path itself. */
static int G_Cod1ScriptModelIndex( const char *name ) {
    const char *stablePath;
    return G_Cod1ScriptModelResolve( name, &stablePath );
}

static void SP_cod1_script_origin( gentity_t *ent ) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef ref;
    if ( !G_Cod1EntityCapture( ent, &ref ) ) return;
#endif
    ent->s.eType = ET_GENERAL;
    ent->s.modelindex = 0;
    ent->s.groundEntityNum = ENTITYNUM_NONE;
    ent->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_NOCLIENT;
    ent->r.contents = 0;
    G_SetOrigin( ent, ent->s.origin );
    G_SetAngle( ent, ent->s.angles );
    trap_LinkEntity( ent );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if ( !G_Cod1EntitySameAllocation( &ref ) ) return;
    Cod1PathEntityOriginBorn( ent, &ref );
    G_Cod1EntityLinkOriginBorn(&ref);
#endif
    ++g_cod1ScriptOriginCount;
}

static void SP_cod1_script_model( gentity_t *ent ) {
    char *value, extra;
    vec3_t scale = { 1, 1, 1 };
    int i, parsed = 3, modelIndex;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef ref;gentity_t before;
    if(!G_Cod1EntityCapture(ent,&ref))return;before=*ent;
#endif

    /* As in the static CoD model producer, modelscale_vec takes precedence
     * regardless of the order of keys in the authored entity string. */
    /* Runtime creation must not read the last BSP entity's spawn variables. */
    if ( level.spawning ) {
        if ( G_SpawnString( "modelscale_vec", "", &value ) ) {
            parsed = sscanf( value, "%f %f %f %c", &scale[0], &scale[1], &scale[2], &extra );
        } else if ( G_SpawnString( "modelscale", "", &value ) ) {
            parsed = sscanf( value, "%f %c", &scale[0], &extra ) == 1 ? 3 : 0;
            scale[1] = scale[2] = scale[0];
        }
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !( scale[i] >= -FLT_MAX && scale[i] <= FLT_MAX ) ) {
            parsed = 0;
        }
    }
    if ( parsed != 3 ) {
        VectorSet( scale, 1, 1, 1 );
        ++g_cod1ScriptScaleRejected;
    }

    modelIndex=G_Cod1ScriptModelIndex(ent->model);
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    /* Configstring operations may retire/re-enter the map or allocation.
     * Resolve to a local first; never publish a result into its replacement. */
    if(!G_Cod1EntitySameAllocation(&ref)||memcmp(ent,&before,sizeof(before)))return;
#endif
    ent->s.eType = ET_GENERAL;
    ent->s.density = COD1_SCRIPT_MODEL_NET_MARKER;
    ent->s.modelindex = modelIndex;
    ent->s.groundEntityNum = ENTITYNUM_NONE;
    ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
    ent->r.contents = 0;
    VectorCopy( scale, ent->s.angles2 );
    G_SetOrigin( ent, ent->s.origin );
    G_SetAngle( ent, ent->s.angles );
    trap_LinkEntity( ent );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(!G_Cod1EntitySameAllocation(&ref))return;
    if(parsed==3)Cod1PathEntityModelBorn(ent,&ref);
#endif
    ++g_cod1ScriptModelCount;
    if ( !ent->s.modelindex ) {
        ++g_cod1ScriptModelUnassigned;
    }
}

static void SP_cod1_script_brushmodel( gentity_t *ent ) {
    const char *digit;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef pathEntity;
    int32_t pathSpawnFlags;
#endif

    if ( !ent->model || ent->model[0] != '*' || !ent->model[1] ) {
        G_Error( "CoD1 script_brushmodel requires an inline model (*N)" );
        return;
    }
    for ( digit = ent->model + 1; *digit; ++digit ) {
        if ( *digit < '0' || *digit > '9' ) {
            G_Error( "CoD1 script_brushmodel: invalid inline model '%s'", ent->model );
            return;
        }
    }

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(!G_Cod1EntityCapture(ent,&pathEntity)){
        G_Error("CoD1 brush spawn requires a current entity allocation");return;
    }
    pathSpawnFlags=ent->spawnflags;
#endif
    ent->s.eType = ET_MOVER;
    ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
    ent->s.pos.trType = TR_STATIONARY;
    ent->s.apos.trType = TR_STATIONARY;
    VectorCopy( ent->s.origin, ent->s.pos.trBase );
    VectorCopy( ent->s.origin, ent->r.currentOrigin );
    VectorCopy( ent->s.angles, ent->s.apos.trBase );
    VectorCopy( ent->s.angles, ent->r.currentAngles );
    ent->moverState = MOVER_POS1;

    /* The engine validates the inline index, supplies bounds, and marks
     * r.bmodel.  It also links once; relink after replacing its -1 contents.
     */
    trap_SetBrushModel( ent, ent->model );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(!G_Cod1EntitySameAllocation(&pathEntity))return;
#endif
    ent->r.contents = CONTENTS_SOLID;
    trap_LinkEntity( ent );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(!G_Cod1EntitySameAllocation(&pathEntity))return;
    if(!Cod1PathEntityBrushBorn(ent,&pathEntity,pathSpawnFlags)){
        G_Error("CoD1 brush path metadata registration refused");return;
    }
#endif
    ++g_cod1StaticBrushCount;
}

#endif
