/* Experimental Native-2 DObj path. The accepted fixed-idle Native-1 loader
 * remains separate. This path retains v14 source weights and twenty XAnim
 * resources, evaluates into one frame-owned submission and never mutates a
 * model surface after it has entered the draw queue. */
#ifndef TR_COD1_NATIVE_ANIMATED_H
#define TR_COD1_NATIVE_ANIMATED_H

#include "cod1_native_anim_eval.h"
#include "cod1_native_source14.h"

#define COD1_NATIVE_ANIM_LIMIT (64u * 1024u * 1024u)
#define COD1_NATIVE_ANIM_ALIGN 16u

struct cod1NativeAnimated_s {
    cnDefinition_t definition;
    cnSourcePart_t part[2];
    int base[2];
};

static size_t R_Cod1AnimatedAlign(size_t n) {
    return (n + COD1_NATIVE_ANIM_ALIGN - 1) & ~(COD1_NATIVE_ANIM_ALIGN - 1);
}

static qboolean R_Cod1AnimatedPart(const char *name, xm14Model *model,
                                    cnSourcePart_t *part, const char **stage) {
    void *data = NULL;
    char path[MAX_QPATH];
    int n;
    qboolean ok = qfalse;
    *stage = "animated xmodel";
    if (!CW_ViewPath(path, name, "xmodel/", 0)) goto done;
    n = ri.FS_ReadFile(path, &data);
    if (n <= 0 || !data || !XM14_Model(data, (size_t)n, model)) goto done;
    ri.FS_FreeFile(data); data = NULL;
    *stage = "animated xmodelparts";
    if (!CW_ViewPath(path, model->lod[0], "xmodelparts/", 0)) goto done;
    n = ri.FS_ReadFile(path, &data);
    if (n <= 0 || !data || !CV14_Parts(data, (size_t)n, &part->bind)) goto done;
    ri.FS_FreeFile(data); data = NULL;
    *stage = "animated xmodelsurfs";
    if (!CW_ViewPath(path, model->lod[0], "xmodelsurfs/", 0)) goto done;
    n = ri.FS_ReadFile(path, &data);
    if (n <= 0 || !data || !CN_SourceParseMesh(data, (size_t)n, &part->bind, part) ||
        part->numSurfaces != model->ns[0]) goto done;
    ok = qtrue;
done:
    if (data) ri.FS_FreeFile(data);
    return ok;
}

/* Loading a second namespace keeps already accepted Native-1 model handles
 * and rendering untouched. Temporary parsed vertices are copied to hunk
 * storage once; only read-only sources and clip bytes remain on the model. */
static qboolean R_LoadCod1NativeAnimated(model_t *mod, const char *resourceName) {
    static const char *fields[COD1_NATIVE_LEAVES] = {
        NULL, "idleAnim", "emptyIdleAnim", "fireAnim", "holdFireAnim",
        "lastShotAnim", "rechamberAnim", "meleeAnim", "reloadAnim",
        "reloadEmptyAnim", "reloadStartAnim", "reloadEndAnim", "raiseAnim",
        "dropAnim", "altRaiseAnim", "altDropAnim", "adsFireAnim",
        "adsLastShotAnim", "adsRechamberAnim", "adsUpAnim", "adsDownAnim"
    };
    cod1NativeAnimated_t *temp = NULL, *runtime = NULL;
    cwDef_t *def = NULL;
    char definitionName[CW_VIEW_PATH],selectedHands[CW_VIEW_PATH];
    cv14Pose *first = NULL;
    xm14Model *models = NULL;
    cod1NativeModel_t *fixed = mod ? mod->cod1Native : NULL;
    void *defData = NULL, *clip[COD1_NATIVE_LEAVES];
    int clipSize[COD1_NATIVE_LEAVES], defSize, i, j, part, bone, partIndex;
    int sourceSurface = 0, attachment = -1;
    size_t bytes, n;
    byte *cursor;
    char path[MAX_QPATH];
    const char *stage = "allocation";
    qboolean ok = qfalse;
    memset(clip, 0, sizeof(clip)); memset(clipSize, 0, sizeof(clipSize));
    if (!mod || !fixed || !CW_ViewResourceParse(resourceName,definitionName,selectedHands)) return qfalse;
    temp = (cod1NativeAnimated_t *)calloc(1, sizeof(*temp));
    def = (cwDef_t *)calloc(1, sizeof(*def));
    first = (cv14Pose *)calloc(1, sizeof(*first));
    models = (xm14Model *)calloc(2, sizeof(*models));
    if (!temp || !def || !first || !models) goto done;

    stage = "animated weapon definition";
    if (strlen(definitionName) + 11 >= sizeof(path)) goto done;
    Com_sprintf(path, sizeof(path), "weapons/sp/%s", definitionName);
    defSize = ri.FS_ReadFile(path, &defData);
    if (defSize <= 0 || !defData || CW_Parse(def, definitionName, defData, defSize) <= 0) goto done;
    ri.FS_FreeFile(defData); defData = NULL;
    if (!R_Cod1AnimatedPart(*selectedHands ? selectedHands : CW_String(def, "handModel"), &models[0], &temp->part[0], &stage) ||
        !R_Cod1AnimatedPart(CW_String(def, "gunModel"), &models[1], &temp->part[1], &stage)) goto done;
    temp->base[1] = temp->part[0].bind.n;
    temp->definition.bones = temp->base[1] + temp->part[1].bind.n;
    if (temp->definition.bones <= 0 || temp->definition.bones > 0x7f ||
        temp->definition.bones != fixed->numBones) { stage = "composite skeleton"; goto done; }
    for (i = 0; i < temp->part[0].bind.n; i++)
        if (!strcmp(temp->part[0].bind.name[i], "tag_weapon")) { attachment = i; break; }
    if (attachment < 0) { stage = "tag_weapon attachment"; goto done; }
    for (part = 0; part < 2; part++) {
        const cv14Skeleton *sk = &temp->part[part].bind;
        for (i = 0; i < sk->n; i++) {
            bone = temp->base[part] + i;
            Q_strncpyz(temp->definition.boneName[bone], sk->name[i], XM14_NAME);
            temp->definition.bindLocal[bone] = sk->local[i];
            temp->definition.parent[bone] = sk->parent[i] >= 0 ?
                temp->base[part] + sk->parent[i] : (part == 0 ? -1 : attachment);
            if (temp->definition.parent[bone] >= bone) { stage = "native hierarchy"; goto done; }
        }
    }
    stage = "animated topology";
    for (part = 0; part < 2; part++) for (j = 0; j < temp->part[part].numSurfaces; j++) {
        const cnSourceSurface_t *src = &temp->part[part].surfaces[j];
        const srfTriangles_t *dst;
        if (sourceSurface >= fixed->numSurfaces) goto done;
        dst = fixed->surfaces[sourceSurface++].geometry;
        if (!dst || dst->numVerts != src->numVerts || dst->numIndexes != src->numIndexes) goto done;
        for (i = 0; i < src->numIndexes; i++) if (dst->indexes[i] != src->indexes[i]) goto done;
    }
    if (sourceSurface != fixed->numSurfaces) goto done;

    for (i = 1; i < COD1_NATIVE_LEAVES; i++) {
        const char *name = CW_String(def, fields[i]);
        cnLeaf_t *leaf = &temp->definition.leaf[i];
        if (!*name) name = CW_String(def, "idleAnim");
        stage = "animated XAnim";
        if (!CW_ViewPath(path, name, "xanim/", 0)) goto done;
        clipSize[i] = ri.FS_ReadFile(path, &clip[i]);
        if (clipSize[i] <= 0 || clipSize[i] > 1024*1024 || !clip[i] ||
            !CV14_SamplePose(clip[i], (size_t)clipSize[i], 0, first)) goto done;
        leaf->size = (size_t)clipSize[i];
        leaf->parts = first->n;
        leaf->lastFrame = (int)(((const byte *)clip[i])[2] | (((const byte *)clip[i])[3] << 8)) - 1 +
                          (((const byte *)clip[i])[6] & 1);
        if (leaf->lastFrame < 0 || leaf->lastFrame >= 4096) goto done;
        memset(leaf->remap, 0x7f, sizeof(leaf->remap));
        for (bone = 0; bone < temp->definition.bones; bone++) {
            for (partIndex = first->n - 1; partIndex >= 0; partIndex--) {
                if (leaf->remap[partIndex] == 0x7f &&
                    !strcmp(temp->definition.boneName[bone], first->name[partIndex])) {
                    leaf->remap[partIndex] = (unsigned char)bone;
                    break;
                }
            }
        }
    }

    bytes = R_Cod1AnimatedAlign(sizeof(*runtime));
    for (part = 0; part < 2; part++) for (j = 0; j < temp->part[part].numSurfaces; j++) {
        n = (size_t)temp->part[part].surfaces[j].numVerts * sizeof(cnSourceVertex_t);
        bytes += R_Cod1AnimatedAlign(n);
        if (bytes > COD1_NATIVE_ANIM_LIMIT) { stage = "animated source limit"; goto done; }
    }
    for (i = 1; i < COD1_NATIVE_LEAVES; i++) {
        bytes += R_Cod1AnimatedAlign((size_t)clipSize[i]);
        if (bytes > COD1_NATIVE_ANIM_LIMIT) { stage = "animated clip limit"; goto done; }
    }
    if (bytes > 0x7fffffffU || mod->dataSize > 0x7fffffff - (int)bytes) goto done;
    runtime = (cod1NativeAnimated_t *)ri.Hunk_Alloc((int)bytes, h_low);
    if (!runtime) goto done;
    memset(runtime, 0, bytes); *runtime = *temp;
    cursor = (byte *)runtime + R_Cod1AnimatedAlign(sizeof(*runtime));
    for (part = 0; part < 2; part++) for (j = 0; j < temp->part[part].numSurfaces; j++) {
        cnSourceSurface_t *dst = &runtime->part[part].surfaces[j];
        const cnSourceSurface_t *src = &temp->part[part].surfaces[j];
        n = (size_t)src->numVerts * sizeof(*src->verts);
        dst->verts = (cnSourceVertex_t *)cursor;
        memcpy(dst->verts, src->verts, n);
        cursor += R_Cod1AnimatedAlign(n);
        dst->indexes = NULL; /* model surface owns immutable draw indexes */
    }
    for (i = 1; i < COD1_NATIVE_LEAVES; i++) {
        n = (size_t)clipSize[i];
        runtime->definition.leaf[i].data = cursor;
        memcpy(cursor, clip[i], n);
        cursor += R_Cod1AnimatedAlign(n);
    }
    if ((size_t)(cursor - (byte *)runtime) != bytes) { stage = "animated allocation accounting"; goto done; }
    fixed->animated = runtime;
    mod->dataSize += (int)bytes;
    ri.Printf(PRINT_ALL, "CoD1 Native-2 %s: %d composite bones, %d surfaces, 20 XAnim leaves\n",
              definitionName, runtime->definition.bones, fixed->numSurfaces);
    ok = qtrue;
done:
    if (defData) ri.FS_FreeFile(defData);
    for (i = 1; i < COD1_NATIVE_LEAVES; i++) if (clip[i]) ri.FS_FreeFile(clip[i]);
    if (temp) { CN_SourceFree(&temp->part[0]); CN_SourceFree(&temp->part[1]); }
    free(temp); free(def); free(first); free(models);
    if (!ok) {
        R_Cod1ViewFailureSet(mod ? mod->name : NULL, "<native-2>", 0, stage);
        ri.Printf(PRINT_WARNING, "CoD1 Native-2: failed %s at %s; accepted adapter remains available\n",
                  definitionName ? definitionName : "<null>", stage);
    }
    return ok;
}

cod1NativeSubmission_t *R_Cod1NativeCaptureAnimated(model_t *model,
                                               cnsArena_t *arena,
                                               const cod1NativeAnimState_t *state) {
    cod1NativeModel_t *fixed = model ? model->cod1Native : NULL;
    const cod1NativeAnimated_t *animated = fixed ? fixed->animated : NULL;
    cod1NativeSubmission_t *out;
    cv14Transform final[CV14_BONES];
    cv14Skeleton posed;
    int part, i, j, k, surface = 0, tags;
    size_t arenaStart;
    if (!animated || !arena || animated->definition.bones != fixed->numBones ||
        !CN_Evaluate(&animated->definition, state, final)) return NULL;
    arenaStart = arena->used;
    out = R_Cod1NativeCapture(model, arena);
    if (!out) return NULL;
    memcpy(out->boneWorld, final, (size_t)out->numBones * sizeof(*final));
    ClearBounds(out->bounds[0], out->bounds[1]);
    for (part = 0; part < 2; part++) {
        posed = animated->part[part].bind;
        for (i = 0; i < posed.n; i++) posed.world[i] = final[animated->base[part] + i];
        for (i = 0; i < animated->part[part].numSurfaces; i++) {
            const cnSourceSurface_t *src = &animated->part[part].surfaces[i];
            srfTriangles_t *tri = &out->surfaces[surface++];
            if (src->numVerts != tri->numVerts) goto failed;
            ClearBounds(tri->bounds[0], tri->bounds[1]);
            for (j = 0; j < src->numVerts; j++) {
                drawVert_t *v = &tri->verts[j];
                if (!CN_SourceSkinVertex(&src->verts[j], &posed, v->xyz, v->normal)) goto failed;
                VectorNormalize(v->normal);
                for (k = 0; k < 2; k++) v->st[k] = src->verts[j].uv[k];
                AddPointToBounds(v->xyz, tri->bounds[0], tri->bounds[1]);
                AddPointToBounds(v->xyz, out->bounds[0], out->bounds[1]);
            }
        }
        if (part == 1) {
            tags = R_Cod1NativeExportTags(&posed, out->tags);
            if (tags < 0) goto failed;
            out->numTags = tags;
        }
    }
    if (surface != out->numSurfaces) goto failed;
    return out;
failed:
    /* Nothing reached the draw queue; this incomplete capture is private. */
    arena->used = arenaStart;
    return NULL;
}

void R_Cod1NativeSubmissionTags(const cod1NativeSubmission_t *snapshot,
                                        cod1NativeViewTags_t *out) {
    int i;
    memset(out, 0, sizeof(*out));
    out->flashIndex = out->brassIndex = -1;
    if (!snapshot) return;
    for (i = 0; i < snapshot->numTags; i++) {
        if (!strcmp(snapshot->tags[i].name, "tag_flash")) {
            out->flashIndex = i; out->flash = snapshot->tags[i].transform;
        }
        if (!strcmp(snapshot->tags[i].name, "tag_brass")) {
            out->brassIndex = i; out->brass = snapshot->tags[i].transform;
        }
    }
}

#endif
