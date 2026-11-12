/* Bounded actor-only XAnim v14 bone sampler. Root channels are validated and
 * omitted from a private copy; this path never infers world displacement.
 * First-person weapon clips continue to use CV14_SamplePose unchanged. */
#ifndef COD1_ACTOR_XANIM14_H
#define COD1_ACTOR_XANIM14_H

#include "cod1_viewmodel14.h"

#define CA14_ACTOR_CLIP_LIMIT (16u * 1024u * 1024u)

typedef struct {
    unsigned char *boneData;
    size_t boneSize;
    int frames, fps, originalFlags;
    int rootRotationKeys, rootTranslationKeys;
} ca14ActorClip;

static void CA14_ActorClipFree(ca14ActorClip *clip) {
    if (!clip) return;
    free(clip->boneData);
    memset(clip, 0, sizeof(*clip));
}

/* The accepted Native R3 viewmodel sampler intentionally accepts a bounded
 * opaque note tail. Actor clips get a stricter one-time structural check; no
 * note is executed or copied into the runtime actor state. */
static int CA14_ActorClipValidateTail(const unsigned char *data, size_t size,
                                      int frameCount, int boneCount) {
    xm14Reader r;
    int frames, flags, bones, i, j, n, count, ids[4096];
    char name[XM14_NAME];
    r.p = data;
    r.n = size;
    r.at = 0;
    r.ok = 1;
    if (XM14_U16(&r) != 14) return 0;
    frames = (int)XM14_U16(&r);
    bones = (int)XM14_U16(&r);
    flags = (int)XM14_U8(&r);
    XM14_U16(&r); /* fps: checked by CA14_ActorClipLoad */
    if (!r.ok || frames != frameCount || bones != boneCount ||
        (flags & ~1)) return 0;
    frames += flags & 1;
    n = (bones + 7) / 8;
    if (!XM14_Take(&r, (size_t)(2 * n))) return 0;
    for (i = 0; i < bones; i++) if (!XM14_String(&r, name)) return 0;
    for (i = 0; i < bones; i++) {
        int simple = data[9 + n + i / 8] & (1 << (i % 8));
        count = (int)XM14_U16(&r);
        if (!r.ok || !CV14_Indices(&r, count, frames, ids) ||
            !XM14_Take(&r, (size_t)count * (simple ? 2u : 6u))) return 0;
        count = (int)XM14_U16(&r);
        if (!r.ok || !CV14_Indices(&r, count, frames, ids) ||
            !XM14_Take(&r, (size_t)count * 3u * sizeof(float))) return 0;
    }
    count = (int)XM14_U8(&r);
    for (j = 0; j < count; j++) {
        if (!XM14_String(&r, name) || XM14_U16(&r) > (unsigned int)frameCount)
            return 0;
    }
    return r.ok && r.at == r.n;
}

/* `out` is assigned only after the entire source, including bone tracks and
 * notetrack tail, has passed CV14_SamplePose. The original file is untouched.
 * out must initially be zero; free an old clip before loading a new one. */
static int CA14_ActorClipLoad(const void *data, size_t size, ca14ActorClip *out) {
    xm14Reader r;
    ca14ActorClip staged;
    cv14Pose pose;
    int count, ids[4096], frames, bones, flags, fps, serialized, i, k;
    size_t prefix;

    if (!data || !out || out->boneData || size < 9 ||
        size > CA14_ACTOR_CLIP_LIMIT) return 0;
    memset(&staged, 0, sizeof(staged));
    r.p = (const unsigned char *)data;
    r.n = size;
    r.at = 0;
    r.ok = 1;
    if (XM14_U16(&r) != 14) return 0;
    frames = (int)XM14_U16(&r);
    bones = (int)XM14_U16(&r);
    flags = (int)XM14_U8(&r);
    fps = (int)XM14_U16(&r);
    if (!r.ok || frames < 1 || frames > 4095 || bones < 1 ||
        bones > CV14_BONES || (flags & ~3) || fps < 1 || fps > 120)
        return 0;
    serialized = frames + (flags & 1);
    if (flags & 2) {
        count = (int)XM14_U16(&r);
        if (!r.ok || !CV14_Indices(&r, count, serialized, ids)) return 0;
        staged.rootRotationKeys = count;
        for (i = 0; i < count; i++) CV14_S16(&r);
        count = (int)XM14_U16(&r);
        if (!r.ok || !CV14_Indices(&r, count, serialized, ids)) return 0;
        staged.rootTranslationKeys = count;
        for (i = 0; i < count; i++)
            for (k = 0; k < 3; k++) XM14_F32(&r);
        if (!r.ok) return 0;
    }
    prefix = r.at - 9;
    staged.boneSize = size - prefix;
    staged.boneData = (unsigned char *)malloc(staged.boneSize);
    if (!staged.boneData) return 0;
    memcpy(staged.boneData, data, 9);
    memcpy(staged.boneData + 9, r.p + r.at, size - r.at);
    staged.boneData[6] &= (unsigned char)~2;
    staged.frames = frames;
    staged.fps = fps;
    staged.originalFlags = flags;
    if (!CA14_ActorClipValidateTail(staged.boneData, staged.boneSize,
                                    frames, bones) ||
        !CV14_SamplePose(staged.boneData, staged.boneSize, 0.f, &pose) ||
        pose.n != bones ||
        !CV14_SamplePose(staged.boneData, staged.boneSize,
                         (float)(frames - 1), &pose)) {
        CA14_ActorClipFree(&staged);
        return 0;
    }
    *out = staged;
    return 1;
}

/* The extra serialized frame for loop clips is sampled only when the elapsed
 * time approaches the next cycle. Double precision keeps long map sessions
 * from losing the fractional interpolation at 30 fps. */
static int CA14_ActorClipSample(const ca14ActorClip *clip,
                                 int elapsedMillis, cv14Pose *out) {
    double frame;
    if (!clip || !clip->boneData || !clip->frames || !clip->fps || !out)
        return 0;
    if (elapsedMillis < 0) elapsedMillis = 0;
    frame = fmod((double)elapsedMillis * (double)clip->fps / 1000.0,
                 (double)clip->frames);
    return CV14_SamplePose(clip->boneData, clip->boneSize, (float)frame, out);
}

#endif /* COD1_ACTOR_XANIM14_H */
