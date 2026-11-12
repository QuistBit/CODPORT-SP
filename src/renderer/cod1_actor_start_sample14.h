/* R208 owned time-zero root translation. The existing actor bone sampler and
 * its private root stripping remain unchanged. Original CoDSP syscall114:
 * 44aece -> 47d1e0 -> 47a270 -> 479cd0; source loader478350..478722.
 * This is a separate resource sample, never an actor movement/playback claim. */
#ifndef COD1_ACTOR_START_SAMPLE14_H
#define COD1_ACTOR_START_SAMPLE14_H
#include <stdint.h>
#include "cod1_actor_xanim14.h"

typedef struct {
    uint32_t frames, fps, flags, intervals, rotationKeys, translationKeys;
    uint32_t rawTranslationBits[3];
    float translation[3];
} ca14StartSample;

static int CA14_StartSampleLoad(const void *data, size_t size,
                               ca14StartSample *out) {
    ca14ActorClip clip;
    ca14StartSample staged;
    xm14Reader r;
    uintptr_t a = (uintptr_t)data, b = (uintptr_t)out;
    int count, ids[4096], i, k, serialized;
    if (!data || !out || !size ||
        (a <= b ? b - a < size : a - b < sizeof(*out))) return 0;
    memset(&clip, 0, sizeof(clip));
    memset(&staged, 0, sizeof(staged));
    /* Entire clip, every root/bone channel and exact note tail first. Never
     * admit a useful-looking root prefix from an otherwise malformed clip. */
    if (!CA14_ActorClipLoad(data, size, &clip)) return 0;
    staged.frames = (uint32_t)clip.frames;
    staged.fps = (uint32_t)clip.fps;
    staged.flags = (uint32_t)clip.originalFlags;
    staged.intervals = staged.frames - 1 + (staged.flags & 1);
    staged.rotationKeys = (uint32_t)clip.rootRotationKeys;
    staged.translationKeys = (uint32_t)clip.rootTranslationKeys;
    CA14_ActorClipFree(&clip);
    r.p = (const unsigned char *)data; r.n = size; r.at = 9; r.ok = 1;
    serialized = (int)(staged.frames + (staged.flags & 1));
    if (staged.flags & 2) {
        count = (int)XM14_U16(&r);
        if (!CV14_Indices(&r, count, serialized, ids)) return 0;
        for (i = 0; i < count; ++i) CV14_S16(&r);
        count = (int)XM14_U16(&r);
        if (!CV14_Indices(&r, count, serialized, ids)) return 0;
        /* The v14 stream is key-major XYZ. Copy the first complete key
         * before advancing over the remaining validated translation triples. */
        if (count) {
            for (k = 0; k < 3; ++k) {
                float value = XM14_F32(&r);
                memcpy(&staged.rawTranslationBits[k], &value, sizeof(value));
            }
            if (!XM14_Take(&r, (size_t)(count - 1) * 12)) return 0;
        }
        if (!r.ok) return 0;
        for (k = 0; k < 3; ++k) {
            uint32_t bits = staged.rawTranslationBits[k];
            /* At normalized time0, first index0 supplies the first finite
             * sample. Native weight1 plus the positive-zero accumulator
             * canonicalizes zero under the selected nearest profile. Keep
             * source bits separately; never normalize origin/angle/result. */
            if (!(bits & UINT32_C(0x7fffffff))) bits = 0;
            memcpy(&staged.translation[k], &bits, sizeof(bits));
        }
    }
    /* No delta or absent translation is explicit native positive zero, not
     * a fallback for unknown/invalid resources. Counts distinguish absence. */
    *out = staged;
    return 1;
}
#endif
