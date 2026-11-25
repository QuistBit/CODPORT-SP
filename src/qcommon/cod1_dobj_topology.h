/* Ordered, headless CoD1 DObj topology. Resources are borrowed immutable
 * records: the caller retains their owning bank leases for this topology's
 * entire lifetime. No renderer state, animation tree or current pose exists
 * here. Bone indices remain the original concatenated model indices, including
 * duplicates; named melding is recorded separately rather than deleting bones.
 */
#ifndef COD1_DOBJ_TOPOLOGY_H
#define COD1_DOBJ_TOPOLOGY_H
#include "cod1_model_resource.h"

#define C1DT_MAX_MODELS 8
#define C1DT_MAX_BONES 128
#define C1DT_DUPLICATE_BYTES (16 + 2 * (C1DT_MAX_BONES - 1) + 1)
#define C1DT_OK 1
#define C1DT_INVALID -1
#define C1DT_CAPACITY -2

typedef struct {
    const c1mrResource_t *resource;
    const char *tagName; /* NULL and empty are the same; copied on acceptance. */
} c1dtModelInput_t;
typedef struct {
    const c1mrResource_t *resource;
    unsigned int inputIndex, firstBone, boneCount;
    int anchorBone; /* -1 is the common object root, never a prepared pose. */
    char tagName[C1MR_NAME_MAX];
} c1dtModel_t;
typedef struct {
    unsigned int modelIndex, localBone;
    int parentBone; /* source hierarchy, roots use the model's anchor. */
    int duplicateBone; /* -1 or the exact raw named-lookup target index. */
    unsigned int canonicalBone;
} c1dtBone_t;
typedef struct {
    unsigned int modelCount, boneCount, duplicateCount;
    unsigned int warningCount, missingTagInputMask, unmeldedRootInputMask;
    unsigned int classificationModelMask;
    c1dtModel_t models[C1DT_MAX_MODELS];
    c1dtBone_t bones[C1DT_MAX_BONES];
    /* Original form: 16-byte bitset, ordered 1-based (duplicate,target)
     * byte pairs and a zero terminator. Empty maps have 17 zero bytes.
     */
    unsigned int duplicateBlobSize;
    unsigned char duplicateBlob[C1DT_DUPLICATE_BYTES];
} c1dtTopology_t;

/* Strings are bounded by the decoded resource schema. Empty bone names are
 * ordinary portable names, not anonymous wildcards. The original loader's
 * empty-string interned-ID assignment is not established by this topology.
 */
static int C1DT_String(const char *s) {
    unsigned int i;
    if (!s) return 0;
    for (i = 0; i < C1MR_NAME_MAX; ++i) if (!s[i]) return 1;
    return 0;
}
static int C1DT_ResourceValid(const c1mrResource_t *resource) {
    const c1mrSkeleton_t *s;
    unsigned int i;
    if (!resource || !C1DT_String(resource->modelName) ||
        !resource->modelName[0] || !C1DT_String(resource->partsName) ||
        !resource->partsName[0]) return 0;
    s = &resource->skeleton;
    if (!s->boneCount || s->boneCount > C1MR_BONES_MAX || !s->rootCount ||
        s->rootCount > s->boneCount ||
        (s->hasClassification != 0 && s->hasClassification != 1)) return 0;
    for (i = 0; i < s->boneCount; ++i) {
        if (!C1DT_String(s->boneName[i])) return 0;
        if (i < s->rootCount) { if (s->parent[i] != -1) return 0; }
        else if (s->parent[i] < 0 || s->parent[i] >= (int)i) return 0;
    }
    return 1;
}

/* Native 0x476190: earliest accepted model wins, and the last matching
 * bone inside that model wins. This deliberately permits repeated names.
 */
static int C1DT_FindBone(const c1dtTopology_t *topology, const char *name) {
    unsigned int i, j;
    const c1dtModel_t *model;
    if (!topology || !C1DT_String(name) ||
        topology->modelCount > C1DT_MAX_MODELS ||
        topology->boneCount >= C1DT_MAX_BONES) return -1;
    for (i = 0; i < topology->modelCount; ++i) {
        model = &topology->models[i];
        if (!model->resource || model->boneCount > C1MR_BONES_MAX ||
            model->boneCount != model->resource->skeleton.boneCount ||
            model->firstBone > topology->boneCount ||
            model->boneCount > topology->boneCount - model->firstBone) return -1;
        for (j = model->boneCount; j > 0; --j) {
            if (!C1DT_String(model->resource->skeleton.boneName[j - 1])) return -1;
            if (!strcmp(name, model->resource->skeleton.boneName[j - 1]))
                return (int)(model->firstBone + j - 1);
        }
    }
    return -1;
}

/* Build-local index of already validated names. The insertion order is
 * earliest model first, last local bone first; existing matches never change.
 * At most 127 bones occupy 256 slots. No borrowed name survives this build. */
#define C1DT_NAME_SLOTS 256
typedef struct {
    const char *name;
    int target;
} c1dtNameSlot_t;

static unsigned int C1DT_NameHash(const char *name) {
    unsigned int h = 2166136261u;
    while (*name) { h ^= (unsigned char)*name++; h *= 16777619u; }
    return h & (C1DT_NAME_SLOTS - 1);
}
static c1dtNameSlot_t *C1DT_NameSlot(c1dtNameSlot_t *slots, const char *name) {
    unsigned int at = C1DT_NameHash(name);
    while (slots[at].name && strcmp(slots[at].name, name))
        at = (at + 1) & (C1DT_NAME_SLOTS - 1);
    return &slots[at];
}
static void C1DT_IndexNames(const c1dtTopology_t *topology, c1dtNameSlot_t *slots) {
    unsigned int i, j;
    memset(slots, 0, C1DT_NAME_SLOTS * sizeof(*slots));
    for (i = 0; i < topology->modelCount; ++i) {
        const c1dtModel_t *m = &topology->models[i];
        for (j = m->boneCount; j > 0; --j) {
            const char *name = m->resource->skeleton.boneName[j - 1];
            c1dtNameSlot_t *slot = C1DT_NameSlot(slots, name);
            if (!slot->name) {
                slot->name = name;
                slot->target = (int)(m->firstBone + j - 1);
            }
        }
    }
}

/* Native 0x476e00 model ordering/tag acceptance and 0x4761f0 named melding.
 * Input zero is the base model: its tag is ignored. A nonempty secondary tag
 * searches only previously accepted models; missing tags warn and skip that
 * input. Empty-tag secondaries retain every bone and meld matching names.
 * Invalid data/capacity leave output untouched. A valid empty list succeeds.
 * Valid immutable resources are a precondition; all supplied resources are
 * validated even when their model would subsequently be skipped by its tag.
 */
static int C1DT_Build(const c1dtModelInput_t *inputs, unsigned int inputCount,
                      c1dtTopology_t *out) {
    c1dtTopology_t result;
    c1dtModel_t *model;
    c1dtBone_t *bone;
    const c1mrSkeleton_t *s;
    const char *tag;
    unsigned int i, j, global, at;
    int anchor, target;
    c1dtNameSlot_t names[C1DT_NAME_SLOTS];
    int indexed = 0;
    if (!out || (inputCount && !inputs)) return C1DT_INVALID;
    if (inputCount > C1DT_MAX_MODELS) return C1DT_CAPACITY;
    for (i = 0; i < inputCount; ++i)
        if (!C1DT_ResourceValid(inputs[i].resource) ||
            (inputs[i].tagName && !C1DT_String(inputs[i].tagName)))
            return C1DT_INVALID;
    memset(&result, 0, sizeof(result));
    for (i = 0; i < inputCount; ++i) {
        tag = inputs[i].tagName ? inputs[i].tagName : "";
        anchor = -1;
        if (i && tag[0]) {
            anchor = C1DT_FindBone(&result, tag);
            if (anchor < 0) {
                result.missingTagInputMask |= 1U << i;
                ++result.warningCount;
                continue;
            }
        }
        s = &inputs[i].resource->skeleton;
        if (s->boneCount >= C1DT_MAX_BONES - result.boneCount)
            return C1DT_CAPACITY; /* Native accumulated bones must be <128. */
        model = &result.models[result.modelCount];
        model->resource = inputs[i].resource;
        model->inputIndex = i;
        model->firstBone = result.boneCount;
        model->boneCount = s->boneCount;
        model->anchorBone = anchor;
        /* The base tag is ignored, matching the native constructor. */
        if (i && tag[0]) memcpy(model->tagName, tag, strlen(tag) + 1);
        if (s->hasClassification)
            result.classificationModelMask |= 1U << result.modelCount;
        for (j = 0; j < s->boneCount; ++j) {
            global = result.boneCount + j;
            bone = &result.bones[global];
            bone->modelIndex = result.modelCount;
            bone->localBone = j;
            bone->parentBone = s->parent[j] < 0 ? anchor :
                               (int)(result.boneCount + (unsigned int)s->parent[j]);
            bone->duplicateBone = -1;
            bone->canonicalBone = global;
        }
        result.boneCount += s->boneCount;
        ++result.modelCount;
    }
    at = 16;
    /* Lookup is over the completed list, exactly as the lazy native builder.
     * A repeated name within an empty-tag model can target its own last bone.
     */
    for (i = 1; i < result.modelCount; ++i) {
        model = &result.models[i];
        if (model->anchorBone >= 0) continue;
        s = &model->resource->skeleton;
        if (!indexed) { C1DT_IndexNames(&result, names); indexed = 1; }
        for (j = 0; j < model->boneCount; ++j) {
            global = model->firstBone + j;
            {
                c1dtNameSlot_t *slot = C1DT_NameSlot(names, s->boneName[j]);
                target = slot->name ? slot->target : -1;
            }
            if (target < 0) return C1DT_INVALID;
            if ((unsigned int)target == global) continue;
            result.bones[global].duplicateBone = target;
            result.bones[global].canonicalBone = (unsigned int)target;
            result.duplicateBlob[global / 8] |= (unsigned char)(1U << (global % 8));
            result.duplicateBlob[at++] = (unsigned char)(global + 1);
            result.duplicateBlob[at++] = (unsigned char)(target + 1);
            ++result.duplicateCount;
        }
        if (result.bones[model->firstBone].duplicateBone < 0) {
            result.unmeldedRootInputMask |= 1U << model->inputIndex;
            ++result.warningCount;
        }
    }
    result.duplicateBlob[at++] = 0;
    result.duplicateBlobSize = at;
    *out = result;
    return C1DT_OK;
}

/* Metadata stays source metadata. These queries do not prepare a pose,
 * union attachments, reinterpret classifications or provide hit tests.
 */
static int C1DT_FirstModelBounds(const c1dtTopology_t *topology,
                                 c1mBounds_t *out) {
    if (!topology || !out || !topology->modelCount ||
        topology->modelCount > C1DT_MAX_MODELS ||
        !topology->models[0].resource) return 0;
    memcpy(out, &topology->models[0].resource->model.serializedBounds, sizeof(*out));
    return 1;
}
static int C1DT_BoneMetadata(const c1dtTopology_t *topology, unsigned int index,
                             c1mBoneMetadata_t *out, int *hasClassification) {
    const c1dtBone_t *bone;
    const c1mrResource_t *resource;
    if (!topology || !out || !hasClassification ||
        topology->modelCount > C1DT_MAX_MODELS ||
        topology->boneCount >= C1DT_MAX_BONES || index >= topology->boneCount)
        return 0;
    bone = &topology->bones[index];
    if (bone->modelIndex >= topology->modelCount) return 0;
    resource = topology->models[bone->modelIndex].resource;
    if (!resource || bone->localBone >= resource->skeleton.boneCount ||
        bone->localBone >= C1MR_BONES_MAX) return 0;
    memcpy(out, &resource->skeleton.metadata[bone->localBone], sizeof(*out));
    *hasClassification = resource->skeleton.hasClassification;
    return 1;
}
#endif
