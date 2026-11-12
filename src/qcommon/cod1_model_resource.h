/* Owned, headless CoD1 v14 model and LOD0 skeleton source records.
 * This decoder creates neither a renderer model nor a DObj/animated pose.
 * Collision records are bounded and skipped; their counts are metadata only.
 */
#ifndef COD1_MODEL_RESOURCE_H
#define COD1_MODEL_RESOURCE_H
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "cod1_model_metadata.h"

#define C1MR_NAME_MAX 128
#define C1MR_BONES_MAX 128

typedef struct { float p[3], q[4]; } c1mrTransform_t;
typedef struct {
    char lodName[3][C1MR_NAME_MAX];
    float lodDistance[3];
    c1mBounds_t serializedBounds;
    unsigned int rawHeaderWord;
    unsigned int collisionGroupCount, collisionPrimitiveCount;
    unsigned int surfaceCount[3];
} c1mrModelHeader_t;
typedef struct {
    unsigned int boneCount, rootCount;
    int parent[C1MR_BONES_MAX];
    char boneName[C1MR_BONES_MAX][C1MR_NAME_MAX];
    c1mrTransform_t local[C1MR_BONES_MAX];
    c1mBoneMetadata_t metadata[C1MR_BONES_MAX];
    int hasClassification;
} c1mrSkeleton_t;
typedef struct {
    /* Caller-supplied model identity and exact serialized LOD0 parts identity.
     * Both are copied. FS path policy, blob hashes and leases belong to loader.
     */
    char modelName[C1MR_NAME_MAX], partsName[C1MR_NAME_MAX];
    c1mrModelHeader_t model;
    c1mrSkeleton_t skeleton;
} c1mrResource_t;

typedef struct {
    const unsigned char *data;
    size_t size, at;
    int ok;
} c1mrReader_t;

static int C1MR_Take(c1mrReader_t *r, size_t count) {
    if (!r->ok || r->at > r->size || count > r->size - r->at) {
        r->ok = 0;
        return 0;
    }
    r->at += count;
    return 1;
}
static unsigned int C1MR_U8(c1mrReader_t *r) {
    return C1MR_Take(r, 1) ? r->data[r->at - 1] : 0;
}
static unsigned int C1MR_U16(c1mrReader_t *r) {
    unsigned int a, b;
    a = C1MR_U8(r); b = C1MR_U8(r);
    return a | (b << 8);
}
static unsigned int C1MR_U32(c1mrReader_t *r) {
    unsigned int a, b;
    a = C1MR_U16(r); b = C1MR_U16(r);
    return a | (b << 16);
}
static int C1MR_S16(c1mrReader_t *r) {
    unsigned int value;
    value = C1MR_U16(r);
    return value < 32768 ? (int)value : (int)value - 65536;
}
static float C1MR_F32(c1mrReader_t *r) {
    unsigned int bits;
    float value;
    bits = C1MR_U32(r);
    if (sizeof(bits) != 4 || sizeof(value) != 4) {
        r->ok = 0; return 0;
    }
    memcpy(&value, &bits, 4);
    if (!(value >= -FLT_MAX && value <= FLT_MAX)) {
        r->ok = 0; return 0;
    }
    return value;
}
static int C1MR_String(c1mrReader_t *r, char out[C1MR_NAME_MAX]) {
    unsigned int value;
    size_t i;
    for (i = 0; i < C1MR_NAME_MAX; ++i) {
        value = C1MR_U8(r);
        if (!r->ok || (i == C1MR_NAME_MAX - 1 && value)) {
            r->ok = 0; return 0;
        }
        out[i] = (char)value;
        if (!value) return 1;
    }
    r->ok = 0;
    return 0;
}
static int C1MR_CopyName(const char *name, char out[C1MR_NAME_MAX]) {
    size_t i;
    if (!name || !name[0]) return 0;
    for (i = 0; i < C1MR_NAME_MAX; ++i) {
        out[i] = name[i];
        if (!name[i]) return 1;
    }
    return 0;
}
static void C1MR_ReadQuaternion(c1mrReader_t *r, float out[4]) {
    double sum, length;
    int k, value;
    sum = 0;
    /* Retail reconstructs and rounds w in the signed-short domain, then
     * normalizes. This follows CV14_ReadQ's non-simple, positive-w parts lane.
     */
    for (k = 0; k < 3; ++k) {
        value = C1MR_S16(r); out[k] = (float)value;
        sum += (double)value * value;
    }
    if (sum > 32767.0 * 32767.0 * 1.001) {
        r->ok = 0; return;
    }
    out[3] = (float)floor(sqrt(sum < 32767.0 * 32767.0 ?
                        32767.0 * 32767.0 - sum : 0) + .5);
    length = sqrt(sum + (double)out[3] * out[3]);
    if (length < 1) { r->ok = 0; return; }
    for (k = 0; k < 4; ++k) out[k] = (float)(out[k] / length);
}

/* Exact complete model stream. Failure never changes caller's output.
 * Serialized bounds retain their binary32 bits, including exceptional values;
 * a later geometry consumer must validate them before arithmetic/collision.
 */
static int C1MR_DecodeModel(const void *data, size_t size,
                           c1mrModelHeader_t *out) {
    c1mrReader_t r;
    c1mrModelHeader_t result;
    char ignored[C1MR_NAME_MAX];
    unsigned int i, j, count, primitives;
    size_t remaining;
    if (!data || !out || sizeof(unsigned int) != 4 || sizeof(float) != 4)
        return 0;
    memset(&result, 0, sizeof(result));
    r.data = (const unsigned char *)data; r.size = size; r.at = 0; r.ok = 1;
    if (C1MR_U16(&r) != 14 || !C1M_ReadBounds(r.data, r.size, &r.at,
                                            &result.serializedBounds)) return 0;
    for (i = 0; i < 3; ++i) {
        result.lodDistance[i] = C1MR_F32(&r);
        if (!C1MR_String(&r, result.lodName[i])) return 0;
    }
    result.rawHeaderWord = C1MR_U32(&r);
    count = C1MR_U32(&r);
    if (!r.ok || count > size / 40) return 0;
    result.collisionGroupCount = count;
    for (i = 0; i < count; ++i) {
        primitives = C1MR_U32(&r);
        if (!r.ok || r.at > r.size) return 0;
        remaining = r.size - r.at;
        if (remaining < 36 || primitives > (remaining - 36) / 48 ||
            primitives > (unsigned int)-1 - result.collisionPrimitiveCount)
            return 0;
        result.collisionPrimitiveCount += primitives;
        if (!C1MR_Take(&r, 36 + (size_t)primitives * 48)) return 0;
    }
    for (i = 0; i < 3; ++i) if (result.lodName[i][0]) {
        result.surfaceCount[i] = C1MR_U16(&r);
        for (j = 0; j < result.surfaceCount[i]; ++j)
            if (!C1MR_String(&r, ignored)) return 0;
    }
    if (!r.ok || r.at != r.size || !result.lodName[0][0] ||
        !result.surfaceCount[0]) return 0;
    *out = result;
    return 1;
}

/* Owned local skeleton only: no current/world transforms or source pointers.
 * Up to 128 serialized bones are representable here, matching retained CV14
 * decoder capacity. Retail's resource loader and DObj both require <128;
 * runtime consumers must enforce that policy before publishing a resource.
 * The legacy no-classification tail is explicit.
 */
static int C1MR_DecodeParts(const void *data, size_t size,
                           c1mrSkeleton_t *out) {
    c1mrReader_t r;
    c1mrSkeleton_t result;
    unsigned int children, roots, i;
    int k;
    if (!data || !out || sizeof(unsigned int) != 4 || sizeof(float) != 4)
        return 0;
    memset(&result, 0, sizeof(result));
    r.data = (const unsigned char *)data; r.size = size; r.at = 0; r.ok = 1;
    if (C1MR_U16(&r) != 14) return 0;
    children = C1MR_U16(&r); roots = C1MR_U16(&r);
    if (!r.ok || !roots || roots > C1MR_BONES_MAX ||
        children > C1MR_BONES_MAX - roots) return 0;
    result.rootCount = roots; result.boneCount = children + roots;
    for (i = 0; i < result.boneCount; ++i) {
        result.parent[i] = -1; result.local[i].q[3] = 1;
        if (i >= roots) {
            result.parent[i] = (int)C1MR_U8(&r);
            if (!r.ok || result.parent[i] >= (int)i) return 0;
            for (k = 0; k < 3; ++k) result.local[i].p[k] = C1MR_F32(&r);
            C1MR_ReadQuaternion(&r, result.local[i].q);
            if (!r.ok) return 0;
        }
    }
    for (i = 0; i < result.boneCount; ++i)
        if (!C1MR_String(&r, result.boneName[i]) ||
            !C1M_ReadBounds(r.data, r.size, &r.at, &result.metadata[i].bounds))
            return 0;
    if (r.at > r.size) return 0;
    if (r.size - r.at == result.boneCount) {
        if (!C1M_ReadBoneClassifications(r.data, r.size, &r.at,
                                       result.metadata, result.boneCount,
                                       C1MR_BONES_MAX)) return 0;
        result.hasClassification = 1;
    }
    if (!r.ok || r.at != r.size) return 0;
    *out = result;
    return 1;
}

static int C1MR_DecodeResource(const char *modelName,
                              const void *modelData, size_t modelSize,
                              const char *partsName,
                              const void *partsData, size_t partsSize,
                              c1mrResource_t *out) {
    c1mrResource_t result;
    if (!out) return 0;
    memset(&result, 0, sizeof(result));
    if (!C1MR_CopyName(modelName, result.modelName) ||
        !C1MR_CopyName(partsName, result.partsName) ||
        !C1MR_DecodeModel(modelData, modelSize, &result.model) ||
        strcmp(result.partsName, result.model.lodName[0]) ||
        !C1MR_DecodeParts(partsData, partsSize, &result.skeleton)) return 0;
    *out = result;
    return 1;
}
#endif
