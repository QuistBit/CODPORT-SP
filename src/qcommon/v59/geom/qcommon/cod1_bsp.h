/*
===========================================================================

CoD1 BSP probe used by the RTCW port.

This file only describes the retail Call of Duty 1 IBSP 59 container.  It
does not replace the RTCW/Quake 3 BSP loader; keeping the probe separate lets
the renderer and collision backends be added without weakening the existing
version and bounds checks.

===========================================================================
*/

#ifndef COD1_BSP_H
#define COD1_BSP_H

#include "../game/q_shared.h"

#define COD1_BSP_VERSION       59
#define COD1_BSP_LUMP_COUNT    33
#define COD1_BSP_HEADER_SIZE   ( 8 + COD1_BSP_LUMP_COUNT * 8 )

/* Lump ids used by the first renderer/collision adapter. */
#define COD1_BSP_LUMP_MATERIALS       0
#define COD1_BSP_LUMP_LIGHTMAPS       1
#define COD1_BSP_LUMP_TRIANGLE_SOUPS  6
#define COD1_BSP_LUMP_DRAWVERTS       7
#define COD1_BSP_LUMP_MESHVERTS       8
#define COD1_BSP_LUMP_MODELS         27
#define COD1_BSP_LUMP_ENTITIES       29

#define COD1_BSP_MATERIAL_STRIDE      72
#define COD1_BSP_TRIANGLE_SOUP_STRIDE 16
#define COD1_BSP_DRAWVERT_STRIDE      44
#define COD1_BSP_MESHVERT_STRIDE       2
#define COD1_BSP_MODEL_STRIDE         48
#define COD1_BSP_LIGHTMAP_SIZE        512
#define COD1_BSP_LIGHTMAP_TILE_BYTES  ( COD1_BSP_LIGHTMAP_SIZE * COD1_BSP_LIGHTMAP_SIZE * 3 )

typedef struct {
	int fileofs;
	int filelen;
} cod1BspLump_t;

typedef struct {
	const byte *base;
	int length;
	cod1BspLump_t lumps[COD1_BSP_LUMP_COUNT];
} cod1Bsp_t;

typedef struct {
	int materialCount;
	int lightmapTiles;
	int soupCount;
	int drawVertCount;
	int meshVertCount;
	int worldFirstSoup;
	int worldSoupCount;
	int modelCount;
	const byte *entities;
	int entitiesLength;
} cod1BspWorldInfo_t;

typedef struct {
	qboolean valid;
	char classname[64];
	vec3_t origin;
	vec3_t angles;
} cod1BspSpawn_t;

/* Parse and bounds-check an entire CoD1 header/lump table. */
qboolean COD1_BSP_Parse( const void *data, int length, cod1Bsp_t *out );

/* Return a validated lump pointer, or NULL for an empty/invalid lump id. */
const byte *COD1_BSP_LumpData( const cod1Bsp_t *bsp, int lumpId, int *length );

/* Gather the fixed-stride counts and the world model surface range. */
qboolean COD1_BSP_GetWorldInfo( const cod1Bsp_t *bsp, cod1BspWorldInfo_t *out );

/* Read bounds for a world/inline model from the CoD1 model lump. */
qboolean COD1_BSP_GetModelBounds( const cod1Bsp_t *bsp, int modelIndex, vec3_t mins, vec3_t maxs );

/* Locate the entity text block, accepting tools that moved it from lump 29. */
qboolean COD1_BSP_FindEntities( const cod1Bsp_t *bsp, const byte **data, int *length );

/* Select the highest-priority usable player spawn from the entity text. */
qboolean COD1_BSP_FindPlayerStart( const cod1Bsp_t *bsp, cod1BspSpawn_t *out );

#endif /* COD1_BSP_H */
