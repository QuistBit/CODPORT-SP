/*
===========================================================================

CoD1 BSP probe used by the RTCW port.

===========================================================================
*/

#include "cod1_bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

static int COD1_StrideCount( const cod1Bsp_t *bsp, int lumpId, int stride );

static int COD1_ReadLE32( const byte *p ) {
	return (int)( (unsigned int)p[0] |
				  ( (unsigned int)p[1] << 8 ) |
				  ( (unsigned int)p[2] << 16 ) |
				  ( (unsigned int)p[3] << 24 ) );
}

static float COD1_ReadLEFloat( const byte *p ) {
	unsigned int bits = (unsigned int)COD1_ReadLE32( p );
	float value;
	memcpy( &value, &bits, sizeof( value ) );
	return value;
}

void COD1_BSP_SpatialRecord( const byte *records, int index, int fields[9] ) {
	int i;
	const byte *record = records + index * COD1_BSP_NODE_STRIDE;
	for ( i = 0; i < 9; i++ ) {
		fields[i] = COD1_ReadLE32( record + 4 * i );
	}
}

void COD1_BSP_SpatialPlane( const cod1BspSpatial_t *spatial, int index,
								vec3_t normal, float *dist ) {
	int i;
	const byte *plane = spatial->planes + index * COD1_BSP_PLANE_STRIDE;
	for ( i = 0; i < 3; i++ ) {
		normal[i] = COD1_ReadLEFloat( plane + 4 * i );
	}
	*dist = COD1_ReadLEFloat( plane + 12 );
}

/* Limit the parent recursion used by the renderer and reject cycles, even
 * when an invalid node is not reachable from the root. */
static qboolean COD1_CheckSpatialTree( const cod1BspSpatial_t *spatial,
								int index, byte *color, int depth ) {
	int fields[9], i;
	if ( depth > 1024 || color[index] == 1 ) {
		return qfalse;
	}
	if ( color[index] == 2 ) {
		return qtrue;
	}
	color[index] = 1;
	COD1_BSP_SpatialRecord( spatial->nodes, index, fields );
	for ( i = 1; i <= 2; i++ ) {
		if ( fields[i] >= 0 && !COD1_CheckSpatialTree( spatial,
				fields[i], color, depth + 1 ) ) {
			return qfalse;
		}
	}
	color[index] = 2;
	return qtrue;
}

qboolean COD1_BSP_GetSpatial( const cod1Bsp_t *bsp, cod1BspSpatial_t *out ) {
	int length, i, j, fields[9], highestCluster = -1;
	int *parents = NULL;
	byte *color = NULL;
	const byte *vis;
	unsigned int clusters, rowBytes;
	qboolean valid = qfalse;
	if ( !bsp || !out ) {
		return qfalse;
	}
	memset( out, 0, sizeof( *out ) );
	out->planeCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_PLANES, COD1_BSP_PLANE_STRIDE );
	out->nodeCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_NODES, COD1_BSP_NODE_STRIDE );
	out->leafCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_LEAVES, COD1_BSP_LEAF_STRIDE );
	if ( out->planeCount <= 0 || out->nodeCount <= 0 || out->leafCount <= 0 ||
		 out->nodeCount > INT_MAX - out->leafCount ) {
		goto fail;
	}
	out->planes = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_PLANES, NULL );
	out->nodes = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_NODES, NULL );
	out->leaves = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_LEAVES, NULL );
	for ( i = 0; i < out->planeCount; i++ ) {
		const byte *plane = out->planes + i * COD1_BSP_PLANE_STRIDE;
		for ( j = 0; j < 4; j++ ) {
			if ( ( (unsigned int)COD1_ReadLE32( plane + j * 4 ) & 0x7f800000u ) == 0x7f800000u ) {
				goto fail;
			}
		}
	}
	for ( i = 0; i < out->leafCount; i++ ) {
		COD1_BSP_SpatialRecord( out->leaves, i, fields );
		if ( fields[0] < -1 || fields[1] < -1 || fields[1] >= MAX_MAP_AREA_BYTES * 8 ) {
			goto fail;
		}
		if ( fields[0] > highestCluster ) {
			highestCluster = fields[0];
		}
	}
	vis = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_VISIBILITY, &length );
	if ( vis ) {
		if ( length < 8 ) {
			goto fail;
		}
		clusters = (unsigned int)COD1_ReadLE32( vis );
		rowBytes = (unsigned int)COD1_ReadLE32( vis + 4 );
		if ( !clusters || clusters > INT_MAX || !rowBytes || rowBytes > INT_MAX ||
			rowBytes < ( clusters + 7u ) / 8u ||
			clusters > (unsigned int)( length - 8 ) / rowBytes ||
			(unsigned int)( length - 8 ) != clusters * rowBytes ||
			highestCluster >= (int)clusters ) {
			goto fail;
		}
		out->clusterCount = (int)clusters;
		out->clusterBytes = (int)rowBytes;
		out->visibility = vis + 8;
	} else {
		out->clusterCount = highestCluster + 1;
		if ( !out->clusterCount ) out->clusterCount = 1;
		out->clusterBytes = ( ( out->clusterCount - 1 ) / 8 ) + 1;
	}
	parents = calloc( (size_t)out->nodeCount, sizeof( *parents ) );
	color = calloc( (size_t)out->nodeCount, sizeof( *color ) );
	if ( !parents || !color ) {
		goto fail;
	}
	for ( i = 0; i < out->nodeCount; i++ ) {
		COD1_BSP_SpatialRecord( out->nodes, i, fields );
		if ( fields[0] < 0 || fields[0] >= out->planeCount ) {
			goto fail;
		}
		for ( j = 0; j < 3; j++ ) {
			if ( fields[3 + j] > fields[6 + j] ) {
				goto fail;
			}
		}
		for ( j = 1; j <= 2; j++ ) {
			if ( fields[j] == INT_MIN || fields[j] >= out->nodeCount ||
				( fields[j] < 0 && -1 - fields[j] >= out->leafCount ) ) {
				goto fail;
			}
			if ( fields[j] >= 0 && ++parents[fields[j]] > 1 ) {
				goto fail;
			}
		}
	}
	for ( i = 0; i < out->nodeCount; i++ ) {
		if ( !COD1_CheckSpatialTree( out, i, color, 1 ) ) {
			goto fail;
		}
	}
	valid = qtrue;
fail:
	free( parents );
	free( color );
	if ( !valid ) {
		memset( out, 0, sizeof( *out ) );
	}
	return valid;
}

static void COD1_VisitBoxNode( const cod1BspSpatial_t *spatial, int index,
								const vec3_t mins, const vec3_t maxs,
								void ( *visit )( void *, int ), void *context ) {
	int fields[9], i;
	vec3_t normal;
	float dist, lo = 0.0f, hi = 0.0f;
	if ( index < 0 ) {
		visit( context, -1 - index );
		return;
	}
	COD1_BSP_SpatialRecord( spatial->nodes, index, fields );
	COD1_BSP_SpatialPlane( spatial, fields[0], normal, &dist );
	for ( i = 0; i < 3; i++ ) {
		if ( normal[i] >= 0.0f ) {
			lo += normal[i] * mins[i];
			hi += normal[i] * maxs[i];
		} else {
			lo += normal[i] * maxs[i];
			hi += normal[i] * mins[i];
		}
	}
	/* On-plane surfaces belong to both leaves. The margin accounts for
	 * roundoff on the author's integer BSP node bounds. */
	if ( hi >= dist - 1.0f ) {
		COD1_VisitBoxNode( spatial, fields[1], mins, maxs, visit, context );
	}
	if ( lo <= dist + 1.0f ) {
		COD1_VisitBoxNode( spatial, fields[2], mins, maxs, visit, context );
	}
}

void COD1_BSP_VisitBoxLeaves( const cod1BspSpatial_t *spatial,
								const vec3_t mins, const vec3_t maxs,
								void ( *visit )( void *, int ), void *context ) {
	if ( spatial && spatial->nodeCount && mins && maxs && visit ) {
		COD1_VisitBoxNode( spatial, 0, mins, maxs, visit, context );
	}
}

static int COD1_StrideCount( const cod1Bsp_t *bsp, int lumpId, int stride ) {
	int length;

	if ( !bsp || lumpId < 0 || lumpId >= COD1_BSP_LUMP_COUNT || stride <= 0 ) {
		return -1;
	}
	length = bsp->lumps[lumpId].filelen;
	if ( length < 0 || length % stride ) {
		return -1;
	}
	return length / stride;
}

static qboolean COD1_ContainsBytes( const byte *haystack, int haystackLength,
									const char *needle, int needleLength ) {
	int i;

	if ( !haystack || !needle || haystackLength < needleLength || needleLength <= 0 ) {
		return qfalse;
	}
	for ( i = 0; i <= haystackLength - needleLength; i++ ) {
		if ( !memcmp( haystack + i, needle, needleLength ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

qboolean COD1_BSP_Parse( const void *data, int length, cod1Bsp_t *out ) {
	const byte *bytes;
	int i;

	if ( !data || !out || length < COD1_BSP_HEADER_SIZE ) {
		return qfalse;
	}

	bytes = (const byte *)data;
	if ( memcmp( bytes, "IBSP", 4 ) != 0 ||
		 COD1_ReadLE32( bytes + 4 ) != COD1_BSP_VERSION ) {
		return qfalse;
	}

	memset( out, 0, sizeof( *out ) );
	out->base = bytes;
	out->length = length;

	for ( i = 0; i < COD1_BSP_LUMP_COUNT; i++ ) {
		int filelen = COD1_ReadLE32( bytes + 8 + i * 8 );
		int fileofs = COD1_ReadLE32( bytes + 12 + i * 8 );

		/* Signed overflow and header overlap are rejected explicitly. */
		if ( filelen < 0 || fileofs < COD1_BSP_HEADER_SIZE ||
			 fileofs > length || filelen > length - fileofs ) {
			if ( filelen == 0 && fileofs == 0 ) {
				out->lumps[i].fileofs = 0;
				out->lumps[i].filelen = 0;
				continue;
			}
			memset( out, 0, sizeof( *out ) );
			return qfalse;
		}

		out->lumps[i].fileofs = fileofs;
		out->lumps[i].filelen = filelen;
	}

	return qtrue;
}

const byte *COD1_BSP_LumpData( const cod1Bsp_t *bsp, int lumpId, int *length ) {
	if ( length ) {
		*length = 0;
	}
	if ( !bsp || !bsp->base || lumpId < 0 || lumpId >= COD1_BSP_LUMP_COUNT ) {
		return NULL;
	}
	if ( length ) {
		*length = bsp->lumps[lumpId].filelen;
	}
	if ( !bsp->lumps[lumpId].filelen ) {
		return NULL;
	}
	return bsp->base + bsp->lumps[lumpId].fileofs;
}

qboolean COD1_BSP_FindEntities( const cod1Bsp_t *bsp, const byte **data, int *length ) {
	int i;

	if ( data ) {
		*data = NULL;
	}
	if ( length ) {
		*length = 0;
	}
	if ( !bsp || !data || !length ) {
		return qfalse;
	}

	/* Retail files use lump 29; keep a marker-based fallback for tools that
	   preserve the container but reorder optional lumps. */
	for ( i = -1; i < COD1_BSP_LUMP_COUNT; i++ ) {
		const byte *candidate;
		int candidateLength;
		int textLength;
		int lumpId;
		const byte *terminator;

		lumpId = ( i < 0 ) ? COD1_BSP_LUMP_ENTITIES : i;
		if ( i >= 0 && lumpId == COD1_BSP_LUMP_ENTITIES ) {
			continue;
		}
		candidate = COD1_BSP_LumpData( bsp, lumpId, &candidateLength );
		if ( !candidate || candidateLength < 20 ) {
			continue;
		}
		textLength = candidateLength;
		terminator = (const byte *)memchr( candidate, '\0', candidateLength );
		if ( terminator ) {
			textLength = (int)( terminator - candidate );
		}
		if ( textLength >= 2 && memchr( candidate, '{', textLength ) &&
			 COD1_ContainsBytes( candidate, textLength, "worldspawn", 10 ) &&
			 COD1_ContainsBytes( candidate, textLength, "classname", 9 ) ) {
			*data = candidate;
			*length = textLength;
			return qtrue;
		}
	}

	return qfalse;
}

qboolean COD1_BSP_GetWorldInfo( const cod1Bsp_t *bsp, cod1BspWorldInfo_t *out ) {
	const byte *models;
	const byte *entities;
	int modelLength;
	int entitiesLength;
	int count;

	if ( !bsp || !out ) {
		return qfalse;
	}
	memset( out, 0, sizeof( *out ) );

	out->materialCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_MATERIALS,
										 COD1_BSP_MATERIAL_STRIDE );
	out->soupCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS,
										COD1_BSP_TRIANGLE_SOUP_STRIDE );
	out->drawVertCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_DRAWVERTS,
										 COD1_BSP_DRAWVERT_STRIDE );
	out->meshVertCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_MESHVERTS,
										 COD1_BSP_MESHVERT_STRIDE );
		count = COD1_StrideCount( bsp, COD1_BSP_LUMP_LIGHTMAPS, 1 );
	if ( out->materialCount < 0 || out->soupCount < 0 || out->drawVertCount < 0 ||
		 out->meshVertCount < 0 || count < 0 || count % COD1_BSP_LIGHTMAP_TILE_BYTES ) {
		return qfalse;
	}
	out->lightmapTiles = count / COD1_BSP_LIGHTMAP_TILE_BYTES;

	models = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_MODELS, &modelLength );
	if ( !models || modelLength < COD1_BSP_MODEL_STRIDE ||
		 modelLength % COD1_BSP_MODEL_STRIDE ) {
		return qfalse;
	}

	out->modelCount = modelLength / COD1_BSP_MODEL_STRIDE;

	/* The world model stores firstSoup and soupCount at byte offsets 24/28. */
	out->worldFirstSoup = COD1_ReadLE32( models + 24 );
	out->worldSoupCount = COD1_ReadLE32( models + 28 );
	if ( out->worldFirstSoup < 0 || out->worldSoupCount < 0 ||
		 out->worldFirstSoup > out->soupCount ||
		 out->worldSoupCount > out->soupCount - out->worldFirstSoup ) {
		return qfalse;
	}

	if ( COD1_BSP_FindEntities( bsp, &entities, &entitiesLength ) ) {
		out->entities = entities;
		out->entitiesLength = entitiesLength;
	}

	return qtrue;
}

static qboolean COD1_RenderRange( int first, int count, int limit ) {
	return first >= 0 && count >= 0 && first <= limit && count <= limit - first;
}

void COD1_BSP_RenderCell( const cod1BspRenderCells_t *render, int index,
		int *treeRoot, int *firstGroup, int *groupCount ) {
	const byte *p = render->cells + index * COD1_BSP_CELL_STRIDE;
	*treeRoot = COD1_ReadLE32( p + 24 );
	*firstGroup = COD1_ReadLE32( p + 36 );
	*groupCount = COD1_ReadLE32( p + 40 );
}

void COD1_BSP_RenderGroup( const cod1BspRenderCells_t *render, int index,
		int *firstSoup, int *soupCount ) {
	const byte *p = render->groups + index * COD1_BSP_CULL_GROUP_STRIDE;
	*firstSoup = COD1_ReadLE32( p + 24 );
	*soupCount = COD1_ReadLE32( p + 28 );
}

int COD1_BSP_RenderGroupIndex( const cod1BspRenderCells_t *render, int index ) {
	return COD1_ReadLE32( render->groupIndices + index * 4 );
}

void COD1_BSP_RenderTree( const cod1BspRenderCells_t *render, int index,
		int *firstSoup, int *soupCount ) {
	const byte *p = render->trees + index * COD1_BSP_AABB_TREE_STRIDE;
	*firstSoup = COD1_ReadLE32( p );
	*soupCount = COD1_ReadLE32( p + 4 );
}

/* Siblings are stored together, followed by their descendants in order.
 * Validate that every child range partitions its parent's soup range, and
 * that every tree record belongs to exactly one cell root. */
static qboolean COD1_CheckRenderTree( const cod1BspRenderCells_t *render,
		int index, int *cursor, int end, int depth ) {
	int first, count, childCount, childStart, child, next, childFirst, childCountSoups;
	if ( depth > 1024 ) return qfalse;
	COD1_BSP_RenderTree( render, index, &first, &count );
	childCount = COD1_ReadLE32( render->trees + index * COD1_BSP_AABB_TREE_STRIDE + 8 );
	if ( childCount < 0 || childCount > end - *cursor ) return qfalse;
	childStart = *cursor;
	*cursor += childCount;
	next = first;
	for ( child = childStart; child < childStart + childCount; child++ ) {
		COD1_BSP_RenderTree( render, child, &childFirst, &childCountSoups );
		if ( childFirst != next || childCountSoups > first + count - next ) return qfalse;
		next += childCountSoups;
	}
	if ( childCount && next != first + count ) return qfalse;
	for ( child = childStart; child < childStart + childCount; child++ ) {
		if ( !COD1_CheckRenderTree( render, child, cursor, end, depth + 1 ) ) return qfalse;
	}
	return qtrue;
}

qboolean COD1_BSP_GetRenderCells( const cod1Bsp_t *bsp,
		const cod1BspWorldInfo_t *world, const cod1BspSpatial_t *spatial,
		cod1BspRenderCells_t *out ) {
	const byte *portals, *leaf;
	int i, j, first, count, root, firstGroup, groupCount, prevRoot = -1;
	int portalCount, vertexCount, worldEnd;
	if ( !bsp || !world || !spatial || !out ) return qfalse;
	memset( out, 0, sizeof( *out ) );
	out->groupCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_CULL_GROUPS, COD1_BSP_CULL_GROUP_STRIDE );
	out->groupIndexCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_CULL_INDICES, 4 );
	out->treeCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_AABB_TREES, COD1_BSP_AABB_TREE_STRIDE );
	out->cellCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_CELLS, COD1_BSP_CELL_STRIDE );
	portalCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_PORTALS, COD1_BSP_PORTAL_STRIDE );
	vertexCount = COD1_StrideCount( bsp, COD1_BSP_LUMP_PORTAL_VERTS, 12 );
	if ( out->groupCount < 0 || out->groupIndexCount < 0 || out->treeCount < 0 ||
		out->cellCount < 1 || portalCount < 0 || vertexCount < 0 ||
		world->worldFirstSoup < 0 || world->worldSoupCount < 0 ||
		world->worldFirstSoup > world->soupCount ||
		world->worldSoupCount > world->soupCount - world->worldFirstSoup ) goto fail;
	worldEnd = world->worldFirstSoup + world->worldSoupCount;
	out->groups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_CULL_GROUPS, NULL );
	out->groupIndices = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_CULL_INDICES, NULL );
	out->trees = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_AABB_TREES, NULL );
	out->cells = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_CELLS, NULL );
	portals = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_PORTALS, NULL );
	if ( !out->cells || ( out->groupCount && !out->groups ) ||
		( out->groupIndexCount && !out->groupIndices ) ||
		( out->treeCount && !out->trees ) || ( portalCount && !portals ) ) goto fail;
	for ( i = 0; i < out->groupCount; i++ ) {
		const byte *p = out->groups + i * COD1_BSP_CULL_GROUP_STRIDE;
		COD1_BSP_RenderGroup( out, i, &first, &count );
		if ( !COD1_RenderRange( first, count, worldEnd ) ||
			( count && first < world->worldFirstSoup ) ) goto fail;
		if ( count ) for ( j = 0; j < 3; j++ ) {
			float lo = COD1_ReadLEFloat( p + j * 4 );
			float hi = COD1_ReadLEFloat( p + ( j + 3 ) * 4 );
			if ( !isfinite( lo ) || !isfinite( hi ) || lo > hi ) goto fail;
		}
	}
	for ( i = 0; i < out->groupIndexCount; i++ ) {
		if ( !COD1_RenderRange( COD1_BSP_RenderGroupIndex( out, i ), 1,
			out->groupCount ) ) goto fail;
	}
	for ( i = 0; i < out->treeCount; i++ ) {
		COD1_BSP_RenderTree( out, i, &first, &count );
		if ( !COD1_RenderRange( first, count, worldEnd ) ||
			( count && first < world->worldFirstSoup ) ||
			!COD1_RenderRange( 0, COD1_ReadLE32( out->trees + i * COD1_BSP_AABB_TREE_STRIDE + 8 ),
				out->treeCount ) ) goto fail;
	}
	for ( i = 0; i < out->cellCount; i++ ) {
		const byte *p = out->cells + i * COD1_BSP_CELL_STRIDE;
		COD1_BSP_RenderCell( out, i, &root, &firstGroup, &groupCount );
		if ( root < 0 || ( root && root >= out->treeCount ) ||
			!COD1_RenderRange( firstGroup, groupCount, out->groupIndexCount ) ||
			!COD1_RenderRange( COD1_ReadLE32( p + 28 ), COD1_ReadLE32( p + 32 ),
				portalCount ) ) goto fail;
		for ( j = 0; j < 3; j++ ) {
			float lo = COD1_ReadLEFloat( p + j * 4 );
			float hi = COD1_ReadLEFloat( p + ( j + 3 ) * 4 );
			/* Dam has an unused cell with the compiler's empty AABB
			 * sentinel. It has no tree, portals or groups, but one collision
			 * leaf still names it. Reject inverted bounds in populated cells. */
			if ( !isfinite( lo ) || !isfinite( hi ) ||
				( lo > hi && ( i == 0 || root != 0 || groupCount != 0 ||
					COD1_ReadLE32( p + 32 ) != 0 ||
					lo != 262144.0f || hi != -262144.0f ) ) ) goto fail;
		}
	}
	for ( i = 0; i < out->cellCount && out->treeCount; i++ ) {
		int nextRoot = out->treeCount, cursor;
		COD1_BSP_RenderCell( out, i, &root, &firstGroup, &groupCount );
		if ( !root && i ) continue; /* repeated zero means no tree */
		if ( root < prevRoot ) goto fail;
		if ( root == prevRoot ) continue;
		prevRoot = root;
		for ( j = i + 1; j < out->cellCount; j++ ) {
			int candidate, groupFirstUnused, groupCountUnused;
			COD1_BSP_RenderCell( out, j, &candidate, &groupFirstUnused, &groupCountUnused );
			if ( candidate > root ) { nextRoot = candidate; break; }
			if ( candidate && candidate < root ) goto fail;
		}
		cursor = root + 1;
		if ( !COD1_CheckRenderTree( out, root, &cursor, nextRoot, 1 ) ||
			cursor != nextRoot ) goto fail;
	}
	for ( i = 0; i < portalCount; i++ ) {
		const byte *p = portals + i * COD1_BSP_PORTAL_STRIDE;
		if ( !COD1_RenderRange( COD1_ReadLE32( p ), 1, spatial->planeCount ) ||
			!COD1_RenderRange( COD1_ReadLE32( p + 4 ), 1, out->cellCount ) ||
			!COD1_RenderRange( COD1_ReadLE32( p + 8 ), COD1_ReadLE32( p + 12 ),
				vertexCount ) ) goto fail;
	}
	for ( i = 0; i < spatial->leafCount; i++ ) {
		leaf = spatial->leaves + i * COD1_BSP_LEAF_STRIDE;
		if ( COD1_ReadLE32( leaf ) >= 0 &&
			!COD1_RenderRange( COD1_ReadLE32( leaf + 24 ), 1,
				out->cellCount ) ) goto fail;
	}
	return qtrue;
fail:
	memset( out, 0, sizeof( *out ) );
	return qfalse;
}


qboolean COD1_BSP_GetModelBounds( const cod1Bsp_t *bsp, int modelIndex, vec3_t mins, vec3_t maxs ) {
	const byte *models, *m;
	int modelLength, count, i;
	if ( !bsp || !mins || !maxs || modelIndex < 0 ) return qfalse;
	models = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_MODELS, &modelLength );
	if ( !models || modelLength % COD1_BSP_MODEL_STRIDE ) return qfalse;
	count = modelLength / COD1_BSP_MODEL_STRIDE;
	if ( modelIndex >= count ) return qfalse;
	m = models + modelIndex * COD1_BSP_MODEL_STRIDE;
	for ( i = 0; i < 3; i++ ) {
		unsigned bitsMin = COD1_ReadLE32( m + i * 4 );
		unsigned bitsMax = COD1_ReadLE32( m + 12 + i * 4 );
		memcpy( &mins[i], &bitsMin, 4 );
		memcpy( &maxs[i], &bitsMax, 4 );
	}
	return qtrue;
}

static const byte *COD1_SkipSpace( const byte *p, const byte *end ) {
	while ( p < end && ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) ) {
		p++;
	}
	return p;
}

static const byte *COD1_ReadToken( const byte *p, const byte *end,
								char *token, int tokenSize ) {
	int n = 0;

	if ( !token || tokenSize < 2 ) {
		return NULL;
	}
	token[0] = '\0';
	p = COD1_SkipSpace( p, end );
	if ( p >= end ) {
		return NULL;
	}
	if ( *p == '{' || *p == '}' ) {
		token[0] = (char)*p++;
		token[1] = '\0';
		return p;
	}
	if ( *p == '"' ) {
		p++;
		while ( p < end && *p != '"' ) {
			if ( n + 1 < tokenSize ) {
				token[n++] = (char)*p;
			}
			p++;
		}
		if ( p < end && *p == '"' ) {
			p++;
		}
	} else {
		while ( p < end && *p != ' ' && *p != '\t' && *p != '\r' &&
				*p != '\n' && *p != '{' && *p != '}' ) {
			if ( n + 1 < tokenSize ) {
				token[n++] = (char)*p;
			}
			p++;
		}
	}
	token[n] = '\0';
	return p;
}

static qboolean COD1_ParseVec3( const char *text, vec3_t out ) {
	if ( !text || !out || sscanf( text, "%f %f %f", &out[0], &out[1], &out[2] ) != 3 ) {
		return qfalse;
	}
	return qtrue;
}

static void COD1_CopyString( char *dst, int dstSize, const char *src ) {
	if ( !dst || dstSize <= 0 ) {
		return;
	}
	if ( !src ) {
		dst[0] = '\0';
		return;
	}
	strncpy( dst, src, dstSize - 1 );
	dst[dstSize - 1] = '\0';
}

qboolean COD1_BSP_FindPlayerStart( const cod1Bsp_t *bsp, cod1BspSpawn_t *out ) {
	const byte *data;
	int length;
	const byte *p, *end;
	char token[128];
	int bestPriority = 100;

	if ( !out ) {
		return qfalse;
	}
	memset( out, 0, sizeof( *out ) );
	if ( !COD1_BSP_FindEntities( bsp, &data, &length ) || length <= 0 ) {
		return qfalse;
	}

	p = data;
	end = data + length;
	while ( ( p = COD1_ReadToken( p, end, token, sizeof( token ) ) ) != NULL ) {
		char classname[64];
		char origin[128];
		char angles[128];
		qboolean inEntity = qfalse;
		qboolean hasOrigin = qfalse;
		qboolean hasAngles = qfalse;
		int priority = 100;

		if ( strcmp( token, "{" ) ) {
			continue;
		}
		classname[0] = origin[0] = angles[0] = '\0';
		inEntity = qtrue;
		while ( inEntity &&
				( p = COD1_ReadToken( p, end, token, sizeof( token ) ) ) != NULL ) {
			char value[128];
			if ( !strcmp( token, "}" ) ) {
				inEntity = qfalse;
				break;
			}
			p = COD1_ReadToken( p, end, value, sizeof( value ) );
			if ( !p ) {
				break;
			}
			if ( !strcmp( token, "classname" ) ) {
				COD1_CopyString( classname, sizeof( classname ), value );
			} else if ( !strcmp( token, "origin" ) ) {
				COD1_CopyString( origin, sizeof( origin ), value );
				hasOrigin = qtrue;
			} else if ( !strcmp( token, "angle" ) || !strcmp( token, "angles" ) ) {
				COD1_CopyString( angles, sizeof( angles ), value );
				hasAngles = qtrue;
			}
		}

		if ( !strcmp( classname, "info_player_start" ) ) {
			priority = 0;
		} else if ( !strcmp( classname, "info_player_deathmatch" ) ) {
			priority = 1;
		} else if ( !strcmp( classname, "mp_team_allies_spawn" ) ||
					!strcmp( classname, "mp_team_axis_spawn" ) ) {
			priority = 2;
		}
		if ( priority >= bestPriority || !hasOrigin ||
			!COD1_ParseVec3( origin, out->origin ) ) {
			continue;
		}
		if ( hasAngles ) {
			COD1_ParseVec3( angles, out->angles );
		}
		out->valid = qtrue;
		COD1_CopyString( out->classname, sizeof( out->classname ), classname );
		bestPriority = priority;
	}

	return out->valid;
}
