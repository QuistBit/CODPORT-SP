/* Real renderer geometry kernels + real C1J scheduler through the pthread
 * Win32 test shim. No game assets, GL context, or MSVC ABI claim. */
#include <assert.h>
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mt_test_runtime.h"
#include "../../src/game/q_shared.h"
#include "../../src/qcommon/qfiles.h"
#include "../../src/renderer/tr_cod1_skin_batch.h"
#define C1AA14_PARALLEL 1
#include "../../src/renderer/cod1_actor_assembly14.h"
/* The native MSVC-only animation sampler is unrelated to capture geometry.
 * C1TAM_CapturePose below consumes an already supplied immutable pose. */
#define COD1_TURRET_ANIMATED_POSE14_H
#include "../../src/renderer/cod1_turret_animated_mesh.h"

static void init_pose(cv14Skeleton *pose, int bones) {
    int i;
    memset(pose, 0, sizeof(*pose));
    pose->n = bones;
    for (i = 0; i < bones; i++) {
        snprintf(pose->name[i], sizeof(pose->name[i]), "bone_%d", i);
        pose->parent[i] = i ? i - 1 : -1;
        pose->world[i].q[2] = .6f;
        pose->world[i].q[3] = .8f;
        pose->world[i].p[0] = (float)i * 3.25f;
        pose->world[i].p[1] = (float)i * -.5f;
        pose->world[i].p[2] = (float)i * 1.125f;
        pose->local[i] = pose->world[i];
    }
}

static void init_vertex(cnSourceVertex_t *v, unsigned index, int weights) {
    unsigned i;
    memset(v, 0, sizeof(*v));
    v->normalBone = index % 2;
    v->normal[0] = index % 3 ? .2f : -.0f;
    v->normal[1] = .7f;
    v->normal[2] = -.3f;
    v->uv[0] = index % 7 ? (float)(index % 31) / 32.f : -.0f;
    v->uv[1] = (float)(index % 17) / 16.f;
    v->numWeights = (unsigned char)weights;
    for (i = 0; i < (unsigned)weights; i++) {
        v->weight[i].bone = i % 2;
        v->weight[i].weight = weights == 1 ? 1.f : i ? .75f : .25f;
        v->weight[i].offset[0] = (float)(index % 29) / 4.f - 2.f;
        v->weight[i].offset[1] = (float)(index % 13) / 8.f;
        v->weight[i].offset[2] = (float)(index % 11) / 16.f - 1.f;
    }
}

static void init_source(cnSourceSurface_t *surface, unsigned count, int weights) {
    unsigned i;
    memset(surface, 0, sizeof(*surface));
    surface->numVerts = (int)count;
    surface->numIndexes = 3;
    surface->verts = calloc(count, sizeof(*surface->verts));
    surface->indexes = calloc(3, sizeof(*surface->indexes));
    assert(surface->verts && surface->indexes);
    surface->indexes[1] = 1;
    surface->indexes[2] = 2;
    for (i = 0; i < count; i++) init_vertex(surface->verts + i, i, weights);
}

/* Same serial caller loop; actual VectorNormalize is linked from q_math.c. */
static int serial_draw(const c1DrawSkinSpan *spans, unsigned count) {
    unsigned i, j;
    int k;
    for (i = 0; i < count; i++) {
        const c1DrawSkinSpan *span = spans + i;
        for (j = 0; j < span->count; j++) {
            drawVert_t *v = span->vertices + j;
            if (!CN_SourceSkinVertex(span->source + j, span->pose, v->xyz, v->normal)) return 0;
            VectorNormalize(v->normal);
            for (k = 0; k < 2; k++) v->st[k] = span->source[j].uv[k];
            if (span->white) v->color[0] = v->color[1] = v->color[2] = v->color[3] = 255;
        }
    }
    return 1;
}

static void test_draw_spans(int round_mode) {
    const unsigned counts[3] = { 255, 257, 515 };
    cnSourceSurface_t source[3];
    cv14Skeleton pose[3];
    c1DrawSkinSpan serial[3], parallel[3];
    drawVert_t *a[3], *b[3];
    unsigned i;
    int flags_a, flags_b;
    c1jStatus before, after;
    for (i = 0; i < 3; i++) {
        init_source(source + i, counts[i], 2);
        init_pose(pose + i, 2);
        pose[i].world[0].p[2] = (float)i;
        a[i] = malloc((counts[i] + 2) * sizeof(drawVert_t));
        b[i] = malloc((counts[i] + 2) * sizeof(drawVert_t));
        assert(a[i] && b[i]);
        memset(a[i], 0xa5, (counts[i] + 2) * sizeof(drawVert_t));
        memset(b[i], 0xa5, (counts[i] + 2) * sizeof(drawVert_t));
        serial[i].source = source[i].verts;
        serial[i].pose = pose + i;
        serial[i].vertices = a[i] + 1;
        serial[i].count = counts[i];
        serial[i].white = (int)(i & 1);
        parallel[i] = serial[i];
        parallel[i].vertices = b[i] + 1;
    }
    assert(fesetround(round_mode) == 0);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    assert(serial_draw(serial, 3));
    flags_a = fetestexcept(FE_ALL_EXCEPT);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    C1J_Configure(1, 3);
    C1J_Status(&before);
    assert(R_Cod1TryDrawSkin(parallel, 3, counts[0] + counts[1] + counts[2]));
    C1J_Status(&after);
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    assert(after.batches == before.batches + 1);
    assert(flags_a == flags_b && fegetround() == round_mode);
    for (i = 0; i < 3; i++)
        assert(!memcmp(a[i], b[i], (counts[i] + 2) * sizeof(drawVert_t)));

    /* Failure in a later range must not leak worker FP flags. The ordinary
     * capture then replays from the original environment and rolls back. */
    source[1].verts[31].numWeights = 0;
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    assert(!R_Cod1TryDrawSkin(parallel, 3, counts[0] + counts[1] + counts[2]));
    assert(fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO);
    assert(!serial_draw(parallel, 3));
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_DIVBYZERO);
    assert(!serial_draw(serial, 3));
    assert(flags_b == fetestexcept(FE_ALL_EXCEPT));
    C1J_Configure(0, 3);
    assert(!R_Cod1TryDrawSkin(parallel, 3, counts[0] + counts[1] + counts[2]));
    C1J_Configure(1, 3);
    assert(!R_Cod1TryDrawSkin(parallel, 1, 512));
    for (i = 0; i < 3; i++) {
        free(a[i]); free(b[i]); free(source[i].verts); free(source[i].indexes);
    }
}

static c1aa14Resource_t *actor_resource(unsigned parts) {
    c1aa14Resource_t *r = calloc(1, sizeof(*r));
    c1dtModelInput_t input[C1AA14_PARTS];
    unsigned p, i;
    assert(r);
    assert(parts > 0 && parts <= C1AA14_PARTS);
    r->partCount = parts;
    for (p = 0; p < parts; p++) {
        c1aa14Part_t *part = r->part + p;
        snprintf(part->source.modelName, sizeof(part->source.modelName), "model_%u", p);
        snprintf(part->source.partsName, sizeof(part->source.partsName), "parts_%u", p);
        part->source.skeleton.boneCount = 2;
        part->source.skeleton.rootCount = 1;
        init_pose(&part->mesh.bind, 2);
        for (i = 0; i < 2; i++) {
            snprintf(part->mesh.bind.name[i], XM14_NAME, "part%u_bone%u", p, i);
            strcpy(part->source.skeleton.boneName[i], part->mesh.bind.name[i]);
            part->source.skeleton.parent[i] = i ? 0 : -1;
        }
        part->mesh.numSurfaces = 2;
        part->model.ns[0] = 2;
        strcpy(part->model.skin[0][0], "skin_a");
        strcpy(part->model.skin[0][1], "skin_b");
        init_source(&part->mesh.surfaces[0], 257 + p * 19, 2);
        init_source(&part->mesh.surfaces[1], 263 + p * 23, 2);
        if (p) strcpy(part->tag, "part0_bone1");
        input[p].resource = &part->source;
        input[p].tagName = part->tag;
    }
    assert(C1DT_Build(input, parts, &r->topology) == C1DT_OK);
    return r;
}

static void compare_actor(const c1aa14Frame_t *a, const c1aa14Frame_t *b) {
    c1aa14Frame_t ca = *a, cb = *b;
    unsigned i;
    ca.surface = cb.surface = NULL;
    assert(!memcmp(&ca, &cb, sizeof(ca)));
    for (i = 0; i < a->surfaceCount; i++) {
        c1aa14Surface_t sa = a->surface[i], sb = b->surface[i];
        assert(!memcmp(sa.vertex, sb.vertex, (size_t)sa.vertices * sizeof(*sa.vertex)));
        assert(!memcmp(sa.index, sb.index, (size_t)sa.indexes * sizeof(*sa.index)));
        sa.vertex = sb.vertex = NULL;
        sa.index = sb.index = NULL;
        assert(!memcmp(&sa, &sb, sizeof(sa)));
    }
}

static void test_actor_capture(int round_mode, unsigned parts) {
    c1aa14Resource_t *r = actor_resource(parts);
    cnsArena_t a = { malloc(1024 * 1024), 1024 * 1024, 32 };
    cnsArena_t b = { malloc(1024 * 1024), 1024 * 1024, 32 };
    cv14Transform pose[C1AA14_PARTS * 2];
    c1aa14Frame_t *fa, *fb;
    unsigned i;
    int flags_a, flags_b;
    c1jStatus before, after;
    assert(a.data && b.data);
    for (i = 0; i < parts * 2; i++) pose[i] = r->part[i / 2].mesh.bind.world[i % 2];
    assert(fesetround(round_mode) == 0);
    C1J_Configure(0, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fa = C1AA14_CaptureFull(r, pose, parts * 2, &a);
    flags_a = fetestexcept(FE_ALL_EXCEPT);
    C1J_Configure(1, 3);
    C1J_Status(&before);
    feclearexcept(FE_ALL_EXCEPT);
    fb = C1AA14_CaptureFull(r, pose, parts * 2, &b);
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    C1J_Status(&after);
    assert(fa && fb && a.used == b.used && flags_a == flags_b);
    assert(after.batches == before.batches + 1); /* Was one join per part. */
    compare_actor(fa, fb);

    /* Late normal refusal exercises the real capture's serial replay and
     * rollback, rather than only invoking the worker callback directly. */
    memset(r->part[parts - 1].mesh.surfaces[1].verts[7].normal, 0, 12);
    a.used = b.used = 32;
    C1J_Configure(0, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fa = C1AA14_CaptureFull(r, pose, parts * 2, &a);
    flags_a = fetestexcept(FE_ALL_EXCEPT);
    C1J_Configure(1, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fb = C1AA14_CaptureFull(r, pose, parts * 2, &b);
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    assert(!fa && !fb && a.used == 32 && b.used == 32 && flags_a == flags_b);
    assert(fegetround() == round_mode);
    free(a.data); free(b.data);
    C1AA14_Free(r);
}

static void compare_turret(const c1tmFrame_t *a, const c1tmFrame_t *b) {
    c1tmFrame_t ca = *a, cb = *b;
    unsigned i;
    for (i = 0; i < a->surfaces; i++) {
        assert(!memcmp(a->surface[i].vertex, b->surface[i].vertex,
                       (size_t)a->surface[i].vertices * sizeof(c1tmVertex_t)));
        assert(!memcmp(a->surface[i].index, b->surface[i].index,
                       (size_t)a->surface[i].indexes * sizeof(unsigned short)));
        ca.surface[i].vertex = cb.surface[i].vertex = NULL;
        ca.surface[i].index = cb.surface[i].index = NULL;
    }
    assert(!memcmp(&ca, &cb, sizeof(ca)));
}

static void test_turret_capture(int round_mode) {
    c1tmResource_t *r = calloc(1, sizeof(*r));
    c1tpStaticPose_t pose;
    cnsArena_t a = { malloc(1024 * 1024), 1024 * 1024, 32 };
    cnsArena_t b = { malloc(1024 * 1024), 1024 * 1024, 32 };
    c1tmFrame_t *fa, *fb;
    float angles[3] = { 0, 0, 0 };
    unsigned i;
    int flags_a, flags_b;
    c1jStatus before, after;
    assert(r && a.data && b.data);
    init_pose(&r->mesh.bind, 13);
    memset(&pose, 0, sizeof(pose));
    pose.bones = 13; pose.flash = 2;
    strcpy(r->mesh.bind.name[2], "tag_flash");
    for (i = 0; i < 13; i++) {
        strcpy(r->bones.skeleton.boneName[i], r->mesh.bind.name[i]);
        r->bones.skeleton.parent[i] = r->mesh.bind.parent[i];
        memcpy(pose.q[i], r->mesh.bind.world[i].q, 16);
        memcpy(pose.p[i], r->mesh.bind.world[i].p, 12);
    }
    r->mesh.numSurfaces = r->model.ns[0] = 2;
    for (i = 0; i < 2; i++) {
        strcpy(r->model.skin[0][i], "mg42_skin");
        init_source(&r->mesh.surfaces[i], 277 + 31 * i, 1);
    }
    assert(fesetround(round_mode) == 0);
    C1J_Configure(0, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fa = C1TAM_CapturePose(r, angles, &pose, &a);
    flags_a = fetestexcept(FE_ALL_EXCEPT);
    C1J_Configure(1, 3);
    C1J_Status(&before);
    feclearexcept(FE_ALL_EXCEPT);
    fb = C1TAM_CapturePose(r, angles, &pose, &b);
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    C1J_Status(&after);
    assert(fa && fb && a.used == b.used && flags_a == flags_b);
    assert(after.batches == before.batches + 1);
    compare_turret(fa, fb);

    memset(r->mesh.surfaces[1].verts[12].normal, 0, 12);
    a.used = b.used = 32;
    C1J_Configure(0, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fa = C1TAM_CapturePose(r, angles, &pose, &a);
    flags_a = fetestexcept(FE_ALL_EXCEPT);
    C1J_Configure(1, 3);
    feclearexcept(FE_ALL_EXCEPT);
    fb = C1TAM_CapturePose(r, angles, &pose, &b);
    flags_b = fetestexcept(FE_ALL_EXCEPT);
    assert(!fa && !fb && a.used == 32 && b.used == 32 && flags_a == flags_b);
    assert(fegetround() == round_mode);
    free(a.data); free(b.data); C1TM_Free(r);
}

int main(void) {
    const int modes[4] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };
    unsigned i;
    for (i = 0; i < 4; i++) {
        test_draw_spans(modes[i]);
        test_actor_capture(modes[i], 2);
        test_actor_capture(modes[i], C1AA14_PARTS);
        test_turret_capture(modes[i]);
    }
    C1J_Shutdown();
    fesetround(FE_TONEAREST);
    printf("geometry: 16 rounding-mode suites passed; vertices, metadata, bounds, FP flags, failure replay and rollback agree\n");
    printf("caller batch storage: skeleton=%zu, prepared=%zu, eight parts=%zu bytes (portable build)\n",
           sizeof(cv14Skeleton), sizeof(cnSkinPose_t),
           (sizeof(cv14Skeleton) + sizeof(cnSkinPose_t)) * C1AA14_PARTS);
    return 0;
}
