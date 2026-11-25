#include "cod1_collision.h"
#include "../game/bg_cod1_impact.h"

#include <float.h>
#include <math.h>
#include <string.h>
#include "cod1_collision_blocks583.h"

static unsigned COD1_CollisionReadLE16( const byte *p ) {
	return (unsigned)p[0] | ( (unsigned)p[1] << 8 );
}

static unsigned COD1_CollisionReadLE32( const byte *p ) {
	return (unsigned)p[0] | ( (unsigned)p[1] << 8 ) |
			( (unsigned)p[2] << 16 ) | ( (unsigned)p[3] << 24 );
}

static float COD1_CollisionFloat( const byte *p ) {
	unsigned bits = COD1_CollisionReadLE32( p );
	float value;
	memcpy( &value, &bits, sizeof( value ) );
	return value;
}

#include "cod1_ladders.h"

static void COD1_CollisionCross( const vec3_t a, const vec3_t b, vec3_t out ) {
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

static float COD1_CollisionDot( const vec3_t a, const vec3_t b ) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static qboolean COD1_CollisionDecodeSoupTriangle( const cod1Bsp_t *bsp,
											 unsigned soupIndex, unsigned triangle,
											 cod1CollisionTri_t *out ) {
	const byte *soups, *verts, *meshverts;
	int soupLength, vertLength, meshLength;
	unsigned soupOffset, firstVert, vertCount, indexCount, firstIndex;
	unsigned local[3];
	int k, j;
	vec3_t edge1, edge2;
	float normalLength;

	soups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS, &soupLength );
	verts = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_DRAWVERTS, &vertLength );
	meshverts = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_MESHVERTS, &meshLength );
	if ( !soups || !verts || !meshverts || soupIndex >= (unsigned)( soupLength / COD1_BSP_TRIANGLE_SOUP_STRIDE ) ) {
		return qfalse;
	}
	soupOffset = soupIndex * COD1_BSP_TRIANGLE_SOUP_STRIDE;
    /* PATCH25: visual geometry is not automatically a solid obstacle.
     * Keep solid/player-clip surfaces; ignore visual-only, trigger and water
     * surfaces in this provisional player collision backend. Native brush
     * collision and per-query content masks are still separate work. */
    {
        const byte *materials;
        int materialLength;
        unsigned materialIndex, contents;
        materials = COD1_BSP_LumpData(bsp, COD1_BSP_LUMP_MATERIALS, &materialLength);
        materialIndex = COD1_CollisionReadLE16(soups + soupOffset);
        if (!materials || materialIndex >= (unsigned)(materialLength / COD1_BSP_MATERIAL_STRIDE))
            return qfalse;
        contents = COD1_CollisionReadLE32(materials + materialIndex * COD1_BSP_MATERIAL_STRIDE + 68);
        if (!(contents & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP))) return qfalse;
        out->cod1SurfaceFlags = COD1_CollisionReadLE32(materials + materialIndex * COD1_BSP_MATERIAL_STRIDE + 64);
        out->cod1SurfaceValid = 1;
    }

	firstVert = COD1_CollisionReadLE32( soups + soupOffset + 4 );
	vertCount = COD1_CollisionReadLE16( soups + soupOffset + 8 );
	indexCount = COD1_CollisionReadLE16( soups + soupOffset + 10 );
	firstIndex = COD1_CollisionReadLE32( soups + soupOffset + 12 );
	if ( triangle >= indexCount / 3 ||
		 firstVert > (unsigned)( vertLength / COD1_BSP_DRAWVERT_STRIDE ) ||
		 vertCount > (unsigned)( vertLength / COD1_BSP_DRAWVERT_STRIDE ) - firstVert ||
		 firstIndex > (unsigned)( meshLength / COD1_BSP_MESHVERT_STRIDE ) ||
		 indexCount > (unsigned)( meshLength / COD1_BSP_MESHVERT_STRIDE ) - firstIndex ) {
		return qfalse;
	}
	for ( k = 0; k < 3; k++ ) {
		local[k] = COD1_CollisionReadLE16( meshverts + ( firstIndex + triangle * 3 + k ) * 2 );
		if ( local[k] >= vertCount ) {
			return qfalse;
		}
		for ( j = 0; j < 3; j++ ) {
			float value = COD1_CollisionFloat( verts + ( firstVert + local[k] ) * COD1_BSP_DRAWVERT_STRIDE + j * 4 );
			if ( k == 0 ) out->a[j] = value;
			if ( k == 1 ) out->b[j] = value;
			if ( k == 2 ) out->c[j] = value;
		}
	}
	VectorSubtract( out->b, out->a, edge1 );
	VectorSubtract( out->c, out->a, edge2 );
	COD1_CollisionCross( edge1, edge2, out->normal );
	normalLength = (float)sqrt( COD1_CollisionDot( out->normal, out->normal ) );
	if ( normalLength < 0.00001f ) {
		return qfalse;
	}
	VectorScale( out->normal, 1.0f / normalLength, out->normal );
	for ( j = 0; j < 3; j++ ) {
		out->mins[j] = out->a[j];
		out->maxs[j] = out->a[j];
		if ( out->b[j] < out->mins[j] ) out->mins[j] = out->b[j];
		if ( out->c[j] < out->mins[j] ) out->mins[j] = out->c[j];
		if ( out->b[j] > out->maxs[j] ) out->maxs[j] = out->b[j];
		if ( out->c[j] > out->maxs[j] ) out->maxs[j] = out->c[j];
	}
	out->surfaceFlags = 0;
	out->contents = CONTENTS_SOLID;
	return qtrue;
}

int COD1_BSP_CollisionTriangleCount( const cod1Bsp_t *bsp ) {
	cod1BspWorldInfo_t info;
	const byte *soups;
	int soupLength;
	int i, total = 0;
	if ( !bsp || !COD1_BSP_GetWorldInfo( bsp, &info ) ) return 0;
	soups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS, &soupLength );
	if ( !soups ) return 0;
	for ( i = 0; i < info.worldSoupCount; i++ ) {
		unsigned count = COD1_CollisionReadLE16( soups + ( info.worldFirstSoup + i ) * 16 + 10 ) / 3;
		if ( count > 0x7fffffffU - (unsigned)total ) return 0;
		total += (int)count;
	}
	{ int ladders=COD1_LadderTriangles(bsp,NULL,0);
	  if(ladders<0 || total>0x7fffffff-ladders)return 0;
	  return total+ladders; }
}

qboolean COD1_BSP_FillCollisionTriangles( const cod1Bsp_t *bsp,
										 cod1CollisionTri_t *triangles,
										 int maxTriangles, int *written,
										 vec3_t mins, vec3_t maxs ) {
	cod1BspWorldInfo_t info;
	int i, total = 0;
	if ( written ) *written = 0;
	if ( mins ) VectorSet( mins, FLT_MAX, FLT_MAX, FLT_MAX );
	if ( maxs ) VectorSet( maxs, -FLT_MAX, -FLT_MAX, -FLT_MAX );
	if ( !bsp || !triangles || maxTriangles <= 0 || !COD1_BSP_GetWorldInfo( bsp, &info ) ) return qfalse;
	for ( i = 0; i < info.worldSoupCount; i++ ) {
		const byte *soups;
		int soupLength;
		unsigned soupIndex = info.worldFirstSoup + i;
		unsigned indexCount;
		int k;
		soups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS, &soupLength );
		if ( !soups || soupIndex >= (unsigned)( soupLength / 16 ) ) return qfalse;
		indexCount = COD1_CollisionReadLE16( soups + soupIndex * 16 + 10 );
		for ( k = 0; k < (int)( indexCount / 3 ); k++ ) {
			if ( total >= maxTriangles || !COD1_CollisionDecodeSoupTriangle( bsp, soupIndex, (unsigned)k, &triangles[total] ) ) continue;
			if ( mins && maxs ) {
				int j;
				for ( j = 0; j < 3; j++ ) {
					if ( triangles[total].mins[j] < mins[j] ) mins[j] = triangles[total].mins[j];
					if ( triangles[total].maxs[j] > maxs[j] ) maxs[j] = triangles[total].maxs[j];
				}
			}
			total++;
		}
	}
    {
        int added=COD1_LadderTriangles(bsp,triangles+total,maxTriangles-total);
        int k,j;
        if(added<0)return qfalse;
        if(mins && maxs)for(k=total;k<total+added;k++)for(j=0;j<3;j++){
            if(triangles[k].mins[j]<mins[j])mins[j]=triangles[k].mins[j];
            if(triangles[k].maxs[j]>maxs[j])maxs[j]=triangles[k].maxs[j];
        }
        total+=added;
    }
	if ( written ) *written = total;
	return total > 0;
}

/* Continuous separating-axis test: three box axes, the triangle normal,
   and nine edge/box-axis cross products. Unlike a projected center test,
   this also detects contacts at triangle edges and box corners. */
typedef struct {
	float enter, leave;
	qboolean inside;
	vec3_t normal;
	float dist;
} cod1Sweep_t;

static qboolean COD1_SweepAxis( const cod1CollisionTri_t *tri,
		const vec3_t axis, const vec3_t start, const vec3_t delta,
		const vec3_t extent, cod1Sweep_t *sweep ) {
	vec3_t n;
	float length, lo, hi, p, radius, velocity, t0, t1, sign;
	int j;
	length = (float)sqrt( DotProduct( axis, axis ) );
	if ( length < 0.000001f ) return qtrue;
	VectorScale( axis, 1.0f / length, n );
	lo = hi = DotProduct( tri->a, n );
	p = DotProduct( tri->b, n );
	if ( p < lo ) lo = p;
	if ( p > hi ) hi = p;
	p = DotProduct( tri->c, n );
	if ( p < lo ) lo = p;
	if ( p > hi ) hi = p;
	radius = 0;
	for ( j = 0; j < 3; j++ ) radius += (float)fabs( n[j] ) * extent[j];
	lo -= radius;
	hi += radius;
	p = DotProduct( start, n );
	velocity = DotProduct( delta, n );
	if ( p <= lo || p >= hi ) sweep->inside = qfalse;
	if ( fabs( velocity ) < 0.0000001f ) {
		/* Touching while moving along a face is not penetration. */
		return p > lo && p < hi;
	}
	if ( velocity > 0 ) {
		t0 = ( lo - p ) / velocity;
		t1 = ( hi - p ) / velocity;
		sign = -1.0f;
	} else {
		t0 = ( hi - p ) / velocity;
		t1 = ( lo - p ) / velocity;
		sign = 1.0f;
	}
	if ( t0 > sweep->enter ) {
		sweep->enter = t0;
		VectorScale( n, sign, sweep->normal );
		sweep->dist = sign > 0 ? hi - radius : -( lo + radius );
	}
	if ( t1 < sweep->leave ) sweep->leave = t1;
	return sweep->enter <= sweep->leave;
}

void COD1_CollisionTrace( const cod1CollisionTri_t *triangles, int triangleCount,
		const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs, trace_t *out ) {
	vec3_t center, delta, extent, sweepMins, sweepMaxs;
    const c1cb583Bounds *blocks;
	int i, j, k, edge;
	if ( !out ) return;
	memset( out, 0, sizeof( *out ) );
	out->fraction = 1.0f;
	VectorCopy( end, out->endpos );
	for ( j = 0; j < 3; j++ ) {
		center[j] = start[j] + ( mins[j] + maxs[j] ) * 0.5f;
		extent[j] = ( maxs[j] - mins[j] ) * 0.5f;
		delta[j] = end[j] - start[j];
		sweepMins[j] = center[j] + ( delta[j] < 0 ? delta[j] : 0 ) - extent[j];
		sweepMaxs[j] = center[j] + ( delta[j] > 0 ? delta[j] : 0 ) + extent[j];
	}
    blocks=C1CB583_Select(triangles,triangleCount,sweepMins,sweepMaxs);
	for ( i = 0; triangles && i < triangleCount; i++ ) {
        if(blocks && !(i%C1CB583_SIZE)){
            int skip=C1CB583_Skip(blocks,i,triangleCount,sweepMins,sweepMaxs);
            if(skip){i+=skip-1;continue;}
        }
		const cod1CollisionTri_t *tri = &triangles[i];
		cod1Sweep_t sweep;
		vec3_t axis, edges[3];
		qboolean separated = qfalse;
		float fraction, speed;
		for ( j = 0; j < 3; j++ )
			if ( sweepMaxs[j] < tri->mins[j] || sweepMins[j] > tri->maxs[j] ) break;
		if ( j != 3 ) continue;
		memset( &sweep, 0, sizeof( sweep ) );
		sweep.enter = -FLT_MAX;
		sweep.leave = FLT_MAX;
		sweep.inside = qtrue;
		if ( !COD1_SweepAxis( tri, tri->normal, center, delta, extent, &sweep ) ) continue;
		for ( j = 0; j < 3; j++ ) {
			VectorClear( axis );
			axis[j] = 1;
			if ( !COD1_SweepAxis( tri, axis, center, delta, extent, &sweep ) ) separated = qtrue;
		}
		if ( separated ) continue;
		VectorSubtract( tri->b, tri->a, edges[0] );
		VectorSubtract( tri->c, tri->b, edges[1] );
		VectorSubtract( tri->a, tri->c, edges[2] );
		for ( edge = 0; edge < 3 && !separated; edge++ ) {
			for ( j = 0; j < 3; j++ ) {
				vec3_t boxAxis;
				VectorClear( boxAxis );
				boxAxis[j] = 1;
				COD1_CollisionCross( edges[edge], boxAxis, axis );
				if ( !COD1_SweepAxis( tri, axis, center, delta, extent, &sweep ) ) {
					separated = qtrue;
					break;
				}
			}
		}
		if ( separated || sweep.leave < 0 || sweep.enter > 1 ) continue;
		if ( sweep.inside ) {
			out->startsolid = qtrue;
			if ( sweep.leave >= 1 ) {
				out->allsolid = qtrue;
				out->fraction = 0;
				out->contents = tri->contents;
			}
			/* RTCW permits escaping an initially overlapping obstacle. */
			continue;
		}
		if ( sweep.enter < 0 || sweep.leave <= 0 ) continue;
		speed = (float)fabs( DotProduct( delta, sweep.normal ) );
		/* Match the engine's 1/8-unit clearance to avoid rounding into walls. */
		fraction = sweep.enter - ( speed > 0 ? 0.125f / speed : 0 );
		if ( fraction < 0 ) fraction = 0;
		if ( fraction >= out->fraction ) continue;
		out->fraction = fraction;
		VectorCopy( sweep.normal, out->plane.normal );
		out->plane.dist = sweep.dist;
		out->plane.type = PlaneTypeForNormal( out->plane.normal );
		out->plane.signbits = 0;
		for ( k = 0; k < 3; k++ )
			if ( out->plane.normal[k] < 0 ) out->plane.signbits |= 1 << k;
		out->surfaceFlags = tri->surfaceFlags;
        out->plane.pad[0]=out->plane.pad[1]=0;
        if(tri->cod1SurfaceValid)CI_TraceSurface(out,tri->cod1SurfaceFlags);
		out->contents = tri->contents;
	}
	for ( j = 0; j < 3; j++ )
		out->endpos[j] = start[j] + out->fraction * delta[j];
}

#include "cod1_capsule.h"
