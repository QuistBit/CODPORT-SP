/* CoD1 v14 serialized model metadata, usable without renderer or game state.
 * These records are source data, not a DObj, posed bounds or collision result.
 * Bounds keep the serialized float bits; consumers own their geometry policy.
 */
#ifndef COD1_MODEL_METADATA_H
#define COD1_MODEL_METADATA_H
#include <stddef.h>
#include <string.h>

typedef struct {
    float mins[3], maxs[3];
} c1mBounds_t;

typedef struct {
    c1mBounds_t bounds;
    unsigned char classification;
} c1mBoneMetadata_t;

/* Little-endian bounds: six binary32 values. Failure leaves both output and
 * offset unchanged. No alignment, host endian or float conversion is assumed.
 * A caller must validate geometry before using these values for collision.
 */
static int C1M_ReadBounds(const unsigned char *data, size_t size,
                         size_t *offset, c1mBounds_t *out) {
    c1mBounds_t result;
    unsigned int bits;
    float value;
    size_t at;
    int i;
    if (!data || !offset || !out || sizeof(bits) != 4 || sizeof(value) != 4)
        return 0;
    at = *offset;
    if (at > size || size - at < 24) return 0;
    for (i = 0; i < 6; ++i) {
        bits = (unsigned int)data[at] |
               ((unsigned int)data[at + 1] << 8) |
               ((unsigned int)data[at + 2] << 16) |
               ((unsigned int)data[at + 3] << 24);
        memcpy(&value, &bits, 4);
        if (i < 3) memcpy(&result.mins[i], &value, 4);
        else memcpy(&result.maxs[i - 3], &value, 4);
        at += 4;
    }
    *out = result;
    *offset = at;
    return 1;
}

/* One classification byte per bone, following all named bone bounds. This
 * does not assign names to the classification values or alter bone bounds.
 */
static int C1M_ReadBoneClassifications(const unsigned char *data, size_t size,
                                      size_t *offset,
                                      c1mBoneMetadata_t *bones,
                                      size_t count, size_t capacity) {
    size_t at, i;
    if (!data || !offset || !bones || !count || count > capacity) return 0;
    at = *offset;
    if (at > size || count > size - at) return 0;
    for (i = 0; i < count; ++i) bones[i].classification = data[at + i];
    *offset = at + count;
    return 1;
}
#endif
