/* Pilot CoD1 actor: one immutable weighted resource, one skeleton per entity,
 * and one skinned draw snapshot per view. This is deliberately separate from
 * first-person Native R3 and the bind-pose world XModel adapter. */
#ifndef TR_COD1_ACTOR_CAPTURE_H
#define TR_COD1_ACTOR_CAPTURE_H

#include "tr_cod1_actor_resource.h"

struct cod1ActorSubmission_s {
    cv14Skeleton part[COD1_ACTOR_PARTS];
};

typedef struct {
    const cod1ActorSurface_t *source;
    srfTriangles_t geometry;
} cod1ActorDrawSurface_t;

typedef struct cod1ActorDraw_s {
    int numSurfaces;
    cod1ActorDrawSurface_t *surface;
    vec3_t bounds[2];
    vec3_t localOrigin;
    float radius;
} cod1ActorDraw_t;

typedef struct {
    cv14Transform weaponRight; /* local to the actor entity */
    cv14Transform flash;       /* local to the actor entity */
} cod1ActorPilotTags_t;

int RE_AddCod1ActorPilot(const refEntity_t *ent, const cv14Pose *pose,
                         cod1ActorPilotTags_t *tags);

cod1ActorSubmission_t *R_Cod1ActorCapturePose(
        const cod1ActorModel_t *resource, cnsArena_t *arena,
        const cv14Pose *bodyPose);
int R_Cod1ActorFindTag(const cod1ActorSubmission_t *snapshot, int part,
                       const char *name, cv14Transform *out);
cod1ActorDraw_t *R_Cod1ActorSkinLOD(
        const cod1ActorModel_t *resource,
        const cod1ActorSubmission_t *snapshot, cnsArena_t *arena,
        const int selected[COD1_ACTOR_PARTS]);

#ifdef COD1_ACTOR_CAPTURE_IMPLEMENTATION

static int R_Cod1ActorBone(const cv14Skeleton *sk, const char *name) {
    int i;
    if (!sk || !name || sk->n < 1 || sk->n > CV14_BONES) return -1;
    for (i = 0; i < sk->n; i++)
        if (!strcmp(sk->name[i], name)) return i;
    return -1;
}

static int R_Cod1ActorValidSkeleton(const cv14Skeleton *sk) {
    int i, j;
    if (!sk || sk->n < 1 || sk->n > CV14_BONES) return 0;
    for (i = 0; i < sk->n; i++) {
        if (!memchr(sk->name[i], 0, XM14_NAME)) return 0;
        for (j = 0; j < 3; j++)
            if (!(sk->world[i].p[j] > -1000000.f &&
                  sk->world[i].p[j] < 1000000.f)) return 0;
        for (j = 0; j < 4; j++)
            if (!(sk->world[i].q[j] >= -1.001f &&
                  sk->world[i].q[j] <= 1.001f)) return 0;
    }
    return 1;
}

static int R_Cod1ActorAttach(cv14Skeleton *child, const cv14Pose *pose,
                             const cv14Skeleton *parent,
                             const char *anchor, const char *rootName) {
    int i, roots = 0, at = R_Cod1ActorBone(parent, anchor);
    int root = R_Cod1ActorBone(child, rootName);
    if (at < 0 || root < 0 || child->parent[root] != -1) return 0;
    for (i = 0; i < child->n; i++) roots += child->parent[i] == -1;
    if (roots != 1) return 0;
    CV14_ApplyAttachedPose(child, pose, &parent->world[at], rootName);
    return R_Cod1ActorValidSkeleton(child);
}

/* The selected character clip stores translation offsets from XModel bind
 * positions, including zero (omitted) offsets. Weapon absolute tracks keep
 * their existing path. Complete each component against its own skeleton. */
static void R_Cod1FoleyLocalPose(const cv14Skeleton *bind,
                                 const cv14Pose *offsets, cv14Pose *out) {
    int i, j, k;
    *out = *offsets;
    for (i = 0; i < out->n; i++) {
        j = R_Cod1ActorBone(bind, out->name[i]);
        if (j < 0) continue;
        for (k = 0; k < 3; k++) out->t[i].p[k] += bind->local[j].p[k];
    }
}

/* The caller supplies a decoded body pose; character XAnim root-motion
 * semantics are not guessed here. Both the head and body consume the same
 * named tracks. The gun follows the actual right-hand attachment. */
cod1ActorSubmission_t *R_Cod1ActorCapturePose(
        const cod1ActorModel_t *resource, cnsArena_t *arena,
        const cv14Pose *bodyPose) {
    cod1ActorSubmission_t *out;
    cv14Transform identity;
    cv14Pose empty;
    cv14Pose foleyPose[3];
    const cv14Pose *body = bodyPose, *head = bodyPose, *gear = bodyPose;
    size_t before;
    int i, j;
    if (!resource || !arena || !bodyPose || bodyPose->n < 0 ||
        bodyPose->n > CV14_BONES) return NULL;
    for (i = 0; i < bodyPose->n; i++) {
        if (!memchr(bodyPose->name[i], 0, XM14_NAME)) return NULL;
        for (j = 0; j < 3; j++)
            if (!(bodyPose->t[i].p[j] > -1000000.f &&
                  bodyPose->t[i].p[j] < 1000000.f)) return NULL;
        for (j = 0; j < 4; j++)
            if (!(bodyPose->t[i].q[j] >= -1.001f &&
                  bodyPose->t[i].q[j] <= 1.001f)) return NULL;
    }
    for (i = 0; i < COD1_ACTOR_PARTS; i++)
        if (resource->part[i].bind.n < 1 ||
            resource->part[i].bind.n > CV14_BONES) return NULL;
    before = arena->used;
    out = (cod1ActorSubmission_t *)CNS_Alloc(arena, sizeof(*out), 16);
    if (!out) return NULL;
    memset(out, 0, sizeof(*out));
    memset(&empty, 0, sizeof(empty));
    CV14_Identity(&identity);
    for (i = 0; i < COD1_ACTOR_PARTS; i++)
        out->part[i] = resource->part[i].bind;
    if (resource->profile != 0) {
        R_Cod1FoleyLocalPose(&resource->part[0].bind,bodyPose,&foleyPose[0]);
        R_Cod1FoleyLocalPose(&resource->part[1].bind,bodyPose,&foleyPose[1]);
        R_Cod1FoleyLocalPose(&resource->part[3].bind,bodyPose,&foleyPose[2]);
        body=&foleyPose[0];head=&foleyPose[1];gear=&foleyPose[2];
    }
    CV14_ApplyPose(&out->part[COD1_ACTOR_BODY], body, &identity);
    if (!R_Cod1ActorValidSkeleton(&out->part[COD1_ACTOR_BODY]) ||
        !R_Cod1ActorAttach(&out->part[COD1_ACTOR_HEAD], head,
            &out->part[COD1_ACTOR_BODY], "bip01 spine2", "bip01 spine2") ||
        (resource->profile!=0?
         (!R_Cod1ActorAttach(&out->part[COD1_ACTOR_HELMET], &empty,
            &out->part[COD1_ACTOR_BODY], "tag_helmet", "tag_helmet") ||
          !R_Cod1ActorAttach(&out->part[COD1_ACTOR_WEAPON], gear,
            &out->part[COD1_ACTOR_BODY], resource->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine", resource->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine")):
         (!R_Cod1ActorAttach(&out->part[COD1_ACTOR_HELMET], &empty,
            &out->part[COD1_ACTOR_HEAD], "bip01 head", "bip01 head") ||
          !R_Cod1ActorAttach(&out->part[COD1_ACTOR_WEAPON], &empty,
            &out->part[COD1_ACTOR_BODY], "tag_weapon_right", "tag_weapon")))) {
        arena->used = before;
        return NULL;
    }
    return out;
}

/* Part-qualified lookup prevents duplicate tag names on different models
 * from silently resolving to a different character instance or component. */
int R_Cod1ActorFindTag(const cod1ActorSubmission_t *snapshot, int part,
                       const char *name, cv14Transform *out) {
    int bone;
    if (!snapshot || !out || part < 0 || part >= COD1_ACTOR_PARTS ||
        !name || strncmp(name, "tag_", 4)) return -1;
    bone = R_Cod1ActorBone(&snapshot->part[part], name);
    if (bone < 0) return -1;
    *out = snapshot->part[part].world[bone];
    return bone;
}

static size_t R_Cod1ActorAlign(size_t n) {
    return (n + 15u) & ~(size_t)15u;
}

/* A single checked reservation owns the header, every surface, and all
 * skinned vertices. Index buffers and shader pointers remain immutable on
 * the model. On a bad source/pose, rollback happens before anything is drawn. */
cod1ActorDraw_t *R_Cod1ActorSkinLOD(
        const cod1ActorModel_t *resource,
        const cod1ActorSubmission_t *snapshot, cnsArena_t *arena,
        const int selected[COD1_ACTOR_PARTS]) {
    cod1ActorDraw_t *draw;
    unsigned char *cursor;
    size_t bytes, vertices, before;
    int part, i, j, k, count = 0, surface = 0;
    if (!resource || !snapshot || !arena || !selected) return NULL;
    bytes = R_Cod1ActorAlign(sizeof(*draw));
    vertices = 0;
    for (part = 0; part < COD1_ACTOR_PARTS; part++) {
        const cod1ActorLod_t *lod;
        if (selected[part] < 0 || selected[part] >= resource->part[part].numLods ||
            !R_Cod1ActorValidSkeleton(&snapshot->part[part])) return NULL;
        lod = &resource->part[part].lod[selected[part]];
        if (lod->numSurfaces < 1 || lod->numSurfaces > XM14_SURFS) return NULL;
        count += lod->numSurfaces;
        for (i = 0; i < lod->numSurfaces; i++) {
            const cod1ActorSurface_t *src = &lod->surface[i];
            if (!src->shader || !src->verts || !src->indexes ||
                src->numVerts < 1 || src->numVerts > CV14_VERTS ||
                src->numIndexes < 3 || src->numIndexes > CV14_TRIS * 3 ||
                vertices > ((size_t)-1) / sizeof(drawVert_t) - (size_t)src->numVerts)
                return NULL;
            vertices += (size_t)src->numVerts;
        }
    }
    if ((size_t)count > ((size_t)-1 - bytes) / sizeof(cod1ActorDrawSurface_t))
        return NULL;
    bytes = R_Cod1ActorAlign(bytes +
        (size_t)count * sizeof(cod1ActorDrawSurface_t));
    if (vertices > ((size_t)-1 - bytes) / sizeof(drawVert_t)) return NULL;
    bytes += vertices * sizeof(drawVert_t);
    before = arena->used;
    draw = (cod1ActorDraw_t *)CNS_Alloc(arena, bytes, 16);
    if (!draw) return NULL;
    memset(draw, 0, bytes);
    draw->numSurfaces = count;
    draw->surface = (cod1ActorDrawSurface_t *)((unsigned char *)draw +
        R_Cod1ActorAlign(sizeof(*draw)));
    cursor = (unsigned char *)draw + R_Cod1ActorAlign(
        R_Cod1ActorAlign(sizeof(*draw)) +
        (size_t)count * sizeof(cod1ActorDrawSurface_t));
    ClearBounds(draw->bounds[0], draw->bounds[1]);
    for (part = 0; part < COD1_ACTOR_PARTS; part++) {
        const cod1ActorLod_t *lod = &resource->part[part].lod[selected[part]];
        const cv14Skeleton *sk = &snapshot->part[part];
        for (i = 0; i < lod->numSurfaces; i++, surface++) {
            const cod1ActorSurface_t *src = &lod->surface[i];
            cod1ActorDrawSurface_t *dst = &draw->surface[surface];
            srfTriangles_t *tri = &dst->geometry;
            dst->source = src;
            tri->surfaceType = SF_TRIANGLES;
            tri->numVerts = src->numVerts;
            tri->numIndexes = src->numIndexes;
            tri->verts = (drawVert_t *)cursor;
            tri->indexes = (int *)src->indexes;
            cursor += (size_t)src->numVerts * sizeof(drawVert_t);
            ClearBounds(tri->bounds[0], tri->bounds[1]);
            for (j = 0; j < src->numVerts; j++) {
                drawVert_t *v = &tri->verts[j];
                if (!CN_SourceSkinVertex(&src->verts[j], sk, v->xyz, v->normal))
                    goto bad;
                VectorNormalize(v->normal);
                for (k = 0; k < 2; k++) v->st[k] = src->verts[j].uv[k];
                v->color[0] = v->color[1] = v->color[2] = v->color[3] = 255;
                AddPointToBounds(v->xyz, tri->bounds[0], tri->bounds[1]);
                AddPointToBounds(v->xyz, draw->bounds[0], draw->bounds[1]);
            }
            VectorAdd(tri->bounds[0], tri->bounds[1], tri->localOrigin);
            VectorScale(tri->localOrigin, 0.5f, tri->localOrigin);
            tri->radius = RadiusFromBounds(tri->bounds[0], tri->bounds[1]);
        }
    }
    VectorAdd(draw->bounds[0], draw->bounds[1], draw->localOrigin);
    VectorScale(draw->localOrigin, 0.5f, draw->localOrigin);
    draw->radius = RadiusFromBounds(draw->bounds[0], draw->bounds[1]);
    return draw;
bad:
    arena->used = before;
    return NULL;
}

#endif /* COD1_ACTOR_CAPTURE_IMPLEMENTATION */

#endif /* TR_COD1_ACTOR_CAPTURE_H */
