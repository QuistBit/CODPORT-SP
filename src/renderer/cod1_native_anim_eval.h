/* Shared-resource, per-request pose evaluator for CoD1 v14 viewmodels.
 * Definitions and clip bytes are immutable. No mutable animation state or
 * scratch pose is stored in a model or shared between submissions.
 *
 * This reproduces the validated bone-level offline evaluator. Time advance,
 * transition weights and event selection are supplied by the caller; this
 * function does not interpret WeaponDef actions or execute notetracks.
 */
#ifndef COD1_NATIVE_ANIM_EVAL_H
#define COD1_NATIVE_ANIM_EVAL_H

#include "cod1_viewmodel14.h"
#include "../game/bg_cod1_native_runtime.h"

typedef struct {
    const unsigned char *data;
    size_t size;
    int parts, lastFrame;
    unsigned char remap[CV14_BONES]; /* 0x7f means no matching DObj bone. */
} cnLeaf_t;

typedef struct {
    int bones;
    int parent[CV14_BONES];
    char boneName[CV14_BONES][XM14_NAME];
    cv14Transform bindLocal[CV14_BONES];
    cnLeaf_t leaf[COD1_NATIVE_LEAVES];
} cnDefinition_t;

/* Returns zero on invalid input, clip, remap or hierarchy. The caller must
 * discard the complete submission on failure; final[] can be partly written.
 * A copy of final[] belongs to the caller, never to the shared definition. */
static int CN_Evaluate(const cnDefinition_t *definition,
                       const cod1NativeAnimState_t *state,
                       cv14Transform final[CV14_BONES]) {
    float sumP[CV14_BONES][3], sumQ[CV14_BONES][4], total[CV14_BONES];
    cv14Transform local;
    cv14Pose sample;
    const cnLeaf_t *clip;
    int leaf, part, bone, k, fps, frames, flags, lastFrame;
    float weight, dot, sign, length;

    if (!definition || !state || !final ||
        state->version != COD1_NATIVE_STATE_VERSION ||
        state->size != (int)sizeof(*state) ||
        definition->bones <= 0 || definition->bones > 0x7f ||
        definition->bones > CV14_BONES)
        return 0;

    memset(sumP, 0, sizeof(sumP));
    memset(sumQ, 0, sizeof(sumQ));
    memset(total, 0, sizeof(total));

    /* Check even dormant slots, so an uninitialized request is never accepted
     * merely because that leaf currently has zero weight. Slot 0 is the root. */
    for (leaf = 0; leaf < COD1_NATIVE_LEAVES; leaf++) {
        if (!(state->weight[leaf] >= 0.0f && state->weight[leaf] <= 1.0f) ||
            !(state->time[leaf] >= 0.0f && state->time[leaf] <= 1.0f))
            return 0;
    }

    for (leaf = 1; leaf < COD1_NATIVE_LEAVES; leaf++) {
        weight = state->weight[leaf];
        if (weight == 0.0f) continue;
        clip = &definition->leaf[leaf];
        if (!clip->data || clip->size < 9 ||
            clip->data[0] != 14 || clip->data[1] != 0 ||
            clip->parts <= 0 || clip->parts > CV14_BONES ||
            clip->parts != (int)(clip->data[4] | (clip->data[5] << 8)))
            return 0;
        flags = clip->data[6];
        fps = clip->data[7] | (clip->data[8] << 8);
        frames = clip->data[2] | (clip->data[3] << 8);
        lastFrame = frames - 1 + (flags & 1);
        if ((flags & ~1) || fps <= 0 || frames <= 0 ||
            lastFrame < 0 || lastFrame >= 4096 ||
            clip->lastFrame != lastFrame ||
            !CV14_SamplePose(clip->data, clip->size,
                             state->time[leaf] * lastFrame, &sample) ||
            sample.n != clip->parts)
            return 0;

        for (part = 0; part < sample.n; part++) {
            bone = clip->remap[part];
            if (bone == 0x7f) continue;
            if (bone >= definition->bones ||
                strcmp(definition->boneName[bone], sample.name[part]))
                return 0;
            sign = 1.0f;
            if (total[bone] > 0.0f) {
                dot = 0.0f;
                for (k = 0; k < 4; k++)
                    dot += sumQ[bone][k] * sample.t[part].q[k];
                if (dot < 0.0f) sign = -1.0f;
            }
            for (k = 0; k < 3; k++)
                sumP[bone][k] += weight * sample.t[part].p[k];
            for (k = 0; k < 4; k++)
                sumQ[bone][k] += weight * sign * sample.t[part].q[k];
            total[bone] += weight;
        }
    }

    for (bone = 0; bone < definition->bones; bone++) {
        if (definition->parent[bone] < -1 || definition->parent[bone] >= bone)
            return 0;
        local = definition->bindLocal[bone];
        if (total[bone] > 0.0f) {
            for (k = 0; k < 3; k++)
                local.p[k] = sumP[bone][k] / total[bone];
            length = 0.0f;
            for (k = 0; k < 4; k++)
                length += sumQ[bone][k] * sumQ[bone][k];
            if (!(length > 1.0e-15f)) return 0;
            length = sqrtf(length);
            for (k = 0; k < 4; k++)
                local.q[k] = sumQ[bone][k] / length;
        }
        if (definition->parent[bone] < 0) final[bone] = local;
        else CV14_Compose(&final[definition->parent[bone]], &local, &final[bone]);
        for (k = 0; k < 3; k++)
            if (!(final[bone].p[k] >= -1000000.0f && final[bone].p[k] <= 1000000.0f))
                return 0;
        for (k = 0; k < 4; k++)
            if (!(final[bone].q[k] >= -2.0f && final[bone].q[k] <= 2.0f))
                return 0;
    }
    return 1;
}

#endif /* COD1_NATIVE_ANIM_EVAL_H */
