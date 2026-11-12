/* SPDX-License-Identifier: GPL-3.0-or-later
 * GRENADE_FEEDBACK_R1, included only by cg_weapons.c.
 * Native CoD sound alias families; independent, bounded compatibility FX.
 * This is NOT an interpreter for the original .efx files. No retail assets
 * are embedded here. See docs/reverse/GRENADE_FEEDBACK_R1.md.
 */
#ifndef CG_COD1_GRENADE_FEEDBACK_H
#define CG_COD1_GRENADE_FEEDBACK_H
#include "../game/bg_cod1_inventory.h"
#include "../game/bg_cod1_soundalias.h"

#define CG_COD1_FEEDBACK_VARIANTS 16
#define CG_COD1_FEEDBACK_CSV_LIMIT (1024 * 1024)
#define CG_COD1_FEEDBACK_FX_FILE "scripts/cod1_grenade_feedback.shader"

typedef struct {
    sfxHandle_t sound[CG_COD1_FEEDBACK_VARIANTS];
    int count;
} cgCod1FeedbackSounds;
typedef struct {
    char *data;
    int size;
} cgCod1FeedbackTable;
enum {
    CG_CF_DEFAULT, CG_CF_WOOD, CG_CF_METAL, CG_CF_DIRT, CG_CF_GRASS,
    CG_CF_GRAVEL, CG_CF_SNOW, CG_CF_GLASS, CG_CF_CARPET, CG_CF_WATER,
    CG_CF_COUNT
};
static const char *cg_cod1FeedbackMaterials[CG_CF_COUNT] = {
    "default", "wood", "metal", "dirt", "grass", "gravel", "snow",
    "glass", "carpet", "water"
};
static struct {
    qboolean loaded;
    cgCod1FeedbackSounds bounce[CG_CF_COUNT], explode[CG_CF_COUNT];
    cgCod1FeedbackSounds weaponExplode[CW_MAX_DEFS];
    qhandle_t flash, smoke, fleck, scorch;
    unsigned int seed;  /* Cosmetic RNG must not alter weapon recoil RNG. */
} cg_cod1Feedback;

void CG_Cod1ResetGrenadeFeedback(void) {
    CG_Cod1EfxReset();
    memset(&cg_cod1Feedback, 0, sizeof(cg_cod1Feedback));
    cg_cod1Feedback.seed = 0x734921abU;
}

static unsigned int CG_Cod1FeedbackRandom(void) {
    cg_cod1Feedback.seed = cg_cod1Feedback.seed * 1664525U + 1013904223U;
    return cg_cod1Feedback.seed;
}
static float CG_Cod1FeedbackUnit(void) {
    return (CG_Cod1FeedbackRandom() >> 8) * (1.0f / 16777216.0f);
}
static float CG_Cod1FeedbackSigned(void) {
    return 2.0f * CG_Cod1FeedbackUnit() - 1.0f;
}

static qboolean CG_Cod1FeedbackFile(const char *path) {
    fileHandle_t file = 0;
    int size = trap_FS_FOpenFile(path, &file, FS_READ);
    if (!file) return qfalse;
    trap_FS_FCloseFile(file);
    return size > 0;
}

/* Each table is read at most once at initial registration, not on impacts.
 * File precedence within PK3 search paths is still handled by the engine. */
static void CG_Cod1FeedbackReadTable(cgCod1FeedbackTable *table,
    const char *path) {
    fileHandle_t file = 0;
    int size;
    memset(table, 0, sizeof(*table));
    size = trap_FS_FOpenFile(path, &file, FS_READ);
    if (!file) return;
    if (size <= 0 || size > CG_COD1_FEEDBACK_CSV_LIMIT) {
        trap_FS_FCloseFile(file);
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1: ignored invalid CSV %s (%d bytes)\n", path, size);
        return;
    }
    table->data = (char *)malloc((size_t)size);
    if (table->data) {
        trap_FS_Read(table->data, size, file);
        table->size = size;
    } else {
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1: cannot allocate CSV %s\n", path);
    }
    trap_FS_FCloseFile(file);
}

static void CG_Cod1FeedbackLoadAlias(const char *alias,
    const cgCod1FeedbackTable *tables, int tableCount,
    cgCod1FeedbackSounds *out) {
    char paths[CG_COD1_FEEDBACK_VARIANTS][CW_VIEW_PATH];
    int t, n, i, j;
    sfxHandle_t sound;
    memset(out, 0, sizeof(*out));
    if (!alias || !*alias) return;
    for (t = 0; t < tableCount; ++t) {
        n = CW_SoundAliasPaths(tables[t].data, tables[t].size, alias,
            paths, CG_COD1_FEEDBACK_VARIANTS);
        for (i = 0; i < n; ++i) {
            /* Do not register missing files as placeholder sounds. */
            if (!CG_Cod1FeedbackFile(paths[i])) continue;
            sound = trap_S_RegisterSound(paths[i]);
            if (!sound) continue;
            for (j = 0; j < out->count; ++j)
                if (out->sound[j] == sound) break;
            if (j != out->count) continue;
            out->sound[out->count++] = sound;
            CG_Printf("CoD1 GRENADE_FEEDBACK_R1 sound: %s -> %s\n", alias, paths[i]);
        }
        /* First table with usable variants wins, like existing weapon audio. */
        if (out->count) return;
    }
}

static void CG_Cod1RegisterGrenadeFeedback(void) {
    static const char *tableNames[] = {
        "soundaliases/iw_sound.csv", "soundaliases/iw_sound2.csv"
    };
    static const char *images[] = {
        "gfx/cod1_grenade_feedback/flash.tga",
        "gfx/cod1_grenade_feedback/smoke.tga",
        "gfx/cod1_grenade_feedback/fleck.tga",
        "gfx/cod1_grenade_feedback/scorch.tga"
    };
    cgCod1FeedbackTable tables[2];
    const cwDef_t *def;
    char alias[96];
    const char *effect;
    int i, anyGrenade = 0, fxPresent = 1;
    if (cg_cod1Feedback.loaded) return;
    for (i = 0; i < CW_Count(); ++i) {
        def = CW_Get(i);
        if (def && !Q_stricmp(CW_String(def, "weaponType"), "grenade")) anyGrenade = 1;
    }
    if (!anyGrenade) return;
    cg_cod1Feedback.loaded = qtrue;
    CG_Cod1EfxPrepare();
    for (i = 0; i < 2; ++i) CG_Cod1FeedbackReadTable(&tables[i], tableNames[i]);
    for (i = 0; i < CG_CF_COUNT; ++i) {
        Com_sprintf(alias, sizeof(alias), "grenade_bounce_%s", cg_cod1FeedbackMaterials[i]);
        CG_Cod1FeedbackLoadAlias(alias, tables, 2, &cg_cod1Feedback.bounce[i]);
        Com_sprintf(alias, sizeof(alias), "grenade_explode_%s", cg_cod1FeedbackMaterials[i]);
        CG_Cod1FeedbackLoadAlias(alias, tables, 2, &cg_cod1Feedback.explode[i]);
    }
    /* Some data sets supply the unqualified alias instead. */
    if (!cg_cod1Feedback.bounce[CG_CF_DEFAULT].count)
        CG_Cod1FeedbackLoadAlias("grenade_bounce", tables, 2, &cg_cod1Feedback.bounce[CG_CF_DEFAULT]);
    if (!cg_cod1Feedback.explode[CG_CF_DEFAULT].count)
        CG_Cod1FeedbackLoadAlias("grenade_explode", tables, 2, &cg_cod1Feedback.explode[CG_CF_DEFAULT]);
    for (i = 0; i < CW_Count(); ++i) {
        def = CW_Get(i);
        if (!def || Q_stricmp(CW_String(def, "weaponType"), "grenade")) continue;
        CG_Cod1FeedbackLoadAlias(CW_String(def, "projExplosionSound"), tables, 2,
            &cg_cod1Feedback.weaponExplode[i]);
        effect = CW_String(def, "projExplosionEffect");
        CG_Cod1EfxPrecacheWeapon(effect);
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1 %s: sound='%s', effect='%s' (native EFX attempted; compatibility fallback)\n",
            def->name, CW_String(def, "projExplosionSound"), effect);
        if (*CW_String(def, "projExplosionSound") && !cg_cod1Feedback.weaponExplode[i].count)
            CG_Printf("CoD1 GRENADE_FEEDBACK_R1: unresolved explosion alias '%s'; using grenade surface bank\n",
                CW_String(def, "projExplosionSound"));
    }
    for (i = 0; i < 2; ++i) free(tables[i].data);
    if (!cg_cod1Feedback.bounce[CG_CF_DEFAULT].count)
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1: no default bounce sound; check original iw_sound*.csv and referenced audio files\n");
    if (!cg_cod1Feedback.explode[CG_CF_DEFAULT].count)
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1: no default explosion sound; check original iw_sound*.csv and referenced audio files\n");
    if (!CG_Cod1FeedbackFile(CG_COD1_FEEDBACK_FX_FILE)) fxPresent = 0;
    for (i = 0; i < 4; ++i) if (!CG_Cod1FeedbackFile(images[i])) fxPresent = 0;
    if (fxPresent) {
        cg_cod1Feedback.flash = trap_R_RegisterShader("cod1/grenade_feedback/flash");
        cg_cod1Feedback.smoke = trap_R_RegisterShader("cod1/grenade_feedback/smoke");
        cg_cod1Feedback.fleck = trap_R_RegisterShader("cod1/grenade_feedback/fleck");
        cg_cod1Feedback.scorch = trap_R_RegisterShader("cod1/grenade_feedback/scorch");
    } else {
        CG_Printf("CoD1 GRENADE_FEEDBACK_R1: Optional fallback FX pack missing: cod1_grenade_feedback_r1.pk3. Native EFX may still run; failed native effects retain only fallback audio/light.\n");
    }
    CG_Printf("CoD1 GRENADE_FEEDBACK_R1: initialized; no RTCW grenade media required. IBSP59 material IDs are not yet exposed by the trace adapter: default surface sounds may be used.\n");
}

static int CG_Cod1FeedbackSurface(unsigned int flags) {
    if (flags & SURF_WOOD) return CG_CF_WOOD;
    if (flags & (SURF_METAL | SURF_ROOF)) return CG_CF_METAL;
    if (flags & SURF_GLASS) return CG_CF_GLASS;
    if (flags & SURF_SNOW) return CG_CF_SNOW;
    if (flags & SURF_GRAVEL) return CG_CF_GRAVEL;
    if (flags & SURF_GRASS) return CG_CF_GRASS;
    if (flags & SURF_CARPET) return CG_CF_CARPET;
    return CG_CF_DEFAULT;
}
static const cgCod1FeedbackSounds *CG_Cod1FeedbackBank(
    const cgCod1FeedbackSounds *banks, int material) {
    if (material >= 0 && material < CG_CF_COUNT && banks[material].count)
        return &banks[material];
    return &banks[CG_CF_DEFAULT];
}
static void CG_Cod1FeedbackPlay(const cgCod1FeedbackSounds *bank,
    const vec3_t origin) {
    vec3_t soundOrigin;
    if (bank->count > 0) {
        sfxHandle_t sound = bank->sound[CG_Cod1FeedbackRandom() % (unsigned int)bank->count];
        VectorCopy(origin, soundOrigin);
        if (sound) trap_S_StartSound(soundOrigin, ENTITYNUM_WORLD, CHAN_AUTO, sound);
    }
}

qboolean CG_Cod1GrenadeBounce(int weapon, const vec3_t origin, int packedFlags) {
    const cwDef_t *def = CW_WeaponDef(weapon);
    int material;
    if (!def || Q_stricmp(CW_String(def, "weaponType"), "grenade")) return qfalse;
    CG_Cod1RegisterGrenadeFeedback();
    /* Existing RTCW event format: no new network fields or protocol change. */
    material = CG_Cod1FeedbackSurface(((unsigned int)packedFlags & 255U) << 12);
    CG_Cod1FeedbackPlay(CG_Cod1FeedbackBank(cg_cod1Feedback.bounce, material), origin);
    return qtrue; /* Missing CoD media must not fall through to RTCW audio. */
}

/* Isolated puff allocation: the legacy CG_SmokePuff uses Q_rand's signed
 * overflow RNG. These new particles use only the unsigned cosmetic stream. */
static localEntity_t *CG_Cod1FeedbackPuff(const vec3_t origin, const vec3_t velocity,
    float radius, float r, float g, float b, float a, int duration,
    int fadeMs, int flags, qhandle_t shader) {
    localEntity_t *le = CG_AllocLocalEntity();
    refEntity_t *re = &le->refEntity;
    le->leType = LE_MOVE_SCALE_FADE;
    le->leFlags = flags;
    le->radius = radius;
    le->startTime = cg.time;
    le->endTime = cg.time + duration;
    le->fadeInTime = cg.time + fadeMs;
    le->lifeRate = 1.0f / (duration - fadeMs);
    le->color[0] = r; le->color[1] = g; le->color[2] = b; le->color[3] = a;
    le->pos.trType = TR_LINEAR;
    le->pos.trTime = cg.time;
    VectorCopy(origin, le->pos.trBase);
    VectorCopy(velocity, le->pos.trDelta);
    re->reType = RT_SPRITE;
    re->customShader = shader;
    re->shaderTime = cg.time * 0.001f;
    re->radius = radius;
    re->rotation = 360 * CG_Cod1FeedbackUnit();
    re->shaderRGBA[0] = (byte)(r * 255);
    re->shaderRGBA[1] = (byte)(g * 255);
    re->shaderRGBA[2] = (byte)(b * 255);
    re->shaderRGBA[3] = (byte)(a * 255);
    VectorCopy(origin, re->origin);
    VectorCopy(origin, re->oldorigin);
    return le;
}

static qboolean CG_Cod1GrenadeExplosion(int weapon, const vec3_t origin,
    const vec3_t direction, int surfFlags) {
    const cwDef_t *def = CW_WeaponDef(weapon);
    const cgCod1FeedbackSounds *sound;
    trace_t ground;
    vec3_t up, start, end, fxOrigin, velocity, point;
    localEntity_t *le;
    int i, root, material, wet;
    qboolean onGround;
    if (!def || Q_stricmp(CW_String(def, "weaponType"), "grenade")) return qfalse;
    if (surfFlags & SURF_SKY) return qtrue;
    CG_Cod1RegisterGrenadeFeedback();
    VectorCopy(direction, up);
    if (VectorNormalize(up) < 0.01f) VectorSet(up, 0, 0, 1);
    VectorCopy(origin, start); start[2] += 4;
    VectorCopy(origin, end); end[2] -= 28;
    CG_Trace(&ground, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID);
    onGround = !ground.startsolid && !ground.allsolid && ground.fraction < 1.0f;
    wet = (CG_PointContents(origin, ENTITYNUM_NONE) & MASK_WATER) != 0;
    material = wet ? CG_CF_WATER : CG_Cod1FeedbackSurface(onGround ?
        (unsigned int)ground.surfaceFlags : (unsigned int)surfFlags);
    root = CW_WeaponRoot(weapon);
    sound = CG_Cod1FeedbackBank(cg_cod1Feedback.explode, material);
    if (root >= 0 && root < CW_MAX_DEFS && cg_cod1Feedback.weaponExplode[root].count)
        sound = &cg_cod1Feedback.weaponExplode[root];
    CG_Cod1FeedbackPlay(sound, origin);
    if (CG_Cod1EfxGrenade(origin, onGround ? ground.plane.normal : up,
        cg_cod1FeedbackMaterials[material], CW_String(def, "projExplosionEffect"))) {
        VectorCopy(origin, start);
        CG_StartShakeCamera(0.15f, 1000, start, 600);
        return qtrue; /* No second cloud/light/decal after native playback. */
    }
    VectorMA(origin, 3, up, fxOrigin);

    /* Always allocate a short dynamic light, even when the separate FX pack
     * is absent. Negative shader suppresses geometry, avoiding a checkerboard.
     * Direct allocation avoids CG_MakeExplosion's shared rand() stream. */
    le = CG_AllocLocalEntity();
    le->leType = LE_SPRITE_EXPLOSION;
    le->startTime = cg.time;
    le->endTime = cg.time + (wet ? 140 : 200);
    le->refEntity.customShader = cg_cod1Feedback.flash ? cg_cod1Feedback.flash : -1;
    le->refEntity.shaderTime = cg.time * 0.001f;
    le->refEntity.rotation = 360 * CG_Cod1FeedbackUnit();
    VectorCopy(fxOrigin, le->refEntity.origin);
    VectorCopy(fxOrigin, le->refEntity.oldorigin);
    VectorCopy(fxOrigin, le->pos.trBase);
    le->pos.trType = TR_STATIONARY;
    VectorClear(le->pos.trDelta);
    le->light = wet ? 150 : 280;
    VectorSet(le->lightColor, wet ? 0.55f : 1.0f, wet ? 0.75f : 0.65f, wet ? 1.0f : 0.30f);

    if (cg_cod1Feedback.smoke) {
        for (i = 0; i < (wet ? 5 : 8); ++i) {
            VectorSet(point, fxOrigin[0] + 8 * CG_Cod1FeedbackSigned(),
                fxOrigin[1] + 8 * CG_Cod1FeedbackSigned(),
                fxOrigin[2] + 6 * CG_Cod1FeedbackUnit());
            VectorSet(velocity, 16 * CG_Cod1FeedbackSigned(), 16 * CG_Cod1FeedbackSigned(),
                12 + 16 * CG_Cod1FeedbackUnit());
            le = CG_Cod1FeedbackPuff(point, velocity, 30 + 24 * CG_Cod1FeedbackUnit(),
                wet ? 0.65f : 0.38f, wet ? 0.75f : 0.35f, wet ? 0.85f : 0.31f,
                wet ? 0.25f : 0.65f, 1000 + 700 * CG_Cod1FeedbackUnit(),
                90, 0, cg_cod1Feedback.smoke);
            (void)le;
        }
    }
    if (!wet && cg_cod1Feedback.fleck) {
        for (i = 0; i < 10; ++i) {
            VectorSet(velocity, 100 * CG_Cod1FeedbackSigned(), 100 * CG_Cod1FeedbackSigned(),
                60 + 110 * CG_Cod1FeedbackUnit());
            le = CG_Cod1FeedbackPuff(fxOrigin, velocity, 1.0f + 2.0f * CG_Cod1FeedbackUnit(),
                0.34f, 0.30f, 0.25f, 0.9f, 350 + 250 * CG_Cod1FeedbackUnit(),
                0, LEF_PUFF_DONT_SCALE, cg_cod1Feedback.fleck);
            le->pos.trType = TR_GRAVITY; /* cosmetic only, no damage/collision */
        }
    }
    if (!wet && onGround && cg_cod1Feedback.scorch &&
        !(ground.surfaceFlags & (SURF_SKY | SURF_NOMARKS | SURF_NOIMPACT))) {
        CG_ImpactMark(cg_cod1Feedback.scorch, ground.endpos, ground.plane.normal,
            360 * CG_Cod1FeedbackUnit(), 0.12f, 0.10f, 0.08f, 0.7f,
            qtrue, 28, qfalse, 12000);
    }
    /* Retain the existing grenade wall-event shake; physical state untouched. */
    VectorCopy(origin, start);
    CG_StartShakeCamera(0.15f, 1000, start, 600);
    return qtrue;
}
#endif
