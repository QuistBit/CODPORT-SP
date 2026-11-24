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


#include "qfiles.h"
#include "../game/cod1_navigation_resource.h"
/* Resident ENGINE observer; never a retained GAME function pointer. */
void CM_Cod1NavigationSetObserver(void (*observer)(uint64_t));
int CM_Cod1NavigationResource(const cod1NavigationRequest_t *,cod1NavigationResource_t *,void *,uint32_t);

#include "../cgame/tr_types.h"

void        CM_ClearMap(void);
void        CM_LoadMap( const char *name, qboolean clientload, int *checksum );
clipHandle_t CM_InlineModel( int index );       // 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule );
/* R103 native query hull. No conversion of CoD contents to RTCW BODY. */
clipHandle_t CM_TempBoxModelContents( const vec3_t mins, const vec3_t maxs, int capsule, int contents );
int CM_TempModelContents( void );
qboolean CM_TempModelHasNativeContents( void );

void        CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

int         CM_NumClusters( void );
int         CM_NumInlineModels( void );
char        *CM_EntityString( void );

// returns an ORed contents mask
int         CM_PointContents( const vec3_t p, clipHandle_t model );
int         CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );

void        CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						 const vec3_t mins, const vec3_t maxs,
						 clipHandle_t model, int brushmask, int capsule );
/* R56 renderer bake queries: native BSP59 geometry only, mask 0x2001.
 * Return false when no native world is loaded; no legacy triangle substitution. */
qboolean CM_Cod1LightTrace(trace_t *out,const vec3_t start,const vec3_t end);
/* R103 authored CoD collision only; raw mask, no player-clip diagnostic
 * override and no render-triangle substitution. Unsupported shapes fail. */
qboolean CM_HasV59(void);
qboolean CM_Cod1PathBrushSight(const vec3_t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,unsigned,int*);
qboolean CM_Cod1PathNativeSight(const vec3_t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,unsigned,int*);
qboolean CM_Cod1NativeWorldTrace(trace_t *out, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, int mask, int capsule);
qboolean CM_PointContentsV59(const vec3_t point,int model,int *out);
void        CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
									const vec3_t mins, const vec3_t maxs,
									clipHandle_t model, int brushmask,
									const vec3_t origin, const vec3_t angles, int capsule );
void CM_Cod1NativeTransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, clipHandle_t model, int mask,
    const vec3_t origin, const vec3_t angles, int capsule );

byte        *CM_ClusterPVS( int cluster );

int         CM_PointLeafnum( const vec3_t p );

// only returns non-solid leafs
// overflow if return listsize and if *lastLeaf != list[listsize-1]
int         CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list,
							int listsize, int *lastLeaf );

int         CM_LeafCluster( int leafnum );
int         CM_LeafArea( int leafnum );

void        CM_AdjustAreaPortalState( int area1, int area2, qboolean open );
qboolean    CM_AreasConnected( int area1, int area2 );

int         CM_WriteAreaBits( byte *buffer, int area );

// cm_tag.c
int         CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );


// cm_marks.c
int CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
					  int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

// cm_patch.c
void CM_DrawDebugSurface( void ( *drawPoly )( int color, int numPoints, float *points ) );
