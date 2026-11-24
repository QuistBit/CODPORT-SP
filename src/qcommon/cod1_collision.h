#ifndef COD1_COLLISION_H
#define COD1_COLLISION_H

#include "cod1_bsp.h"

typedef struct cod1CollisionTri_s {
	vec3_t a;
	vec3_t b;
	vec3_t c;
	vec3_t normal;
	vec3_t mins;
	vec3_t maxs;
	int surfaceFlags;
	int contents;
	unsigned cod1SurfaceFlags;
	int cod1SurfaceValid;
} cod1CollisionTri_t;

/* Count and decode the world triangle soups without allocating memory. */
int COD1_BSP_CollisionTriangleCount( const cod1Bsp_t *bsp );
qboolean COD1_BSP_FillCollisionTriangles( const cod1Bsp_t *bsp,
										 cod1CollisionTri_t *triangles,
										 int maxTriangles, int *written,
										 vec3_t mins, vec3_t maxs );

/* CM-owned immutable world triangles; reset before their storage retires.
 * Failure only disables acceleration. Query semantics retain the full scan. */
int COD1_CollisionIndexBuild(const cod1CollisionTri_t *,int);
void COD1_CollisionIndexReset(void);

/* Conservative swept AABB trace used by the first RTCW CoD1 backend. */
void COD1_CollisionTrace( const cod1CollisionTri_t *triangles, int triangleCount,
							 const vec3_t start, const vec3_t end,
							 const vec3_t mins, const vec3_t maxs,
							 trace_t *out );

void COD1_CollisionTraceShape(const cod1CollisionTri_t*, int, const vec3_t, const vec3_t, const vec3_t, const vec3_t, trace_t*, int);

#endif /* COD1_COLLISION_H */
