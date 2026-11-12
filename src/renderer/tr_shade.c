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

// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
================
R_ArrayElementDiscrete

This is just for OpenGL conformance testing, it should never be the fastest
================
*/
static void APIENTRY R_ArrayElementDiscrete( GLint index ) {
	qglColor4ubv( tess.svars.colors[ index ] );
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	if ( glState.currenttmu == 2 ) {
		int b;
		for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
			qglMultiTexCoord2fARB( GL_TEXTURE0_ARB + b,
				tess.svars.texcoords[b][index][0], tess.svars.texcoords[b][index][1] );
		}
	} else
#endif
	if ( glState.currenttmu ) {
		qglMultiTexCoord2fARB( 0, tess.svars.texcoords[ 0 ][ index ][0], tess.svars.texcoords[ 0 ][ index ][1] );
		qglMultiTexCoord2fARB( 1, tess.svars.texcoords[ 1 ][ index ][0], tess.svars.texcoords[ 1 ][ index ][1] );
	} else {
		qglTexCoord2fv( tess.svars.texcoords[ 0 ][ index ] );
	}
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	if ( tess.shader && tess.shader->cod1SunDirectional ) {
		qglVertex4fv( tess.xyz[ index ] );
	} else
#endif
	{
		qglVertex3fv( tess.xyz[ index ] );
	}
}

/*
===================
R_DrawStripElements

===================
*/
static int c_vertexes;          // for seeing how long our average strips are
static int c_begins;
static void R_DrawStripElements( int numIndexes, const glIndex_t *indexes, void ( APIENTRY *element )( GLint ) ) {
	int i;
	int last[3] = { -1, -1, -1 };
	qboolean even;

	c_begins++;

	if ( numIndexes <= 0 ) {
		return;
	}

	qglBegin( GL_TRIANGLE_STRIP );

	// prime the strip
	element( indexes[0] );
	element( indexes[1] );
	element( indexes[2] );
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for ( i = 3; i < numIndexes; i += 3 )
	{
		// odd numbered triangle in potential strip
		if ( !even ) {
			// check previous triangle to see if we're continuing a strip
			if ( ( indexes[i + 0] == last[2] ) && ( indexes[i + 1] == last[1] ) ) {
				element( indexes[i + 2] );
				c_vertexes++;
				assert( indexes[i + 2] < tess.numVertexes );
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i + 0] );
				element( indexes[i + 1] );
				element( indexes[i + 2] );

				c_vertexes += 3;

				even = qfalse;
			}
		} else
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( last[2] == indexes[i + 1] ) && ( last[0] == indexes[i + 0] ) ) {
				element( indexes[i + 2] );
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i + 0] );
				element( indexes[i + 1] );
				element( indexes[i + 2] );
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i + 0];
		last[1] = indexes[i + 1];
		last[2] = indexes[i + 2];
	}

	qglEnd();
}



/*
==================
R_DrawElements

Optionally performs our own glDrawElements that looks for strip conditions
instead of using the single glDrawElements call that may be inefficient
without compiled vertex arrays.
==================
*/
static void R_DrawElements( int numIndexes, const glIndex_t *indexes ) {
	int primitives;

	primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if ( primitives == 0 ) {
		if ( qglLockArraysEXT ) {
			primitives = 2;
		} else {
			primitives = 1;
		}
	}


	if ( primitives == 2 ) {
		qglDrawElements( GL_TRIANGLES,
						 numIndexes,
						 GL_INDEX_TYPE,
						 indexes );
		return;
	}

	if ( primitives == 1 ) {
		R_DrawStripElements( numIndexes,  indexes, qglArrayElement );
		return;
	}

	if ( primitives == 3 ) {
		R_DrawStripElements( numIndexes,  indexes, R_ArrayElementDiscrete );
		return;
	}

	// anything else will cause no drawing
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;
static qboolean setArraysOnce;

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage( textureBundle_t *bundle ) {
	int index;

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic( bundle->videoMapHandle );
		ri.CIN_UploadCinematic( bundle->videoMapHandle );
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		if ( bundle->isLightmap && ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) ) {
			GL_Bind( tr.whiteImage );
		} else {
			GL_Bind( bundle->image[0] );
		}
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = myftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;  // may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	if ( bundle->isLightmap && ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) ) {
		GL_Bind( tr.whiteImage );
	} else {
		GL_Bind( bundle->image[ index ] );
	}
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris( shaderCommands_t *input ) {
	qboolean arraysLocked = qfalse;

	GL_Bind( tr.whiteImage );
	qglColor3f( 1,1,1 );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	if ( r_showtris->integer == 1 ) {
		qglDepthRange( 0, 0 );
	}

	qglDisableClientState( GL_COLOR_ARRAY );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz ); // padded for SIMD

	// Cloudless sky can leave zero vertices; CVA requires a positive count.
	if ( input->numVertexes > 0 && qglLockArraysEXT && qglUnlockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		arraysLocked = qtrue;
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	if ( arraysLocked ) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}
	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals( shaderCommands_t *input ) {
	int i;
	vec3_t temp;

	GL_Bind( tr.whiteImage );
	qglColor3f( 1,1,1 );

	if ( r_shownormals->integer == 1 ) {
		qglDepthRange( 0, 0 );  // never occluded
	}
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	qglBegin( GL_LINES );
	for ( i = 0 ; i < input->numVertexes ; i++ ) {
		qglVertex3fv( input->xyz[i] );
		VectorMA( input->xyz[i], 2, input->normal[i], temp );
		qglVertex3fv( temp );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

	shader_t *state = ( shader->remappedShader ) ? shader->remappedShader : shader;

	tess.ATI_tess = qfalse;     //----(SA)	added
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;        // will be OR'd in by surface functions
	tess.cod1PreparedModelDiffuse = qfalse;
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if ( tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime ) {
		tess.shaderTime = tess.shader->clampTime;
	}
	// done.
}

#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
static void Cod1StageFogForDraw( const shaderStage_t *stage, unsigned long stateBits );
static qboolean Cod1StageFogSceneAvailable( void );
#endif
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
static void Cod1FogStateProbeDraw( const shaderStage_t *stage );
static void Cod1FogSnapshotDraw( const shaderStage_t *stage, int stageIndex, int iteratorKind );
#endif

#if defined(COD1_WATER_GL_DRAW_PROBE) && COD1_WATER_GL_DRAW_PROBE
/* Read-only diagnostic of the state that reaches a water draw. Never call
 * GL_SelectTexture here: sample unit 0 while the renderer has it selected,
 * then unit 1 at the draw. The hardware state, not glState's cache, is read. */
#ifndef GL_BLEND_SRC
#define GL_BLEND_SRC 0x0BE1
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_BLEND_DST
#define GL_BLEND_DST 0x0BE0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_DEPTH_WRITEMASK
#define GL_DEPTH_WRITEMASK 0x0B72
#endif
#ifndef GL_DEPTH_FUNC
#define GL_DEPTH_FUNC 0x0B74
#endif
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif
#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif

typedef struct {
	int selectedUnit;
	int enabled;
	int binding;
	int envMode;
} cod1WaterProbeTexture_t;

static int cod1WaterProbeDraws;

static qboolean Cod1WaterProbeWanted( void ) {
	return cod1WaterProbeDraws < 128 && tess.shader &&
	       !strcmp( tess.shader->name, "textures/sfx/damwater" ) &&
	       tess.numIndexes > 0 && r_primitives &&
	       r_primitives->integer >= 0 && r_primitives->integer <= 3 &&
	       ri.Printf && qglIsEnabled && qglGetBooleanv && qglGetFloatv &&
	       qglGetIntegerv && qglGetTexEnviv;
}

static void Cod1WaterProbeTexture( cod1WaterProbeTexture_t *sample ) {
	GLint value;
	sample->selectedUnit = glState.currenttmu;
	sample->enabled = qglIsEnabled( GL_TEXTURE_2D ) ? 1 : 0;
	qglGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
	sample->binding = value;
	qglGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &value );
	sample->envMode = value;
}

static void Cod1WaterProbeDraw( int stage, int indexes,
		const cod1WaterProbeTexture_t *base ) {
	cod1WaterProbeTexture_t active;
	GLfloat color[4], start, end, density;
	GLboolean writeDepth;
	GLint mode, src, dst, depthFunc;
	shaderStage_t *chosen;
	const char *image0, *image1;

	if ( !Cod1WaterProbeWanted() ) {
		return;
	}
	chosen = tess.xstages[stage];
	Cod1WaterProbeTexture( &active );
	if ( !base ) {
		base = &active;
	}
	qglGetIntegerv( GL_FOG_MODE, &mode );
	qglGetFloatv( GL_FOG_COLOR, color );
	qglGetFloatv( GL_FOG_START, &start );
	qglGetFloatv( GL_FOG_END, &end );
	qglGetFloatv( GL_FOG_DENSITY, &density );
	qglGetIntegerv( GL_BLEND_SRC, &src );
	qglGetIntegerv( GL_BLEND_DST, &dst );
	qglGetIntegerv( GL_DEPTH_FUNC, &depthFunc );
	qglGetBooleanv( GL_DEPTH_WRITEMASK, &writeDepth );
	image0 = chosen->bundle[0].image[0] ? chosen->bundle[0].image[0]->imgName : "(none)";
	image1 = chosen->bundle[1].image[0] ? chosen->bundle[1].image[0]->imgName : "(none)";
	ri.Printf( PRINT_ALL,
		"COD1_WATER_GL_DRAW frame=%d draw=%d shader=%s stage=%d indexes=%d "
		"fog=%d fogMode=0x%x fogColor=%g,%g,%g,%g fogStart=%g fogEnd=%g fogDensity=%g "
		"blend=%d blendSrc=0x%x blendDst=0x%x depthTest=%d depthWrite=%d depthFunc=0x%x "
		"unit0=%d,%d,%d,0x%x active=%d,%d,%d,0x%x image0=%s image1=%s\n",
		tr.frameCount, cod1WaterProbeDraws++, tess.shader->name, stage, indexes,
		qglIsEnabled( GL_FOG ) ? 1 : 0, mode,
		color[0], color[1], color[2], color[3], start, end, density,
		qglIsEnabled( GL_BLEND ) ? 1 : 0, src, dst,
		qglIsEnabled( GL_DEPTH_TEST ) ? 1 : 0, writeDepth ? 1 : 0, depthFunc,
		base->selectedUnit, base->enabled, base->binding, base->envMode,
		active.selectedUnit, active.enabled, active.binding, active.envMode,
		image0, image1 );
}
#endif

/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void DrawMultitextured( shaderCommands_t *input, int stage ) {
	shaderStage_t   *pStage;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	qboolean thirdBundle;
#endif
#if defined(COD1_WATER_GL_DRAW_PROBE) && COD1_WATER_GL_DRAW_PROBE
	cod1WaterProbeTexture_t waterBase;
	qboolean waterBaseValid = qfalse;
#endif

	pStage = tess.xstages[stage];
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	thirdBundle = pStage->bundle[2].image[0] != NULL;
	if ( thirdBundle && ( glConfig.maxActiveTextures < 3 ||
		 !qglActiveTextureARB || !qglClientActiveTextureARB ) ) {
		ri.Error( ERR_DROP, "CoD1 ordinary third bundle: texture unit unavailable" );
		return;
	}
#endif

	// Ridah
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
	if ( pStage->cod1NativeFogColor && !Cod1StageFogSceneAvailable() ) {
		/* R_FogOn checks glfogNum, but not RDF_NOWORLDMODEL. Do not
		 * undo SetIteratorFog's explicit off decision for opt-in stages. */
		R_FogOff();
	} else
#endif
	if ( tess.shader->noFog && pStage->isFogged ) {
		R_FogOn();
	} else if ( tess.shader->noFog && !pStage->isFogged ) {
		R_FogOff(); // turn it back off
	} else {    // make sure it's on
		R_FogOn();
	}
	// done.

	GL_State( pStage->stateBits );

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if ( backEnd.viewParms.isPortal ) {
		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	R_BindAnimatedImage( &pStage->bundle[0] );
#if defined(COD1_WATER_GL_DRAW_PROBE) && COD1_WATER_GL_DRAW_PROBE
	if ( Cod1WaterProbeWanted() ) {
		Cod1WaterProbeTexture( &waterBase );
		waterBaseValid = qtrue;
	}
#endif

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	/* The lightmap debug override applies to lightmaps, never to a regular
	 * second authored texture. Keep ordinary RTCW handling unchanged. */
	if ( (tess.shader->cod1ShaderType || tess.shader->cod1StaticModelSource
#if defined(COD1_WATER_FIXED2_CANDIDATE) && COD1_WATER_FIXED2_CANDIDATE
          || tess.shader->cod1WaterFixed2Candidate
#endif
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
          || tess.shader->cod1Bsp59WorldMaterialSource
#endif
          ) &&
	     !pStage->bundle[1].isLightmap && pStage->cod1TexEnv ) {
		GL_TexEnv( pStage->cod1TexEnv );
	} else
#endif
	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
		GL_TexEnv( pStage->cod1TexEnv ? pStage->cod1TexEnv : tess.shader->multitextureEnv );
#else
		GL_TexEnv( tess.shader->multitextureEnv );
#endif
	}

	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

	R_BindAnimatedImage( &pStage->bundle[1] );

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	if ( thirdBundle ) {
		GL_SelectTexture( 2 );
		qglEnable( GL_TEXTURE_2D );
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		GL_TexEnv( r_lightmap->integer && pStage->bundle[2].isLightmap ?
			GL_REPLACE : ( pStage->cod1TexEnv2 ? pStage->cod1TexEnv2 : GL_MODULATE ) );
		qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[2] );
		R_BindAnimatedImage( &pStage->bundle[2] );
	}
#endif

#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
	Cod1StageFogForDraw( pStage, glState.glStateBits );
#endif
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
	Cod1FogStateProbeDraw( pStage );
	Cod1FogSnapshotDraw( pStage, stage, 1 );
#endif
#if defined(COD1_WATER_GL_DRAW_PROBE) && COD1_WATER_GL_DRAW_PROBE
	Cod1WaterProbeDraw( stage, input->numIndexes,
		waterBaseValid ? &waterBase : NULL );
#endif
	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable downstream texturing, then select TEXTURE0
	//
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	if ( thirdBundle ) {
		/* Keep unit two neutral and disabled for the following two-unit draw.
		 * Its actual image binding and renderer cache remain coherent. */
		GL_TexEnv( GL_MODULATE );
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 1 );
	}
#endif
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable( GL_TEXTURE_2D );

	GL_SelectTexture( 0 );
}



/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture( void ) {
	int i, l;
	vec3_t origin;
	float   *texCoords;
	byte    *colors;
	byte clipBits[SHADER_MAX_VERTEXES];
	MAC_STATIC float texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte colorArray[SHADER_MAX_VERTEXES][4];
	unsigned hitIndexes[SHADER_MAX_INDEXES];
	int numIndexes;
	float scale;
	float radius;
	vec3_t floatColor;

	/* Model diffuse already contains its selected scene lights. */
	if ( tess.cod1PreparedModelDiffuse ) return;
	if ( !backEnd.refdef.num_dlights ) {
		return;
	}


	if ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {  // no dlights for snooper
		return;
	}


	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
		dlight_t    *dl;

		if ( !( tess.dlightBits & ( 1u << l ) ) ) {
			continue;   // this surface definately doesn't have any of this light
		}
		texCoords = texCoordsArray[0];
		colors = colorArray[0];

		dl = &backEnd.refdef.dlights[l];
		VectorCopy( dl->transformed, origin );
		radius = dl->radius;
		scale = 1.0f / radius;
		floatColor[0] = dl->color[0] * 255.0f;
		floatColor[1] = dl->color[1] * 255.0f;
		floatColor[2] = dl->color[2] * 255.0f;

		for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4 ) {
			vec3_t dist;
			int clip;
			float modulate;

			if ( 0 ) {
				clipBits[i] = 255;  // definately not dlighted
				continue;
			}

			VectorSubtract( origin, tess.xyz[i], dist );

//			if(!r_dlightBacks->integer) {
//				vec3_t	dir;
//				VectorNormalize2(dist, dir);
//				if( DotProduct( tess.normal[i], dir) < 0) {
//					clipBits[i] = 255;	// not lighted (backface)
//					continue;
//				}
//			}

			backEnd.pc.c_dlightVertexes++;

			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			} else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			} else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}
			// modulate the strength based on the height and color
			if ( dist[2] > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist[2] < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist[2] = Q_fabs( dist[2] );
				if ( dist[2] < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * ( radius - dist[2] ) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = myftol( floatColor[0] * modulate );
			colors[1] = myftol( floatColor[1] * modulate );
			colors[2] = myftol( floatColor[2] * modulate );
			colors[3] = 255;
		}

		// build a list of triangles that need light
		numIndexes = 0;
		for ( i = 0 ; i < tess.numIndexes ; i += 3 ) {
			int a, b, c;

			a = tess.indexes[i];
			b = tess.indexes[i + 1];
			c = tess.indexes[i + 2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;   // not lighted
			}

//			if(!r_dlightBacks->integer) {
//				vec3_t	dir;
//				VectorSubtract( origin, tess.xyz[a], dir );
//				VectorNormalize(dir);
//				if( DotProduct( tess.normal[i], dir) < 0) {
//					continue;	// not lighted (backface)
//				}
//			}

			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes + 1] = b;
			hitIndexes[numIndexes + 2] = c;
			numIndexes += 3;
		}

		if ( !numIndexes ) {
			continue;
		}

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		//----(SA) creating dlight shader to allow for special blends or alternate dlight texture
		{
			shader_t *dls = dl->dlshader;
			if ( dls ) {
//				if (!qglActiveTextureARB || dls->numUnfoggedPasses < 2) {
				for ( i = 0; i < dls->numUnfoggedPasses; i++ )
				{
					shaderStage_t *stage = dls->stages[i];
					R_BindAnimatedImage( &dls->stages[i]->bundle[0] );
					GL_State( stage->stateBits | GLS_DEPTHFUNC_EQUAL );
					R_DrawElements( numIndexes, hitIndexes );
					backEnd.pc.c_totalIndexes += numIndexes;
					backEnd.pc.c_dlightIndexes += numIndexes;
				}
/*
				} else {	// optimize for multitexture

					for(i=0;i<dls->numUnfoggedPasses;)
					{
						shaderStage_t *stage = dls->stages[i];

						GL_State(stage->stateBits | GLS_DEPTHFUNC_EQUAL);

						// setup each TMU
						for (tmu=0; tmu<glConfig.maxActiveTextures && i<dls->numUnfoggedPasses; tmu++, i++) {

							GL_SelectTexture( tmu );

							if (tmu) {
								qglEnable( GL_TEXTURE_2D );
							}

							R_BindAnimatedImage( &dls->stages[i]->bundle[0] );
						}

						// draw the elements
						R_DrawElements( numIndexes, hitIndexes );
						backEnd.pc.c_totalIndexes += numIndexes;
						backEnd.pc.c_dlightIndexes += numIndexes;
					}

					// turn off unused TMU's
					for (tmu=1; tmu<glConfig.maxActiveTextures; tmu++) {
						// set back to default state
						GL_SelectTexture( tmu );
						qglDisable( GL_TEXTURE_2D );
					}

					// return to TEXTURE0
					GL_SelectTexture( 0 );
				}
*/
			} else
			{
				R_FogOff();

//				if (!dl->overdraw || !qglActiveTextureARB) {
				GL_Bind( tr.dlightImage );
				// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
				// where they aren't rendered
				GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
				R_DrawElements( numIndexes, hitIndexes );
				backEnd.pc.c_totalIndexes += numIndexes;
				backEnd.pc.c_dlightIndexes += numIndexes;

				// Ridah, overdraw lights several times, rather than sending
				//	multiple lights through
				for ( i = 0; i < dl->overdraw; i++ ) {
					R_DrawElements( numIndexes, hitIndexes );
					backEnd.pc.c_totalIndexes += numIndexes;
					backEnd.pc.c_dlightIndexes += numIndexes;
				}
/*
				} else {	// optimize for multitexture

					GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );

					// setup each TMU (use all available TMU's)
					for (tmu=0; tmu<glConfig.maxActiveTextures && tmu<(dl->overdraw+1); tmu++) {
						GL_SelectTexture( tmu );
						if (tmu) {
							qglEnable( GL_TEXTURE_2D );
							GL_TexEnv( GL_ADD );
							GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
							qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
							qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );
							qglEnableClientState( GL_COLOR_ARRAY );
							qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );
						}
						GL_Bind( tr.dlightImage );
					}

					// draw each bundle
					for(i=0; i<(dl->overdraw+1); i+=glConfig.maxActiveTextures)
					{
						// make sure we dont draw with too many TMU's
						if (i+glConfig.maxActiveTextures>(dl->overdraw+1)) {
							for (tmu=0; tmu<glConfig.maxActiveTextures; tmu++) {
								if (tmu+i>=(dl->overdraw+1)) {
									GL_SelectTexture( tmu );
									qglDisable( GL_TEXTURE_2D );
								}
							}
						}
						// draw the elements
						R_DrawElements( numIndexes, hitIndexes );
						backEnd.pc.c_totalIndexes += numIndexes;
						backEnd.pc.c_dlightIndexes += numIndexes;
					}

					// turn off unused TMU's
					for (tmu=1; tmu<glConfig.maxActiveTextures; tmu++) {
						// set back to default state
						GL_SelectTexture( tmu );
						qglDisable( GL_TEXTURE_2D );
					}

					// return to TEXTURE0
					GL_SelectTexture( 0 );
				}
*/
				R_FogOn();
			}
		}
	}
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
	fog_t       *fog;
	int i;

	if ( tr.refdef.rdflags & RDF_SNOOPERVIEW ) { // no fog pass in snooper
		return;
	}

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );

	fog = tr.world->fogs + tess.fogNum;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		*( int * )&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );
}

/*
===============
ComputeColors
===============
*/
static void ComputeColors( shaderStage_t *pStage ) {
	int i;
    qboolean cod1PrecalcConstant=qfalse;

	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
	case CGEN_IDENTITY:
		memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
		break;
	default:
	case CGEN_IDENTITY_LIGHTING:
		memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
		break;
	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );
		break;
	case CGEN_EXACT_VERTEX:
		memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
		break;
	case CGEN_CONST:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
		}
		break;
	case CGEN_VERTEX:
		if ( tr.identityLight == 1 ) {
			memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
		} else
		{
			for ( i = 0; i < tess.numVertexes; i++ )
			{
				tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
				tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
				tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case CGEN_ONE_MINUS_VERTEX:
		if ( tr.identityLight == 1 ) {
			for ( i = 0; i < tess.numVertexes; i++ )
			{
				tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
				tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
				tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
			}
		} else
		{
			for ( i = 0; i < tess.numVertexes; i++ )
			{
				tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
				tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
				tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
			}
		}
		break;
	case CGEN_FOG:
	{
		fog_t       *fog;

		fog = tr.world->fogs + tess.fogNum;

		for ( i = 0; i < tess.numVertexes; i++ ) {
			*( int * )&tess.svars.colors[i] = fog->colorInt;
		}
	}
	break;
	case CGEN_WAVEFORM:
		RB_CalcWaveColor( &pStage->rgbWave, ( unsigned char * ) tess.svars.colors );
		break;
	case CGEN_COD1_PRECALC:
        if (tess.shader->cod1StaticModelSource) {
            memcpy(tess.svars.colors,tess.cod1PrecalcColors,
                   tess.numVertexes*sizeof(tess.cod1PrecalcColors[0]));
        } else {
            cod1PrecalcConstant=RB_CalcCod1PrecalcModelColor(pStage,(unsigned char*)tess.svars.colors);
        }
        break;
	case CGEN_ENTITY:
		RB_CalcColorFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
	}

	//
	// alphaGen
	//
	if(!cod1PrecalcConstant)switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
		// Ridah
	case AGEN_NORMALZFADE:
	{
		float alpha, range, lowest, highest, dot;
		vec3_t worldUp;
		qboolean zombieEffect = qfalse;

		if ( VectorCompare( backEnd.currentEntity->e.fireRiseDir, vec3_origin ) ) {
			VectorSet( backEnd.currentEntity->e.fireRiseDir, 0, 0, 1 );
		}

		if ( backEnd.currentEntity->e.hModel ) {    // world surfaces dont have an axis
			VectorRotate( backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp );
		} else {
			VectorCopy( backEnd.currentEntity->e.fireRiseDir, worldUp );
		}

		lowest = pStage->zFadeBounds[0];
		if ( lowest == -1000 ) {    // use entity alpha
			lowest = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		highest = pStage->zFadeBounds[1];
		if ( highest == -1000 ) {   // use entity alpha
			highest = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		range = highest - lowest;
		for ( i = 0; i < tess.numVertexes; i++ ) {
			dot = DotProduct( tess.normal[i], worldUp );

			// special handling for Zombie fade effect
			if ( zombieEffect ) {
				alpha = (float)backEnd.currentEntity->e.shaderRGBA[3] * ( dot + 1.0 ) / 2.0;
				alpha += ( 2.0 * (float)backEnd.currentEntity->e.shaderRGBA[3] ) * ( 1.0 - ( dot + 1.0 ) / 2.0 );
				if ( alpha > 255.0 ) {
					alpha = 255.0;
				} else if ( alpha < 0.0 ) {
					alpha = 0.0;
				}
				tess.svars.colors[i][3] = (byte)( alpha );
				continue;
			}

			if ( dot < highest ) {
				if ( dot > lowest ) {
					if ( dot < lowest + range / 2 ) {
						alpha = ( (float)pStage->constantColor[3] * ( ( dot - lowest ) / ( range / 2 ) ) );
					} else {
						alpha = ( (float)pStage->constantColor[3] * ( 1.0 - ( ( dot - lowest - range / 2 ) / ( range / 2 ) ) ) );
					}
					if ( alpha > 255.0 ) {
						alpha = 255.0;
					} else if ( alpha < 0.0 ) {
						alpha = 0.0;
					}

					// finally, scale according to the entity's alpha
					if ( backEnd.currentEntity->e.hModel ) {
						alpha *= (float)backEnd.currentEntity->e.shaderRGBA[3] / 255.0;
					}

					tess.svars.colors[i][3] = (byte)( alpha );
				} else {
					tess.svars.colors[i][3] = 0;
				}
			} else {
				tess.svars.colors[i][3] = 0;
			}
		}
	}
	break;
		// done.
	case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case AGEN_ONE_MINUS_VERTEX:
		for ( i = 0; i < tess.numVertexes; i++ )
		{
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
		}
		break;
	case AGEN_PORTAL:
	{
		unsigned char alpha;

		for ( i = 0; i < tess.numVertexes; i++ )
		{
			float len;
			vec3_t v;

			VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
			len = VectorLength( v );

			len /= tess.shader->portalRange;

			if ( len < 0 ) {
				alpha = 0;
			} else if ( len > 1 )   {
				alpha = 0xff;
			} else
			{
				alpha = len * 0xff;
			}

			tess.svars.colors[i][3] = alpha;
		}
	}
	break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum ) {
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords( shaderStage_t *pStage ) {
	int i;
	int b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
		case TCGEN_IDENTITY:
			memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
			break;
		case TCGEN_TEXTURE:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
			}
			break;
		case TCGEN_LIGHTMAP:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
			}
			break;
		case TCGEN_VECTOR:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
				tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
			}
			break;
		case TCGEN_FOG:
			RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
			break;
		case TCGEN_FIRERISEENV_MAPPED:
			RB_CalcFireRiseEnvTexCoords( ( float * ) tess.svars.texcoords[b] );
			break;
		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;        // break out of for loop
				break;

			case TMOD_SWAP:
				RB_CalcSwapTexCoords( ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave,
										   ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
										( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll,
										( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale,
									   ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave,
										 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm],
										   ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed,
										( float * ) tess.svars.texcoords[b] );
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}



extern void R_Fog( glfog_t *curfog );

#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
/* Opt-in r_speeds 5 diagnostics read actual backend GL state at a world
 * draw. They do not change fog parameters or consume the GL error queue. */
static void Cod1FogStateProbeDraw( const shaderStage_t *stage ) {
	static qboolean logged;
	static int lastTime;
	GLint mode, hint, distanceMode;
	GLfloat density, start, end, color[4];
	int time;

	if ( !cod1Bsp59MapActive || !r_speeds || r_speeds->integer != 5 ||
		 !stage || !tess.shader ||
		 backEnd.projection2D || backEnd.viewParms.isPortal ||
		 ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL | RDF_DRAWINGSKY |
								 RDF_SKYBOXPORTAL ) ) ) {
		return;
	}
	time = backEnd.refdef.time;
	if ( logged && time >= lastTime && (double)time - (double)lastTime < 1000.0 ) {
		return;
	}
	logged = qtrue;
	lastTime = time;
	qglGetIntegerv( GL_FOG_MODE, &mode );
	qglGetIntegerv( GL_FOG_HINT, &hint );
	qglGetFloatv( GL_FOG_DENSITY, &density );
	qglGetFloatv( GL_FOG_START, &start );
	qglGetFloatv( GL_FOG_END, &end );
	qglGetFloatv( GL_FOG_COLOR, color );
	distanceMode = 0;
	if ( glConfig.NVFogAvailable ) {
		qglGetIntegerv( GL_FOG_DISTANCE_MODE_NV, &distanceMode );
	}
	ri.Printf( PRINT_ALL,
		"CoD1 fog draw: shader=%s enabled=%i cache=%i mode=0x%x den=%.9g "
		"near=%.9g far=%.9g GLrgb=(%.6g %.6g %.6g) "
		"rawRGB=(%.6g %.6g %.6g) identity=%.6g overbright=%i "
		"fullscreen=%i hwGamma=%i hint=0x%x NVdistance=0x%x "
		"noFog=%i stageFog=%i blend=0x%lx view=(%.3f %.3f %.3f) zFar=%.3f\n",
		tess.shader->name, qglIsEnabled( GL_FOG ) ? 1 : 0, fogIsOn,
		(unsigned int)mode, density, start, end, color[0], color[1], color[2],
		glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1],
		glfogsettings[FOG_CURRENT].color[2], tr.identityLight, tr.overbrightBits,
		glConfig.isFullscreen, glConfig.deviceSupportsGamma, (unsigned int)hint,
		(unsigned int)distanceMode, tess.shader->noFog, stage->isFogged,
		glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ),
		backEnd.refdef.vieworg[0], backEnd.refdef.vieworg[1],
		backEnd.refdef.vieworg[2], backEnd.viewParms.zFar );
}

/* r_speeds 7 observes one main-world frame. The immutable draw tuple is
 * sampled for each distinct shader/stage/iterator/effective blend; depth
 * bounds include every submitted batch of that tuple in the frame. Nothing
 * here enables, binds, modifies or clears OpenGL state. */
#define COD1_FOG_SNAPSHOT_CAP 128
#ifndef GL_FOG_COORDINATE_SOURCE_EXT
#define GL_FOG_COORDINATE_SOURCE_EXT 0x8450
#endif
#ifndef GL_CURRENT_FOG_COORDINATE_EXT
#define GL_CURRENT_FOG_COORDINATE_EXT 0x8453
#endif
#ifndef GL_FOG_COORDINATE_ARRAY_EXT
#define GL_FOG_COORDINATE_ARRAY_EXT 0x8457
#endif

typedef struct {
	const shader_t *shaderKey;
	const shaderStage_t *stageKey;
	int stageIndex, iteratorKind;
	unsigned long blend;
	char shaderName[MAX_QPATH];
	char images[3][MAX_QPATH];
	int passes, rgbGen, alphaGen, nativeFog, noFog, stageFog;
	int draws, vertices;
	GLint enabled, cache, mode, hint, distanceMode;
	GLfloat density, start, end, color[4];
	GLint coordSource, coordArray;
	GLfloat coordCurrent;
	GLfloat eyeMin, eyeMax;
} cod1FogSnapshotEntry_t;

typedef struct {
	/* 0 waits for a world draw, 1 collects, 2 has already been printed. */
	int phase, count, droppedDraws;
	int frame, scene, time;
	qboolean coordAvailable;
	vec3_t view;
	float zFar, identity;
	int overbright, fullscreen, hwGamma;
	int fogNum, wolfFog, uiFullscreen;
	glfog_t currentFog, targetFog;
	vec3_t rawColor;
	cod1FogSnapshotEntry_t entries[COD1_FOG_SNAPSHOT_CAP];
} cod1FogSnapshot_t;

static cod1FogSnapshot_t cod1FogSnapshot;

static qboolean Cod1FogSnapshotMode( void ) {
	if ( !r_speeds || r_speeds->integer != 7 ) {
		if ( cod1FogSnapshot.phase ) {
			memset( &cod1FogSnapshot, 0, sizeof( cod1FogSnapshot ) );
		}
		return qfalse;
	}
	return qtrue;
}

static qboolean Cod1FogSnapshotCoordAvailable( void ) {
	const char *version, *extensions, *found;
	const char *token = "GL_EXT_fog_coord";
	int major = 0, minor = 0;
	size_t length = strlen( token );

	version = (const char *)qglGetString( GL_VERSION );
	if ( version && sscanf( version, "%d.%d", &major, &minor ) == 2 &&
		 ( major > 1 || ( major == 1 && minor >= 4 ) ) ) {
		return qtrue;
	}
	/* GL 1.1 supports querying its extension string, but not fog-coordinate
 	 * enums. Require a complete advertised token before querying those. */
	extensions = (const char *)qglGetString( GL_EXTENSIONS );
	if ( !extensions ) {
		return qfalse;
	}
	for ( found = strstr( extensions, token ); found;
		  found = strstr( found + length, token ) ) {
		if ( ( found == extensions || found[-1] == ' ' ) &&
			 ( !found[length] || found[length] == ' ' ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void Cod1FogSnapshotDraw( const shaderStage_t *stage, int stageIndex, int iteratorKind ) {
	cod1FogSnapshotEntry_t *entry = NULL;
	GLfloat matrix[16], depth;
	unsigned long blend;
	int i, b;

	if ( !Cod1FogSnapshotMode() || cod1FogSnapshot.phase == 2 ||
		 !cod1Bsp59MapActive || !stage || !tess.shader ||
		 tess.numVertexes <= 0 || tess.numIndexes <= 0 ||
		 backEnd.projection2D || backEnd.viewParms.isPortal ||
		 ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL | RDF_DRAWINGSKY |
								 RDF_SKYBOXPORTAL ) ) ) {
		return;
	}
	if ( !cod1FogSnapshot.phase ) {
		cod1FogSnapshot.phase = 1;
		cod1FogSnapshot.frame = backEnd.viewParms.frameCount;
		cod1FogSnapshot.scene = backEnd.viewParms.frameSceneNum;
		cod1FogSnapshot.time = backEnd.refdef.time;
		VectorCopy( backEnd.refdef.vieworg, cod1FogSnapshot.view );
		cod1FogSnapshot.zFar = backEnd.viewParms.zFar;
		cod1FogSnapshot.identity = tr.identityLight;
		cod1FogSnapshot.overbright = tr.overbrightBits;
		cod1FogSnapshot.fullscreen = glConfig.isFullscreen;
		cod1FogSnapshot.hwGamma = glConfig.deviceSupportsGamma;
		cod1FogSnapshot.fogNum = glfogNum;
		cod1FogSnapshot.wolfFog = r_wolffog ? r_wolffog->integer : -1;
		cod1FogSnapshot.uiFullscreen = r_uiFullScreen ? r_uiFullScreen->integer : -1;
		memcpy( &cod1FogSnapshot.currentFog, &glfogsettings[FOG_CURRENT],
			sizeof( cod1FogSnapshot.currentFog ) );
		memcpy( &cod1FogSnapshot.targetFog, &glfogsettings[FOG_TARGET],
			sizeof( cod1FogSnapshot.targetFog ) );
		VectorCopy( glfogsettings[FOG_CURRENT].color, cod1FogSnapshot.rawColor );
		cod1FogSnapshot.coordAvailable = Cod1FogSnapshotCoordAvailable();
	} else if ( cod1FogSnapshot.frame != backEnd.viewParms.frameCount ||
				cod1FogSnapshot.scene != backEnd.viewParms.frameSceneNum ||
				cod1FogSnapshot.time != backEnd.refdef.time ) {
		/* An extra view must not add a different scene to the first snapshot. */
		return;
	}
	blend = glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
	for ( i = 0; i < cod1FogSnapshot.count; ++i ) {
		entry = &cod1FogSnapshot.entries[i];
		if ( entry->shaderKey == tess.shader && entry->stageKey == stage &&
			 entry->stageIndex == stageIndex && entry->iteratorKind == iteratorKind &&
			 entry->blend == blend ) {
			break;
		}
	}
	if ( i == cod1FogSnapshot.count ) {
		if ( cod1FogSnapshot.count == COD1_FOG_SNAPSHOT_CAP ) {
			++cod1FogSnapshot.droppedDraws;
			return;
		}
		entry = &cod1FogSnapshot.entries[cod1FogSnapshot.count++];
		memset( entry, 0, sizeof( *entry ) );
		entry->shaderKey = tess.shader;
		entry->stageKey = stage;
		entry->stageIndex = stageIndex;
		entry->iteratorKind = iteratorKind;
		entry->blend = blend;
		Q_strncpyz( entry->shaderName, tess.shader->name, sizeof( entry->shaderName ) );
		for ( b = 0; b < 3; ++b ) {
			/* Record compiled bundle metadata without changing active texture
			 * units. The first authored image is not a GL binding assertion. */
			Q_strncpyz( entry->images[b],
				b < NUM_TEXTURE_BUNDLES && stage->bundle[b].image[0] ?
				stage->bundle[b].image[0]->imgName : "(none)", sizeof( entry->images[b] ) );
		}
		entry->passes = tess.shader->numUnfoggedPasses;
		entry->rgbGen = stage->rgbGen;
		entry->alphaGen = stage->alphaGen;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
		entry->nativeFog = stage->cod1NativeFogColor;
#endif
		entry->noFog = tess.shader->noFog;
		entry->stageFog = stage->isFogged;
		entry->enabled = qglIsEnabled( GL_FOG ) ? 1 : 0;
		entry->cache = fogIsOn;
		qglGetIntegerv( GL_FOG_MODE, &entry->mode );
		qglGetIntegerv( GL_FOG_HINT, &entry->hint );
		qglGetFloatv( GL_FOG_DENSITY, &entry->density );
		qglGetFloatv( GL_FOG_START, &entry->start );
		qglGetFloatv( GL_FOG_END, &entry->end );
		qglGetFloatv( GL_FOG_COLOR, entry->color );
		if ( glConfig.NVFogAvailable ) {
			qglGetIntegerv( GL_FOG_DISTANCE_MODE_NV, &entry->distanceMode );
		}
		if ( cod1FogSnapshot.coordAvailable ) {
			qglGetIntegerv( GL_FOG_COORDINATE_SOURCE_EXT, &entry->coordSource );
			qglGetFloatv( GL_CURRENT_FOG_COORDINATE_EXT, &entry->coordCurrent );
			entry->coordArray = qglIsEnabled( GL_FOG_COORDINATE_ARRAY_EXT ) ? 1 : 0;
		}
	}

	qglGetFloatv( GL_MODELVIEW_MATRIX, matrix );
	for ( i = 0; i < tess.numVertexes; ++i ) {
		/* All four iterator vertex pointers submit three components, so the
		 * affine input w is 1 even when the padding in tess.xyz[][3] is 0. */
		depth = -( matrix[2] * tess.xyz[i][0] + matrix[6] * tess.xyz[i][1] +
				   matrix[10] * tess.xyz[i][2] + matrix[14] );
		if ( !entry->vertices && !i ) {
			entry->eyeMin = entry->eyeMax = depth;
		} else {
			if ( depth < entry->eyeMin ) entry->eyeMin = depth;
			if ( depth > entry->eyeMax ) entry->eyeMax = depth;
		}
	}
	++entry->draws;
	entry->vertices += tess.numVertexes;
}

void RB_Cod1FogSnapshotFlush( void ) {
	static const char *iteratorNames[4] = {
		"generic", "generic_multitexture", "vertexlit", "lightmapped"
	};
	cod1FogSnapshotEntry_t *entry;
	int i;

	if ( !Cod1FogSnapshotMode() ) {
		return;
	}
	if ( !cod1Bsp59MapActive ) {
		/* Do not report an interrupted native view after a map/context
		 * change. A legacy-only frame never consumes the armed snapshot. */
		if ( cod1FogSnapshot.phase == 1 ) {
			memset( &cod1FogSnapshot, 0, sizeof( cod1FogSnapshot ) );
		}
		return;
	}
	if ( cod1FogSnapshot.phase != 1 || !cod1FogSnapshot.count ) {
		return;
	}
	cod1FogSnapshot.phase = 2;
	ri.Printf( PRINT_ALL,
		"CoD1 fog snapshot: frame=%i scene=%i time=%i entries=%i cap=%i "
		"truncated=%i droppedDraws=%i view=(%.3f %.3f %.3f) zFar=%.3f "
		"identity=%.6g overbright=%i fullscreen=%i hwGamma=%i "
		"rawRGB=(%.6g %.6g %.6g) fogCoords=%i\n",
		cod1FogSnapshot.frame, cod1FogSnapshot.scene, cod1FogSnapshot.time,
		cod1FogSnapshot.count, COD1_FOG_SNAPSHOT_CAP,
		cod1FogSnapshot.droppedDraws ? 1 : 0, cod1FogSnapshot.droppedDraws,
		cod1FogSnapshot.view[0], cod1FogSnapshot.view[1], cod1FogSnapshot.view[2],
		cod1FogSnapshot.zFar, cod1FogSnapshot.identity, cod1FogSnapshot.overbright,
		cod1FogSnapshot.fullscreen, cod1FogSnapshot.hwGamma,
		cod1FogSnapshot.rawColor[0], cod1FogSnapshot.rawColor[1],
		cod1FogSnapshot.rawColor[2], cod1FogSnapshot.coordAvailable );
	ri.Printf( PRINT_ALL,
		"CoD1 fog scene: selected=%i wolfFog=%i uiFullscreen=%i "
		"currentRegistered=%i currentMode=0x%x currentDen=%.9g "
		"currentNear=%.9g currentFar=%.9g currentRGB=(%.6g %.6g %.6g) currentDrawSky=%i "
		"targetRegistered=%i targetMode=0x%x targetDen=%.9g "
		"targetNear=%.9g targetFar=%.9g targetRGB=(%.6g %.6g %.6g) targetDrawSky=%i\n",
		cod1FogSnapshot.fogNum, cod1FogSnapshot.wolfFog, cod1FogSnapshot.uiFullscreen,
		cod1FogSnapshot.currentFog.registered, (unsigned int)cod1FogSnapshot.currentFog.mode,
		cod1FogSnapshot.currentFog.density, cod1FogSnapshot.currentFog.start,
		cod1FogSnapshot.currentFog.end, cod1FogSnapshot.currentFog.color[0],
		cod1FogSnapshot.currentFog.color[1], cod1FogSnapshot.currentFog.color[2],
		cod1FogSnapshot.currentFog.drawsky, cod1FogSnapshot.targetFog.registered,
		(unsigned int)cod1FogSnapshot.targetFog.mode, cod1FogSnapshot.targetFog.density,
		cod1FogSnapshot.targetFog.start, cod1FogSnapshot.targetFog.end,
		cod1FogSnapshot.targetFog.color[0], cod1FogSnapshot.targetFog.color[1],
		cod1FogSnapshot.targetFog.color[2], cod1FogSnapshot.targetFog.drawsky );
	for ( i = 0; i < cod1FogSnapshot.count; ++i ) {
		entry = &cod1FogSnapshot.entries[i];
		ri.Printf( PRINT_ALL,
			"CoD1 fog surface: shader=%s stage=%i passes=%i iterator=%s "
			"draws=%i verts=%i eyeDepth=(%.6g %.6g) enabled=%i cache=%i "
			"mode=0x%x den=%.9g near=%.9g far=%.9g GLrgb=(%.6g %.6g %.6g) "
			"hint=0x%x NVdistance=0x%x coordSource=0x%x coordCurrent=%.6g "
			"coordArray=%i noFog=%i stageFog=%i nativeFog=%i blend=0x%lx "
			"rgbGen=%i alphaGen=%i bundles=(%s %s %s)\n",
			entry->shaderName, entry->stageIndex, entry->passes,
			iteratorNames[entry->iteratorKind], entry->draws, entry->vertices,
			entry->eyeMin, entry->eyeMax, entry->enabled, entry->cache,
			(unsigned int)entry->mode, entry->density, entry->start, entry->end,
			entry->color[0], entry->color[1], entry->color[2], (unsigned int)entry->hint,
			(unsigned int)entry->distanceMode, (unsigned int)entry->coordSource,
			entry->coordCurrent, entry->coordArray, entry->noFog, entry->stageFog,
			entry->nativeFog, entry->blend, entry->rgbGen, entry->alphaGen,
			entry->images[0], entry->images[1], entry->images[2] );
	}
	ri.Printf( PRINT_ALL, "CoD1 fog snapshot end: entries=%i; set r_speeds 0 then 7 to capture again\n",
		cod1FogSnapshot.count );
}
#endif

#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
/* CoDSP 0x4C8A20 selects black RGB when the *effective* destination blend
 * bits equal ONE, while retaining the scene fog alpha. Otherwise native
 * fog RGB is scaled by identityLight when submitted to GL. This context is
 * populated only after RTCW has accepted a registered fog for this iterator.
 * Per-stage opt-in keeps ordinary RTCW materials on their existing path. */
static vec4_t cod1StageFogSceneColor;
static qboolean cod1StageFogSceneValid;
static qboolean cod1StageFogBlack;

static qboolean Cod1StageFogSceneAvailable( void ) {
	return cod1StageFogSceneValid;
}

static void Cod1StageFogRestore( void ) {
	if ( cod1StageFogBlack && cod1StageFogSceneValid ) {
		qglFogfv( GL_FOG_COLOR, cod1StageFogSceneColor );
	}
	cod1StageFogBlack = qfalse;
}

static void Cod1StageFogScene( const glfog_t *fog ) {
	cod1StageFogSceneValid = fog && fog->registered && r_wolffog &&
		r_wolffog->integer && fogIsOn;
	if ( cod1StageFogSceneValid ) {
		memcpy( cod1StageFogSceneColor, fog->color, sizeof( cod1StageFogSceneColor ) );
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
		/* Match R_Fog's submitted base colour, without changing the authored
		 * record or multiplying again when restoring after an additive pass. */
		if ( cod1Bsp59MapActive ) {
			cod1StageFogSceneColor[0] *= tr.identityLight;
			cod1StageFogSceneColor[1] *= tr.identityLight;
			cod1StageFogSceneColor[2] *= tr.identityLight;
		}
#endif
	}
}

static void Cod1StageFogForDraw( const shaderStage_t *stage, unsigned long stateBits ) {
	GLfloat black[4];

	if ( !cod1StageFogSceneValid || !stage->cod1NativeFogColor || !fogIsOn ) {
		Cod1StageFogRestore();
		return;
	}
	if ( ( stateBits & GLS_DSTBLEND_BITS ) != GLS_DSTBLEND_ONE ) {
		Cod1StageFogRestore();
		return;
	}
	if ( cod1StageFogBlack ) {
		return;
	}
	black[0] = black[1] = black[2] = 0.0f;
	black[3] = cod1StageFogSceneColor[3];
	qglFogfv( GL_FOG_COLOR, black );
	cod1StageFogBlack = qtrue;
}
#endif

/*
==============
SetIteratorFog
	set the fog parameters for this pass
==============
*/
void SetIteratorFog( void ) {
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
	Cod1StageFogRestore();
	cod1StageFogSceneValid = qfalse;
#endif
	// changed for problem when you start the game with r_fastsky set to '1'
//	if(r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
		R_FogOff();
		return;
	}

	if ( backEnd.refdef.rdflags & RDF_DRAWINGSKY ) {
		if ( glfogsettings[FOG_SKY].registered ) {
			R_Fog( &glfogsettings[FOG_SKY] );
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
			Cod1StageFogScene( &glfogsettings[FOG_SKY] );
#endif
		} else {
			R_FogOff();
		}

		return;
	}

	if ( skyboxportal && backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) {
		if ( glfogsettings[FOG_PORTALVIEW].registered ) {
			R_Fog( &glfogsettings[FOG_PORTALVIEW] );
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
			Cod1StageFogScene( &glfogsettings[FOG_PORTALVIEW] );
#endif
		} else {
			R_FogOff();
		}
	} else {
		if ( glfogNum > FOG_NONE ) {
			R_Fog( &glfogsettings[FOG_CURRENT] );
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
			Cod1StageFogScene( &glfogsettings[FOG_CURRENT] );
#endif
		} else {
			R_FogOff();
		}
	}
}


/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric( shaderCommands_t *input ) {
	int stage;

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if ( !pStage ) {
			break;
		}

		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce ) {
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}

		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != 0 ) {
			DrawMultitextured( input, stage );
		} else
		{
			int fadeStart, fadeEnd;

			if ( !setArraysOnce ) {
				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
			}

			//
			// set state
			//
			if ( pStage->bundle[0].vertexLightmap && ( ( r_vertexLight->integer && !r_uiFullScreen->integer ) || glConfig.hardwareType == GLHW_PERMEDIA2 ) && r_lightmap->integer ) {
				GL_Bind( tr.whiteImage );
			} else {
				R_BindAnimatedImage( &pStage->bundle[0] );
			}

			// Ridah, per stage fogging (detail textures)
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
			if ( pStage->cod1NativeFogColor && !Cod1StageFogSceneAvailable() ) {
				R_FogOff();
			} else
#endif
			if ( tess.shader->noFog && pStage->isFogged ) {
				R_FogOn();
			} else if ( tess.shader->noFog && !pStage->isFogged ) {
				R_FogOff(); // turn it back off
			} else {    // make sure it's on
				R_FogOn();
			}
			// done.

			//----(SA)	fading model stuff
			fadeStart = backEnd.currentEntity->e.fadeStartTime;

			if ( fadeStart ) {
				fadeEnd = backEnd.currentEntity->e.fadeEndTime;
				if ( fadeStart > tr.refdef.time ) {       // has not started to fade yet
					GL_State( pStage->stateBits );
				} else
				{
					int i;
					unsigned int tempState;
					float alphaval;

					if ( fadeEnd < tr.refdef.time ) {     // entity faded out completely
						continue;
					}

					alphaval = (float)( fadeEnd - tr.refdef.time ) / (float)( fadeEnd - fadeStart );

					tempState = pStage->stateBits;
					// remove the current blend, and don't write to Z buffer
					tempState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE );
					// set the blend to src_alpha, dst_one_minus_src_alpha
					tempState |= ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
					GL_State( tempState );
					GL_Cull( CT_FRONT_SIDED );
					// modulate the alpha component of each vertex in the render list
					for ( i = 0; i < tess.numVertexes; i++ ) {
						tess.svars.colors[i][0] *= alphaval;
						tess.svars.colors[i][1] *= alphaval;
						tess.svars.colors[i][2] *= alphaval;
						tess.svars.colors[i][3] *= alphaval;
					}
				}
			} else {
				GL_State( pStage->stateBits );
			}
			//----(SA)	end

			//
			// draw
			//
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
			Cod1StageFogForDraw( pStage, glState.glStateBits );
#endif
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
			Cod1FogStateProbeDraw( pStage );
			Cod1FogSnapshotDraw( pStage, stage, 0 );
#endif
#if defined(COD1_WATER_GL_DRAW_PROBE) && COD1_WATER_GL_DRAW_PROBE
			Cod1WaterProbeDraw( stage, input->numIndexes, NULL );
#endif
			R_DrawElements( input->numIndexes, input->indexes );
		}
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
			 || pStage->bundle[2].isLightmap
#endif
			 ) ) {
			break;
		}
	}
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
	/* The stage override must not leak into dlights, fog passes or the next
	 * shader, including after a debug lightmap break or skipped fade. */
	Cod1StageFogRestore();
#endif
}


#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#include "tr_cod1_perlight_runtime.h"
#endif

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void ) {
	shaderCommands_t *input;
	qboolean arraysLocked = qfalse;

	input = &tess;

	RB_DeformTessGeometry();

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name ) );
	}

	// set GL fog
	SetIteratorFog();

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		// RF< so we can send the normals as an array
		qglEnableClientState( GL_NORMAL_ARRAY );
#ifdef __MACOS__    //DAJ ATI
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 1 );
#else
		qglEnable( GL_PN_TRIANGLES_ATI ); // ATI PN-Triangles extension
#endif
	}


	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset ) {
		float factor = r_offsetFactor->value;
		float units = r_offsetUnits->value;
		if ( input->shader->cod1PolygonOffsetMode == 1 ) {
			factor *= 2.0f;
			units *= 2.0f;
		} else if ( input->shader->cod1PolygonOffsetMode == 2 ) {
			factor = 0.0f;
		}
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( factor, units );
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 || input->shader->multitextureEnv
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
         || input->shader->cod1PerlightRuntime
#endif
         ) {
		setArraysOnce = qfalse;
		qglDisableClientState( GL_COLOR_ARRAY );
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	} else
	{
		setArraysOnce = qtrue;

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
	}

	// RF, send normals only if required
	// This must be done first, since we can't change the arrays once they have been
	// locked
	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(input->shader->cod1PerlightRuntime && qglClientActiveTextureARB) {
        int unit;
        /* Secondary coordinate arrays may remain enabled after an ordinary
         * multitexture pass. Keep mutable perLight UVs outside CVA locking. */
        for(unit=1;unit<NUM_TEXTURE_BUNDLES && unit<glConfig.maxActiveTextures;++unit) {
            qglClientActiveTextureARB(GL_TEXTURE0_ARB+unit);
            qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    }
#endif
	//
	// lock XYZ
	//
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	qglVertexPointer( input->shader->cod1SunDirectional ? 4 : 3,
		GL_FLOAT, 16, input->xyz ); // preserve native directional w=0
#else
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz ); // padded for SIMD
#endif
	// Cloudless sky can leave zero vertices; CVA requires a positive count.
	if ( input->numVertexes > 0 && qglLockArraysEXT && qglUnlockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		arraysLocked = qtrue;
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if ( !setArraysOnce ) {
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnableClientState( GL_COLOR_ARRAY );
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

	//
	// now do any dynamic lighting needed
	//
	if (
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
         !RB_Cod1PerlightDynamic() &&
#endif
         tess.dlightBits && tess.shader->sort <= SS_OPAQUE
		 && !( tess.shader->surfaceFlags & ( SURF_NODLIGHT | SURF_SKY ) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( arraysLocked ) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	// turn truform back off
	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
#ifdef __MACOS__    //DAJ ATI
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 0 );
#else
		qglDisable( GL_PN_TRIANGLES_ATI );    // ATI PN-Triangles extension
#endif
		qglDisableClientState( GL_NORMAL_ARRAY );
	}

}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void ) {
	shaderCommands_t *input;
	shader_t        *shader;
	qboolean arraysLocked = qfalse;

	input = &tess;

	shader = input->shader;

	//
	// compute colors
	//
	RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name ) );
	}


	// set GL fog
	SetIteratorFog();

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set arrays and lock
	//
	qglEnableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
#ifdef __MACOS__    //DAJ ATI
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 1 );
#else
		qglEnable( GL_PN_TRIANGLES_ATI ); // ATI PN-Triangles extension
#endif
		qglEnableClientState( GL_NORMAL_ARRAY );         // RF< so we can send the normals as an array
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );


	// Cloudless sky can leave zero vertices; CVA requires a positive count.
	if ( input->numVertexes > 0 && qglLockArraysEXT && qglUnlockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		arraysLocked = qtrue;
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	//
	// call special shade routine
	//
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	GL_State( tess.xstages[0]->stateBits );
#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
	Cod1FogStateProbeDraw( tess.xstages[0] );
	Cod1FogSnapshotDraw( tess.xstages[0], 0, 2 );
#endif
	R_DrawElements( input->numIndexes, input->indexes );

	//
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( arraysLocked ) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	if ( qglPNTrianglesiATI && tess.ATI_tess )
#ifdef __MACOS__ //DAJ ATI{
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 0 );
	}
#else
	{ qglDisable( GL_PN_TRIANGLES_ATI );    // ATI PN-Triangles extension
	}
#endif
}

//define	REPLACE_MODE

void RB_StageIteratorLightmappedMultitexture( void ) {
	shaderCommands_t *input;
	qboolean arraysLocked = qfalse;

	input = &tess;

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name ) );
	}

	// set GL fog
	SetIteratorFog();

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set color, pointers, and lock
	//
	GL_State( GLS_DEFAULT );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	if ( qglPNTrianglesiATI && tess.ATI_tess ) {
#ifdef __MACOS__    //DAJ ATI
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 1 );
#else
		qglEnable( GL_PN_TRIANGLES_ATI ); // ATI PN-Triangles extension
#endif
		qglNormalPointer( GL_FLOAT, 16, input->normal );
	}

#ifdef REPLACE_MODE
	qglDisableClientState( GL_COLOR_ARRAY );
	qglColor3f( 1, 1, 1 );
	qglShadeModel( GL_FLAT );
#else
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.constantColor255 );
#endif

	//
	// select base stage
	//
	GL_SelectTexture( 0 );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );

	//
	// configure second stage
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( GL_MODULATE );
	}

//----(SA)	modified for snooper
	if ( tess.xstages[0]->bundle[1].isLightmap && ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) ) {
		GL_Bind( tr.whiteImage );
	} else {
		R_BindAnimatedImage( &tess.xstages[0]->bundle[1] );
	}

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][1] );

	//
	// lock arrays
	//
	// Cloudless sky can leave zero vertices; CVA requires a positive count.
	if ( input->numVertexes > 0 && qglLockArraysEXT && qglUnlockArraysEXT ) {
		qglLockArraysEXT( 0, input->numVertexes );
		arraysLocked = qtrue;
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

#if defined(COD1_DIRECT_FOG_WIP) && COD1_DIRECT_FOG_WIP
	Cod1FogStateProbeDraw( tess.xstages[0] );
	Cod1FogSnapshotDraw( tess.xstages[0], 0, 3 );
#endif
	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	qglDisable( GL_TEXTURE_2D );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	GL_SelectTexture( 0 );
#ifdef REPLACE_MODE
	GL_TexEnv( GL_MODULATE );
	qglShadeModel( GL_SMOOTH );
#endif

	//
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( arraysLocked ) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	if ( qglPNTrianglesiATI && tess.ATI_tess )
#ifdef __MACOS__ //DAJ ATI{
		qglPNTrianglesiATI( GL_PN_TRIANGLES_ATI, 0 );
	}
#else
	{ qglDisable( GL_PN_TRIANGLES_ATI );    // ATI PN-Triangles extension
	}
#endif
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if ( input->numIndexes == 0 ) {
		return;
	}

	if ( input->indexes[SHADER_MAX_INDEXES - 1] != 0 ) {
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit" );
	}
	if ( input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0 ) {
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit" );
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	if ( skyboxportal ) {
		// world
		if ( !( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) ) {
			if ( tess.currentStageIteratorFunc == RB_StageIteratorSky ) {  // don't process these tris at all
				return;
			}
		}
		// portal sky
		else {
			if ( !drawskyboxportal ) {
				if ( !( tess.currentStageIteratorFunc == RB_StageIteratorSky ) ) {  // /only/ process sky tris
					return;
				}
			}
		}
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris( input );
	}
	if ( r_shownormals->integer ) {
		DrawNormals( input );
	}


	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

	GLimp_LogComment( "----------\n" );
}

#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
#include "tr_cod1_water_runtime.h"
#endif
