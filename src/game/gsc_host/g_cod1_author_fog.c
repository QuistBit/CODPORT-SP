/* R78: project a fresh map's initial authored fog profile into the existing
 * native GSC session/typed fog bridge. The complete original source is read and
 * inspected before effects, but the original mission main is NOT executed.
 * Unrelated mission statements, later functions and event threads remain out
 * of this adapter's scope. No map-specific parameters or shader overrides.
 * Keep below game/ to remain outside the legacy save-function source scan. */
#include "../g_cod1_author_fog.h"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#if defined(Q3_VM)
#error R78 authored fog requires a native game DLL, not a QVM
#endif
#if !defined(COD1_DIRECT_FOG_WIP) || !COD1_DIRECT_FOG_WIP
#error R78 authored fog requires the existing direct-fog build profile
#endif
#include "../g_local.h"
#include "../g_cod1_gsc_test.h"
#include "../bg_cod1_direct_fog.h"
#include "../cod1_gsc/gsc_session.h"
#include "cod1_author_fog.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef COD1_AUTHOR_FOG_CALLOC
#define COD1_AUTHOR_FOG_CALLOC calloc
#endif
#ifndef COD1_AUTHOR_FOG_FREE
#define COD1_AUTHOR_FOG_FREE free
#endif
/* At most 16 calls with six round-trip float literals each. The separate
 * original-source inspection cap does not widen the VM's source limit. */
#define AUTHOR_FOG_PROGRAM_CAP 4096u

typedef struct {
    char path[GSC_SESSION_PATH_CAP];
    char *program;
    size_t programSize;
    const Cod1AuthorFogPlan *plan;
    uint64_t sessionId;
    unsigned emitted;
} AuthorFogHost;

static vmCvar_t authorFogEnabled;
static int authorFogBusy, authorFogCancelled;
/* A refused destroy cannot be retried after its stack-backed host unwinds.
 * Keep save admission closed for this DLL lifetime; never clear on map reset. */
static int authorFogRawSaveUnresolved;
int G_Cod1AuthorFogBlocksRawSave(void){return authorFogBusy||authorFogRawSaveUnresolved;}
static int AuthorFogDestroySession(GscSession*session,char*error,size_t capacity){
    if(!session||gsc_session_destroy(session,error,capacity))return 1;
    authorFogRawSaveUnresolved=1;return 0;
}

static void AuthorFogError( char *error, size_t capacity, const char *message ) {
    size_t length;
    if (!error || !capacity) return;
    length = strlen(message);
    if (length >= capacity) length = capacity - 1;
    memcpy(error, message, length);
    error[length] = '\0';
}

static int AuthorFogAsciiFirst( unsigned char c ) {
    return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int AuthorFogMapPath( char *path, size_t capacity,
                            char *error, size_t errorCapacity ) {
    char map[GSC_SESSION_PATH_CAP];
    size_t i, length;
    /* Reading to a wider buffer than the admitted stem detects any truncation:
     * even an overlong truncated value cannot pass the smaller path limit. */
    memset(map, 0, sizeof(map));
    trap_Cvar_VariableStringBuffer("mapname", map, sizeof(map));
    map[sizeof(map)-1] = '\0';
    length = strlen(map);
    if (!length || capacity < 10 || length > capacity - 10 ||
        !AuthorFogAsciiFirst((unsigned char)map[0])) {
        AuthorFogError(error, errorCapacity, "mapname missing, truncated or noncanonical");
        return 0;
    }
    for (i = 1; i < length; ++i) {
        unsigned char c = (unsigned char)map[i];
        if (!AuthorFogAsciiFirst(c) && !(c >= '0' && c <= '9')) {
            AuthorFogError(error, errorCapacity, "mapname must be a single ASCII identifier");
            return 0;
        }
    }
    memcpy(path, "maps/", 5);
    memcpy(path + 5, map, length);
    memcpy(path + 5 + length, ".gsc", 5);
    return 1;
}

static int AuthorFogReadOriginal( const char *path, char **out, size_t *outSize,
                                  char *error, size_t errorCapacity ) {
    fileHandle_t file = 0;
    int length;
    char *bytes;
    *out = NULL; *outSize = 0;
    length = trap_FS_FOpenFile(path, &file, FS_READ);
    if (length <= 0 || !file || (unsigned)length > COD1_AUTHOR_FOG_MAX_SOURCE) {
        if (file) trap_FS_FCloseFile(file);
        AuthorFogError(error, errorCapacity, "authored GSC missing, empty or above 1048576 bytes");
        return 0;
    }
    bytes = (char *)COD1_AUTHOR_FOG_CALLOC((size_t)length + 1, 1);
    if (!bytes) {
        trap_FS_FCloseFile(file);
        AuthorFogError(error, errorCapacity, "authored GSC source allocation failed");
        return 0;
    }
    /* trap_FS_Read is VOID in this engine; zero-fill and reject every embedded
     * NUL, including an untouched tail after an incomplete copy. Do not invent
     * a read byte count or a guarantee against corrupt FS implementations. */
    trap_FS_Read(bytes, length, file);
    trap_FS_FCloseFile(file);
    if (memchr(bytes, '\0', (size_t)length)) {
        COD1_AUTHOR_FOG_FREE(bytes);
        AuthorFogError(error, errorCapacity, "authored GSC contains NUL or incomplete VFS read");
        return 0;
    }
    *out = bytes; *outSize = (size_t)length;
    return 1;
}

static int AuthorFogReadCvar( void *user, const char *name, char *value,
                             size_t capacity ) {
    char buffer[COD1_AUTHOR_FOG_CVAR_CAP + 1];
    size_t length;
    (void)user;
    if (!value || !capacity || !name) return 0;
    value[0] = '\0';
    memset(buffer, 0, sizeof(buffer));
    trap_Cvar_VariableStringBuffer(name, buffer, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    length = strlen(buffer);
    if (length >= COD1_AUTHOR_FOG_CVAR_CAP || length >= capacity) return 0;
    memcpy(value, buffer, length + 1);
    return !authorFogCancelled;
}

static int AuthorFogProgram( AuthorFogHost *context, char *error,
                              size_t errorCapacity ) {
    unsigned i, j;
    size_t used;
    int written;
    context->program = (char *)COD1_AUTHOR_FOG_CALLOC(AUTHOR_FOG_PROGRAM_CAP, 1);
    if (!context->program) {
        AuthorFogError(error, errorCapacity, "authored fog projection allocation failed");
        return 0;
    }
    memcpy(context->program, "main(){\n", 8); used = 8;
    for (i = 0; i < context->plan->count; ++i) {
        const Cod1AuthorFogCall *call = &context->plan->calls[i];
        unsigned argc = call->kind == COD1_AUTHOR_FOG_EXP ? 5 : 6;
        const char *name = call->kind == COD1_AUTHOR_FOG_EXP ? "setExpFog(" : "setCullFog(";
        size_t nameSize = strlen(name);
        if ((call->kind != COD1_AUTHOR_FOG_EXP && call->kind != COD1_AUTHOR_FOG_CULL) ||
            used + nameSize >= AUTHOR_FOG_PROGRAM_CAP) {
            AuthorFogError(error, errorCapacity, "invalid authored fog projection plan");
            return 0;
        }
        memcpy(context->program + used, name, nameSize); used += nameSize;
        for (j = 0; j < argc; ++j) {
            /* Nine significant decimal digits round-trip binary32. The VM
             * performs its existing seconds-to-ms conversion exactly once. */
            written = snprintf(context->program + used, AUTHOR_FOG_PROGRAM_CAP - used,
                               "%s%.9g", j ? "," : "", (double)call->args[j]);
            if (written < 0 || (size_t)written >= AUTHOR_FOG_PROGRAM_CAP - used) {
                AuthorFogError(error, errorCapacity, "authored fog projection exceeded buffer");
                return 0;
            }
            used += (size_t)written;
        }
        if (used + 3 >= AUTHOR_FOG_PROGRAM_CAP) {
            AuthorFogError(error, errorCapacity, "authored fog projection exceeded buffer");
            return 0;
        }
        memcpy(context->program + used, ");\n", 3); used += 3;
    }
    if (used + 3 > AUTHOR_FOG_PROGRAM_CAP) {
        AuthorFogError(error, errorCapacity, "authored fog projection exceeded buffer");
        return 0;
    }
    memcpy(context->program + used, "}\n", 3); context->programSize = used + 2;
    return 1;
}

static int AuthorFogReadProjection( void *user, const char *path,
                                    GscSessionSource *out, char *error,
                                    size_t errorCapacity ) {
    AuthorFogHost *context = (AuthorFogHost *)user;
    memset(out, 0, sizeof(*out));
    if (authorFogCancelled || !path || strcmp(path, context->path) ||
        !context->program || !context->programSize) {
        AuthorFogError(error, errorCapacity, "invalid authored fog projection source lease");
        return 0;
    }
    out->data = context->program;
    out->size = context->programSize;
    out->token = context->program;
    context->program = NULL; /* released exactly once by the unchanged session */
    return 1;
}

static void AuthorFogReleaseProjection( void *user, const GscSessionSource *source ) {
    (void)user;
    COD1_AUTHOR_FOG_FREE(source->token);
}

static int AuthorFogEmit( void *user, uint64_t sessionId, int logicalTime,
                         const GscFogEvent *fog ) {
    AuthorFogHost *context = (AuthorFogHost *)user;
    const Cod1AuthorFogCall *call;
    cod1DirectFogEvent_t event;
    if (authorFogCancelled || !fog || sessionId != context->sessionId ||
        logicalTime != 0 || context->emitted >= context->plan->count) return 0;
    call = &context->plan->calls[context->emitted];
    event.nearDist = fog->near_dist;
    event.farDist = fog->far_dist;
    event.density = fog->density;
    event.color[0] = fog->red; event.color[1] = fog->green; event.color[2] = fog->blue;
    event.transitionMs = fog->transition_ms; /* no second conversion */
    /* Preserve the authoritative qualified-BSP59 map gate in g_spawn.c. */
    if (!G_Cod1DirectFogEvent(&event)) return 0;
    ++context->emitted;
    G_Printf("[GSC R78] authored fog %s:%u:%u logical=%dms transition=%dms near=%.9g far=%.9g density=%.9g rgb=%.9g,%.9g,%.9g\n",
             context->path, call->line, call->column, logicalTime,
             event.transitionMs, (double)event.nearDist, (double)event.farDist,
             (double)event.density, (double)event.color[0],
             (double)event.color[1], (double)event.color[2]);
    return !authorFogCancelled;
}

void G_Cod1AuthorFogInitAfterMapBootstrap( int levelTime ) {
    char error[GSC_SESSION_ERROR_CAP];
    char *original = NULL;
    size_t originalSize = 0;
    Cod1AuthorFogPlan plan;
    AuthorFogHost context;
    GscSessionHost host;
    GscSessionSnapshot snapshot;
    GscSession *session = NULL;
    if (authorFogBusy) { authorFogCancelled = 1; return; }
    if (authorFogRawSaveUnresolved) return;
    authorFogBusy = 1; authorFogCancelled = 0;
    memset(&context, 0, sizeof(context)); memset(&plan, 0, sizeof(plan));
    error[0] = '\0';
    trap_Cvar_Register(&authorFogEnabled, "g_cod1MapFog", "1", 0);
    if (trap_Cvar_VariableIntegerValue("g_cod1MapFog") != 1) goto cleanup;
    if (g_gametype.integer != GT_SINGLE_PLAYER ||
        trap_Cvar_VariableIntegerValue("savegame_loading") != 0 ||
        G_Cod1GscFeature47(C1CP47_F_ENABLE) == 1 ||
        !G_Cod1MapEntities()) {
        G_Printf("[GSC R78] initial authored fog skipped: requires fresh qualified SP map without isolated GSC test\n");
        goto cleanup;
    }
    if (!AuthorFogMapPath(context.path, sizeof(context.path), error, sizeof(error)) ||
        !AuthorFogReadOriginal(context.path, &original, &originalSize, error, sizeof(error))) {
        G_Printf("[GSC R78] initial authored fog unavailable: %s\n", error);
        goto cleanup;
    }
    if (authorFogCancelled ||
        !Cod1AuthorFogParse(original, originalSize, AuthorFogReadCvar, NULL,
                           &plan, error, sizeof(error))) {
        G_Printf("[GSC R78] initial authored fog refused %s: %s\n", context.path,
                 authorFogCancelled ? "adapter cancelled by reentry" : error);
        goto cleanup;
    }
    COD1_AUTHOR_FOG_FREE(original); original = NULL;
    if (authorFogCancelled) goto cleanup;
    if (!plan.count) {
        G_Printf("[GSC R78] %s has no supported initial authored fog profile\n", context.path);
        goto cleanup;
    }
    context.plan = &plan;
    if (!AuthorFogProgram(&context, error, sizeof(error)) || authorFogCancelled) {
        G_Printf("[GSC R78] initial authored fog projection refused %s: %s\n",
                 context.path, authorFogCancelled ? "adapter cancelled by reentry" : error);
        goto cleanup;
    }
    memset(&host, 0, sizeof(host));
    host.enabled = 1; host.user = &context;
    host.read = AuthorFogReadProjection;
    host.release = AuthorFogReleaseProjection;
    host.fog = AuthorFogEmit;
    session = gsc_session_create(&host, error, sizeof(error));
    if (!session || authorFogCancelled ||
        !gsc_session_load(session, context.path, 1, &context.sessionId,
                          error, sizeof(error))) {
        G_Printf("[GSC R78] initial authored fog projection preflight failed %s: %s\n",
                 context.path, authorFogCancelled ? "adapter cancelled by reentry" : error);
        goto cleanup;
    }
    G_Printf("[GSC R78] prepared initial fog projection %s calls=%u sourceBytes=%u (mission GSC is not executed)\n",
             context.path, plan.count, (unsigned)originalSize);
    if (authorFogCancelled) goto cleanup;
    if (!gsc_session_start(session, context.sessionId, (int64_t)levelTime,
                           error, sizeof(error))) {
        G_Printf("[GSC R78] initial authored fog projection start failed %s: %s\n",
                 context.path, error);
        goto cleanup;
    }
    if (!gsc_session_snapshot(session, &snapshot) ||
        snapshot.state != GSC_SESSION_COMPLETED || snapshot.active_threads != 0 ||
        context.emitted != plan.count) {
        G_Printf("[GSC R78] initial authored fog projection incomplete %s emitted=%u expected=%u\n",
                 context.path, context.emitted, plan.count);
        goto cleanup;
    }
    G_Printf("[GSC R78] applied initial authored fog %s calls=%u gameTime=%dms\n",
             context.path, context.emitted, levelTime);
cleanup:
    /* All session APIs have unwound. Normal destruction leaves only retained
     * fog state. A refused destroy keeps raw-save admission closed through
     * the unresolved-owner latch; it is not successful VM retirement. */
    if (!AuthorFogDestroySession(session, error, sizeof(error))) {
        G_Printf("[GSC R78] initial authored fog cleanup refused: %s\n", error);
    }
    if (context.program) COD1_AUTHOR_FOG_FREE(context.program);
    if (original) COD1_AUTHOR_FOG_FREE(original);
    authorFogBusy = 0;
}
#else
typedef int cod1_author_fog_disabled_translation_unit;
#endif
