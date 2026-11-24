/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef V59_COLLISION_H
#define V59_COLLISION_H
#include <stddef.h>
#include <stdint.h>
#if defined(V59_EMBEDDED)
#define V59_API
#elif defined(_WIN32)
#define V59_API __declspec(dllexport)
#else
#define V59_API __attribute__((visibility("default")))
#endif
typedef struct v59_world v59_world;
typedef struct v59_context v59_context;
/* Map construction is serialized (the legacy facet builder has scratch globals).
 * The loaded world is immutable. Independent contexts can query it concurrently.
 * Contexts must be destroyed before the world. No query-time allocations. */
V59_API v59_world *v59_load(const void *bsp,size_t size,char *error,size_t error_size);
V59_API void v59_free(v59_world *);
V59_API v59_context *v59_context_create(const v59_world *);
V59_API void v59_context_free(v59_context *);
/* Positive vertical capsules only, in model-local coordinates; no point/box,
 * rotated model, or special engine handle 510 yet. Returns 0 or -1. */
V59_API int v59_query(v59_context *,const float start[3],const float end[3],
    const float mins[3],const float maxs[3],uint32_t mask,int model,float fraction);
/* Exact point-in-brush contents from the native leaf brush references (world)
 * or the native model brush range (inline). Returns 0, or -1 for bad input. */
V59_API int v59_point_contents(const v59_world *,const float point[3],int model,uint32_t *contents);
/* Diagnostic serialization: two 280-byte PC24 trace-work records. */
V59_API const void *v59_prepared(const v59_context *);
V59_API const void *v59_result(const v59_context *);
V59_API int v59_stationary(const v59_context *);
/* Event kinds: 0 leaf, 1 brush, 2 surface, 3 model_leaf. */
V59_API size_t v59_event_count(const v59_context *);
V59_API const int *v59_events(const v59_context *);
V59_API size_t v59_leaf_count(const v59_context *);
V59_API const int *v59_leaves(const v59_context *);
typedef struct {
    float fraction, end[3], normal[3];
    uint32_t surface_flags, contents;
    int startsolid, allsolid;
} v59_hit;
V59_API void v59_read_hit(const v59_context *, v59_hit *);
V59_API int v59_model_count(const v59_world *);
/* R9: immutable native brush export for the legacy point/box leaf adapter.
 * Capsule query storage/order/math is not changed. All accessors validate IDs. */
typedef struct { float mins[3], maxs[3]; uint32_t contents; int sides; } v59_brush_info;
typedef struct { float normal[3], distance; uint32_t surface_flags; } v59_brush_side;
V59_API int v59_model_brush_range(const v59_world *,int model,int *first,int *count,int *surfaces);
V59_API int v59_get_brush(const v59_world *,int index,v59_brush_info *);
V59_API int v59_get_brush_side(const v59_world *,int index,int side,v59_brush_side *);
typedef struct { float bounds[6];uint32_t flags,contents;int type,planes,facets; } v59_surface_info;
typedef struct { float plane[4];int signbits; } v59_curve_plane;
typedef struct { int surface,numBorders,border[26],inward[26],noAdjust[26]; } v59_curve_facet;
V59_API int v59_model_surface(const v59_world *,int model,int ordinal);
V59_API int v59_get_surface(const v59_world *,int index,v59_surface_info *);
V59_API int v59_get_curve_plane(const v59_world *,int surface,int plane,v59_curve_plane *);
V59_API int v59_get_curve_facet(const v59_world *,int surface,int facet,v59_curve_facet *);
#endif
