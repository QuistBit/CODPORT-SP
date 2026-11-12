/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cmodel.c -- model loading

#include "cm_local.h"
#include "cod1_bsp.h"
#include "cod1_collision.h"
#include <limits.h>

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits( cplane_t *out ) {
	int bits, j;

	// for fast box on planeside test
	bits = 0;
	for ( j = 0 ; j < 3 ; j++ ) {
		if ( out->normal[j] < 0 ) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map

#define LL( x ) x = LittleLong( x )


clipMap_t cm;
int c_pointcontents;
int c_traces, c_brush_traces, c_patch_traces;


byte        *cmod_base;

#ifndef BSPC
cvar_t      *cm_noAreas;
cvar_t      *cm_noCurves;
cvar_t      *cm_playerCurveClip;
#endif

cmodel_t box_model;
cplane_t    *box_planes;
cbrush_t    *box_brush;
static qboolean boxNativeContents;



void    CM_InitBoxHull( void );
void    CM_FloodAreaConnections( void );


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders( lump_t *l ) {
	dshader_t   *in, *out;
	int i, count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "CMod_LoadShaders: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no shaders" );
	}
	cm.shaders = Hunk_Alloc( count * sizeof( *cm.shaders ), h_high );
	cm.numShaders = count;

	Com_Memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );

	if ( LittleLong( 1 ) != 1 ) {
		out = cm.shaders;
		for ( i = 0 ; i < count ; i++, in++, out++ ) {
			out->contentFlags = LittleLong( out->contentFlags );
			out->surfaceFlags = LittleLong( out->surfaceFlags );
		}
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( lump_t *l ) {
	dmodel_t    *in;
	cmodel_t    *out;
	int i, j, count;
	int         *indexes;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "CMod_LoadSubmodels: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no models" );
	}
	cm.cmodels = Hunk_Alloc( count * sizeof( *cm.cmodels ), h_high );
	cm.numSubModels = count;

	if ( count > MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "MAX_SUBMODELS exceeded" );
	}

	for ( i = 0 ; i < count ; i++, in++, out++ )
	{
		out = &cm.cmodels[i];

		for ( j = 0 ; j < 3 ; j++ )
		{   // spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat( in->mins[j] ) - 1;
			out->maxs[j] = LittleFloat( in->maxs[j] ) + 1;
		}

		if ( i == 0 ) {
			continue;   // world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = Hunk_Alloc( out->leaf.numLeafBrushes * 4, h_high );
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for ( j = 0 ; j < out->leaf.numLeafBrushes ; j++ ) {
			indexes[j] = LittleLong( in->firstBrush ) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
		indexes = Hunk_Alloc( out->leaf.numLeafSurfaces * 4, h_high );
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for ( j = 0 ; j < out->leaf.numLeafSurfaces ; j++ ) {
			indexes[j] = LittleLong( in->firstSurface ) + j;
		}
	}
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes( lump_t *l ) {
	dnode_t     *in;
	int child;
	cNode_t     *out;
	int i, j, count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map has no nodes" );
	}
	cm.nodes = Hunk_Alloc( count * sizeof( *cm.nodes ), h_high );
	cm.numNodes = count;

	out = cm.nodes;

	for ( i = 0 ; i < count ; i++, out++, in++ )
	{
		out->plane = cm.planes + LittleLong( in->planeNum );
		for ( j = 0 ; j < 2 ; j++ )
		{
			child = LittleLong( in->children[j] );
			out->children[j] = child;
		}
	}

}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b ) {
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes( lump_t *l ) {
	dbrush_t    *in;
	cbrush_t    *out;
	int i, count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	cm.brushes = Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ), h_high );
	cm.numBrushes = count;

	out = cm.brushes;

	for ( i = 0 ; i < count ; i++, out++, in++ ) {
		out->sides = cm.brushsides + LittleLong( in->firstSide );
		out->numsides = LittleLong( in->numSides );

		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
		}
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush( out );
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs( lump_t *l ) {
	int i;
	cLeaf_t     *out;
	dleaf_t     *in;
	int count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no leafs" );
	}

	cm.leafs = Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ), h_high );
	cm.numLeafs = count;

	out = cm.leafs;
	for ( i = 0 ; i < count ; i++, in++, out++ )
	{
		out->cluster = LittleLong( in->cluster );
		out->area = LittleLong( in->area );
		out->firstLeafBrush = LittleLong( in->firstLeafBrush );
		out->numLeafBrushes = LittleLong( in->numLeafBrushes );
		out->firstLeafSurface = LittleLong( in->firstLeafSurface );
		out->numLeafSurfaces = LittleLong( in->numLeafSurfaces );

		if ( out->cluster >= cm.numClusters ) {
			cm.numClusters = out->cluster + 1;
		}
		if ( out->area >= cm.numAreas ) {
			cm.numAreas = out->area + 1;
		}
	}

	cm.areas = Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ), h_high );
	cm.areaPortals = Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ), h_high );
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes( lump_t *l ) {
	int i, j;
	cplane_t    *out;
	dplane_t    *in;
	int count;
	int bits;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no planes" );
	}
	cm.planes = Hunk_Alloc( ( BOX_PLANES + count ) * sizeof( *cm.planes ), h_high );
	cm.numPlanes = count;

	out = cm.planes;

	for ( i = 0 ; i < count ; i++, in++, out++ )
	{
		bits = 0;
		for ( j = 0 ; j < 3 ; j++ )
		{
			out->normal[j] = LittleFloat( in->normal[j] );
			if ( out->normal[j] < 0 ) {
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes( lump_t *l ) {
	int i;
	int         *out;
	int         *in;
	int count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	cm.leafbrushes = Hunk_Alloc( count * sizeof( *cm.leafbrushes ), h_high );
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces( lump_t *l ) {
	int i;
	int         *out;
	int         *in;
	int count;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	cm.leafsurfaces = Hunk_Alloc( count * sizeof( *cm.leafsurfaces ), h_high );
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides( lump_t *l ) {
	int i;
	cbrushside_t    *out;
	dbrushside_t    *in;
	int count;
	int num;

	in = ( void * )( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	count = l->filelen / sizeof( *in );

	cm.brushsides = Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ), h_high );
	cm.numBrushSides = count;

	out = cm.brushsides;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		num = LittleLong( in->planeNum );
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
		}
		out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( lump_t *l ) {
	cm.entityString = Hunk_Alloc( l->filelen, h_high );
	cm.numEntityChars = l->filelen;
	Com_Memcpy( cm.entityString, cmod_base + l->fileofs, l->filelen );
}

/*
=================
CMod_LoadVisibility
=================
*/
#define VIS_HEADER  8
void CMod_LoadVisibility( lump_t *l ) {
	int len;
	byte    *buf;

	len = l->filelen;
	if ( !len ) {
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = Hunk_Alloc( cm.clusterBytes, h_high );
		Com_Memset( cm.visibility, 255, cm.clusterBytes );
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility = Hunk_Alloc( len, h_high );
	cm.numClusters = LittleLong( ( (int *)buf )[0] );
	cm.clusterBytes = LittleLong( ( (int *)buf )[1] );
	Com_Memcpy( cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define MAX_PATCH_VERTS     1024
void CMod_LoadPatches( lump_t *surfs, lump_t *verts ) {
	drawVert_t  *dv, *dv_p;
	dsurface_t  *in;
	int count;
	int i, j;
	int c;
	cPatch_t    *patch;
	vec3_t points[MAX_PATCH_VERTS];
	int width, height;
	int shaderNum;

	in = ( void * )( cmod_base + surfs->fileofs );
	if ( surfs->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	cm.numSurfaces = count = surfs->filelen / sizeof( *in );
	cm.surfaces = Hunk_Alloc( cm.numSurfaces * sizeof( cm.surfaces[0] ), h_high );

	dv = ( void * )( cmod_base + verts->fileofs );
	if ( verts->filelen % sizeof( *dv ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for ( i = 0 ; i < count ; i++, in++ ) {
		if ( LittleLong( in->surfaceType ) != MST_PATCH ) {
			continue;       // ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cm.surfaces[ i ] = patch = Hunk_Alloc( sizeof( *patch ), h_high );

		// load the full drawverts onto the stack
		width = LittleLong( in->patchWidth );
		height = LittleLong( in->patchHeight );
		c = width * height;
		if ( c > MAX_PATCH_VERTS ) {
			Com_Error( ERR_DROP, "ParseMesh: MAX_PATCH_VERTS" );
		}

		dv_p = dv + LittleLong( in->firstVert );
		for ( j = 0 ; j < c ; j++, dv_p++ ) {
			points[j][0] = LittleFloat( dv_p->xyz[0] );
			points[j][1] = LittleFloat( dv_p->xyz[1] );
			points[j][2] = LittleFloat( dv_p->xyz[2] );
		}

		shaderNum = LittleLong( in->shaderNum );
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points );
	}
}

//==================================================================


#if 0 //BSPC
/*
==================
CM_FreeMap

Free any loaded map and all submodels
==================
*/
void CM_FreeMap( void ) {
	Com_Memset( &cm, 0, sizeof( cm ) );
	Hunk_ClearHigh();
	CM_ClearLevelPatches();
}
#endif //BSPC

unsigned CM_LumpChecksum( lump_t *lump ) {
	return LittleLong( Com_BlockChecksum( cmod_base + lump->fileofs, lump->filelen ) );
}

unsigned CM_Checksum( dheader_t *header ) {
	unsigned checksums[16];
	checksums[0] = CM_LumpChecksum( &header->lumps[LUMP_SHADERS] );
	checksums[1] = CM_LumpChecksum( &header->lumps[LUMP_LEAFS] );
	checksums[2] = CM_LumpChecksum( &header->lumps[LUMP_LEAFBRUSHES] );
	checksums[3] = CM_LumpChecksum( &header->lumps[LUMP_LEAFSURFACES] );
	checksums[4] = CM_LumpChecksum( &header->lumps[LUMP_PLANES] );
	checksums[5] = CM_LumpChecksum( &header->lumps[LUMP_BRUSHSIDES] );
	checksums[6] = CM_LumpChecksum( &header->lumps[LUMP_BRUSHES] );
	checksums[7] = CM_LumpChecksum( &header->lumps[LUMP_MODELS] );
	checksums[8] = CM_LumpChecksum( &header->lumps[LUMP_NODES] );
	checksums[9] = CM_LumpChecksum( &header->lumps[LUMP_SURFACES] );
	checksums[10] = CM_LumpChecksum( &header->lumps[LUMP_DRAWVERTS] );

	return LittleLong( Com_BlockChecksum( checksums, 11 * 4 ) );
}

/*
==================
CM_LoadCod1Map

IBSP59 native capsule geometry plus legacy point/box compatibility.
The native tree and PVS are exposed to engine leaf/cluster queries.
==================
*/
static void CM_LoadCod1Map( const char *name, const cod1Bsp_t *bsp,
							const cod1BspSpatial_t *spatial, int length,
							qboolean clientload, int *checksum ) {
	const byte *entities;
	int entityLength;
	cod1BspWorldInfo_t info;
	cod1BspSpawn_t spawn;
	cplane_t *plane;
	int triangleCount;
	int trianglesWritten;
	int i, j, fields[9];
	vec3_t collisionMins, collisionMaxs;

	if ( !COD1_BSP_GetWorldInfo( bsp, &info ) ||
		 !COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_ENTITIES, &entityLength ) ||
		 entityLength < 2 ) {
		Com_Error( ERR_DROP, "CM_LoadMap: invalid CoD1 map %s", name );
		return;
	}

	entities = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_ENTITIES, &entityLength );
	if ( !entities || entities[entityLength - 1] != '\0' ) {
		Com_Error( ERR_DROP, "CM_LoadMap: CoD1 entity lump is not terminated" );
		return;
	}

	Com_Memset( &cm, 0, sizeof( cm ) );
	Q_strncpyz( cm.name, name, sizeof( cm.name ) );
	*checksum = LittleLong( Com_BlockChecksum( bsp->base, length ) );
	if ( spatial->planeCount > INT_MAX / (int)sizeof( cplane_t ) - BOX_PLANES ||
		 spatial->nodeCount > INT_MAX / (int)sizeof( cNode_t ) ||
		 spatial->leafCount > INT_MAX / (int)sizeof( cLeaf_t ) - BOX_LEAFS ) {
		Com_Error( ERR_DROP, "CM_LoadMap: CoD1 BSP spatial allocation exceeds hunk size" );
		return;
	}

	cm.planes = Hunk_Alloc( ( spatial->planeCount + BOX_PLANES ) * sizeof( *cm.planes ), h_high );
	cm.numPlanes = spatial->planeCount;
	for ( i = 0; i < spatial->planeCount; i++ ) {
		plane = cm.planes + i;
		COD1_BSP_SpatialPlane( spatial, i, plane->normal, &plane->dist );
		plane->type = PlaneTypeForNormal( plane->normal );
		SetPlaneSignbits( plane );
	}
	cm.nodes = Hunk_Alloc( spatial->nodeCount * sizeof( *cm.nodes ), h_high );
	cm.numNodes = spatial->nodeCount;
	for ( i = 0; i < spatial->nodeCount; i++ ) {
		COD1_BSP_SpatialRecord( spatial->nodes, i, fields );
		cm.nodes[i].plane = cm.planes + fields[0];
		for ( j = 0; j < 2; j++ ) cm.nodes[i].children[j] = fields[1 + j];
	}
	cm.leafs = Hunk_Alloc( ( spatial->leafCount + BOX_LEAFS ) * sizeof( *cm.leafs ), h_high );
	cm.numLeafs = spatial->leafCount;
	cm.numAreas = 1;
	for ( i = 0; i < spatial->leafCount; i++ ) {
		COD1_BSP_SpatialRecord( spatial->leaves, i, fields );
		cm.leafs[i].cluster = fields[0];
		cm.leafs[i].area = fields[1];
		if ( fields[1] >= cm.numAreas ) cm.numAreas = fields[1] + 1;
		/* Lump 22/23 indexes name native brushes and collision facets. The
		 * inline adapter owns a separate RTCW brush array, so world leaves
		 * must not index that array through the native offsets. */
	}

	/* Keep engine-visible model handles/bounds. Capsule geometry for these
       models is owned by the native v59 world loaded by CM_LoadMap. */
	cm.numSubModels = info.modelCount > 0 ? info.modelCount : 1;
	if ( cm.numSubModels > MAX_INLINE_MODELS ) {
		Com_Error( ERR_DROP, "CoD1 BSP has %d models; collision handles support at most %d",
			cm.numSubModels, MAX_INLINE_MODELS );
		return;
	}
	cm.cmodels = Hunk_Alloc( cm.numSubModels * sizeof( *cm.cmodels ), h_high );
	{
		int modelIndex;
		for ( modelIndex = 0; modelIndex < cm.numSubModels; modelIndex++ ) {
			if ( !COD1_BSP_GetModelBounds( bsp, modelIndex,
					cm.cmodels[modelIndex].mins, cm.cmodels[modelIndex].maxs ) ) {
				VectorSet( cm.cmodels[modelIndex].mins, MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
				VectorSet( cm.cmodels[modelIndex].maxs, MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
			}
		}
	}
	Com_Printf( "CoD1 collision: exposed %d inline models\n", cm.numSubModels );

	/* Retain the previous world triangle adapter for point/box queries only.
       Ordinary player capsules now use native v59 collision geometry. */
	triangleCount = COD1_BSP_CollisionTriangleCount( bsp );
	trianglesWritten = 0;
	if ( triangleCount > 0 ) {
		cm.cod1Triangles = Hunk_Alloc( triangleCount * sizeof( *cm.cod1Triangles ), h_high );
		if ( COD1_BSP_FillCollisionTriangles( bsp, cm.cod1Triangles, triangleCount,
				&trianglesWritten, collisionMins, collisionMaxs ) && trianglesWritten > 0 ) {
			cm.cod1NumTriangles = trianglesWritten;
			VectorCopy( collisionMins, cm.cmodels[0].mins );
			VectorCopy( collisionMaxs, cm.cmodels[0].maxs );
			Com_DPrintf( "CM_LoadCod1Map: collision triangles %d bounds (%g %g %g)-(%g %g %g)\n",
					trianglesWritten,
					collisionMins[0], collisionMins[1], collisionMins[2],
					collisionMaxs[0], collisionMaxs[1], collisionMaxs[2] );
		} else {
			cm.cod1Triangles = NULL;
			Com_DPrintf( "CM_LoadCod1Map: CoD1 collision decode produced no triangles\n" );
		}
	} else {
		Com_DPrintf( "CM_LoadCod1Map: CoD1 map has no triangle soup collision data\n" );
	}

	cm.numClusters = spatial->clusterCount;
	cm.clusterBytes = spatial->clusterBytes;
	cm.visibility = Hunk_Alloc( ( spatial->visibility ? cm.numClusters : 1 ) *
		cm.clusterBytes, h_high );
	if ( spatial->visibility ) {
		Com_Memcpy( cm.visibility, spatial->visibility, cm.numClusters * cm.clusterBytes );
		cm.vised = qtrue;
	} else {
		Com_Memset( cm.visibility, 255, cm.clusterBytes );
		cm.vised = qfalse;
	}
	cm.areas = Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ), h_high );
	cm.areaPortals = Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ), h_high );

	/* Extra slots are required by CM_InitBoxHull for temporary player boxes. */
    /* R9: populate only inline brush leaves from the validated native world.
     * World triangles and native player capsules keep their existing routes. */
    CM_LoadV59InlineBrushes();
    /* Inline surface arrays are also supplied by CM_LoadV59InlineBrushes. */

	cm.entityString = Hunk_Alloc( entityLength + 1, h_high );
	Com_Memcpy( cm.entityString, entities, entityLength );
	cm.entityString[entityLength] = '\0';
	cm.numEntityChars = entityLength;
	if ( COD1_BSP_FindPlayerStart( bsp, &spawn ) ) {
		Com_DPrintf( "CM_LoadCod1Map: player start %s at (%g %g %g) yaw %g\n",
				spawn.classname, spawn.origin[0], spawn.origin[1], spawn.origin[2],
				spawn.angles[1] );
	} else {
		Com_DPrintf( "CM_LoadCod1Map: no usable CoD1 player start\n" );
	}

	CM_InitBoxHull();
	CM_FloodAreaConnections();

	if ( clientload ) {
		/* Keep the usual client-load cache behavior without retaining the BSP. */
		cm.name[0] = '\0';
	}
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	int             *buf;
	int i;
	dheader_t header;
	int length;
	static unsigned last_checksum;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get( "cm_noAreas", "0", CVAR_CHEAT );
	cm_noCurves = Cvar_Get( "cm_noCurves", "0", CVAR_CHEAT );
	cm_playerCurveClip = Cvar_Get( "cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload ) {
		*checksum = last_checksum;
		return;
	}

	// free old stuff
	CM_ClearMap();

	if ( !name[0] ) {
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = Hunk_Alloc( sizeof( *cm.cmodels ), h_high );
		*checksum = 0;
		return;
	}

	//
	// load the file
	//
#ifndef BSPC
	length = FS_ReadFile( name, (void **)&buf );
#else
	length = LoadQuakeFile( (quakefile_t *) name, (void **)&buf );
#endif

	if ( !buf ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	last_checksum = LittleLong( Com_BlockChecksum( buf, length ) );
	*checksum = last_checksum;

	/* CoD1 uses a 33-lump IBSP 59 container.  Do this before copying the
	   legacy 17-lump dheader_t, otherwise CM_LoadMap would read wrong offsets. */
	{
		cod1Bsp_t cod1;
		if ( COD1_BSP_Parse( buf, length, &cod1 ) ) {
            cod1BspSpatial_t spatial;
            char error[256] = "IBSP59 allocation failed";
            if (!COD1_BSP_GetSpatial(&cod1, &spatial)) {
                FS_FreeFile(buf);
                Com_Error(ERR_DROP, "CM_LoadMap: %s: invalid CoD1 BSP tree or PVS", name);
                return;
            }
            if (!CM_LoadV59(buf, length, error, sizeof(error))) {
                FS_FreeFile(buf);
                Com_Error(ERR_DROP, "CM_LoadMap: %s: %s", name, error);
                return;
            }
			CM_LoadCod1Map( name, &cod1, &spatial, length, clientload, checksum );
			FS_FreeFile( buf );
			return;
		}
	}

	header = *(dheader_t *)buf;
	for ( i = 0 ; i < sizeof( dheader_t ) / 4 ; i++ ) {
		( (int *)&header )[i] = LittleLong( ( (int *)&header )[i] );
	}

#ifndef _SKIP_BSP_CHECK
	if ( header.version != BSP_VERSION ) {
		Com_Error( ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
				   , name, header.version, BSP_VERSION );
	}
#endif

	cmod_base = (byte *)buf;

	// load into heap
	CMod_LoadShaders( &header.lumps[LUMP_SHADERS] );
	CMod_LoadLeafs( &header.lumps[LUMP_LEAFS] );
	CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
	CMod_LoadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES] );
	CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
	CMod_LoadBrushSides( &header.lumps[LUMP_BRUSHSIDES] );
	CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES] );
	CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
	CMod_LoadNodes( &header.lumps[LUMP_NODES] );
	CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
	CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS] );

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile( buf );

	CM_InitBoxHull();

	CM_FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		Q_strncpyz( cm.name, name, sizeof( cm.name ) );
	}
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t    *CM_ClipHandleToModel( clipHandle_t handle ) {
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cm.numSubModels ) {
		return &cm.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE ) {
		return &box_model;
	}
	if ( handle < MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i",
				   cm.numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;

}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t    CM_InlineModel( int index ) {
	if ( index < 0 || index >= cm.numSubModels ) {
		Com_Error( ERR_DROP, "CM_InlineModel: bad number %d (CoD/RTCW models=%d)", index, cm.numSubModels );
	}
	return index;
}

int     CM_NumClusters( void ) {
	return cm.numClusters;
}

int     CM_NumInlineModels( void ) {
	return cm.numSubModels;
}

char    *CM_EntityString( void ) {
	return cm.entityString;
}

int     CM_LeafCluster( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cm.numLeafs ) {
		Com_Error( ERR_DROP, "CM_LeafCluster: bad number" );
	}
	return cm.leafs[leafnum].cluster;
}

int     CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cm.numLeafs ) {
		Com_Error( ERR_DROP, "CM_LeafArea: bad number" );
	}
	return cm.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull( void ) {
	int i;
	int side;
	cplane_t    *p;
	cbrushside_t    *s;

	box_planes = &cm.planes[cm.numPlanes];

	box_brush = &cm.brushes[cm.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = CONTENTS_BODY;
	boxNativeContents = qfalse;

	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cm.numBrushes;
	box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
	cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;

	for ( i = 0 ; i < 6 ; i++ )
	{
		side = i & 1;

		// brush sides
		s = &cm.brushsides[cm.numBrushSides + i];
		s->plane =  cm.planes + ( cm.numPlanes + i * 2 + side );
		s->surfaceFlags = 0;

		// planes
		p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + ( i >> 1 );
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = -1;

		SetPlaneSignbits( p );
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModelContents( const vec3_t mins, const vec3_t maxs, int capsule, int contents ) {

	box_brush->contents = contents;
	boxNativeContents = qtrue;

	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	return BOX_MODEL_HANDLE;
}

clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {
	clipHandle_t handle = CM_TempBoxModelContents( mins, maxs, capsule, CONTENTS_BODY );
	boxNativeContents = qfalse;
	return handle;
}

int CM_TempModelContents( void ) {
	return box_brush->contents;
}

qboolean CM_TempModelHasNativeContents( void ) {
	return boxNativeContents;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t    *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}

void CM_ClearMap(void) {
    CM_ClearV59();
    Com_Memset(&cm, 0, sizeof(cm));
    CM_ClearLevelPatches();
}
