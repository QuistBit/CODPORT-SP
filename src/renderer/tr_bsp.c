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

// tr_map.c

#include "tr_local.h"
#include "../qcommon/cod1_bsp.h"
#include <limits.h>

/* R12 research trace. The default build has no extra renderer path. */
#ifndef COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
#define COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS 0
#endif

/* Research-only CoD v59 RGB conversion. Keep the installed RTCW path by default. */
#ifndef COD1_NATIVE_LIGHTMAP_RGB
#define COD1_NATIVE_LIGHTMAP_RGB 0
#endif
#ifndef COD1_NATIVE_LIGHTMAP_GL
#define COD1_NATIVE_LIGHTMAP_GL 0
#endif
#ifndef COD1_NATIVE_LIGHTMAP_PACK
#define COD1_NATIVE_LIGHTMAP_PACK 0
#endif
#if COD1_NATIVE_LIGHTMAP_PACK && ( !COD1_NATIVE_LIGHTMAP_RGB || !COD1_NATIVE_LIGHTMAP_GL )
#error "Packed CoD1 lightmaps require native RGB conversion and direct GL upload"
#endif
#if COD1_NATIVE_LIGHTMAP_PACK
#include "tr_cod1_lightmap_packer.h"
/* Only the BSP soup registration window uses exact packed-group shader keys. */
qboolean cod1PackedWorldShaders;
#endif

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static world_t s_worldData;
static byte        *fileBase;
qboolean cod1Bsp59MapActive;

#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
static qboolean cod1LmTraceDam;
static qboolean R_Cod1LmTraceDamName( const char *name ) {
	const char *base, *slash, *backslash;
	if ( !name ) return qfalse;
	slash = strrchr( name, '/' );
	backslash = strrchr( name, '\\' );
	base = slash && ( !backslash || slash > backslash ) ? slash + 1 :
		backslash ? backslash + 1 : name;
	return !Q_stricmp( base, "dam.bsp" ) || !Q_stricmp( base, "dam" );
}

/* These two verified dam world soups use atlas 0 (mountain) and atlas 12
 * (waterfall). A stage may be a legacy fallback: report what was registered. */
static void R_Cod1LmTraceSoup( int index, const char *name,
		int requestedAtlas, const shader_t *registered ) {
	int stage, bundle;
	if ( !cod1LmTraceDam || ( index != 49 && index != 113 ) || !registered ) return;
	ri.Printf( PRINT_ALL,
		"CoD1 LM soup=%d material=%s requestedAtlas=%d shaderAtlas=%d passes=%d sort=%g env=%d fallback=%d\n",
		index, name, requestedAtlas, registered->lightmapIndex,
		registered->numUnfoggedPasses, registered->sort,
		registered->multitextureEnv, registered->defaultShader );
	for ( stage = 0; stage < registered->numUnfoggedPasses; stage++ ) {
		const shaderStage_t *s = registered->stages[stage];
		if ( !s ) continue;
		for ( bundle = 0; bundle < NUM_TEXTURE_BUNDLES; bundle++ ) {
			const textureBundle_t *b = &s->bundle[bundle];
			const image_t *image = b->image[0];
			ri.Printf( PRINT_ALL,
				"CoD1 LM soup=%d stage=%d bundle=%d image=%s isLightmap=%d tcGen=%d stateBits=%u\n",
				index, stage, bundle, image ? image->imgName : "(none)",
				b->isLightmap, b->tcGen, s->stateBits );
		}
	}
}
#endif

int c_subdivisions;
int c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] ) {
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/*
===============
R_LoadLightmaps

===============
*/
#define LIGHTMAP_SIZE   128
static void R_LoadLightmaps( lump_t *l ) {
	byte        *buf, *buf_p;
	int len;
	MAC_STATIC byte image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	int i, j;
	float maxIntensity = 0;
	double sumIntensity = 0;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	tr.numLightmaps = len / ( LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3 );
	if ( tr.numLightmaps == 1 ) {
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		tr.numLightmaps++;
	}

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		return;
	}

	for ( i = 0 ; i < tr.numLightmaps ; i++ ) {
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		if ( r_lightmap->integer == 2 ) { // color code by intensity as development tool	(FIXME: check range)
			for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
			{
				float r = buf_p[j * 3 + 0];
				float g = buf_p[j * 3 + 1];
				float b = buf_p[j * 3 + 2];
				float intensity;
				float out[3];

				intensity = 0.33f * r + 0.685f * g + 0.063f * b;

				if ( intensity > 255 ) {
					intensity = 1.0f;
				} else {
					intensity /= 255.0f;
				}

				if ( intensity > maxIntensity ) {
					maxIntensity = intensity;
				}

				HSVtoRGB( intensity, 1.00, 0.50, out );

				image[j * 4 + 0] = out[0] * 255;
				image[j * 4 + 1] = out[1] * 255;
				image[j * 4 + 2] = out[2] * 255;
				image[j * 4 + 3] = 255;

				sumIntensity += intensity;
			}
		} else {
			for ( j = 0 ; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ ) {
				R_ColorShiftLightingBytes( &buf_p[j * 3], &image[j * 4] );
				image[j * 4 + 3] = 255;
			}
		}
		tr.lightmaps[i] = R_CreateImage( va( "*lightmap%d",i ), image,
										 LIGHTMAP_SIZE, LIGHTMAP_SIZE, qfalse, qfalse, GL_CLAMP );
	}

	if ( r_lightmap->integer == 2 ) {
		ri.Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}
}


/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void        RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility( lump_t *l ) {
	int len;
	byte    *buf;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri.Hunk_Alloc( len, h_low );
	memset( s_worldData.novis, 0xff, len );

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ( (int *)buf )[0] );
	s_worldData.clusterBytes = LittleLong( ( (int *)buf )[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData ) {
		s_worldData.vis = tr.externalVisData;
	} else {
		byte    *dest;

		dest = ri.Hunk_Alloc( len - 8, h_low );
		memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, int lightmapNum,
		cod1SurfaceMaterial_t *source ) {
	shader_t    *shader;
	dshader_t   *dsh;

	shaderNum = LittleLong( shaderNum );
	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders ) {
		ri.Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );
	}
	dsh = &s_worldData.shaders[ shaderNum ];
	if ( source ) source->requestedLightmap = lightmapNum;

	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if ( r_fullbright->integer ) {
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}
	if ( source ) source->effectiveLightmap = lightmapNum;

	shader = R_FindShaderForBspMaterial( dsh->shader, lightmapNum, qtrue, s_worldData.isCod1 );
	if ( source ) {
		source->route = shader->cod1MaterialRoute;
		source->registeredLightmap = shader->lightmapIndex;
		source->resolvedShaderIndex = shader->index;
		source->fromBackup = shader->cod1MaterialFromBackup;
	}

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

// Ridah, optimizations here
// memory block for use by surfaces
static byte *surfHunkPtr;
static int surfHunkSize;
#define SURF_HUNK_MAXSIZE 0x40000
#define LL( x ) LittleLong( x )

/*
==============
R_InitSurfMemory
==============
*/
void R_InitSurfMemory( void ) {
	// allocate a new chunk
	surfHunkPtr = ri.Hunk_Alloc( SURF_HUNK_MAXSIZE, h_low );
	surfHunkSize = 0;
}

/*
==============
R_GetSurfMemory
==============
*/
void *R_GetSurfMemory( int size ) {
	byte *retval;

	// round to cacheline
	size = ( size + 31 ) & ~31;

	surfHunkSize += size;
	if ( surfHunkSize >= SURF_HUNK_MAXSIZE ) {
		// allocate a new chunk
		R_InitSurfMemory();
		surfHunkSize += size;   // since it just got reset
	}
	retval = surfHunkPtr;
	surfHunkPtr += size;

	return (void *)retval;
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes  ) {
	int i, j;
	srfSurfaceFace_t    *cv;
	int numPoints, numIndexes;
	int lightmapNum;
	int sfaceSize, ofsIndexes;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, NULL );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numPoints = LittleLong( ds->numVerts );
	if ( numPoints > MAX_FACE_POINTS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints );
		numPoints = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong( ds->numIndexes );

	// create the srfSurfaceFace_t
	sfaceSize = ( int ) &( (srfSurfaceFace_t *)0 )->points[numPoints];
	ofsIndexes = sfaceSize;
	sfaceSize += sizeof( int ) * numIndexes;

	//cv = ri.Hunk_Alloc( sfaceSize );
	cv = R_GetSurfMemory( sfaceSize );

	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			cv->points[i][j] = LittleFloat( verts[i].xyz[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			cv->points[i][3 + j] = LittleFloat( verts[i].st[j] );
			cv->points[i][5 + j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, (byte *)&cv->points[i][7] );
	}

	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		( ( int * )( (byte *)cv + cv->ofsIndices ) )[i] = LittleLong( indexes[ i ] );
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->plane.dist = DotProduct( cv->points[0], cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = (surfaceType_t *)cv;
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh( dsurface_t *ds, drawVert_t *verts, msurface_t *surf ) {
	srfGridMesh_t   *grid;
	int i, j;
	int width, height, numPoints;
	MAC_STATIC drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	int lightmapNum;
	vec3_t bounds[2];
	vec3_t tmpVec;
	static surfaceType_t skipData = SF_SKIP;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum, NULL );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			points[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			points[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			points[i].st[j] = LittleFloat( verts[i].st[j] );
			points[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, points[i].color );
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfTriangles_t  *tri;
	int i, j;
	int numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX, NULL );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	//tri = ri.Hunk_Alloc( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] )
	//	+ numIndexes * sizeof( tri->indexes[0] ) );
	tri = R_GetSurfMemory( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] )
						   + numIndexes * sizeof( tri->indexes[0] ) );

	tri->surfaceType = SF_TRIANGLES;
	tri->cod1PrecalcValid = qfalse;
	memset(tri->cod1PrecalcRGBA, 0, sizeof(tri->cod1PrecalcRGBA));
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = ( drawVert_t * )( tri + 1 );
	tri->indexes = ( int * )( tri->verts + tri->numVerts );

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	ClearBounds( tri->bounds[0], tri->bounds[1] );
	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numVerts ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tri->verts[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			tri->verts[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		AddPointToBounds( tri->verts[i].xyz, tri->bounds[0], tri->bounds[1] );
		for ( j = 0 ; j < 2 ; j++ ) {
			tri->verts[i].st[j] = LittleFloat( verts[i].st[j] );
			tri->verts[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}

		R_ColorShiftLightingBytes( verts[i].color, tri->verts[i].color );
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		tri->indexes[i] = LittleLong( indexes[i] );
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= numVerts ) {
			ri.Error( ERR_DROP, "Bad index in triangle surface" );
		}
	}
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t      *flare;
	int i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX, NULL );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	flare = ri.Hunk_Alloc( sizeof( *flare ), h_low );
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints( srfGridMesh_t *grid, int offset ) {
	int i, j;

	for ( i = 1; i < grid->width - 1; i++ ) {
		for ( j = i + 1; j < grid->width - 1; j++ ) {
			if ( Q_fabs( grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2] ) > .1 ) {
				continue;
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints( srfGridMesh_t *grid, int offset ) {
	int i, j;

	for ( i = 1; i < grid->height - 1; i++ ) {
		for ( j = i + 1; j < grid->height - 1; j++ ) {
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2] ) > .1 ) {
				continue;
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) {
			continue;
		}
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) {
			continue;
		}
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) {
			continue;
		}
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) {
			continue;
		}
		//
		touch = qfalse;
		for ( n = 0; n < 2; n++ ) {
			//
			if ( n ) {
				offset1 = ( grid1->height - 1 ) * grid1->width;
			} else { offset1 = 0;}
			if ( R_MergedWidthPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->width - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else { offset2 = 0;}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = grid2->width - 1;
					} else { offset2 = 0;}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for ( n = 0; n < 2; n++ ) {
			//
			if ( n ) {
				offset1 = grid1->width - 1;
			} else { offset1 = 0;}
			if ( R_MergedHeightPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->height - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else { offset2 = 0;}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = grid2->width - 1;
					} else { offset2 = 0;}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if ( touch ) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID ) {
			continue;
		}
		//
		if ( grid1->lodFixed ) {
			continue;
		}
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1 );
	}
}


/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num ) {
	int k, l, m, n, offset1, offset2, row, column;
	srfGridMesh_t *grid1, *grid2;
	float *v1, *v2;

	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else { offset1 = 0;}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = 0; k < grid1->width - 2; k += 2 ) {

			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = grid1->width - 1;
		} else { offset1 = 0;}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = 0; k < grid1->height - 2; k += 2 ) {
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else { offset1 = 0;}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = grid1->width - 1; k > 1; k -= 2 ) {

			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					if ( !grid2 ) {
						break;
					}
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = grid1->width - 1;
		} else { offset1 = 0;}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = grid1->height - 1; k > 1; k -= 2 ) {
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) {
			continue;
		}
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) {
			continue;
		}
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) {
			continue;
		}
		//
		while ( R_StitchPatches( grid1num, j ) )
		{
			numstitches++;
		}
	}
	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;
	srfGridMesh_t *grid1;

	numstitches = 0;
	do
	{
		stitched = qfalse;
		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID ) {
				continue;
			}
			//
			if ( grid1->lodStitched ) {
				continue;
			}
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while ( stitched );
	ri.Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk( void ) {
	int i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID ) {
			continue;
		}
		//
		size = ( grid->width * grid->height - 1 ) * sizeof( drawVert_t ) + sizeof( *grid );
		hunkgrid = ri.Hunk_Alloc( size, h_low );
		Com_Memcpy( hunkgrid, grid, size );

		hunkgrid->widthLodError = ri.Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = ri.Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( grid->heightLodError, grid->heightLodError, grid->height * 4 );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (void *) hunkgrid;
	}
}

/*
===============
R_LoadSurfaces
===============
*/
static void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump ) {
	dsurface_t  *in;
	msurface_t  *out;
	drawVert_t  *dv;
	int         *indexes;
	int count;
	int numFaces, numMeshes, numTriSurfs, numFlares;
	int i;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	in = ( void * )( fileBase + surfs->fileofs );
	if ( surfs->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = surfs->filelen / sizeof( *in );

	dv = ( void * )( fileBase + verts->fileofs );
	if ( verts->filelen % sizeof( *dv ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}

	indexes = ( void * )( fileBase + indexLump->fileofs );
	if ( indexLump->filelen % sizeof( *indexes ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}

	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	// Ridah, init the surface memory. This is optimization, so we don't have to
	// look for memory for each surface, we allocate a big block and just chew it up
	// as we go
	R_InitSurfMemory();

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh( in, dv, out );
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, out, indexes );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, out, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes );
			numFlares++;
			break;
		default:
			ri.Error( ERR_DROP, "Bad surfaceType" );
		}
	}

#ifdef PATCH_STITCHING
	R_StitchAllPatches();
#endif

	R_FixSharedVertexLodError();

#ifdef PATCH_STITCHING
	R_MovePatchSurfacesToHunk();
#endif

	ri.Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n",
			   numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static void R_LoadSubmodels( lump_t *l ) {
	dmodel_t    *in;
	bmodel_t    *out;
	int i, j, count;

	in = ( void * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *in );

	s_worldData.bmodels = out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );            // this should never happen

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for ( j = 0 ; j < 3 ; j++ ) {
			out->bounds[0][j] = LittleFloat( in->mins[j] );
			out->bounds[1][j] = LittleFloat( in->maxs[j] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static void R_SetParent( mnode_t *node, mnode_t *parent ) {
	node->parent = parent;
	if ( node->contents != -1 ) {
		return;
	}
	R_SetParent( node->children[0], node );
	R_SetParent( node->children[1], node );
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs( lump_t *nodeLump, lump_t *leafLump ) {
	int i, j, p;
	dnode_t     *in;
	dleaf_t     *inLeaf;
	mnode_t     *out;
	int numNodes, numLeafs;

	in = ( void * )( fileBase + nodeLump->fileofs );
	if ( nodeLump->filelen % sizeof( dnode_t ) ||
		 leafLump->filelen % sizeof( dleaf_t ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	numNodes = nodeLump->filelen / sizeof( dnode_t );
	numLeafs = leafLump->filelen / sizeof( dleaf_t );

	out = ri.Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low );

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i = 0 ; i < numNodes; i++, in++, out++ )
	{
		for ( j = 0 ; j < 3 ; j++ )
		{
			out->mins[j] = LittleLong( in->mins[j] );
			out->maxs[j] = LittleLong( in->maxs[j] );
		}

		p = LittleLong( in->planeNum );
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;  // differentiate from leafs

		for ( j = 0 ; j < 2 ; j++ )
		{
			p = LittleLong( in->children[j] );
			if ( p >= 0 ) {
				out->children[j] = s_worldData.nodes + p;
			} else {
				out->children[j] = s_worldData.nodes + numNodes + ( -1 - p );
			}
		}
	}

	// load leafs
	inLeaf = ( void * )( fileBase + leafLump->fileofs );
	for ( i = 0 ; i < numLeafs ; i++, inLeaf++, out++ )
	{
		for ( j = 0 ; j < 3 ; j++ )
		{
			out->mins[j] = LittleLong( inLeaf->mins[j] );
			out->maxs[j] = LittleLong( inLeaf->maxs[j] );
		}

		out->cluster = LittleLong( inLeaf->cluster );
		out->area = LittleLong( inLeaf->area );

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
								LittleLong( inLeaf->firstLeafSurface );
		out->nummarksurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain decendants
	R_SetParent( s_worldData.nodes, NULL );
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders( lump_t *l ) {
	int i, count;
	dshader_t   *in, *out;

	in = ( void * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	memcpy( out, in, count * sizeof( *out ) );

	for ( i = 0 ; i < count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}


/*
=================
R_LoadMarksurfaces
=================
*/
static void R_LoadMarksurfaces( lump_t *l ) {
	int i, j, count;
	int     *in;
	msurface_t **out;

	in = ( void * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( i = 0 ; i < count ; i++ )
	{
		j = LittleLong( in[i] );
		out[i] = s_worldData.surfaces + j;
	}
}


/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes( lump_t *l ) {
	int i, j;
	cplane_t    *out;
	dplane_t    *in;
	int count;
	int bits;

	in = ( void * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * 2 * sizeof( *out ), h_low );

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		bits = 0;
		for ( j = 0 ; j < 3 ; j++ ) {
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
R_LoadFogs

=================
*/
static void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int i;
	fog_t       *out;
	dfog_t      *fogs;
	dbrush_t    *brushes, *brush;
	dbrushside_t    *sides;
	int count, brushesCount, sidesCount;
	int sideNum;
	int planeNum;
	shader_t    *shader;
	float d;
	int firstSide;

	fogs = ( void * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *fogs ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *fogs );

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = ri.Hunk_Alloc( s_worldData.numfogs * sizeof( *out ), h_low );
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = ( void * )( fileBase + brushesLump->fileofs );
	if ( brushesLump->filelen % sizeof( *brushes ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	brushesCount = brushesLump->filelen / sizeof( *brushes );

	sides = ( void * )( fileBase + sidesLump->fileofs );
	if ( sidesLump->filelen % sizeof( *sides ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	sidesCount = sidesLump->filelen / sizeof( *sides );

	for ( i = 0 ; i < count ; i++, fogs++ ) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
			ri.Error( ERR_DROP, "fog brushNumber out of range" );
		}
		brush = brushes + out->originalBrushNumber;

		firstSide = LittleLong( brush->firstSide );

		if ( (unsigned)firstSide > sidesCount - 6 ) {
			ri.Error( ERR_DROP, "fog brush sideNumber out of range" );
		}

		// brushes are always sorted with the axial sides first
		sideNum = firstSide + 0;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, qtrue );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4( shader->fogParms.color[0] * tr.identityLight,
									 shader->fogParms.color[1] * tr.identityLight,
									 shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}


/*
==============
R_FindLightGridBounds
==============
*/
void R_FindLightGridBounds( vec3_t mins, vec3_t maxs ) {
	world_t *w;
	msurface_t  *surf;
	srfSurfaceFace_t *surfFace;
//	cplane_t	*plane;
	struct shader_s     *shd;

	qboolean foundGridBrushes = qfalse;
	int i,j;

	w = &s_worldData;

//----(SA)	temp - disable this whole thing for now
	VectorCopy( w->bmodels[0].bounds[0], mins );
	VectorCopy( w->bmodels[0].bounds[1], maxs );
	return;
//----(SA)	temp




	ClearBounds( mins, maxs );

// wrong!
	for ( i = 0; i < w->bmodels[0].numSurfaces; i++ ) {
		surf = w->bmodels[0].firstSurface + i;
		shd = surf->shader;

		if ( !( *surf->data == SF_FACE ) ) {
			continue;
		}

		if ( !( shd->contentFlags & CONTENTS_LIGHTGRID ) ) {
			continue;
		}

		foundGridBrushes = qtrue;
	}


// wrong!
	for ( i = 0; i < w->numsurfaces; i++ ) {
		surf = &w->surfaces[i];
		shd = surf->shader;
		if ( !( *surf->data == SF_FACE ) ) {
			continue;
		}

		if ( !( shd->contentFlags & CONTENTS_LIGHTGRID ) ) {
			continue;
		}

		foundGridBrushes = qtrue;

		surfFace = ( srfSurfaceFace_t * )surf->data;

		for ( j = 0; j < surfFace->numPoints; j++ ) {
			AddPointToBounds( surfFace->points[j], mins, maxs );
		}

	}

	// go through brushes looking for lightgrid
//	for ( i = 0 ; i < numbrushes ; i++ ) {
//		db = &dbrushes[i];
//
//		if (!(dshaders[db->shaderNum].contentFlags & CONTENTS_LIGHTGRID)) {
//			continue;
//		}
//
//		foundGridBrushes = qtrue;
//
//		// go through light grid surfaces for bounds
//		for ( j = 0 ; j < db->numSides ; j++ ) {
//			s = &dbrushsides[ db->firstSide + j ];
//
//			surfmin[0] = -dplanes[ dbrushsides[ db->firstSide + 0 ].planeNum ].dist - 1;
//			surfmin[1] = -dplanes[ dbrushsides[ db->firstSide + 2 ].planeNum ].dist - 1;
//			surfmin[2] = -dplanes[ dbrushsides[ db->firstSide + 4 ].planeNum ].dist - 1;
//			surfmax[0] = dplanes[ dbrushsides[ db->firstSide + 1 ].planeNum ].dist + 1;
//			surfmax[1] = dplanes[ dbrushsides[ db->firstSide + 3 ].planeNum ].dist + 1;
//			surfmax[2] = dplanes[ dbrushsides[ db->firstSide + 5 ].planeNum ].dist + 1;
//			AddPointToBounds (surfmin, mins, maxs);
//			AddPointToBounds (surfmax, mins, maxs);
//		}
//	}


//----(SA)	temp
	foundGridBrushes = qfalse;  // disable this whole thing for now
//----(SA)	temp

	if ( !foundGridBrushes ) {
		VectorCopy( w->bmodels[0].bounds[0], mins );
		VectorCopy( w->bmodels[0].bounds[1], maxs );
	}
}

/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( lump_t *l ) {
	int i;
	vec3_t maxs;
	int numGridPoints;
	world_t *w;
//	float	*wMins, *wMaxs;
	vec3_t wMins, wMaxs;

	w = &s_worldData;

	w->lightGridInverseSize[0] = 1.0 / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0 / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0 / w->lightGridSize[2];

//----(SA)	modified
	R_FindLightGridBounds( wMins, wMaxs );
//	wMins = w->bmodels[0].bounds[0];
//	wMaxs = w->bmodels[0].bounds[1];
//----(SA)	end

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = ( maxs[i] - w->lightGridOrigin[i] ) / w->lightGridSize[i] + 1;
	}

	numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( l->filelen != numGridPoints * 8 ) {
		ri.Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = ri.Hunk_Alloc( l->filelen, h_low );
	memcpy( w->lightGridData, ( void * )( fileBase + l->fileofs ), l->filelen );

	// deal with overbright bits
	for ( i = 0 ; i < numGridPoints ; i++ ) {
		R_ColorShiftLightingBytes( &w->lightGridData[i * 8], &w->lightGridData[i * 8] );
		R_ColorShiftLightingBytes( &w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3] );
	}
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l ) {
	char *p, *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t *w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = ( char * )( fileBase + l->fileofs );

	// store for reference by the cgame
	w->entityString = ri.Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( w->entityString, p );
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if ( !*token || *token != '{' ) {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz( value, token, sizeof( value ) );

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if ( !Q_strncmp( keyname, s, strlen( s ) ) ) {
			s = strchr( value, ';' );
			if ( !s ) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if ( r_vertexLight->integer ) {
				R_RemapShader( value, s, "0" );
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if ( !Q_strncmp( keyname, s, strlen( s ) ) ) {
			s = strchr( value, ';' );
			if ( !s ) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader( value, s, "0" );
			continue;
		}
		// check for a different grid size
		if ( !Q_stricmp( keyname, "gridsize" ) ) {
			sscanf( value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}
	}
}

/*
================
R_LoadCod1Surfaces

Translate CoD1 triangle soups into the renderer's existing triangle-surface
representation.  The map remains in its original coordinate system; this is
deliberately a render-only bridge, not a Q3 BSP reinterpretation.
================
*/
static const char *R_Cod1MaterialRouteName( cod1MaterialRoute_t route ) {
	switch ( route ) {
	case COD1_MATERIAL_SCRIPT_LEGACY: return "script-legacy";
	case COD1_MATERIAL_SCRIPT_WINDOW: return "script-static-glow";
    case COD1_MATERIAL_SCRIPT_PERLIGHT_PARTIAL: return "script-static-perlight-deferred";
	case COD1_MATERIAL_SCRIPT_IR: return "script-ir";
	case COD1_MATERIAL_SCRIPT_WATER_CANDIDATE: return "script-water-candidate";
	case COD1_MATERIAL_SCRIPT_DIFFUSE: return "script-diffuse-fallback";
	case COD1_MATERIAL_IMPLICIT_IMAGE: return "implicit-image";
	case COD1_MATERIAL_SCRIPT_PARSE_FAILED: return "script-parse-failed";
	case COD1_MATERIAL_MISSING_IMAGE: return "implicit-missing-image";
	case COD1_MATERIAL_STYPE_SUBSET: return "stype-subset";
	case COD1_MATERIAL_STYPE_FAILED: return "stype-unsupported";
	default: return "unresolved";
	}
}

/* This is the surface registration state, before remaps and frame rendering.
 * r_cod1MaterialTrace=1 prints one representative of each BSP material,
 * signed atlas and renderer route used by world or inline soups. */
static void R_Cod1ReportMaterials( const cod1BspWorldInfo_t *info ) {
	const cod1SurfaceMaterial_t *sources = s_worldData.cod1SurfaceMaterials;
	cvar_t *trace = ri.Cvar_Get( "r_cod1MaterialTrace", "0", 0 );
	int routes[COD1_MATERIAL_ROUTE_COUNT], flagSkipped = 0, sky = 0, drawn = 0;
	int defaults = 0, backup = 0, negativeAtlas = 0;
	int i, s, b, used = 0, slots = 0;
	int *firstByKey = NULL;
	byte *usedMaterial = ri.Hunk_AllocateTempMemory( s_worldData.numShaders );
	if ( !usedMaterial ) return;
	if ( trace->integer && info->soupCount > 0 && info->soupCount < INT_MAX / 16 ) {
		slots = 1;
		while ( slots < info->soupCount * 2 ) slots <<= 1;
		firstByKey = ri.Hunk_AllocateTempMemory( slots * sizeof( *firstByKey ) );
		if ( firstByKey ) memset( firstByKey, 0, slots * sizeof( *firstByKey ) );
	}
	memset( routes, 0, sizeof( routes ) );
	memset( usedMaterial, 0, s_worldData.numShaders );
	for ( i = 0; i < info->soupCount; i++ ) {
		const cod1SurfaceMaterial_t *source = &sources[i];
		const shader_t *sh = s_worldData.surfaces[i].shader;
		const shader_t *resolved = source->resolvedShaderIndex >= 0 &&
			source->resolvedShaderIndex < tr.numShaders ?
			tr.shaders[source->resolvedShaderIndex] : NULL;
		unsigned slot;
		if ( !usedMaterial[source->materialIndex] ) {
			usedMaterial[source->materialIndex] = 1;
			used++;
		}
		if ( source->fromBackup ) backup++;
		if ( source->sourceAtlas < 0 ) negativeAtlas++;
		if ( source->status == COD1_SURFACE_FLAG_SKIP ) flagSkipped++;
		else if ( source->status == COD1_SURFACE_SKY_SKIP ) sky++;
		else {
			drawn++;
			if ( source->status == COD1_SURFACE_DEFAULT_SHADER ) defaults++;
			if ( source->route >= 0 && source->route < COD1_MATERIAL_ROUTE_COUNT )
				routes[source->route]++;
			else routes[COD1_MATERIAL_UNRESOLVED]++;
		}
		if ( !firstByKey ) continue;
		slot = ( (unsigned)source->materialIndex * 2654435761u ^
			(unsigned)source->sourceAtlas * 2246822519u ^
			(unsigned)source->effectiveLightmap * 3266489917u ^
			(unsigned)source->route * 668265263u ^
			(unsigned)source->status ) & (unsigned)( slots - 1 );
		while ( firstByKey[slot] ) {
			const cod1SurfaceMaterial_t *previous = &sources[firstByKey[slot] - 1];
			if ( previous->materialIndex == source->materialIndex &&
				 previous->sourceAtlas == source->sourceAtlas &&
				 previous->effectiveLightmap == source->effectiveLightmap &&
				 previous->route == source->route &&
				 previous->status == source->status &&
				 previous->fromBackup == source->fromBackup ) break;
			slot = ( slot + 1 ) & (unsigned)( slots - 1 );
		}
		if ( firstByKey[slot] ) continue;
		firstByKey[slot] = i + 1;
		ri.Printf( PRINT_ALL,
			"CoD1 material path: soup=%d id=%d name=%.*s flags=%08x contents=%08x rawAtlas=%d portRequestLm=%d effectiveLm=%d shaderLm=%d resolvedShader=%d surfaceShader=%d status=%s route=%s backupCache=%d combinedScriptOffset=%d passes=%d sort=%g\n",
			i, source->materialIndex,
			(int)sizeof(s_worldData.shaders[source->materialIndex].shader),
			s_worldData.shaders[source->materialIndex].shader,
			source->surfaceFlags, source->contentFlags, source->sourceAtlas,
			source->requestedLightmap, source->effectiveLightmap,
			source->registeredLightmap, source->resolvedShaderIndex,
			sh ? sh->index : -1,
			source->status == COD1_SURFACE_FLAG_SKIP ? "flag-skip" :
			(source->status == COD1_SURFACE_SKY_SKIP ? "sky-skip" :
			 (source->status == COD1_SURFACE_DEFAULT_SHADER ? "default-shader" : "drawn")),
			R_Cod1MaterialRouteName( source->route ), source->fromBackup,
			resolved && !source->fromBackup && resolved->cod1HasScriptSource ?
				(int)resolved->cod1ScriptOffset : -1,
			sh ? sh->numUnfoggedPasses : 0, sh ? sh->sort : 0.0f );
		if ( !sh || source->status == COD1_SURFACE_FLAG_SKIP ||
			 source->status == COD1_SURFACE_SKY_SKIP ) continue;
		for ( s = 0; s < sh->numUnfoggedPasses; s++ ) {
			const shaderStage_t *stage = sh->stages[s];
			if ( !stage ) continue;
			for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
				const textureBundle_t *bundle = &stage->bundle[b];
				if ( !bundle->image[0] ) continue;
				ri.Printf( PRINT_ALL,
					"CoD1 material stage: soup=%d id=%d stage=%d bundle=%d image=%s lightmap=%d tcGen=%d rgbGen=%d alphaGen=%d state=%08x\n",
					i, source->materialIndex, s, b, bundle->image[0]->imgName,
					bundle->isLightmap, bundle->tcGen, stage->rgbGen,
					stage->alphaGen, stage->stateBits );
			}
		}
	}
	ri.Printf( PRINT_ALL,
		"CoD1 material routes: %d used BSP records, %d drawn soups (%d default shader), %d flag skips, %d sky skips, %d signed-negative atlas soups, %d backup-cache soups; legacy=%d staticGlow=%d ir=%d waterCandidate=%d diffuseFallback=%d implicit=%d scriptParseFailed=%d missingImage=%d unresolved=%d stype=%d stypeUnsupported=%d partialPerlight=%d\n",
		used, drawn, defaults, flagSkipped, sky, negativeAtlas, backup,
		routes[COD1_MATERIAL_SCRIPT_LEGACY],
		routes[COD1_MATERIAL_SCRIPT_WINDOW], routes[COD1_MATERIAL_SCRIPT_IR],
		routes[COD1_MATERIAL_SCRIPT_WATER_CANDIDATE],
		routes[COD1_MATERIAL_SCRIPT_DIFFUSE], routes[COD1_MATERIAL_IMPLICIT_IMAGE],
		routes[COD1_MATERIAL_SCRIPT_PARSE_FAILED],
		routes[COD1_MATERIAL_MISSING_IMAGE], routes[COD1_MATERIAL_UNRESOLVED],
        routes[COD1_MATERIAL_STYPE_SUBSET],routes[COD1_MATERIAL_STYPE_FAILED],routes[COD1_MATERIAL_SCRIPT_PERLIGHT_PARTIAL] );
	if ( firstByKey ) ri.Hunk_FreeTempMemory( firstByKey );
	ri.Hunk_FreeTempMemory( usedMaterial );
}

static void R_LoadCod1Surfaces( const cod1Bsp_t *bsp,
								const cod1BspWorldInfo_t *info,
#if COD1_NATIVE_LIGHTMAP_PACK
								const bsp59_lm_layout_t *lightmapLayout,
#endif
								vec3_t bounds[2] ) {
	const byte *soups;
	const byte *verts;
	const byte *meshverts;
	int soupLength, vertLength, meshvertLength;
	int i, j;
	int cod1SkippedSurfaces = 0;
	int cod1DefaultedSurfaces = 0;
	int cod1DefaultedMaterials = 0;
	byte *cod1ReportedMaterials;
	msurface_t *out;
	static surfaceType_t cod1SkipSurface = SF_SKIP;

	soups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS, &soupLength );
	verts = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_DRAWVERTS, &vertLength );
	meshverts = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_MESHVERTS, &meshvertLength );
	if ( !soups || !verts || !meshverts || info->soupCount <= 0 ) {
		ri.Error( ERR_DROP, "CoD1 BSP has no world triangle soups" );
	}

	if ( soupLength % COD1_BSP_TRIANGLE_SOUP_STRIDE ||
		 vertLength % COD1_BSP_DRAWVERT_STRIDE ||
		 meshvertLength % COD1_BSP_MESHVERT_STRIDE ) {
		ri.Error( ERR_DROP, "CoD1 BSP has a malformed geometry lump" );
	}

	s_worldData.surfaces = out = ri.Hunk_Alloc( info->soupCount * sizeof( *out ), h_low );
	s_worldData.numsurfaces = info->soupCount;
    s_worldData.cod1FirstWorldSurface=info->worldFirstSoup;
    s_worldData.cod1WorldSurfaceCount=info->worldSoupCount;
    s_worldData.cod1MarkFlags=ri.Hunk_Alloc(info->soupCount*sizeof(unsigned),h_low);
	s_worldData.cod1SurfaceMaterials=ri.Hunk_Alloc(
		info->soupCount*sizeof(*s_worldData.cod1SurfaceMaterials),h_low);
	cod1ReportedMaterials = ri.Hunk_Alloc( s_worldData.numShaders, h_low );
	memset( cod1ReportedMaterials, 0, s_worldData.numShaders );
	R_InitSurfMemory();
	ClearBounds( bounds[0], bounds[1] );

	for ( i = 0; i < info->soupCount; i++, out++ ) {
		const byte *soup;
		cod1SurfaceMaterial_t *source = &s_worldData.cod1SurfaceMaterials[i];
		int material;
		int lightmap;
#if COD1_NATIVE_LIGHTMAP_PACK
		int shaderLightmap;
#endif
		int firstVert;
		int numVerts;
		int numIndexes;
		int firstIndex;
		int k;
		int triangleBytes;
		srfTriangles_t *tri;

		soup = soups + i * COD1_BSP_TRIANGLE_SOUP_STRIDE;
		material = (int) ( soup[0] | ( soup[1] << 8 ) );
		lightmap = (int)(short)( soup[2] | ( soup[3] << 8 ) );
		source->materialIndex = material;
		source->sourceAtlas = lightmap;
		firstVert = (int) ( (unsigned int)soup[4] |
							 ( (unsigned int)soup[5] << 8 ) |
							 ( (unsigned int)soup[6] << 16 ) |
							 ( (unsigned int)soup[7] << 24 ) );
		numVerts = (int) ( soup[8] | ( soup[9] << 8 ) );
		numIndexes = (int) ( soup[10] | ( soup[11] << 8 ) );
		firstIndex = (int) ( (unsigned int)soup[12] |
							  ( (unsigned int)soup[13] << 8 ) |
							  ( (unsigned int)soup[14] << 16 ) |
							  ( (unsigned int)soup[15] << 24 ) );

		if ( material < 0 || material >= s_worldData.numShaders ||
			 firstVert < 0 || numVerts <= 0 ||
			 firstVert > info->drawVertCount - numVerts ||
			 firstIndex < 0 || numIndexes <= 0 || ( numIndexes % 3 ) ||
			 firstIndex > info->meshVertCount - numIndexes ) {
			ri.Error( ERR_DROP, "CoD1 triangle soup is out of range" );
		}

		source->surfaceFlags=(unsigned)s_worldData.shaders[material].surfaceFlags;
		source->contentFlags=(unsigned)s_worldData.shaders[material].contentFlags;
		source->requestedLightmap=source->effectiveLightmap=
			source->registeredLightmap=LIGHTMAP_NONE;
		source->resolvedShaderIndex=-1;
		source->fromBackup=qfalse;
		source->route=COD1_MATERIAL_UNRESOLVED;
		source->status=COD1_SURFACE_DRAWN;
        s_worldData.cod1MarkFlags[i]=source->surfaceFlags;
		if ( lightmap < 0 || lightmap >= info->lightmapTiles ) {
			lightmap = LIGHTMAP_BY_VERTEX;
		}
#if COD1_NATIVE_LIGHTMAP_PACK
		shaderLightmap = lightmap >= 0 ?
			(int)lightmapLayout->page[lightmap].group : LIGHTMAP_BY_VERTEX;
#endif
		out->viewCount = 0;
		out->fogIndex = 0;
		/* An asset folder is not a visibility flag. In dam, a real metal
		 * quad is authored under textures/common/ with neither no-draw bit.
		 * Keep only explicit compiler surface flags as the early skip rule;
		 * shader registration may still select its own render path. */
		if ( source->surfaceFlags & ( SURF_NODRAW | SURF_SKIP ) ) {
			out->shader = tr.defaultShader;
			out->data = &cod1SkipSurface;
			source->status=COD1_SURFACE_FLAG_SKIP;
			cod1SkippedSurfaces++;
			continue;
		}
		out->shader = ShaderForShaderNum( material,
#if COD1_NATIVE_LIGHTMAP_PACK
			shaderLightmap,
#else
			lightmap,
#endif
			source );
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
		#if COD1_NATIVE_LIGHTMAP_PACK
		if ( cod1LmTraceDam && ( i == 49 || i == 113 ) ) {
			ri.Printf( PRINT_ALL, "CoD1 LM soup=%d sourcePage=%d packedGroup=%d\n",
				i, lightmap, shaderLightmap );
		}
		R_Cod1LmTraceSoup( i, s_worldData.shaders[material].shader,
			shaderLightmap, out->shader );
		#else
		R_Cod1LmTraceSoup( i, s_worldData.shaders[material].shader,
			lightmap, out->shader );
		#endif
#endif
		/* Native 0x4d02cb..0x4d02db binds a failed registration to the
		 * common default material. ShaderForShaderNum already performs that
		 * substitution. A texture directory does not select visibility:
		 * retain valid ordinary materials and the native default fallback,
		 * while accepted sky shaders keep their own sky iterator. */
		if ( out->shader == tr.defaultShader ) {
			source->status=COD1_SURFACE_DEFAULT_SHADER;
			cod1DefaultedSurfaces++;
			if ( !cod1ReportedMaterials[material] ) {
				cod1ReportedMaterials[material] = 1;
				cod1DefaultedMaterials++;
				if ( cod1DefaultedMaterials <= 32 ) {
					ri.Printf( PRINT_WARNING, "CoD1 material defaulted: %s\n",
						s_worldData.shaders[material].shader );
				}
			}
		}
		/* Retail factory/hurtgen/rocket contain soups larger than the RTCW
		 * 256 KiB surface slab. R_GetSurfMemory cannot fit such a request. */
		triangleBytes = sizeof( *tri ) + numVerts * sizeof( tri->verts[0] ) +
			numIndexes * sizeof( tri->indexes[0] );
		tri = triangleBytes > SURF_HUNK_MAXSIZE ?
			ri.Hunk_Alloc( triangleBytes, h_low ) : R_GetSurfMemory( triangleBytes );
		tri->surfaceType = SF_TRIANGLES;
	tri->cod1PrecalcValid = qfalse;
	memset(tri->cod1PrecalcRGBA, 0, sizeof(tri->cod1PrecalcRGBA));
		tri->numVerts = numVerts;
		tri->numIndexes = numIndexes;
		tri->verts = ( drawVert_t * )( tri + 1 );
		tri->indexes = ( int * )( tri->verts + tri->numVerts );
		out->data = (surfaceType_t *)tri;

		ClearBounds( tri->bounds[0], tri->bounds[1] );
		for ( j = 0; j < numVerts; j++ ) {
			const byte *vertex = verts + ( firstVert + j ) * COD1_BSP_DRAWVERT_STRIDE;
			int bits;

			for ( k = 0; k < 3; k++ ) {
				bits = (int) ( (unsigned int)vertex[k * 4] |
								   ( (unsigned int)vertex[k * 4 + 1] << 8 ) |
								   ( (unsigned int)vertex[k * 4 + 2] << 16 ) |
								   ( (unsigned int)vertex[k * 4 + 3] << 24 ) );
				memcpy( &tri->verts[j].xyz[k], &bits, sizeof( float ) );
			}
			for ( k = 0; k < 2; k++ ) {
				bits = (int) ( (unsigned int)vertex[12 + k * 4] |
								   ( (unsigned int)vertex[13 + k * 4] << 8 ) |
								   ( (unsigned int)vertex[14 + k * 4] << 16 ) |
								   ( (unsigned int)vertex[15 + k * 4] << 24 ) );
				memcpy( &tri->verts[j].st[k], &bits, sizeof( float ) );
				bits = (int) ( (unsigned int)vertex[20 + k * 4] |
								   ( (unsigned int)vertex[21 + k * 4] << 8 ) |
								   ( (unsigned int)vertex[22 + k * 4] << 16 ) |
								   ( (unsigned int)vertex[23 + k * 4] << 24 ) );
				memcpy( &tri->verts[j].lightmap[k], &bits, sizeof( float ) );
			}
#if COD1_NATIVE_LIGHTMAP_PACK
			if ( lightmapLayout && lightmap >= 0 ) {
				const bsp59_lm_page_t *placement = &lightmapLayout->page[lightmap];
				tri->verts[j].lightmap[0] = tri->verts[j].lightmap[0] *
					placement->scale_u + placement->offset_u;
				tri->verts[j].lightmap[1] = tri->verts[j].lightmap[1] *
					placement->scale_v + placement->offset_v;
			}
#endif
			for ( k = 0; k < 3; k++ ) {
				bits = (int) ( (unsigned int)vertex[28 + k * 4] |
								   ( (unsigned int)vertex[29 + k * 4] << 8 ) |
								   ( (unsigned int)vertex[30 + k * 4] << 16 ) |
								   ( (unsigned int)vertex[31 + k * 4] << 24 ) );
				memcpy( &tri->verts[j].normal[k], &bits, sizeof( float ) );
			}
			memcpy( tri->verts[j].color, vertex + 40, sizeof( tri->verts[j].color ) );
			AddPointToBounds( tri->verts[j].xyz, tri->bounds[0], tri->bounds[1] );
			AddPointToBounds( tri->verts[j].xyz, bounds[0], bounds[1] );
		}
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
		if ( cod1LmTraceDam && ( i == 49 || i == 113 ) ) {
			ri.Printf( PRINT_ALL,
				"CoD1 LM soup=%d atlas=%d firstVertex=%d count=%d firstLmUV=%g,%g\n",
				i, lightmap, firstVert, numVerts,
				tri->verts[0].lightmap[0], tri->verts[0].lightmap[1] );
		}
#endif

		for ( j = 0; j < numIndexes; j++ ) {
			const byte *index = meshverts + ( firstIndex + j ) * COD1_BSP_MESHVERT_STRIDE;
			tri->indexes[j] = index[0] | ( index[1] << 8 );
			if ( tri->indexes[j] < 0 || tri->indexes[j] >= numVerts ) {
				ri.Error( ERR_DROP, "CoD1 triangle index is out of range" );
			}
		}
	}

	ri.Printf( PRINT_ALL,
		"CoD1 renderer: %d BSP surfaces (world and inline), %d flag/sky skipped, %d defaulted surfaces across %d materials\n",
		info->soupCount, cod1SkippedSurfaces, cod1DefaultedSurfaces, cod1DefaultedMaterials );
	if ( cod1DefaultedMaterials > 32 ) {
		ri.Printf( PRINT_ALL, "CoD1 renderer: %d additional defaulted material names suppressed\n",
			cod1DefaultedMaterials - 32 );
	}
	R_Cod1ReportMaterials( info );
}

/*
================
R_LoadCod1World

Build the native BSP tree and PVS for world/static scenery. Inline models
retain their own surface ranges and are rendered through game entities.
================
*/
/* CoD1 lightmaps are 512-square RGB atlases, not RTCW's 128-square tiles.
 * Keep this upload path separate so RTCW maps retain their original behavior. */
#if COD1_NATIVE_LIGHTMAP_RGB
/* The live CoDSP 0x4CFAB0 RGB arithmetic for effective bits 0/1; 0x4CFF30
 * overrides alpha to 255. Unsupported values are rejected before upload. */
static void R_Cod1NativeLightmapRGB( const byte in[3], byte out[4], int effectiveBits ) {
	int r, g, b, max;
	r = in[0] << ( 1 - effectiveBits );
	g = in[1] << ( 1 - effectiveBits );
	b = in[2] << ( 1 - effectiveBits );
	if ( ( r | g | b ) > 255 ) {
		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}
	out[0] = r; out[1] = g; out[2] = b; out[3] = 255;
}
#endif
static void R_LoadCod1Lightmaps( const cod1Bsp_t *bsp ) {
	static byte rgba[COD1_BSP_LIGHTMAP_SIZE * COD1_BSP_LIGHTMAP_SIZE * 4];
	const byte *data, *pixel;
	int length, count, i, j;
	data = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_LIGHTMAPS, &length );
	count = length / COD1_BSP_LIGHTMAP_TILE_BYTES;
	if ( length % COD1_BSP_LIGHTMAP_TILE_BYTES || count > MAX_LIGHTMAPS ) {
		ri.Error( ERR_DROP, "CoD1: invalid lightmap atlas count/size" );
		return;
	}
	tr.numLightmaps = count;
	if ( !count || r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
		if ( cod1LmTraceDam ) ri.Printf( PRINT_ALL,
			"CoD1 LM skipped count=%d r_vertexLight=%d hardwareType=%d mapOver=%d effectiveOver=%d\n",
			count, r_vertexLight->integer, glConfig.hardwareType,
			r_mapOverBrightBits->integer, tr.overbrightBits );
#endif
		return;
	}
	R_SyncRenderThread();
#if COD1_NATIVE_LIGHTMAP_RGB
	if ( tr.overbrightBits != 0 && tr.overbrightBits != 1 ) {
		ri.Error( ERR_DROP,
			"CoD1 LM: native RGB conversion unsupported effectiveOver=%d",
			tr.overbrightBits );
		return;
	}
#endif
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
	if ( cod1LmTraceDam ) ri.Printf( PRINT_ALL,
		"CoD1 LM cvars mapOver=%d requestedOver=%d ignoreHWGamma=%d gamma=%g effectiveOver=%d fullscreen=%d deviceGamma=%d r_lightmap=%d r_vertexLight=%d\n",
		r_mapOverBrightBits->integer, r_overBrightBits->integer,
		r_ignorehwgamma->integer, r_gamma->value, tr.overbrightBits,
		glConfig.isFullscreen, glConfig.deviceSupportsGamma,
		r_lightmap->integer, r_vertexLight->integer );
#endif
	for ( i = 0; i < count; i++ ) {
		pixel = data + i * COD1_BSP_LIGHTMAP_TILE_BYTES;
		for ( j = 0; j < COD1_BSP_LIGHTMAP_SIZE * COD1_BSP_LIGHTMAP_SIZE; j++ ) {
#if COD1_NATIVE_LIGHTMAP_RGB
			R_Cod1NativeLightmapRGB( pixel + j * 3, rgba + j * 4, tr.overbrightBits );
#else
			byte source[4];
			/* Same brightness transform as the existing renderer, with opaque alpha. */
			memcpy( source, pixel + j * 3, 3 );
			source[3] = 255;
			R_ColorShiftLightingBytes( source, rgba + j * 4 );
			rgba[j * 4 + 3] = 255;
#endif
		}
		#if COD1_NATIVE_LIGHTMAP_GL
		tr.lightmaps[i] = R_CreateCod1LightmapImage( va("*lightmap%d", i), rgba,
			COD1_BSP_LIGHTMAP_SIZE, COD1_BSP_LIGHTMAP_SIZE );
		#else
		tr.lightmaps[i] = R_CreateImage( va("*lightmap%d", i), rgba,
			COD1_BSP_LIGHTMAP_SIZE, COD1_BSP_LIGHTMAP_SIZE, qfalse, qfalse, GL_CLAMP );
		#endif
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
		if ( cod1LmTraceDam && ( i == 0 || i == 12 ) ) {
			const image_t *image = tr.lightmaps[i];
			static const int points[3][2] = { { 256, 256 }, { 301, 217 }, { 128, 128 } };
			int sample;
			for ( sample = 0; sample < 3; sample++ ) {
				int x = points[sample][0], y = points[sample][1];
				const byte *source = pixel + ( y * COD1_BSP_LIGHTMAP_SIZE + x ) * 3;
				const byte *out = rgba + ( y * COD1_BSP_LIGHTMAP_SIZE + x ) * 4;
#if COD1_NATIVE_LIGHTMAP_RGB
				ri.Printf( PRINT_ALL,
					"CoD1 LM atlas=%d xy=%d,%d raw=%u,%u,%u native=%u,%u,%u,%u effectiveOver=%d mode=%s\n",
					i, x, y, source[0], source[1], source[2],
					out[0], out[1], out[2], out[3], tr.overbrightBits,
					"live RGB arithmetic" );
#else
				ri.Printf( PRINT_ALL,
					"CoD1 LM atlas=%d xy=%d,%d raw=%u,%u,%u shifted=%u,%u,%u,%u mapOver=%d effectiveOver=%d shift=%d\n",
					i, x, y, source[0], source[1], source[2],
					out[0], out[1], out[2], out[3],
					r_mapOverBrightBits->integer, tr.overbrightBits,
					r_mapOverBrightBits->integer - tr.overbrightBits );
#endif
			}
			ri.Printf( PRINT_ALL,
				"CoD1 LM image atlas=%d name=%s request=%dx%d rgbaBytes=%d mipmap=0 picmip=0 wrap=%d stored=%dx%d upload=%dx%d internalFormat=%d tmu=%d\n",
				i, image ? image->imgName : "(none)", COD1_BSP_LIGHTMAP_SIZE,
				COD1_BSP_LIGHTMAP_SIZE, COD1_BSP_LIGHTMAP_SIZE * COD1_BSP_LIGHTMAP_SIZE * 4,
				GL_CLAMP, image ? image->width : 0, image ? image->height : 0,
				image ? image->uploadWidth : 0, image ? image->uploadHeight : 0,
				image ? image->internalFormat : 0, image ? image->TMU : -1 );
		}
#endif
	}
	ri.Printf( PRINT_ALL, "CoD1: uploaded %d lightmap atlases, 512 x 512 RGB\n", count );
}

#if COD1_NATIVE_LIGHTMAP_PACK
/* Plan from every BSP soup, including inline models. The source page selects
 * a placement; the RTCW shader key selects the resulting packed group. */
static void R_LoadCod1PackedLightmaps( const cod1Bsp_t *bsp,
		const cod1BspWorldInfo_t *info, bsp59_lm_layout_t *layout ) {
	const byte *soups, *data;
	bsp59_lm_soup_t *records;
	byte *rgba;
	unsigned maxBytes = 0;
	char error[192];
	int soupLength, length, i, j, y, x;

	memset( layout, 0, sizeof( *layout ) );
	soups = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_TRIANGLE_SOUPS, &soupLength );
	data = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_LIGHTMAPS, &length );
	if ( !soups || ( !data && info->lightmapTiles ) || info->soupCount < 0 ||
		info->soupCount > INT_MAX / COD1_BSP_TRIANGLE_SOUP_STRIDE ||
		soupLength != info->soupCount * COD1_BSP_TRIANGLE_SOUP_STRIDE ||
		length != info->lightmapTiles * COD1_BSP_LIGHTMAP_TILE_BYTES ||
		info->soupCount > INT_MAX / (int)sizeof( *records ) ) {
		ri.Error( ERR_DROP, "CoD1: malformed soup/lightmap lumps for packed upload" );
		return;
	}
	records = ri.Hunk_AllocateTempMemory( info->soupCount * sizeof( *records ) );
	if ( !records ) {
		ri.Error( ERR_DROP, "CoD1: cannot allocate lightmap soup records" );
		return;
	}
	for ( i = 0; i < info->soupCount; i++ ) {
		const byte *soup = soups + i * COD1_BSP_TRIANGLE_SOUP_STRIDE;
		records[i].material = (int)( soup[0] | ( soup[1] << 8 ) );
		records[i].page = (int)(short)( soup[2] | ( soup[3] << 8 ) );
		records[i].vertices = (int)( soup[8] | ( soup[9] << 8 ) );
	}
	if ( !bsp59_lm_plan( records, (size_t)info->soupCount,
		(unsigned)info->materialCount, (unsigned)info->lightmapTiles,
		glConfig.maxTextureSize, layout, error, sizeof( error ) ) ) {
		ri.Hunk_FreeTempMemory( records );
		ri.Error( ERR_DROP, "CoD1: cannot plan native lightmaps: %s", error );
		return;
	}
	ri.Hunk_FreeTempMemory( records );
	tr.numLightmaps = (int)layout->group_count;
	if ( !tr.numLightmaps || r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		return;
	}
	/* Preflight every group before creating the first GL image. The direct
	 * upload path currently handles power-of-two groups only. */
	if ( tr.overbrightBits != 0 && tr.overbrightBits != 1 ) {
		ri.Error( ERR_DROP, "CoD1: unsupported effective overbright %d", tr.overbrightBits );
		return;
	}
	if ( atof( glConfig.version_string ) < 1.2 &&
		!strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) &&
		!strstr( glConfig.extensions_string, "GL_SGIS_texture_edge_clamp" ) ) {
		ri.Error( ERR_DROP, "CoD1: GL_CLAMP_TO_EDGE unavailable" );
		return;
	}
	if ( layout->group_count > MAX_DRAWIMAGES - tr.numImages ) {
		ri.Error( ERR_DROP, "CoD1: packed lightmaps exceed image registry" );
		return;
	}
	for ( i = 0; i < (int)layout->group_count; i++ ) {
		const bsp59_lm_group_t *group = &layout->groups[i];
		if ( !group->pixel_width || !group->pixel_height ||
			( group->pixel_width & ( group->pixel_width - 1 ) ) ||
			( group->pixel_height & ( group->pixel_height - 1 ) ) ||
			group->pixel_width > (unsigned)glConfig.maxTextureSize ||
			group->pixel_height > (unsigned)glConfig.maxTextureSize ||
			group->pixel_width > (unsigned)INT_MAX / 4u / group->pixel_height ) {
			ri.Error( ERR_DROP, "CoD1: packed lightmap group %d has unsupported dimensions %u x %u",
				i, group->pixel_width, group->pixel_height );
			return;
		}
		if ( group->pixel_width * group->pixel_height * 4u > maxBytes ) {
			maxBytes = group->pixel_width * group->pixel_height * 4u;
		}
		ri.Printf( PRINT_ALL, "CoD1 LM group=%d size=%ux%u pages=%u\n",
			i, group->pixel_width, group->pixel_height, group->slot_count );
	}
	for ( i = 0; i < (int)layout->derived_pages; i++ ) {
		const bsp59_lm_page_t *placement = &layout->page[i];
		ri.Printf( PRINT_DEVELOPER, "CoD1 LM sourcePage=%d group=%u tile=%u,%u UV scale=%g,%g offset=%g,%g\n",
			i, placement->group, placement->tile_x, placement->tile_y,
			placement->scale_u, placement->scale_v,
			placement->offset_u, placement->offset_v );
	}
	rgba = ri.Hunk_AllocateTempMemory( (int)maxBytes );
	if ( !rgba ) {
		ri.Error( ERR_DROP, "CoD1: cannot allocate packed lightmap pixels" );
		return;
	}
	R_SyncRenderThread();
	for ( i = 0; i < (int)layout->group_count; i++ ) {
		const bsp59_lm_group_t *group = &layout->groups[i];
		int width = (int)group->pixel_width;
		int height = (int)group->pixel_height;
		image_t *image;
		for ( j = 0; j < (int)group->slot_count; j++ ) {
			unsigned page = layout->page_order[group->first_slot + j];
			const bsp59_lm_page_t *placement = &layout->page[page];
			const byte *source = data + page * COD1_BSP_LIGHTMAP_TILE_BYTES;
			byte *row = rgba + ( ( placement->tile_y * COD1_BSP_LIGHTMAP_SIZE * width +
				placement->tile_x * COD1_BSP_LIGHTMAP_SIZE ) * 4 );
			for ( y = 0; y < COD1_BSP_LIGHTMAP_SIZE; y++ ) {
				for ( x = 0; x < COD1_BSP_LIGHTMAP_SIZE; x++ ) {
					R_Cod1NativeLightmapRGB( source + x * 3, row + x * 4,
						tr.overbrightBits );
				}
				source += COD1_BSP_LIGHTMAP_SIZE * 3;
				row += width * 4;
			}
		}
		image = R_CreateCod1LightmapImage( va( "*lightmap%d", i ), rgba, width, height );
		if ( !image ) {
			ri.Hunk_FreeTempMemory( rgba );
			ri.Error( ERR_DROP, "CoD1: cannot create packed lightmap group %d", i );
			return;
		}
		tr.lightmaps[i] = image;
	}
	ri.Hunk_FreeTempMemory( rgba );
	ri.Printf( PRINT_ALL, "CoD1: packed %d source lightmap pages into %d images (GL limit %d)\n",
		(int)layout->derived_pages, (int)layout->group_count, glConfig.maxTextureSize );
}
#endif

#include "tr_cod1_static.h"
#include "tr_cod1_spatial.h"

#if (defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50) || \
    (defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA)
/* CoDSP 0x4d26a5..0x4d26d6 parses worldspawn sundirection with AngleVectors.
 * 0x4d35d0..0x4d35fb establishes this default. This is a separate water input,
 * NOT a change to static XModel light/cache/default/ambient calculations. */
static void R_Cod1WorldSunDirection(const cod1Bsp_t *bsp,vec3_t direction,qboolean *valid) {
    const byte *raw;char *copy,*text,*t,key[MAX_TOKEN_CHARS],extra;int len,k;
    vec3_t angles;
    VectorSet(direction,.45f,.3f,.9f);VectorNormalize(direction);
    (*valid)=qtrue;
    raw=COD1_BSP_LumpData(bsp,COD1_BSP_LUMP_ENTITIES,&len);
    if(!raw||len<2||raw[len-1]){(*valid)=qfalse;return;}
    copy=(char*)malloc(len);if(!copy){(*valid)=qfalse;return;}
    memcpy(copy,raw,len);text=copy;t=COM_ParseExt(&text,qtrue);
    if(strcmp(t,"{")){free(copy);(*valid)=qfalse;return;}
    while(text){t=COM_ParseExt(&text,qtrue);if(!*t||!strcmp(t,"}"))break;
        Q_strncpyz(key,t,sizeof(key));t=COM_ParseExt(&text,qtrue);
        if(Q_stricmp(key,"sundirection"))continue;
        if(sscanf(t,"%f %f %f %c",&angles[0],&angles[1],&angles[2],&extra)!=3){(*valid)=qfalse;break;}
        for(k=0;k<3;k++)if(!(angles[k]>=-360000.0f&&angles[k]<=360000.0f))(*valid)=qfalse;
        if(!(*valid))break;
        AngleVectors(angles,direction,NULL,NULL);
    }
    free(copy);
}
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
static void R_Cod1WaterWorldDirection(const cod1Bsp_t *bsp) {
    R_Cod1WorldSunDirection(bsp,tr.cod1WaterSunDirection,&tr.cod1WaterSunValid);
    ri.Printf(PRINT_ALL,"CoD1 R50 water sun input: valid=%d direction=(%.8g %.8g %.8g); model lighting unchanged\n",
      tr.cod1WaterSunValid,tr.cod1WaterSunDirection[0],tr.cod1WaterSunDirection[1],tr.cod1WaterSunDirection[2]);
}
#endif
#endif

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#include <float.h>
/* Native 4d196e stores every type-1 pointer; the last record wins. Its raw
 * direction at offsets 28/32/36 drives flare projection, independently of
 * worldspawn disk/angular direction. Do not normalize or borrow model light. */
static void R_Cod1SunPrimary(const cod1Bsp_t *bsp) {
    const byte *raw;
    unsigned int bits;
    int length,offset,k;
    tr.cod1SunPrimaryValid=qfalse;
    raw=COD1_BSP_LumpData(bsp,30,&length);
    if(!raw || length%72)return;
    for(offset=0;offset<length;offset+=72) {
        memcpy(&bits,raw+offset,4);bits=(unsigned int)LittleLong((int)bits);
        if(bits!=1u)continue;
        tr.cod1SunPrimaryValid=qtrue;
        for(k=0;k<3;k++) {
            memcpy(&bits,raw+offset+28+k*4,4);bits=(unsigned int)LittleLong((int)bits);
            memcpy(&tr.cod1SunPrimaryDirection[k],&bits,4);
            if(!(tr.cod1SunPrimaryDirection[k]>=-FLT_MAX &&
                 tr.cod1SunPrimaryDirection[k]<=FLT_MAX))tr.cod1SunPrimaryValid=qfalse;
        }
    }
}
#endif

static void R_LoadCod1World( const char *name, const cod1Bsp_t *bsp, int fileLength ) {
	cod1BspWorldInfo_t info;
#if COD1_NATIVE_LIGHTMAP_PACK
	bsp59_lm_layout_t lightmapLayout;
#endif
	cod1BspSpatial_t spatial;
	cod1BspRenderCells_t renderCells;
	lump_t lump;
	const byte *entities;
	int entitiesLength, i, staticFirst, staticCount;
	const byte *models;
	int modelLength;
	vec3_t bounds[2];
	model_t *model;

	/* Mark the world before any render frame is emitted.  Patch 07 skips CoD
	 * sky triangle soups until their native sky shader syntax is implemented;
	 * the backend needs to know that uncovered pixels must be cleared. */
	s_worldData.isCod1 = qtrue;
#if COD1_LIGHTMAP_NUMERIC_DIAGNOSTICS
	cod1LmTraceDam = R_Cod1LmTraceDamName( name );
#endif

	if ( !COD1_BSP_GetWorldInfo( bsp, &info ) || info.materialCount <= 0 ||
		 !COD1_BSP_GetSpatial( bsp, &spatial ) ||
		 !COD1_BSP_GetRenderCells( bsp, &info, &spatial, &renderCells ) ) {
		ri.Error( ERR_DROP, "R_LoadCod1World: invalid CoD1 world data in %s", name );
	}

#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
    R_Cod1WaterWorldDirection(bsp);
#endif
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    R_Cod1WorldSunDirection(bsp,tr.cod1SunWorldDirection,&tr.cod1SunWorldDirectionValid);
    R_Cod1SunPrimary(bsp);
#endif
	lump.fileofs = bsp->lumps[COD1_BSP_LUMP_MATERIALS].fileofs;
	lump.filelen = bsp->lumps[COD1_BSP_LUMP_MATERIALS].filelen;
	R_LoadShaders( &lump );
	for ( i = 0; i < s_worldData.numShaders; i++ ) {
		if ( !memchr( s_worldData.shaders[i].shader, 0,
				sizeof( s_worldData.shaders[i].shader ) ) ) {
			ri.Error( ERR_DROP, "CoD1 BSP: unterminated material name at record %d", i );
			return;
		}
	}
#if COD1_NATIVE_LIGHTMAP_PACK
	R_LoadCod1PackedLightmaps( bsp, &info, &lightmapLayout );
#else
	R_LoadCod1Lightmaps( bsp );
#endif
	/* Every inline model keeps its original soup range. Only *0 and misc_model
	 * surfaces enter the world leaf, so movers are not also drawn at rest. */
	models = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_MODELS, &modelLength );
	if ( !models || info.modelCount > MAX_INLINE_MODELS ) {
		ri.Error( ERR_DROP, "CoD1 renderer: unsupported inline model count %d", info.modelCount );
		return;
	}
#if COD1_NATIVE_LIGHTMAP_PACK
	cod1PackedWorldShaders = qtrue;
#endif
	R_LoadCod1Surfaces( bsp, &info,
#if COD1_NATIVE_LIGHTMAP_PACK
		&lightmapLayout,
#endif
		bounds );
#if COD1_NATIVE_LIGHTMAP_PACK
	cod1PackedWorldShaders = qfalse;
#endif
	if ( !COD1_BSP_GetModelBounds( bsp, 0, bounds[0], bounds[1] ) ) {
		ri.Error( ERR_DROP, "CoD1 renderer: invalid world bounds" );
		return;
	}
	staticFirst = s_worldData.numsurfaces;
	R_LoadCod1StaticTrees( bsp, bounds );
	staticCount = s_worldData.numsurfaces - staticFirst;
	s_worldData.bmodels = ri.Hunk_Alloc( info.modelCount * sizeof( *s_worldData.bmodels ), h_low );
	for ( i = 0; i < info.modelCount; i++ ) {
		const byte *record = models + i * COD1_BSP_MODEL_STRIDE;
		unsigned int first, count;
		bmodel_t *bm = s_worldData.bmodels + i;
		first = (unsigned int)record[24] | ((unsigned int)record[25] << 8) |
			((unsigned int)record[26] << 16) | ((unsigned int)record[27] << 24);
		count = (unsigned int)record[28] | ((unsigned int)record[29] << 8) |
			((unsigned int)record[30] << 16) | ((unsigned int)record[31] << 24);
		if ( first > (unsigned int)info.soupCount || count > (unsigned int)info.soupCount - first ||
			!COD1_BSP_GetModelBounds( bsp, i, bm->bounds[0], bm->bounds[1] ) ) {
			ri.Error( ERR_DROP, "CoD1 renderer: model *%d has invalid geometry range", i );
			return;
		}
		bm->firstSurface = s_worldData.surfaces + first;
		bm->numSurfaces = count;
		model = R_AllocModel();
		if ( !model ) {
			ri.Error( ERR_DROP, "R_LoadCod1World: model table is full at *%d", i );
			return;
		}
		model->type = MOD_BRUSH;
		model->bmodel = bm;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );
	}
	VectorCopy( bounds[0], s_worldData.bmodels[0].bounds[0] );
	VectorCopy( bounds[1], s_worldData.bmodels[0].bounds[1] );
	ri.Printf( PRINT_ALL, "PATCH65 CoD1 renderer: %d brush models, %d world soups, %d static surfaces\n",
		info.modelCount, info.worldSoupCount, staticCount );

	R_LoadCod1SpatialWorld( &spatial, &renderCells, &info, staticFirst, staticCount );

	s_worldData.numfogs = 1;
	s_worldData.fogs = ri.Hunk_Alloc( sizeof( *s_worldData.fogs ), h_low );

	entities = COD1_BSP_LumpData( bsp, COD1_BSP_LUMP_ENTITIES, &entitiesLength );
	if ( !entities || entitiesLength < 2 || entities[entitiesLength - 1] != '\0' ) {
		ri.Error( ERR_DROP, "R_LoadCod1World: entity lump is not terminated" );
	}
	lump.fileofs = (int)( entities - fileBase );
	lump.filelen = entitiesLength;
	R_LoadEntities( &lump );

	/* CoD1 uses native visibility records, not RTCW RGB lightgrid bytes.
     * Static XModels consume them in tr_cod1_lighting.h. Dynamic entities
     * still use RTCW's no-grid fallback until their lighting path is ported. */
    s_worldData.lightGridData = NULL;
	s_worldData.dataSize = fileLength;
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	const char  *s;

	s = COM_Parse( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name ) {
	int i;
	int fileLength;
	dheader_t   *header;
	byte        *buffer;
	byte        *startMarker;
	cod1Bsp_t cod1;

	skyboxportal = 0;

	if ( tr.worldMapLoaded ) {
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}
	R_SyncRenderThread();
	R_Cod1ModelLightingReset();
	/* Clear the previous map's format before FS or shader work can fail. */
	cod1Bsp59MapActive = qfalse;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    R_Cod1SunWorldReset();
#endif

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45;
	tr.sunDirection[1] = 0.3;
	tr.sunDirection[2] = 0.9;

	tr.sunShader = 0;   // clear sunshader so it's not there if the level doesn't specify it

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW,0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT,   0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET,    0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER,     0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER,    0, 0, 0, 0, 0, 0 );

	VectorNormalize( tr.sunDirection );

	tr.worldMapLoaded = qtrue;

	// load it
	fileLength = ri.FS_ReadFile( name, (void **)&buffer );
	if ( !buffer ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;
#if COD1_NATIVE_LIGHTMAP_PACK
	cod1PackedWorldShaders = qfalse;
#endif

	memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension( s_worldData.baseName, s_worldData.baseName );

	startMarker = ri.Hunk_Alloc( 0, h_low );
	c_gridVerts = 0;

	header = (dheader_t *)buffer;
	fileBase = (byte *)header;

	/* CoD1 keeps the IBSP magic but uses version 59 and a 33-lump layout.
	   Route it before the legacy Q3 header is interpreted as dheader_t. */
	if ( COD1_BSP_Parse( buffer, fileLength, &cod1 ) ) {
		/* Shader registration happens inside R_LoadCod1World, before tr.world
		 * points at the loaded world. Keep the format selection for subsequent
		 * non-world material registrations on this map as well. */
		cod1Bsp59MapActive = qtrue;
		R_LoadCod1World( name, &cod1, fileLength );
		s_worldData.dataSize = fileLength;
		tr.world = &s_worldData;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
        R_Cod1SunLoadWorld();
#endif
		ri.FS_FreeFile( buffer );
		return;
	}

	i = LittleLong( header->version );
#ifndef _SKIP_BSP_CHECK
	if ( i != BSP_VERSION ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)",
				  name, i, BSP_VERSION );
	}
#endif

	// swap all the lumps
	for ( i = 0 ; i < sizeof( dheader_t ) / 4 ; i++ ) {
		( (int *)header )[i] = LittleLong( ( (int *)header )[i] );
	}

	// load into heap
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadShaders( &header->lumps[LUMP_SHADERS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadLightmaps( &header->lumps[LUMP_LIGHTMAPS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadPlanes( &header->lumps[LUMP_PLANES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadFogs( &header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadMarksurfaces( &header->lumps[LUMP_LEAFSURFACES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadNodesAndLeafs( &header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadSubmodels( &header->lumps[LUMP_MODELS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadEntities( &header->lumps[LUMP_ENTITIES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// reset fog to world fog (if present)
//	R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0);

//----(SA)	set the sun shader if there is one
	if ( tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, qtrue );
	}

//----(SA)	end
	ri.FS_FreeFile( buffer );
}
