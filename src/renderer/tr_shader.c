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

#include "tr_local.h"
#include "cod1_material_map.h"
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#include "cod1_sun_config.h"
#endif
#if defined(COD1_BSP59_SORT_ADDITIVE_CANDIDATE) && COD1_BSP59_SORT_ADDITIVE_CANDIDATE && \
    (!defined(COD1_BSP59_EXACT_VERTEX_ALPHA) || !COD1_BSP59_EXACT_VERTEX_ALPHA)
#error COD1_BSP59_SORT_ADDITIVE_CANDIDATE requires source-aware BSP59 registration
#endif
#include "cod1_material_probe.h"
#include "cod1_material_eligibility.h"
#include "cod1_material_window_glow.h"
#if defined(COD1_WATER_FIXED2_CANDIDATE) && COD1_WATER_FIXED2_CANDIDATE
#if !defined(COD1_STAGE_BACKEND_INFRA) || !COD1_STAGE_BACKEND_INFRA
#error COD1_WATER_FIXED2_CANDIDATE requires COD1_STAGE_BACKEND_INFRA=1
#endif
#ifndef COD1_WATER_FIXED2_CONSTLIGHT_SCALE
/* No measured CoD profile was supplied to this experimental compiler. */
#define COD1_WATER_FIXED2_CONSTLIGHT_SCALE (-1.0f)
#endif
#include "cod1_material_water_fixed2.h"
#endif
#if defined(COD1_MATERIAL_STAGE_COMPILER) && COD1_MATERIAL_STAGE_COMPILER
#if !defined(COD1_STAGE_BACKEND_INFRA) || !COD1_STAGE_BACKEND_INFRA
#error COD1_MATERIAL_STAGE_COMPILER requires COD1_STAGE_BACKEND_INFRA=1
#endif
#include "cod1_material_compiler_shape.h"
#endif

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t shader;
/* Material provenance, never a name or current-map heuristic. */
static qboolean R_Cod1NativeMaterialSource(void) {
    if(shader.cod1ShaderType || shader.cod1StaticModelSource)return qtrue;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    if(shader.cod1Bsp59WorldMaterialSource)return qtrue;
#endif
    return qfalse;
}
static texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
/* A dedicated scratch array prevents future bundle[1] texmods from aliasing
 * bundle[0]. No script parser assigns bundle[1] mods in this WIP. */
static texModInfo_t cod1Bundle1TexMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static texModInfo_t cod1Bundle2TexMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
#endif
#include "cod1_material_stype.h"
/* Private compile proof, indexed by the projected stage. It is never a
 * draw-time capability guess and cannot leak into the next registration. */
static unsigned cod1BundleAuth[MAX_SHADER_STAGES];
static int cod1BundleAuthStrict;
static unsigned cod1MultiplyIdentityStages;
static qboolean cod1MultiplyIdentityUnsupported;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
static qboolean cod1PerlightCompiling;
#endif
static int R_Cod1NativeTextureUnits(void) {
    int units=qglActiveTextureARB?glConfig.maxActiveTextures:1;
    /* Native 0x4fef7e..0x4fef88 bounds the effective query at eight. */
    return units>8?8:units;
}
static int Cod1PL_NativeSort(const char *name,int *value);
static void R_Cod1ClearBundleAuth(void) {
    memset(cod1BundleAuth,0,sizeof(cod1BundleAuth));cod1BundleAuthStrict=0;
    cod1MultiplyIdentityStages=0;
    cod1MultiplyIdentityUnsupported=qfalse;
}
static void R_Cod1SetBundleAuth(const cod1StResult_t *result,int strictAuth) {
    int i;R_Cod1ClearBundleAuth();cod1BundleAuthStrict=strictAuth;
    for(i=0;i<MAX_SHADER_STAGES && i<8;++i)cod1BundleAuth[i]=result->bundleAuth[i];
}
static unsigned R_Cod1StageBundleAuth(const shaderStage_t *stage) {
    int i;for(i=0;i<MAX_SHADER_STAGES;++i)if(stage==&stages[i])return cod1BundleAuth[i];
    return 0;
}
static qboolean R_Cod1StageMultiplyIdentity(const shaderStage_t *stage) {
    int i;for(i=0;i<MAX_SHADER_STAGES && i<8;++i)if(stage==&stages[i])
        return (cod1MultiplyIdentityStages&(1u<<i))?qtrue:qfalse;
    return qfalse;
}
static qboolean R_Cod1ThirdBundle(const shaderStage_t *stage) {
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    return stage->bundle[2].image[0] || stage->cod1TexEnv2;
#else
    (void)stage;return qfalse;
#endif
}
static qboolean R_Cod1AnyThirdBundle(void) {
    int i;for(i=0;i<MAX_SHADER_STAGES;++i)
        if(stages[i].active && R_Cod1ThirdBundle(&stages[i]))return qtrue;
    return qfalse;
}
static qboolean deferLoad;

#define FILE_HASH_SIZE      4096

static shader_t*       hashTable[FILE_HASH_SIZE];

// Ridah
// Table containing string indexes for each shader found in the scripts, referenced by their checksum
// values.
typedef struct shaderStringPointer_s
{
	char *pStr;
	struct shaderStringPointer_s *next;
} shaderStringPointer_t;
//
shaderStringPointer_t shaderChecksumLookup[FILE_HASH_SIZE];
// done.

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}
		if ( letter == PATH_SEP ) {
			letter = '/';                           // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static shader_t *R_FindShaderByNameForSource( const char *name,
		qboolean cod1Bsp59WorldMaterialSource ) {
	char strippedName[MAX_QPATH];
	shader_t *sh;
	int hash;
	if ( !name || !name[0] ) return tr.defaultShader;
	COM_StripExtension( name, strippedName );
	hash = generateHashValue( strippedName );
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		if ( sh->cod1Bsp59WorldMaterialSource == cod1Bsp59WorldMaterialSource &&
			 !Q_stricmp( sh->name, strippedName ) ) return sh;
	}
	return tr.defaultShader;
}
#endif

void R_RemapShader( const char *shaderName, const char *newShaderName, const char *timeOffset ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName( shaderName );
	if ( sh == NULL || sh == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( shaderName, 0 );
		sh = R_GetShaderByHandle( h );
	}
	if ( sh == NULL || sh == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( newShaderName, 0 );
		sh2 = R_GetShaderByHandle( h );
	}

	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension( shaderName, strippedName );
	hash = generateHashValue( strippedName );
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	{
		shader_t *targets[2] = { NULL, NULL };
		int source;
		for ( sh = hashTable[hash]; sh; sh = sh->next ) {
			if ( Q_stricmp( sh->name, strippedName ) ) continue;
			source = !!sh->cod1Bsp59WorldMaterialSource;
			if ( targets[source] ) continue;
			targets[source] = R_FindShaderByNameForSource( newShaderName, source );
			if ( !targets[source] || targets[source] == tr.defaultShader ) {
				targets[source] = source ?
					R_FindCod1Bsp59WorldMaterialShader( newShaderName, 0, qtrue ) :
					R_FindShader( newShaderName, 0, qtrue );
			}
			if ( targets[source] && targets[source]->defaultShader ) {
				targets[source] = NULL;
				ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found for source %d\n",
					newShaderName, source );
			}
		}
		for ( sh = hashTable[hash]; sh; sh = sh->next ) {
			if ( Q_stricmp( sh->name, strippedName ) ) continue;
			source = !!sh->cod1Bsp59WorldMaterialSource;
			if ( targets[source] ) sh->remappedShader = sh == targets[source] ? NULL : targets[source];
		}
		if ( timeOffset ) {
			if ( targets[0] ) targets[0]->timeOffset = atof( timeOffset );
			if ( targets[1] ) targets[1]->timeOffset = atof( timeOffset );
		}
	}
#else
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		if ( Q_stricmp( sh->name, strippedName ) == 0 ) {
			if ( sh != sh2 ) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if ( timeOffset ) {
		sh2->timeOffset = atof( timeOffset );
	}
#endif
}

/*
===============
ParseVector
===============
*/
#include "cod1_const_lighting.h"
/* Strict numeric input for the newly restored native operation. Existing
 * RTCW ParseVector users retain their historical parser behavior. */
static qboolean R_Cod1ParseConstLighting(char **text,shaderStage_t *stage) {
    float color[3];double value;int i;char *token;
    token=COM_ParseExt(text,qfalse);
    if(strcmp(token,"("))return qfalse;
    for(i=0;i<3;i++) {
        token=COM_ParseExt(text,qfalse);
        if(!Cod1MP_Number(token,&value) || value < -FLT_MAX || value > FLT_MAX)
            return qfalse;
        color[i]=(float)value;
    }
    token=COM_ParseExt(text,qfalse);
    if(strcmp(token,")") || !C1CL_Pack(color,tr.identityLight,stage->constantColor))
        return qfalse;
    stage->rgbGen=CGEN_CONST;
    if(ri.Cvar_Get && ri.Cvar_Get("r_cod1MaterialTrace","0",0)->integer)
        ri.Printf(PRINT_ALL,"CoD1 R55 constLighting: material=%s scale=%.9g rgb=%u/%u/%u; authored constant, no instance-light substitution\n",
            shader.name,tr.identityLight,stage->constantColor[0],stage->constantColor[1],stage->constantColor[2]);
    return qtrue;
}
static qboolean ParseVector( char **text, int count, float *v ) {
	char    *token;
	int i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname ) {
	if ( !Q_stricmp( funcname, "GT0" ) ) {
		return GLS_ATEST_GT_0;
	} else if ( !Q_stricmp( funcname, "LT128" ) )    {
		return GLS_ATEST_LT_80;
	} else if ( !Q_stricmp( funcname, "GE128" ) )    {
		return GLS_ATEST_GE_80;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name ) {
	if ( !Q_stricmp( name, "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	} else if ( !Q_stricmp( name, "GL_ZERO" ) )    {
		return GLS_SRCBLEND_ZERO;
	} else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )    {
		return GLS_SRCBLEND_DST_COLOR;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )    {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name ) {
	if ( !Q_stricmp( name, "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	} else if ( !Q_stricmp( name, "GL_ZERO" ) )    {
		return GLS_DSTBLEND_ZERO;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname ) {
	if ( !Q_stricmp( funcname, "sin" ) ) {
		return GF_SIN;
	} else if ( !Q_stricmp( funcname, "square" ) )    {
		return GF_SQUARE;
	} else if ( !Q_stricmp( funcname, "triangle" ) )    {
		return GF_TRIANGLE;
	} else if ( !Q_stricmp( funcname, "sawtooth" ) )    {
		return GF_SAWTOOTH;
	} else if ( !Q_stricmp( funcname, "inversesawtooth" ) )    {
		return GF_INVERSE_SAWTOOTH;
	} else if ( !Q_stricmp( funcname, "noise" ) )    {
		return GF_NOISE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( char **text, waveForm_t *wave ) {
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( char *_text, shaderStage_t *stage ) {
	const char *token;
	char **text = &_text;
	texModInfo_t *tmi;
    texModInfo_t candidate;
    qboolean nativeSource=R_Cod1NativeMaterialSource();

	if ( !nativeSource && stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name );
		return;
	}

    if(nativeSource){memset(&candidate,0,sizeof(candidate));tmi=&candidate;}
    else {
	    tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	    stage->bundle[0].numTexMods++;
    }

	token = COM_ParseExt( text, qfalse );

	//
	// swap
	//
	if ( !Q_stricmp( token, "swap" ) ) { // swap S/T coords (rotate 90d)
		tmi->type = TMOD_SWAP;
	}
	//
	// turb
	//
	// (SA) added 'else' so it wouldn't claim 'swap' was unknown.
	else if ( !Q_stricmp( token, "turb" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
            if(nativeSource)
                ri.Printf(PRINT_WARNING,"CoD1 R60 native recovery: incomplete tcMod turb ignored; texmod count unchanged in shader '%s'\n",shader.name);
            else
			    ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) ) {
		tmi->type = TMOD_ENTITY_TRANSLATE;
	} else
	{
		ri.Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
    /* Native ParseTexMod (0x4e6f59..0x4e6f7b) commits only after all
     * operands. The unused native slot can contain partial scratch values;
     * its count does not advance. Published modifiers remain unchanged. */
    if(nativeSource && candidate.type!=TMOD_NONE) {
        /* Capacity limits committed storage. A recovered incomplete native
         * operation must return without consuming or requiring a slot. */
        if(stage->bundle[0].numTexMods==TR_MAX_TEXMODS) {
            ri.Error(ERR_DROP,"ERROR: too many tcMod stages in shader '%s'\n",shader.name);
            return;
        }
        stage->bundle[0].texMods[stage->bundle[0].numTexMods++]=candidate;
    }
}


/*
===================
ParseStage
===================
*/
/* PATCH17: resolve images within a stage, preserving alpha/blend/lightmap
 * semantics instead of replacing the whole shader after a missing extension. */
static image_t *R_Cod1StageImageEx(const char *name, qboolean mip, qboolean picmip,
                               qboolean characterMip, int wrap,qboolean multiplyIdentity) {
    static const char *exts[]={".dds",".tga",".jpg",".blend.dds",".blend.tga",".masked.dds",".masked.tga"};
    char stem[MAX_QPATH],path[MAX_QPATH];const char *slash,*dot;image_t *image;int i;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    qboolean unsupported=qfalse;
#endif
    /* R51: an authored CoD image variant is keyed by sampler/mip settings,
     * regardless of whether its program came from .stype or a plain .shader.
     * Keep legacy file-extension candidate ordering unchanged in this pass. */
    qboolean exactVariant=shader.cod1ShaderType || shader.cod1StaticModelSource;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    exactVariant=exactVariant || shader.cod1Bsp59WorldMaterialSource;
#endif
    if(!name || !name[0] || name[0]=='$')return NULL;
    /* A leading slash in retail FX maps means virtual root, not OS root. */
    while(*name=='/')++name;
    if(!*name)return NULL;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(multiplyIdentity) {
        image=R_FindCod1MultiplyIdentityImage(name,mip,picmip,characterMip,wrap,tr.identityLight,&unsupported);
        if(unsupported) {cod1MultiplyIdentityUnsupported=qtrue;return NULL;}
    }
    else
#else
    (void)multiplyIdentity;
#endif
    image=exactVariant?R_FindCod1TypeImage(name,mip,picmip,characterMip,wrap):R_FindImageFileExt(name,mip,picmip,characterMip,wrap);
    if(image)return image;
    if(strlen(name)>=sizeof(stem))return NULL;
    Q_strncpyz(stem,name,sizeof(stem));slash=strrchr(stem,'/');dot=strrchr(stem,'.');
    /* Preserve variant suffixes such as .blend and .masked. */
    if(dot && (!slash || dot>slash) &&
       (!Q_stricmp(dot,".dds") || !Q_stricmp(dot,".tga") || !Q_stricmp(dot,".jpg")))
        stem[dot-stem]=0;
    /* R47 types may resolve storage extensions, but may not guess a different
     * authored image transform (.blend/.masked). Explicit suffixes above stay. */
    for(i=0;i<(shader.cod1ShaderType?3:(int)(sizeof(exts)/sizeof(exts[0])));i++){
        if(strlen(stem)+strlen(exts[i])>=sizeof(path))continue;
        Com_sprintf(path,sizeof(path),"%s%s",stem,exts[i]);
        if(!Q_stricmp(path,name))continue;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
        if(multiplyIdentity) {
            image=R_FindCod1MultiplyIdentityImage(path,mip,picmip,characterMip,wrap,tr.identityLight,&unsupported);
            if(unsupported) {cod1MultiplyIdentityUnsupported=qtrue;return NULL;}
        }
        else
#endif
        image=exactVariant?R_FindCod1TypeImage(path,mip,picmip,characterMip,wrap):R_FindImageFileExt(path,mip,picmip,characterMip,wrap);
        if(image){
            ri.Printf(PRINT_ALL,"CoD1 stage image: %s -> %s (shader %s)\n",name,path,shader.name);
            return image;
        }
    }
    return NULL;
}
static image_t *R_Cod1StageImage(const char *name,qboolean mip,qboolean picmip,
                                qboolean characterMip,int wrap) {
    return R_Cod1StageImageEx(name,mip,picmip,characterMip,wrap,qfalse);
}

/* R53: noopt forbids the original deferred/optimized texture representation.
 * R_FindImageFileExtForType currently resolves every image synchronously: it
 * either returns an already uploaded image or calls R_CreateImageExt before
 * returning. There is NO native texture optimizer in this backend, so that
 * eager postcondition also holds for noopt, including cached images. Do not
 * translate it to noPicMip/noMipMaps/noCompress or disable RTCW RMSE heuristics.
 * Keep the request in the bundle for a future optimizer and diagnostic audits.
 */
static void R_Cod1MapPolicyTrace(const char *name,unsigned flags,image_t *image) {
    if((flags&C1MAP_NOOPT) && image)
        ri.Printf(PRINT_ALL,"CoD1 R53 map policy: material=%s source=%s noopt=1 policy=eager image=%s mipmaps=%d picmip=%d\n",
            shader.name,name,image->imgName,image->mipmap,image->allowPicmip);
}

/* The authored fixed-function branch of opaque glass is base * baked LM +
 * additive glow. The final perlight/$dlight bundle has a different dynamic
 * lighting contract; do not silently pretend it has been implemented. This
 * source-shape plan is active for BSP59 even in an ordinary VS Debug build. */
static qboolean R_Cod1CompileWindowGlow2(const char *body) {
    cod1MpCaps_t caps;
    cod1MaterialProbe_t ir;
    cod1WgPlan_t plan;
    shader_t candidateShader;
    shaderStage_t candidate[3];
    image_t *base,*glow,*lm;
    int index=shader.lightmapIndex;

    if (!body || r_vertexLight->integer || r_lightmap->integer ||
        index<0 || index>=tr.numLightmaps || !tr.lightmaps[index]) return qfalse;
    memset(&caps,0,sizeof(caps));
    /* This backend uses the source's <3 branch. It also works as sequential
     * fixed-function passes when the hardware has only one texture unit. */
    caps.units=glConfig.maxActiveTextures>1?2:1;
    if (!Cod1MP_Parse(body,&caps,&ir) || !Cod1WG_Plan(&ir,&plan)) return qfalse;
    base=R_Cod1StageImage(plan.base,qtrue,qtrue,qfalse,GL_REPEAT);
    if (!base) return qfalse;
    glow=R_Cod1StageImage(plan.glow,qtrue,qtrue,qfalse,GL_REPEAT);
    if (!glow) return qfalse;
    lm=tr.lightmaps[index];

    candidateShader=shader;
    candidateShader.sort=SS_OPAQUE;
    candidateShader.explicitlyDefined=qtrue;
    candidateShader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_WINDOW;
    memset(candidate,0,sizeof(candidate));
    candidate[0].active=candidate[1].active=candidate[2].active=qtrue;
    candidate[0].bundle[0].image[0]=base;
    candidate[0].bundle[0].tcGen=TCGEN_TEXTURE;
    candidate[0].bundle[0].texMods=texMods[0];
    candidate[0].rgbGen=CGEN_IDENTITY;
    candidate[0].alphaGen=AGEN_IDENTITY;
    candidate[0].stateBits=GLS_DEPTHMASK_TRUE;

    candidate[1].bundle[0].image[0]=lm;
    candidate[1].bundle[0].isLightmap=qtrue;
    candidate[1].bundle[0].tcGen=TCGEN_LIGHTMAP;
    candidate[1].bundle[0].texMods=texMods[1];
    candidate[1].rgbGen=CGEN_IDENTITY;
    candidate[1].alphaGen=AGEN_IDENTITY;
    candidate[1].stateBits=GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_ZERO;

    candidate[2].bundle[0].image[0]=glow;
    candidate[2].bundle[0].tcGen=TCGEN_TEXTURE;
    candidate[2].bundle[0].texMods=texMods[2];
    candidate[2].rgbGen=CGEN_IDENTITY_LIGHTING;
    candidate[2].alphaGen=AGEN_IDENTITY;
    candidate[2].stateBits=GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE;

    shader=candidateShader;
    memcpy(stages,candidate,sizeof(candidate));
    ri.Printf(PRINT_ALL,"CoD1 BSP59 opaque glass: %s base+lightmap+glow; perlight deferred (fixed 2-TMU source branch)\n",shader.name);
    return qtrue;
}

#if defined(COD1_WATER_FIXED2_CANDIDATE) && COD1_WATER_FIXED2_CANDIDATE
/* The legacy glConfig string may be truncated to 4095 bytes on modern
 * drivers. Snapshot only the exact operands used by authored water requires
 * from the complete string while the GL context is current. */
static qboolean cod1WaterProfileKnown;
static qboolean cod1WaterExtensions[4];
static const char *const cod1WaterExtensionNames[4]={
    "GL_NV_texture_shader", "GL_NV_register_combiners",
    "GL_ATI_fragment_shader", "GL_ARB_texture_cube_map"
};
/* Accept well-formed GL/WGL/GLX/EGL tokens; only exact GL_ names can satisfy
 * authored requires. The fixed renderer uses two backend bundles. */
static qboolean R_Cod1WaterProfileStringKnown(const char *start,const char *end) {
    const char *p=start,*next;
    qboolean hasMultitexture=qfalse;
    static const char required[]="GL_ARB_multitexture";
    if (!start || !end || start>=end) return qfalse;
    while (p<end) {
        next=p;
        while (next<end && *next!=' ') {
            /* glGetString(GL_EXTENSIONS) is an ASCII space-separated list.
             * A partial/invalid list must never make absent vendor tokens
             * look like reliable FALSE operands in `requires`. */
            if (!((*next>='A' && *next<='Z') ||
                  (*next>='a' && *next<='z') ||
                  (*next>='0' && *next<='9') || *next=='_')) return qfalse;
            ++next;
        }
        if (next-p<4 || !(
            (p[0]=='G' && p[1]=='L' && p[2]=='_') ||
            (next-p>=5 && p[0]=='W' && p[1]=='G' && p[2]=='L' && p[3]=='_') ||
            (next-p>=5 && p[0]=='G' && p[1]=='L' && p[2]=='X' && p[3]=='_') ||
            (next-p>=5 && p[0]=='E' && p[1]=='G' && p[2]=='L' && p[3]=='_'))) return qfalse;
        if ((size_t)(next-p)==sizeof(required)-1 &&
            !strncmp(p,required,sizeof(required)-1)) hasMultitexture=qtrue;
        if (next==end) break;
        p=next+1;
        if (p==end || *p==' ') return qfalse;
    }
    return hasMultitexture;
}
void R_Cod1WaterCaptureGLProfile(const char *extensions) {
    const char *end,*p,*next;
    size_t length=0;
    int i;
    const char *reason="valid";
    cod1WaterProfileKnown=qfalse;
    memset(cod1WaterExtensions,0,sizeof(cod1WaterExtensions));
    if (!extensions) {
        reason="no extensions";
    } else {
        /* Bound a broken driver string without treating a partial list as
         * evidence that one of the GL operands is absent. */
        while (length<65536 && extensions[length]) ++length;
        if (length==65536) {
            reason="unterminated/oversized extensions";
        } else {
            end=extensions+length;
            if (!R_Cod1WaterProfileStringKnown(extensions,end)) {
                reason="invalid extensions or missing GL_ARB_multitexture";
            } else {
                cod1WaterProfileKnown=qtrue;
                for (p=extensions;p<end;p=next+1) {
                    next=p;
                    while (next<end && *next!=' ') ++next;
                    for (i=0;i<4;++i) {
                        size_t n=strlen(cod1WaterExtensionNames[i]);
                        if ((size_t)(next-p)==n && !memcmp(p,cod1WaterExtensionNames[i],n))
                            cod1WaterExtensions[i]=qtrue;
                    }
                    if (next==end) break;
                }
            }
        }
    }
    ri.Printf(PRINT_ALL,"CoD1 water GL profile: %s; raw length=%u, legacy length=%u, NVshader=%d NVcombiner=%d ATIfragment=%d cube=%d\n",
              reason,(unsigned)length,(unsigned)strlen(glConfig.extensions_string),
              cod1WaterExtensions[0],cod1WaterExtensions[1],
              cod1WaterExtensions[2],cod1WaterExtensions[3]);
}
static cod1MpTruth_t R_Cod1WaterExtension(const char *name,void *unused) {
    int i;
    (void)unused;
    for(i=0;i<4;++i)if(!strcmp(name,cod1WaterExtensionNames[i]))break;
    if(i==4 || !cod1WaterProfileKnown)return COD1_MP_UNKNOWN;
    return cod1WaterExtensions[i]?COD1_MP_TRUE:COD1_MP_FALSE;
}
static void R_Cod1WaterScrollScale(texModInfo_t mods[2],const cod1MwBundle_t *b) {
    memset(mods,0,2*sizeof(*mods));
    mods[0].type=TMOD_SCROLL;
    mods[0].scroll[0]=b->scroll[0];mods[0].scroll[1]=b->scroll[1];
    mods[1].type=TMOD_SCALE;
    mods[1].scale[0]=b->scale[0];mods[1].scale[1]=b->scale[1];
}
static qboolean R_Cod1CompileWaterFixed2(const char *body) {
    cod1MpCaps_t caps;
    cod1MaterialProbe_t ir;
    cod1MwPlan_t plan;
    shader_t candidateShader;
    shaderStage_t candidate[2];
    texModInfo_t mods[3][2];
    image_t *base,*overlay;
    const float cod1ConstLightScale=COD1_WATER_FIXED2_CONSTLIGHT_SCALE;
    int i;
    const int backendUnits=2;

    if (!body || !(cod1ConstLightScale>=0.0f && cod1ConstLightScale<=1.0f) ||
        !qglActiveTextureARB || !qglClientActiveTextureARB ||
        glConfig.maxActiveTextures<backendUnits || !cod1WaterProfileKnown ||
        glConfig.hardwareType==GLHW_PERMEDIA2 || r_vertexLight->integer ||
        (shader.lightmapIndex!=LIGHTMAP_BY_VERTEX && shader.lightmapIndex!=LIGHTMAP_NONE) ||
        tr.overbrightBits<0 || tr.overbrightBits>1 ||
        tr.identityLight != 1.0f/(float)(1<<tr.overbrightBits)) return qfalse;

    memset(&caps,0,sizeof(caps));
    caps.units=glConfig.maxActiveTextures;caps.extension=R_Cod1WaterExtension;
    if (!Cod1MW_ProfileKnown(&caps)) return qfalse;
    if (!Cod1MP_Parse(body,&caps,&ir) || !Cod1MW_Plan(&ir,&plan)) return qfalse;

    /* All source operations and runtime guards are checked before image IO.
     * A failed lookup leaves the shader and stage scratch untouched. */
    base=R_Cod1StageImage(plan.base.image,qtrue,qtrue,qfalse,GL_REPEAT);
    if (!base) return qfalse;
    overlay=R_Cod1StageImage(plan.overlay[0].image,qtrue,qtrue,qfalse,GL_REPEAT);
    if (!overlay) return qfalse;

    candidateShader=shader;
    candidateShader.sort=11.0f; /* CoDSP R41B registered damwater sort; port GL queue TBD */
    candidateShader.surfaceFlags|=SURF_NONSOLID|SURF_NOLIGHTMAP;
    candidateShader.contentFlags|=CONTENTS_WATER|CONTENTS_TRANSLUCENT;
    candidateShader.explicitlyDefined=qtrue;
    candidateShader.cod1WaterFixed2Candidate=qtrue;
    candidateShader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_WATER_CANDIDATE;
    memset(candidate,0,sizeof(candidate));
    memset(mods,0,sizeof(mods));
    candidate[0].active=candidate[1].active=qtrue;
    candidate[0].bundle[0].image[0]=base;
    candidate[0].bundle[0].tcGen=TCGEN_TEXTURE;
    candidate[0].bundle[0].numTexMods=2;
    candidate[0].bundle[0].texMods=texMods[0];
    candidate[0].rgbGen=CGEN_CONST;
    candidate[0].alphaGen=AGEN_IDENTITY;
    candidate[0].stateBits=GLS_DEPTHMASK_TRUE;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    candidate[0].cod1NativeFogColor=qtrue;
#endif
    for(i=0;i<3;++i) {
        /* CoDSP's sampled constLighting scalar is 1.0, and the saved
         * damwater stage contains 127 for each .5 source component.
         * Require an explicit measured CoD scalar; the port's identityLight
         * (0.5 in this host profile) is a separate RTCW setting. */
        candidate[0].constantColor[i]=(byte)(plan.constLighting[i]*cod1ConstLightScale*255.0f);
    }
    candidate[0].constantColor[3]=255;
    candidate[1].bundle[0].image[0]=overlay;
    candidate[1].bundle[0].tcGen=TCGEN_TEXTURE;
    candidate[1].bundle[0].numTexMods=2;
    candidate[1].bundle[0].texMods=texMods[1];
    candidate[1].bundle[1].image[0]=overlay;
    candidate[1].bundle[1].tcGen=TCGEN_TEXTURE;
    candidate[1].bundle[1].numTexMods=2;
    candidate[1].bundle[1].texMods=cod1Bundle1TexMods[1];
    candidate[1].cod1TexEnv=GL_MODULATE;
    candidate[1].rgbGen=CGEN_EXACT_VERTEX;
    /* Live CoDSP parser promotes an unset alpha mode after exactVertex to
     * raw mode 4. The R21 live color consumer copies vertex alpha for raw 4
     * with this stage's RGB mode 5; preserve it in the port. */
    candidate[1].alphaGen=AGEN_VERTEX;
    candidate[1].stateBits=GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    candidate[1].cod1NativeFogColor=qtrue;
#endif
    R_Cod1WaterScrollScale(mods[0],&plan.base);
    R_Cod1WaterScrollScale(mods[1],&plan.overlay[0]);
    R_Cod1WaterScrollScale(mods[2],&plan.overlay[1]);

    shader=candidateShader;
    stages[0]=candidate[0];stages[1]=candidate[1];
    memcpy(texMods[0],mods[0],sizeof(mods[0]));
    memcpy(texMods[1],mods[1],sizeof(mods[1]));
    memcpy(cod1Bundle1TexMods[1],mods[2],sizeof(mods[2]));
    return qtrue;
}
#endif

#if defined(COD1_MATERIAL_STAGE_COMPILER) && COD1_MATERIAL_STAGE_COMPILER
/* One authored stage, diffuse + baked lightmap. Both syntax and the current
 * runtime state are checked before image IO. Image resolution can perform its
 * usual FS/log/cache/upload effects; global shader scratch is committed only
 * after successful lookup. This is RTCW behavior, not CoD retail parity. */
static qboolean R_Cod1CompileOneLightmappedStage(const char *body) {
    cod1MpCaps_t caps;
    cod1MeBackend_t backend;
    cod1MeResult_t eligible;
    cod1MaterialProbe_t ir;
    cod1McPlan_t plan;
    shader_t candidateShader;
    shaderStage_t candidateStage;
    texModInfo_t candidateMod;
    image_t *baseImage,*lightmapImage;
    int lm=shader.lightmapIndex;

    if (!qglActiveTextureARB || !qglClientActiveTextureARB ||
        glConfig.maxActiveTextures<2 ||
        r_vertexLight->integer || r_lightmap->integer ||
        glConfig.hardwareType==GLHW_PERMEDIA2 ||
        lm<0 || lm>=tr.numLightmaps || !tr.lightmaps[lm]) return qfalse;
    memset(&caps,0,sizeof(caps));
    caps.units=2;
    backend.maxBundles=2;
    if (!Cod1MP_Parse(body,&caps,&ir)) return qfalse;
    Cod1ME_Evaluate(&ir,&backend,&eligible);
    if (!Cod1MC_Plan(&ir,&eligible,&plan) ||
        strlen(plan.image)>=MAX_QPATH) return qfalse;

    /* No global stage/texmod data is touched until the sole file image and
     * requested BSP atlas are both resolved. */
    lightmapImage=tr.lightmaps[lm];
    baseImage=R_Cod1StageImage(plan.image,qtrue,qtrue,qfalse,GL_REPEAT);
    if (!baseImage) return qfalse;

    candidateShader=shader;
    candidateShader.sort=SS_SEE_THROUGH;
    candidateShader.surfaceFlags|=SURF_NONSOLID|SURF_NOIMPACT|SURF_NODLIGHT;
    candidateShader.contentFlags|=CONTENTS_WATER;
    candidateShader.multitextureEnv=GL_MODULATE;
    candidateShader.explicitlyDefined=qtrue;
    candidateShader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_IR;

    memset(&candidateStage,0,sizeof(candidateStage));
    candidateStage.active=qtrue;
    candidateStage.rgbGen=CGEN_IDENTITY;
    candidateStage.alphaGen=AGEN_IDENTITY;
    candidateStage.stateBits=GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    candidateStage.bundle[0].image[0]=baseImage;
    candidateStage.bundle[0].tcGen=TCGEN_TEXTURE;
    candidateStage.bundle[0].numTexMods=1;
    candidateStage.bundle[0].texMods=texMods[0];
    candidateStage.bundle[1].image[0]=lightmapImage;
    candidateStage.bundle[1].isLightmap=qtrue;
    candidateStage.bundle[1].tcGen=TCGEN_LIGHTMAP;
    candidateStage.bundle[1].texMods=cod1Bundle1TexMods[0];
    candidateStage.cod1TexEnv=GL_MODULATE;

    memset(&candidateMod,0,sizeof(candidateMod));
    candidateMod.type=TMOD_SCROLL;
    candidateMod.scroll[0]=plan.scroll[0];
    candidateMod.scroll[1]=plan.scroll[1];

    shader=candidateShader;
    stages[0]=candidateStage;
    texMods[0][0]=candidateMod;
    return qtrue;
}
#endif

/* CoD1 lightmap bundles and neutral white bundles used by foliage. */
static qboolean R_Cod1LightmapBundle(shaderStage_t *stage,char **text) {
    char *token;int mapped=0,white=0;
    if(stage->bundle[1].image[0])return qfalse;
    while(1){
        token=COM_ParseExt(text,qtrue);
        if(!token[0])return qfalse;
        if(!strcmp(token,"}"))break;
        if(!Q_stricmp(token,"map")){
            int axes=C1MAP_REPEAT;unsigned flags=0;
            token=COM_ParseExt(text,qfalse);
            if(mapped)return qfalse;
            while(C1Map_Option(token,&axes,&flags))token=COM_ParseExt(text,qfalse);
            stage->bundle[1].cod1MapLoadFlags=flags;
            if(!Q_stricmp(token,"$whiteimage"))white=1;
            else if(Q_stricmp(token,"$lightmap"))return qfalse;
            mapped=1;
        }else if(!Q_stricmp(token,"blendFunc")){
            token=COM_ParseExt(text,qfalse);
            if(Q_stricmp(token,"filter"))return qfalse;
        }else return qfalse; /* Unsupported bundle operations keep fallback. */
    }
    if(!mapped)return qfalse;
    /* Multiplication by white is identity, including alpha. Keep the first
     * bundle's alpha test, depth write and vertex colour, without requiring
     * another texture unit. Only map and optional filter are accepted above. */
    if(white){
        ri.Printf(PRINT_ALL,"CoD1 neutral white bundle: %s\n",shader.name);
        return qtrue;
    }
    stage->bundle[1].image[0]=shader.lightmapIndex>=0?tr.lightmaps[shader.lightmapIndex]:tr.whiteImage;
    stage->bundle[1].isLightmap=qtrue;stage->bundle[1].tcGen=TCGEN_LIGHTMAP;
    shader.multitextureEnv=GL_MODULATE;
    ri.Printf(PRINT_ALL,"CoD1 lightmap bundle: %s\n",shader.name);
    return qtrue;
}

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#include "tr_cod1_ordinary_bundle.h"
#endif

/* IMPACT_R1: these mark materials have a native extra per-light pass. Preserve
 * their base pass and explicitly defer dynamic-light replay. This remains a
 * bounded lighting approximation; native depth-bias keywords are generic. */
static qboolean R_Cod1KnownImpactDecal(const char *name) {
    static const char *known[]={"gfx/impact/cratered","gfx/impact/bullethole1",
        "gfx/impact/bullethole2","gfx/impact/stone_singleshot1","gfx/impact/bullethit_wood1","gfx/impact/bullethit_wood2",
        "gfx/impact/bullethit_plaster","gfx/impact/bullethit_plaster2",
        "gfx/impact/bullethit_sand","gfx/impact/bullethit_snow",
        "gfx/impact/bullethit_glass","gfx/impact/bullethit_glass2"};
    int i;for(i=0;i<(int)(sizeof(known)/sizeof(known[0]));i++)if(!Q_stricmp(name,known[i]))return qtrue;
    return qfalse;
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static qboolean ParseStageForSource( shaderStage_t *stage, char **text,
		qboolean cod1Bsp59WorldMaterialSource ) {
#else
static qboolean ParseStage( shaderStage_t *stage, char **text ) {
#endif
	char *token;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	qboolean alphaGenExplicit = qfalse;
#endif

	stage->active = qtrue;

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' ) {
			break;
		}
        if(!Q_stricmp(token,"perlight") && R_Cod1KnownImpactDecal(shader.name)) {
            int depth=1;
            /* No native per-light decal pass in RTCW. Keep the original base
             * stage and explicitly omit this extra pass, not an opaque fallback. */
            while(depth) {
                token=COM_ParseExt(text,qtrue);
                if(!*token)return qfalse;
                if(*token=='{')++depth;
                else if(*token=='}')--depth;
            }
            memset(stage,0,sizeof(*stage));
            ri.Printf(PRINT_DEVELOPER,"CoD1 mark material: %s perlight pass deferred\n",shader.name);
            return qtrue;
        }
        if(!Q_stricmp(token,"nextbundle")){
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
            if(R_Cod1NativeMaterialSource()) {
                if(!R_Cod1OrdinaryBundle(stage,text))return qfalse;
            } else
#endif
            if(!R_Cod1LightmapBundle(stage,text))return qfalse;
            break; /* helper consumed the stage's closing brace */
        }
		//
		// check special case for map16/map32/mapcomp/mapnocomp (compression enabled)
		if ( !Q_stricmp( token, "map16" ) ) {    // only use this texture if 16 bit color depth
			if ( glConfig.colorBits <= 16 ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "map32" ) )    { // only use this texture if 16 bit color depth
			if ( glConfig.colorBits > 16 ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "mapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "mapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "animmapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "animmapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		}
		//
		// map <name>
		//
		if ( !Q_stricmp( token, "map" ) ) {
            int axes=C1MAP_REPEAT;
            int mapWrap;
            unsigned mapLoadFlags=0;
            token=COM_ParseExt(text,qfalse);
            /* R51: the same native qualifiers apply to authored .shader and
             * expanded .stype; consume qualifiers before the image token. */
            while(C1Map_Option(token,&axes,&mapLoadFlags))token=COM_ParseExt(text,qfalse);
            stage->bundle[0].cod1MapLoadFlags=mapLoadFlags;
            mapWrap=C1Map_Wrap(axes,GL_REPEAT,GL_CLAMP_TO_EDGE,COD1_IMAGE_CLAMP_X,COD1_IMAGE_CLAMP_Y);
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

//----(SA)	fixes startup error and allows polygon shadows to work again
			if ( C1Map_Source(token)==C1MAP_WHITE ) {
//----(SA)	end
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
//----(SA) added
			else if ( C1Map_Source(token)==C1MAP_DLIGHT ) {
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
//----(SA) end
			else if ( C1Map_Source(token)==C1MAP_LIGHTMAP ) {
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			} else
			{
//----(SA)	modified
//				stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT );
				stage->bundle[0].image[0] = R_Cod1StageImageEx( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, mapWrap,R_Cod1StageMultiplyIdentity(stage) );
                R_Cod1MapPolicyTrace(token,mapLoadFlags,stage->bundle[0].image[0]);
//----(SA)	end
				if ( !stage->bundle[0].image[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			stage->bundle[0].image[0] = R_Cod1StageImageEx( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, GL_CLAMP,R_Cod1StageMultiplyIdentity(stage) );
			if ( !stage->bundle[0].image[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					stage->bundle[0].image[num] = R_Cod1StageImage( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, GL_REPEAT );
					if ( !stage->bundle[0].image[num] ) {
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		} else if ( !Q_stricmp( token, "videoMap" ) )    {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, ( CIN_loop | CIN_silent | CIN_shader ) );
			if ( stage->bundle[0].videoMapHandle != -1 ) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) ) {
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) ) {
				depthFuncBits = 0;
			} else if ( !Q_stricmp( token, "equal" ) )    {
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) ) {
			stage->isDetail = qtrue;
		}
		//
		// fog
		//
		else if ( !Q_stricmp( token, "fog" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for fog in shader '%s'\n", shader.name );
				continue;
			}
			if ( !Q_stricmp( token, "on" ) ) {
				stage->isFogged = qtrue;
			} else {
				stage->isFogged = qfalse;
			}
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
                if(R_Cod1NativeMaterialSource())
                    ri.Printf(PRINT_WARNING,"CoD1 R60 native recovery: incomplete blendFunc ignored; blend/depth unchanged in shader '%s'\n",shader.name);
                else
				    ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
                int sourceBits=NameToSrcBlendMode(token);
                if(!R_Cod1NativeMaterialSource())blendSrcBits=sourceBits;

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
                    if(R_Cod1NativeMaterialSource())
                        ri.Printf(PRINT_WARNING,"CoD1 R60 native recovery: incomplete blendFunc ignored; blend/depth unchanged in shader '%s'\n",shader.name);
                    else
					    ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
                blendSrcBits=sourceBits;
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit ) {
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			} else if ( !Q_stricmp( token, "const" ) )    {
				vec3_t color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
            } else if (!Q_stricmp(token,"constLighting")) {
                if(!R_Cod1ParseConstLighting(text,stage)) {
                    ri.Printf(PRINT_WARNING,"CoD1 R55 constLighting rejected: malformed/nonfinite or unrepresentable vector in %s\n",shader.name);
                    return qfalse;
                }
			} else if ( !Q_stricmp( token, "identity" ) )    {
				stage->rgbGen = CGEN_IDENTITY;
			} else if ( !Q_stricmp( token, "identityLighting" ) )    {
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			} else if ( !Q_stricmp( token, "entity" ) )    {
				stage->rgbGen = CGEN_ENTITY;
			} else if ( !Q_stricmp( token, "oneMinusEntity" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			} else if ( !Q_stricmp( token, "vertex" ) )    {
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			} else if ( !Q_stricmp( token, "exactVertex" ) )    {
				stage->rgbGen = CGEN_EXACT_VERTEX;
			} else if ( !Q_stricmp( token, "lightingDiffuse" ) )    {
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
            } else if ( !Q_stricmp(token,"lightingPrecalc") ) {
                /* R52 source-proven model input, not an alias for diffuse.
                 * World/2D precalc is not silently assigned model semantics. */
                if (!shader.cod1StaticModelSource &&
                    !(shader.cod1ShaderType && shader.cod1ShaderTypeLightmapKey==LIGHTMAP_NONE)) {
                    ri.Printf(PRINT_WARNING,"R52 lightingPrecalc: native static-instance input required; other producers unverified: %s\n",shader.name);
                    return qfalse;
                }
                stage->rgbGen=CGEN_COD1_PRECALC;
			} else if ( !Q_stricmp( token, "oneMinusVertex" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
                if(R_Cod1NativeMaterialSource()) {
                    /* Native 0x4ee093 warns and skips the source line. It
                     * does not alias an unknown mode to const/constLighting. */
                    SkipRestOfLine(text);
                    ri.Printf(PRINT_WARNING,"CoD1 R60 native recovery: unrecognized rgbGen skipped; prior RGB mode retained in shader '%s'\n",shader.name);
                }
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) ) {
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
			alphaGenExplicit = qtrue;
#endif
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			} else if ( !Q_stricmp( token, "const" ) )    {
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( token );
				stage->alphaGen = AGEN_CONST;
			} else if ( !Q_stricmp( token, "identity" ) )    {
				stage->alphaGen = AGEN_IDENTITY;
			} else if ( !Q_stricmp( token, "entity" ) )    {
				stage->alphaGen = AGEN_ENTITY;
			} else if ( !Q_stricmp( token, "oneMinusEntity" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			// Ridah
			else if ( !Q_stricmp( token, "normalzfade" ) ) {
				stage->alphaGen = AGEN_NORMALZFADE;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->constantColor[3] = 255 * atof( token );
				} else {
					stage->constantColor[3] = 255;
				}

				token = COM_ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->zFadeBounds[0] = atof( token );    // lower range
					token = COM_ParseExt( text, qfalse );
					stage->zFadeBounds[1] = atof( token );    // upper range
				} else {
					stage->zFadeBounds[0] = -1.0;   // lower range
					stage->zFadeBounds[1] =  1.0;   // upper range
				}

			}
			// done.
			else if ( !Q_stricmp( token, "vertex" ) ) {
				stage->alphaGen = AGEN_VERTEX;
			} else if ( !Q_stricmp( token, "lightingSpecular" ) )    {
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			} else if ( !Q_stricmp( token, "oneMinusVertex" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			} else if ( !Q_stricmp( token, "portal" ) )    {
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					shader.portalRange = 256;
					ri.Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				} else
				{
					shader.portalRange = atof( token );
				}
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp( token, "texgen" ) || !Q_stricmp( token, "tcGen" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) ) {
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			} else if ( !Q_stricmp( token, "firerisenv" ) )    {
				stage->bundle[0].tcGen = TCGEN_FIRERISEENV_MAPPED;
			} else if ( !Q_stricmp( token, "lightmap" ) )    {
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			} else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )     {
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			} else if ( !Q_stricmp( token, "vector" ) )    {
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) ) {
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					break;
				}
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) ) {
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		} else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			 blendSrcBits == GLS_SRCBLEND_ONE ||
			 blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	/* Native v59 parser promotes the implicit alpha of an authored
	 * exactVertex stage to vertex alpha (R19 parser and R21 consumer).
	 * Explicit alphaGen, including identity, remains authored. */
	if ( cod1Bsp59WorldMaterialSource && stage->rgbGen == CGEN_EXACT_VERTEX &&
		 !alphaGenExplicit && stage->alphaGen == AGEN_IDENTITY ) {
		stage->alphaGen = AGEN_VERTEX;
	}
#endif


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) ) {
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	/* The original CoD HUD mask uses DrawStretchPic color alpha for the ADS
	 * transition, despite omitting alphaGen. Restrict this compatibility
	 * default to that exact 2D mask; do not alter global RTCW shader defaults
	 * or materials with explicit non-identity alpha animation. */
	if ( shader.lightmapIndex == LIGHTMAP_2D &&
		 !Q_stricmp( shader.name, "ui/assets/reticle_circle_quarter" ) &&
		 stage->alphaGen == AGEN_IDENTITY ) {
		stage->alphaGen = AGEN_VERTEX;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == (R_Cod1NativeMaterialSource()?AGEN_IDENTITY:CGEN_IDENTITY) ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			 || stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
					   blendSrcBits | blendDstBits |
					   atestBits |
					   depthFuncBits;

	return qtrue;
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static qboolean ParseStage( shaderStage_t *stage, char **text ) {
	return ParseStageForSource( stage, text, qfalse );
}
#endif

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( char **text ) {
	char    *token;
	deformStage_t   *ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = DEFORM_TEXT0 + n;
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 ) {
			ds->deformationSpread = 1.0f / atof( token );
		} else
		{
			ds->deformationSpread = 100.0f;
			ri.Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static void ParseSkyParms( char **text ) {
	char        *token;
	static char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char pathname[MAX_QPATH];
	int i;
	int skyWrap = GL_CLAMP;
	/* GL_CLAMP blends in the default black border at UV 0/1. Sky faces
	 * meet at exactly those coordinates; use edge texels at every mip. */
	if ( atof( glConfig.version_string ) >= 1.2 ||
		 strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) ||
		 strstr( glConfig.extensions_string, "GL_SGIS_texture_edge_clamp" ) ) {
		skyWrap = GL_CLAMP_TO_EDGE;
	}

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, skyWrap );
			if ( !shader.sky.outerbox[i] ) {
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, skyWrap );
			if ( !shader.sky.innerbox[i] ) {
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( char **text ) {
	char    *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

    if(cod1BundleAuthStrict) {
        int value;
        if(Cod1PL_NativeSort(token,&value)) {shader.sort=(float)value;return;}
    }
    if(shader.cod1ShaderType) {
        static const struct {const char *name;float value;} extra[]={
            {"ocean",3},{"boathull",4},{"water",11},{"corona",12},
            {"innerBlend",13},{"outerBlend",14},{"blend",15},{"blend2",16},
            {"blend3",17},{"blend4",18}
        };
        unsigned i;
        for(i=0;i<sizeof(extra)/sizeof(extra[0]);++i)if(!Q_stricmp(token,extra[i].name)) {
            shader.sort=extra[i].value;return;
        }
    }
	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !Q_stricmp( token, "decal" ) )   {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
#if defined(COD1_BSP59_SORT_ADDITIVE_CANDIDATE) && COD1_BSP59_SORT_ADDITIVE_CANDIDATE
		/* Live CoDSP parser maps additive to 19.0; R41B also measured 19.0
		 * in two registered shoreline shaders. Port GL queue parity is pending. */
		shader.sort = shader.cod1Bsp59WorldMaterialSource ? 19.0f : SS_BLEND1;
#else
		shader.sort = SS_BLEND1;
#endif
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else {
		shader.sort = atof( token );
	}
}



// this table is also present in q3map

typedef struct {
	char    *name;
	int clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t infoParms[] = {
	// server relevant contents

//----(SA)	modified
	{"clipmissile",  1,  0, CONTENTS_MISSILECLIP},       // impact only specific weapons (rl, gl)
//----(SA)	end

// RF, AI sight
	{"ai_nosight",   1,  0,  CONTENTS_AI_NOSIGHT},
	{"clipshot", 1,  0,  CONTENTS_CLIPSHOT},         // stops bullets
// RF, end

	{"water",        1,  0,  CONTENTS_WATER },
	{"slag",     1,  0,  CONTENTS_SLIME },       // uses the CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.
	// (slime hurts instantly, slag doesn't)
//	{"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
	{"lava",     1,  0,  CONTENTS_LAVA },        // very damaging
	{"playerclip",   1,  0,  CONTENTS_PLAYERCLIP },
	{"monsterclip",  1,  0,  CONTENTS_MONSTERCLIP },
	{"nodrop",       1,  0,  CONTENTS_NODROP },      // don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid", 1,  SURF_NONSOLID,  0},                     // clears the solid flag

	// utility relevant attributes
	{"origin",       1,  0,  CONTENTS_ORIGIN },      // center of rotating brushes
	{"trans",        0,  0,  CONTENTS_TRANSLUCENT }, // don't eat contained surfaces
	{"detail",       0,  0,  CONTENTS_DETAIL },      // don't include in structural bsp
	{"structural",   0,  0,  CONTENTS_STRUCTURAL },  // force into structural bsp even if trnas
	{"areaportal",   1,  0,  CONTENTS_AREAPORTAL },  // divides areas
	{"clusterportal", 1,0,  CONTENTS_CLUSTERPORTAL },    // for bots
	{"donotenter",  1,  0,  CONTENTS_DONOTENTER },       // for bots

	// Rafael - nopass
	{"donotenterlarge", 1, 0,    CONTENTS_DONOTENTER_LARGE }, // for larger bots

	{"fog",          1,  0,  CONTENTS_FOG},          // carves surfaces entering
	{"sky",          0,  SURF_SKY,       0 },        // emit light from an environment map
	{"lightfilter",  0,  SURF_LIGHTFILTER, 0 },      // filter light going through it
	{"alphashadow",  0,  SURF_ALPHASHADOW, 0 },      // test light on a per-pixel basis
	{"hint",     0,  SURF_HINT,      0 },        // use as a primary splitter

	// server attributes
	{"slick",            0,  SURF_SLICK,     0 },
	{"noimpact",     0,  SURF_NOIMPACT,  0 },        // don't make impact explosions or marks
	{"nomarks",          0,  SURF_NOMARKS,   0 },        // don't make impact marks, but still explode
	{"ladder",           0,  SURF_LADDER,    0 },
	{"nodamage",     0,  SURF_NODAMAGE,  0 },

	{"monsterslick", 0,  SURF_MONSTERSLICK,  0},     // surf only slick for monsters

//	{"flesh",		0,	SURF_FLESH,		0 },
	{"glass",        0,  SURF_GLASS,     0 },    //----(SA)	added
	{"ceramic",      0,  SURF_CERAMIC,   0 },    //----(SA)	added

	// steps
	{"metal",        0,  SURF_METAL,     0 },
	{"metalsteps",   0,  SURF_METAL,     0 },    // retain bw compatibility with Q3A metal shaders... (SA)
	{"nosteps",      0,  SURF_NOSTEPS,   0 },
	{"woodsteps",    0,  SURF_WOOD,      0 },
	{"grasssteps",   0,  SURF_GRASS,     0 },
	{"gravelsteps",  0,  SURF_GRAVEL,    0 },
	{"carpetsteps",  0,  SURF_CARPET,    0 },
	{"snowsteps",    0,  SURF_SNOW,      0 },
	{"roofsteps",    0,  SURF_ROOF,      0 },    // tile roof

	{"rubble", 0, SURF_RUBBLE, 0 },

	// drawsurf attributes
	{"nodraw",       0,  SURF_NODRAW,    0 },    // don't generate a drawsurface (or a lightmap)
	{"pointlight",   0,  SURF_POINTLIGHT, 0 },   // sample lighting at vertexes
	{"nolightmap",   0,  SURF_NOLIGHTMAP,0 },        // don't generate a lightmap
	{"nodlight", 0,  SURF_NODLIGHT, 0 },     // don't ever add dynamic lights

	{"monsterslicknorth",    0, SURF_MONSLICK_N,0},
	{"monsterslickeast", 0, SURF_MONSLICK_E,0},
	{"monsterslicksouth",    0, SURF_MONSLICK_S,0},
	{"monsterslickwest", 0, SURF_MONSLICK_W,0}

};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( char **text ) {
	char    *token;
	int numInfoParms = sizeof( infoParms ) / sizeof( infoParms[0] );
	int i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if ( infoParms[i].clearSolid ) {
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static qboolean ParseShaderForSource( char **text, qboolean cod1Bsp59WorldMaterialSource ) {
#else
static qboolean ParseShader( char **text ) {
#endif
	char *token;
	int s;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' ) {
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' ) {
			break;
		}
		// stage definition
		else if ( token[0] == '{' ) {
            if(s>=MAX_SHADER_STAGES) {
                ri.Printf(PRINT_WARNING,"WARNING: too many stages in %s\n",shader.name);
                return qfalse;
            }
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
			if ( !ParseStageForSource( &stages[s], text, cod1Bsp59WorldMaterialSource ) ) {
#else
			if ( !ParseStage( &stages[s], text ) ) {
#endif
				return qfalse;
			}
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
            if(!stages[s].active) {
                stages[s].bundle[1].texMods=cod1Bundle1TexMods[s];
                stages[s].bundle[2].texMods=cod1Bundle2TexMods[s];
            }
#endif
            if(stages[s].active) s++;
            else stages[s].bundle[0].texMods=texMods[s];
            /* Inactive stage was explicitly consumed (cratered perlight). */
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "q3map_sun" ) ) {
			float a, b;

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		} else if ( !Q_stricmp( token, "deformVertexes" ) )    {
			ParseDeform( text );
			continue;
		} else if ( !Q_stricmp( token, "tesssize" ) )    {
			SkipRestOfLine( text );
			continue;
		} else if ( !Q_stricmp( token, "clampTime" ) )    {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] ) {
				shader.clampTime = atof( token );
			}
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) ) {
			shader.noMipMaps = qtrue;
            /* CoDSP 0x4eff63 / 0x4eff82 stores independent mip/pic flags. */
			if ( !shader.cod1ShaderType ) shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) ) {
			shader.noPicMip = qtrue;
			continue;
		}
		// character picmip adjustment
		else if ( !Q_stricmp( token, "picmip2" ) ) {
			shader.characterMip = qtrue;
			continue;
		}
		/* CoDSP 0x4effb0..0x4f001e selects exclusive offset modes, with the
		 * last keyword winning. GL dispatch 0x4cb8ed..0x4cb96d doubles both
		 * cvar values for offset2 and uses factor zero for offsetConst. */
		else if ( !Q_stricmp( token, "polygonOffset" ) ) {
			shader.polygonOffset = qtrue;
			shader.cod1PolygonOffsetMode = 0;
			continue;
		}
		else if ( !Q_stricmp( token, "polygonOffset2" ) ) {
			shader.polygonOffset = qtrue;
			shader.cod1PolygonOffsetMode = 1;
			continue;
		}
		else if ( !Q_stricmp( token, "polygonOffsetConst" ) ) {
			shader.polygonOffset = qtrue;
			shader.cod1PolygonOffsetMode = 2;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) ) {
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) ) {
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp( token, "portal" ) ) {
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) ) {
			ParseSkyParms( text );
			continue;
		}
		else if ( !Q_stricmp( token, "sunfile" ) ) {
            token = COM_ParseExt( text, qfalse );
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA && \
    defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
            if (cod1Bsp59WorldMaterialSource) {
                int captured;
                captured=Cod1Sun_CaptureName(tr.cod1SunFile,token);
                if(captured) {
                    shader.cod1SunFileSeen=qtrue;
                    Q_strncpyz(shader.cod1SunFile,tr.cod1SunFile,sizeof(shader.cod1SunFile));
                }
                if(captured<=0)ri.Printf(PRINT_WARNING,"CoD1 sunfile: %s name in %s\n",
                    captured<0?"overlong":"empty",shader.name);
            }
#else
            if ( !token[0] || !strcmp( token, "}" ) ) return qfalse;
            ri.Printf( PRINT_DEVELOPER, "CoD1 sky: sunfile '%s' deferred\n", token );
#endif
			continue;
		}
		// This is fixed fog for the skybox/clouds determined solely by the shader
		// it will not change in a level and will not be necessary
		// to force clients to use a sky fog the server says to.
		// skyfogvars <(r,g,b)> <dist>
		else if ( !Q_stricmp( token, "skyfogvars" ) ) {
			vec3_t fogColor;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for sky fog\n" );
				continue;
			}

			if ( atof( token ) > 1 ) {
				ri.Printf( PRINT_WARNING, "WARNING: last value for skyfogvars is 'density' which needs to be 0.0-1.0\n" );
				continue;
			}

			R_SetFog( FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], atof( token ) );
			continue;
		} else if ( !Q_stricmp( token, "sunshader" ) )        {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing shader name for 'sunshader'\n" );
				continue;
			}
			tr.sunShaderName = CopyString( token );
		}
//----(SA)	added
		else if ( !Q_stricmp( token, "lightgridmulamb" ) ) { // ambient multiplier for lightgrid
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid ambient multiplier'\n" );
				continue;
			}
			if ( atof( token ) > 0 ) {
				tr.lightGridMulAmbient = atof( token );
			}
		} else if ( !Q_stricmp( token, "lightgridmuldir" ) )        { // directional multiplier for lightgrid
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid directional multiplier'\n" );
				continue;
			}
			if ( atof( token ) > 0 ) {
				tr.lightGridMulDirected = atof( token );
			}
		}
//----(SA)	end
		else if ( !Q_stricmp( token, "waterfogvars" ) ) {
			vec3_t watercolor;
			float fogvar;
			char fogString[64];

			if ( !ParseVector( text, 3, watercolor ) ) {
				return qfalse;
			}
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density/distance value for water fog\n" );
				continue;
			}

			fogvar = atof( token );

			//----(SA)	right now allow one water color per map.  I'm sure this will need
			//			to change at some point, but I'm not sure how to track fog parameters
			//			on a "per-water volume" basis yet.

			if ( fogvar == 0 ) {       // '0' specifies "use the map values for everything except the fog color
				// TODO
			} else if ( fogvar > 1 )      { // distance "linear" fog
				Com_sprintf( fogString, sizeof( fogString ), "0 %d 1.1 %f %f %f 200", (int)fogvar, watercolor[0], watercolor[1], watercolor[2] );
//				R_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
			} else {                      // density "exp" fog
				Com_sprintf( fogString, sizeof( fogString ), "0 5 %f %f %f %f 200", fogvar, watercolor[0], watercolor[1], watercolor[2] );
//				R_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
			}

//		near
//		far
//		density
//		r,g,b
//		time to complete
			ri.Cvar_Set( "r_waterFogColor", fogString );

			continue;
		}
		// fogvars
		else if ( !Q_stricmp( token, "fogvars" ) ) {
			vec3_t fogColor;
			float fogDensity;
			int fogFar;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for the fog\n" );
				continue;
			}


			//----(SA)	NOTE:	fogFar > 1 means the shader is setting the farclip, < 1 means setting
			//					density (so old maps or maps that just need softening fog don't have to care about farclip)

			fogDensity = atof( token );
			if ( fogDensity >= 1 ) { // linear
				fogFar      = fogDensity;
			} else {
				fogFar      = 5;
			}

//			R_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
			ri.Cvar_Set( "r_mapFogColor", va( "0 %d %f %f %f %f 0", fogFar, fogDensity, fogColor[0], fogColor[1], fogColor[2] ) );
//			R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);

			continue;
		}
		// done.
		// Ridah, allow disable fog for some shaders
		else if ( !Q_stricmp( token, "nofog" ) ) {
			shader.noFog = qtrue;
			continue;
		}
		// done.
		// RF, allow each shader to permit compression if available
		else if ( !Q_stricmp( token, "allowcompress" ) ) {
			tr.allowCompress = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "nocompress" ) )   {
			tr.allowCompress = -1;
			continue;
		}
		// done.
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp( token, "light" ) ) {
			token = COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) ) {
				shader.cullType = CT_TWO_SIDED;
			} else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )      {
				shader.cullType = CT_BACK_SIDED;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) ) {
			ParseSort( text );
			continue;
		} else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !( shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	return qtrue;
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static qboolean ParseShader( char **text ) {
	return ParseShaderForSource( text, qfalse );
}
#endif

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void ) {
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
    if(shader.cod1WaterRuntime){shader.optimalStageIteratorFunc=RB_StageIteratorCod1Water;return;}
#endif
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(shader.cod1PerlightRuntime)return;
#endif

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky ) {
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		goto done;
	}

#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
	/* The lightmap fast path uses its own hardcoded GL_TexEnv and therefore
	 * cannot consume an explicit per-stage combine. */
	if ( R_Cod1AnyThirdBundle() || (shader.numUnfoggedPasses == 1 && stages[0].cod1TexEnv) ) {
		return;
	}
#endif

	if ( r_ignoreFastPath->integer ) {
		return;
	}

	//
	// see if this can go into the vertex lit fast path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			if ( stages[0].alphaGen == AGEN_IDENTITY ) {
				if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE ) {
					if ( !shader.polygonOffset ) {
						if ( !shader.multitextureEnv ) {
							if ( !shader.numDeforms ) {
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								goto done;
							}
						}
					}
				}
			}
		}
	}

	//
	// see if this can go into an optimized LM, multitextured path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( ( stages[0].rgbGen == CGEN_IDENTITY ) && ( stages[0].alphaGen == AGEN_IDENTITY ) ) {
			if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE &&
				 stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP ) {
				if ( !shader.polygonOffset ) {
					if ( !shader.numDeforms ) {
						if ( shader.multitextureEnv ) {
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							goto done;
						}
					}
				}
			}
		}
	}

done:
	return;
}

typedef struct {
	int blendA;
	int blendB;

	int multitextureEnv;
	int multitextureBlend;
} collapse_t;

static collapse_t collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
	  GL_DECAL, 0 },
#endif
	{ -1 }
};

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static qboolean CollapseMultitexture( void ) {
	int abits, bbits;
	int i;
	textureBundle_t tmpBundle;

	if ( !qglActiveTextureARB ) {
		return qfalse;
	}

    /* Already bundled CoD stages must not be collapsed a second time. */
    if(stages[0].bundle[1].image[0] || stages[1].bundle[1].image[0] ||
       R_Cod1ThirdBundle(&stages[0]) || R_Cod1ThirdBundle(&stages[1]))return qfalse;
	// make sure both stages are active
	if ( !stages[0].active || !stages[1].active ) {
		return qfalse;
	}

	// on voodoo2, don't combine different tmus
	if ( glConfig.driverType == GLDRV_VOODOO ) {
		if ( stages[0].bundle[0].image[0]->TMU ==
			 stages[1].bundle[0].image[0]->TMU ) {
			return qfalse;
		}
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ( ( abits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) !=
		 ( bbits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) ) {
		return qfalse;
	}

	abits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	bbits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

	// search for a valid multitexture blend function
	for ( i = 0; collapse[i].blendA != -1 ; i++ ) {
		if ( abits == collapse[i].blendA
			 && bbits == collapse[i].blendB ) {
			break;
		}
	}

	// nothing found
	if ( collapse[i].blendA == -1 ) {
		return qfalse;
	}

	// GL_ADD is a separate extension
	if ( collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return qfalse;
	}

	/* Native 0x4f08b7..0x4f08d3 retains the first stage's generators
	 * when a modulating second stage supplies identity RGB and alpha.
	 * In particular, diffuse vertex color and an identity lightmap form
	 * one fogged draw. Keep the legacy generator equality rule otherwise. */
	if ( ( stages[0].rgbGen != stages[1].rgbGen ) ||
		 ( stages[0].alphaGen != stages[1].alphaGen ) ) {
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
		if ( !R_Cod1NativeMaterialSource() ||
			 collapse[i].multitextureEnv != GL_MODULATE ||
			 stages[1].rgbGen != CGEN_IDENTITY ||
			 ( stages[1].alphaGen != AGEN_IDENTITY &&
			   stages[1].alphaGen != AGEN_SKIP ) )
#endif
		{
		return qfalse;
		}
	}

	// an add collapse can only have identity colors
	if ( collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY ) {
		return qfalse;
	}

	if ( stages[0].rgbGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].rgbWave,
					 &stages[1].rgbWave,
					 sizeof( stages[0].rgbWave ) ) ) {
			return qfalse;
		}
	}
	if ( stages[0].alphaGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].alphaWave,
					 &stages[1].alphaWave,
					 sizeof( stages[0].alphaWave ) ) ) {
			return qfalse;
		}
	}


	// make sure that lightmaps are in bundle 1 for 3dfx
	if ( stages[0].bundle[0].isLightmap ) {
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	} else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove( &stages[1], &stages[2], sizeof( stages[0] ) * ( MAX_SHADER_STAGES - 2 ) );
	memset( &stages[MAX_SHADER_STAGES - 1], 0, sizeof( stages[0] ) );

	return qtrue;
}


/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int i;
	float sort;
	shader_t    *newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t    *newShader;
	int i, b;
	int size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n" );
		return tr.defaultShader;
	}

	// Ridah, caching system
	newShader = R_CacheShaderAlloc( sizeof( shader_t ) );

	*newShader = shader;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(shader.cod1PerlightRuntime) {
        cod1PerlightRuntime_t *program=R_CacheShaderAlloc(sizeof(*program));
        *program=*shader.cod1PerlightRuntime;
        for(i=0;i<program->numStages;++i)for(b=0;b<NUM_TEXTURE_BUNDLES;++b) {
            textureBundle_t *bundle=&program->stage[i].bundle[b];
            if(!bundle->numTexMods)bundle->texMods=NULL;
            else {
                texModInfo_t *old=bundle->texMods;
                size=bundle->numTexMods*sizeof(*old);
                bundle->texMods=R_CacheShaderAlloc(size);
                memcpy(bundle->texMods,old,size);
            }
        }
        newShader->cod1PerlightRuntime=program;
    }
#endif

	if ( shader.sort <= SS_OPAQUE ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			newShader->stages[i] = NULL;    // Ridah, make sure it's null
			break;
		}
		// Ridah, caching system
		newShader->stages[i] = R_CacheShaderAlloc( sizeof( stages[i] ) );

		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			if ( !newShader->stages[i]->bundle[b].numTexMods ) {
				// make sure unalloc'd texMods aren't pointing to some random point in memory
				newShader->stages[i]->bundle[b].texMods = NULL;
				continue;
			}
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			// Ridah, caching system
			newShader->stages[i]->bundle[b].texMods = R_CacheShaderAlloc( size );

			memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue( newShader->name );
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int stage;
	shaderStage_t   *bestStage;
	int bestImageRank;
	int rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		memset( pStage, 0, sizeof( *pStage ) );
	}
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage, i;
	qboolean hasLightmapStage;
	qboolean vertexLightmap;

	hasLightmapStage = qfalse;
	vertexLightmap = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		// check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( !shader.cod1ShaderType && !R_Cod1ThirdBundle(pStage) && pStage->isDetail && !r_detailTextures->integer ) {
			if ( stage < ( MAX_SHADER_STAGES - 1 ) ) {
				memmove( pStage, pStage + 1, sizeof( *pStage ) * ( MAX_SHADER_STAGES - stage - 1 ) );
				// kill the last stage, since it's now a duplicate
				for ( i = MAX_SHADER_STAGES - 1; i > stage; i-- ) {
					if ( stages[i].active ) {
						memset(  &stages[i], 0, sizeof( *pStage ) );
						break;
					}
				}
				stage--;    // the next stage is now the current stage, so check it again
			} else {
				memset( pStage, 0, sizeof( *pStage ) );
			}
			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}


        for(i=1;i<NUM_TEXTURE_BUNDLES;++i)
            if(pStage->bundle[i].isLightmap && pStage->bundle[i].image[0])hasLightmapStage=qtrue;

        if(R_Cod1NativeMaterialSource() && !(pStage->stateBits&GLS_ATEST_BITS)) {
            unsigned blend=pStage->stateBits&(GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS);
            if(blend==(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE) ||
               blend==(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
                pStage->stateBits|=GLS_ATEST_GT_0;
        }

		// not a true lightmap but we want to leave existing
		// behaviour in place and not print out a warning
		//if (pStage->rgbGen == CGEN_VERTEX) {
		//  vertexLightmap = qtrue;
		//}



		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				 ( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
                    shader.sort = (shader.cod1ShaderType &&
                        (stages[0].stateBits&GLS_DSTBLEND_BITS)==GLS_DSTBLEND_ONE)?SS_BLEND1:SS_BLEND0;
				}
			}
		}
	}

	/* Static XModels supply prepared diffuse RGB through their vertex stream.
	 * Bind authored lightingDiffuse after stage validation, for both explicit
	 * scripts and @ types, before collapse and iterator selection. The static
	 * registration role owns this candidate; alpha and other RGB inputs retain
	 * their authored rules. No cached or non-static shader is rewritten. */
	shader.cod1StaticLightingDiffuseBridge = qfalse;
	if ( shader.cod1StaticModelSource && !shader.defaultShader ) {
		for ( i = 0; i < stage && stages[i].active; i++ ) {
			if ( stages[i].rgbGen == CGEN_LIGHTING_DIFFUSE ) {
				shader.cod1StaticLightingDiffuseBridge = qtrue;
				stages[i].rgbGen = CGEN_EXACT_VERTEX;
			}
		}
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( !shader.cod1ShaderType && !R_Cod1AnyThirdBundle()
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
         && !shader.cod1WaterRuntime
#endif
         && stage > 1 && ( ( r_vertexLight->integer && !r_uiFullScreen->integer ) || glConfig.hardwareType == GLHW_PERMEDIA2 ) ) {
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if ( stage > 1
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
         && !shader.cod1WaterRuntime
#endif
         && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex >= 0 && !hasLightmapStage ) {
		if ( vertexLightmap ) {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
		} else {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
#if COD1_NATIVE_LIGHTMAP_PACK
			if ( cod1PackedWorldShaders
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
			     && shader.cod1Bsp59WorldMaterialSource
#endif
			     ) {
				shader.cod1PackedLightmapFree=qtrue;
			}
#endif
			shader.lightmapIndex = LIGHTMAP_NONE;
		}
	}

#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP && \
    defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	/* Stage policy follows the v59 material-table request, including the
	 * legacy parser and diffuse fallback. It never follows an image name. */
	if ( shader.cod1Bsp59WorldMaterialSource ) {
		for ( i = 0; i < stage; i++ ) {
			stages[i].cod1NativeFogColor=qtrue;
		}
	}
#endif


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if ( stage == 0 ) {
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// RF default back to no compression for next shader
	if ( r_ext_compressed_textures->integer == 2 ) {
		tr.allowCompress = qfalse;
	}

	return GeneratePermanentShader();
}

#if COD1_NATIVE_LIGHTMAP_PACK
static qboolean R_Cod1PackedLightmapFreeMatch( const shader_t *sh, int lightmapIndex ) {
	return cod1PackedWorldShaders && lightmapIndex >= 0 &&
		sh->lightmapIndex == LIGHTMAP_NONE && sh->cod1PackedLightmapFree;
}
#endif

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char *FindShaderInShaderText( const char *shadername ) {
	char *p = s_shaderText;
	char *token;

	if ( !p ) {
		return NULL;
	}

	// Ridah, optimized shader loading
	{
		shaderStringPointer_t *pShaderString;
		unsigned short int checksum;

		checksum = generateHashValue( shadername );

		// if it's known, skip straight to it's position
		pShaderString = &shaderChecksumLookup[checksum];
		while ( pShaderString && pShaderString->pStr ) {
			p = pShaderString->pStr;

			token = COM_ParseExt( &p, qtrue );

			if ( ( token[0] != 0 ) && !Q_stricmp( token, shadername ) ) {
				return p;
			}

			pShaderString = pShaderString->next;
		}
	}

	// done.

	/*
	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( token[0] == '{' ) {
			// skip the definition
			SkipBracedSection( &p );
		} else if ( !Q_stricmp( token, shadername ) ) {
			return p;
		} else {
			// skip to end of line
			SkipRestOfLine( &p );
		}
	}
	*/

	return NULL;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh;

	if ( ( name == NULL ) || ( name[0] == 0 ) ) {  // bk001205
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( Q_stricmp( sh->name, strippedName ) == 0 ) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}


/*
========================
R_CoD1FindImageCandidate

CoDA's working material viewer taught us that CoD1 shader/material names often
resolve to DDS directly, while some variants use .blend/.masked suffixes.  Keep
that knowledge in the renderer compatibility path instead of hard-coding map
material names.
========================
*/
static image_t *R_CoD1FindImageCandidate( const char *name, qboolean mipRawImage, int wrapMode ) {
	char fileName[MAX_QPATH];
	image_t *image;
	const char *extensions[] = { ".dds", ".tga", ".jpg", ".blend.dds", ".blend.tga", ".masked.dds", ".masked.tga" };
	const char *slash, *backslash, *dot, *lastSep;
	int i;

	if ( !name || !name[0] || name[0] == '$' ) {
		return NULL;
	}

	/* Exact path first when the shader text already supplies an extension. */
	slash = strrchr( name, '/' );
	backslash = strrchr( name, '\\' );
	lastSep = slash;
	if ( backslash && ( !lastSep || backslash > lastSep ) ) {
		lastSep = backslash;
	}
	dot = strrchr( name, '.' );
	if ( dot && ( !lastSep || dot > lastSep ) ) {
		image = R_FindImageFile( name, mipRawImage, mipRawImage, wrapMode );
		if ( image ) {
			return image;
		}
	}

	for ( i = 0; i < (int)( sizeof( extensions ) / sizeof( extensions[0] ) ); i++ ) {
		Q_strncpyz( fileName, name[0] == '*' ? name + 1 : name, sizeof( fileName ) );
		COM_StripExtension( fileName, fileName );
		Q_strcat( fileName, sizeof( fileName ), extensions[i] );
		image = R_FindImageFile( fileName, mipRawImage, mipRawImage, wrapMode );
		if ( image ) {
			ri.Printf( PRINT_DEVELOPER, "CoD material image fallback: %s -> %s\n", name, fileName );
			return image;
		}
	}
	return NULL;
}




/* Source-level probe over any explicit shader. There is no name table and no
 * call into FinishShader or GL. Hash the source following the material name,
 * including whitespace before '{': shaderTextStart points into the combined
 * in-memory script text just after COM_ParseExt has consumed the name.
 */
#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
#define COD1_MP_SEEN_SIZE 4096
typedef struct {
    char name[MAX_QPATH];
    unsigned long sourceOffset;
    unsigned hash;
    unsigned selectedMask;
    unsigned unknownMask;
    unsigned candidateMask;
    unsigned blockedMask;
    unsigned excludedMask;
    int sourceValid;
    unsigned uses;
} cod1MpSeen_t;
static cod1MpSeen_t cod1MpSeen[COD1_MP_SEEN_SIZE];
static unsigned cod1MpUnique, cod1MpDuplicates, cod1MpOverflow;
typedef struct {
    char name[MAX_QPATH];
    unsigned long sourceOffset;
    int requestedLightmap;
} cod1MpCompiledSeen_t;
static cod1MpCompiledSeen_t cod1MpCompiledSeen[COD1_MP_SEEN_SIZE];
static unsigned cod1MpCompiledUnique,cod1MpCompiledDuplicates,cod1MpCompiledOverflow;
static unsigned cod1MpCompiledReentrant;
static int cod1MpCompiledLogging;

static cod1MpTruth_t R_Cod1ProbeExtension(const char *name,void *unused) {
    (void)unused;
    /* The R5 backend does not execute these authored GPU paths even if the
     * driver's extension string advertises them. Other caps stay unknown. */
    if (!strcmp(name,"GL_NV_texture_shader") || !strcmp(name,"GL_NV_register_combiners") ||
        !strcmp(name,"GL_ATI_fragment_shader") || !strcmp(name,"GL_ARB_texture_cube_map"))
        return COD1_MP_FALSE;
    return COD1_MP_UNKNOWN;
}
static cod1MpSeen_t *R_Cod1ProbeClaim(const char *name,unsigned long offset) {
    unsigned i,slot=((unsigned)generateHashValue(name)^(unsigned)offset)&(COD1_MP_SEEN_SIZE-1);
    for (i=0;i<COD1_MP_SEEN_SIZE;++i) {
        cod1MpSeen_t *seen=&cod1MpSeen[(slot+i)&(COD1_MP_SEEN_SIZE-1)];
        if (!seen->name[0]) {
            Q_strncpyz(seen->name,name,sizeof(seen->name));
            seen->sourceOffset=offset;seen->uses=1;++cod1MpUnique;
            return seen;
        }
        if (seen->sourceOffset==offset && !Q_stricmp(seen->name,name)) {
            ++seen->uses;++cod1MpDuplicates;
            return NULL;
        }
    }
    ++cod1MpOverflow;
    return NULL;
}
static const cod1MpSeen_t *R_Cod1ProbeSource(const char *name,unsigned long offset) {
    unsigned i,slot=((unsigned)generateHashValue(name)^(unsigned)offset)&(COD1_MP_SEEN_SIZE-1);
    for (i=0;i<COD1_MP_SEEN_SIZE;++i) {
        const cod1MpSeen_t *seen=&cod1MpSeen[(slot+i)&(COD1_MP_SEEN_SIZE-1)];
        if (!seen->name[0]) return NULL;
        if (seen->sourceOffset==offset && !Q_stricmp(seen->name,name)) return seen;
    }
    return NULL;
}
static int R_Cod1ProbeClaimCompiled(const char *name,unsigned long offset,int requestedLm) {
    unsigned i,slot=((unsigned)generateHashValue(name)^(unsigned)offset^
        ((unsigned)requestedLm*2654435761U))&(COD1_MP_SEEN_SIZE-1);
    for (i=0;i<COD1_MP_SEEN_SIZE;++i) {
        cod1MpCompiledSeen_t *seen=&cod1MpCompiledSeen[(slot+i)&(COD1_MP_SEEN_SIZE-1)];
        if (!seen->name[0]) {
            Q_strncpyz(seen->name,name,sizeof(seen->name));
            seen->sourceOffset=offset;seen->requestedLightmap=requestedLm;
            ++cod1MpCompiledUnique;
            return 1;
        }
        if (seen->sourceOffset==offset && seen->requestedLightmap==requestedLm &&
            !Q_stricmp(seen->name,name)) {
            ++cod1MpCompiledDuplicates;
            return 0;
        }
    }
    ++cod1MpCompiledOverflow;
    return 0;
}
static void R_Cod1ProbeMaterial(const char *name,const char *body) {
    cod1MpCaps_t caps;
    cod1MeBackend_t backend;
    cod1MeResult_t eligibility;
    cod1MaterialProbe_t ir;
    cod1MpSeen_t *seen;
    char stagesReport[512],imageReport[1024],part[256];
    unsigned long offset;
    int i,j,valid,more=0;
    offset=(unsigned long)(body-s_shaderText);
    seen=R_Cod1ProbeClaim(name,offset);
    if (!seen) return;
    caps.units=qglActiveTextureARB?2:1;
    caps.extension=R_Cod1ProbeExtension;
    caps.cvar=NULL;caps.user=NULL;
    valid=Cod1MP_Parse(body,&caps,&ir);
    seen->hash=valid?ir.sourceHash:0;
    seen->sourceValid=valid;
    seen->selectedMask=valid?ir.selectedMask:0;
    seen->unknownMask=valid?ir.unknownMask:0;
    if (!valid) {
        ri.Printf(PRINT_WARNING,
            "CoD1 material probe: source=combined+%lu name=%s parser=unsupported (%s); render path unchanged\n",
            offset,name,ir.error);
        return;
    }
    /* This is a declared implemented profile, not an inference that any
     * advertised driver extension has a corresponding renderer path. */
    backend.maxBundles=caps.units;
    Cod1ME_Evaluate(&ir,&backend,&eligibility);
    seen->candidateMask=eligibility.candidateMask;
    seen->blockedMask=eligibility.blockedMask;
    seen->excludedMask=eligibility.excludedMask;
    stagesReport[0]=0;
    imageReport[0]=0;
    for (i=0;i<ir.numStages;++i) {
        const cod1MpStage_t *s=&ir.stage[i];
        if (s->selected==COD1_MP_FALSE) continue;
        Com_sprintf(part,sizeof(part),"%s%d:%c:%s:b%d",
            stagesReport[0]?",":"",i,s->selected==COD1_MP_TRUE?'T':'?',
            s->blockingOpcode[0]?s->blockingOpcode:"none",s->numBundles);
        Q_strcat(stagesReport,sizeof(stagesReport),part);
        /* Asset probes are deliberately deferred in the live loader:
         * R_FindImageFileExt may upload a texture and mutate image caches. */
        if (s->selected!=COD1_MP_TRUE) continue;
        for (j=0;j<s->numOps;++j) {
            cod1MpImageEvent_t ev;
            int imageOp=Cod1MP_ImageEvent(&ir,&s->op[j],&ev);
            if (!imageOp) continue;
            if (imageOp<0) {
                Com_sprintf(part,sizeof(part),"%ss%d:b%d:invalid-image-op",
                    imageReport[0]?",":"",i,s->op[j].bundle);
            } else if (ev.kind==COD1_MP_NEXTBUNDLE) {
                Com_sprintf(part,sizeof(part),"%ss%d:b%d:nextbundle",
                    imageReport[0]?",":"",i,ev.bundle);
            } else {
                Com_sprintf(part,sizeof(part),"%ss%d:b%d:%s:%s:deferred(%s)",
                    imageReport[0]?",":"",i,ev.bundle,
                    ev.kind==COD1_MP_CLAMPMAP?"clampmap":(ev.clamp?"map(clamp)":"map"),ev.path,
                    ev.path[0]=='$'?"symbol":"file");
            }
            if (strlen(imageReport)+strlen(part)>=sizeof(imageReport)-24) { more=1;break; }
            Q_strcat(imageReport,sizeof(imageReport),part);
        }
        if (more) break;
    }
    if (more) Q_strcat(imageReport,sizeof(imageReport),",...");
    ri.Printf(PRINT_WARNING,
        "CoD1 material probe: source=combined+%lu hash=%08x name=%s backendBundles=%d "
        "selected=%08x unknown=%08x sort=%s topBlock=%s effectiveGlobal=%s stages=[%s] images=[%s] "
        "candidate=%08x blocked=%08x excluded=%08x; "
#if defined(COD1_MATERIAL_STAGE_COMPILER) && COD1_MATERIAL_STAGE_COMPILER
        "source diagnostic only; compiler separately gated\n",
#else
        "render path unchanged\n",
#endif
        offset,ir.sourceHash,name,caps.units,ir.selectedMask,ir.unknownMask,
        ir.sortToken[0]?ir.sortToken:"-",
        ir.topBlockingOpcode[0]?ir.topBlockingOpcode:"none",
        eligibility.globalBlock[0]?eligibility.globalBlock:"none",stagesReport,imageReport,
        eligibility.candidateMask,eligibility.blockedMask,eligibility.excludedMask);
    for (i=0;i<ir.numStages;++i) {
        ri.Printf(PRINT_WARNING,
            "CoD1 stage eligibility: source=combined+%lu hash=%08x name=%s "
            "stage=%d line=%d requires=%s status=%s reason=%s bundles=%d "
            "globalBlock=%s; source-only\n",
            offset,ir.sourceHash,name,i,ir.stage[i].line,
            ir.stage[i].selected==COD1_MP_TRUE?"true":
                (ir.stage[i].selected==COD1_MP_FALSE?"false":"unknown"),
            Cod1ME_Status(eligibility.stage[i].status),eligibility.stage[i].reason,
            ir.stage[i].numBundles,
            eligibility.globalBlock[0]?eligibility.globalBlock:"none");
    }
}

/* Observe the finalized shader only after R_FindShader has completed its
 * existing parse/fallback path. Never call image lookup or GL from here. */
static void R_Cod1ProbeCompiled(const char *name,const char *body,int requestedLm,
                                const shader_t *compiled,const char *result) {
    unsigned long offset=(unsigned long)(body-s_shaderText);
    const cod1MpSeen_t *source;
    char report[1024],part[192];
    const char *iterator="none";
    int i,b;
    if (cod1MpCompiledLogging) { ++cod1MpCompiledReentrant;return; }
    if (!R_Cod1ProbeClaimCompiled(name,offset,requestedLm)) return;
    source=R_Cod1ProbeSource(name,offset);
    report[0]=0;
    if (compiled) {
        if (compiled->optimalStageIteratorFunc==RB_StageIteratorGeneric) iterator="generic";
        else if (compiled->optimalStageIteratorFunc==RB_StageIteratorLightmappedMultitexture) iterator="lightmap-fast";
        else if (compiled->optimalStageIteratorFunc==RB_StageIteratorVertexLitTexture) iterator="vertex-fast";
        else if (compiled->optimalStageIteratorFunc==RB_StageIteratorSky) iterator="sky";
        else iterator="other";
        for (i=0;i<compiled->numUnfoggedPasses && i<MAX_SHADER_STAGES;++i) {
            const shaderStage_t *stage=compiled->stages[i];
            if (!stage) break;
            for (b=0;b<NUM_TEXTURE_BUNDLES;++b) {
                const image_t *im=stage->bundle[b].image[0];
                if (!im) continue;
                Com_sprintf(part,sizeof(part),"%ss%d:b%d:%s%s",
                    report[0] ? ",":"",i,b,
                    stage->bundle[b].isLightmap ? "LM:":"",
                    im->imgName[0] ? im->imgName : "(unnamed)");
                if (strlen(report)+strlen(part)>=sizeof(report)-8) {
                    Q_strcat(report,sizeof(report),",...");
                    i=MAX_SHADER_STAGES;break;
                }
                Q_strcat(report,sizeof(report),part);
            }
        }
    }
    cod1MpCompiledLogging=1;
    ri.Printf(PRINT_WARNING,
        "CoD1 material result: source=combined+%lu hash=%08x name=%s "
        "requestedLm=%d selected=%08x unknown=%08x candidate=%08x blocked=%08x "
        "excluded=%08x sourceIR=%s result=%s "
        "passes=%d sort=%g lm=%d multiEnv=%d iterator=%s "
        "r_vertexLight=%d r_lightmap=%d r_ignoreFastPath=%d compiled=[%s]; observe only\n",
        offset,source?source->hash:0,name,requestedLm,
        source?source->selectedMask:0,source?source->unknownMask:0,
        source?source->candidateMask:0,source?source->blockedMask:0,
        source?source->excludedMask:0,
        source?(source->sourceValid?"ok":"unsupported"):"unavailable",result,
        compiled?compiled->numUnfoggedPasses:-1,compiled?compiled->sort:-1.0f,
        compiled?compiled->lightmapIndex:LIGHTMAP_NONE,
        compiled?compiled->multitextureEnv:0,iterator,
        r_vertexLight?r_vertexLight->integer:-1,
        r_lightmap?r_lightmap->integer:-1,
        r_ignoreFastPath?r_ignoreFastPath->integer:-1,
        report);
    cod1MpCompiledLogging=0;
}
#endif

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
/* R47 replaces family guesses with the original shader-type source path. */

static cod1MpTruth_t R_Cod1StypeExtension(const char *name,void *unused) {
    (void)unused;
    if(Cod1MP_Equal(name,"GL_ARB_texture_env_add"))
        return glConfig.textureEnvAddAvailable?COD1_MP_TRUE:COD1_MP_FALSE;
    /* Advertise implemented fixed-function backend, not vendor hardware. */
    if (Cod1ME_OneOf(name,"GL_NV_texture_shader GL_NV_texture_shader2 GL_NV_texture_shader3 GL_NV_register_combiners GL_NV_register_combiners2 GL_ATI_fragment_shader"))
        return COD1_MP_FALSE;
    return COD1_MP_UNKNOWN;
}
#include "cod1_perlight_static.h"
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#include "tr_cod1_perlight_compile.h"
#endif
static void R_Cod1MaterialErrorStages(void);
static qboolean R_Cod1CompilePerlightStatic(const char *body,qboolean worldSource,
                                           qboolean beforeLegacy) {
    cod1PlStaticResult_t result;cod1MpCaps_t caps;cod1StResult_t auth;
    char *projected,*cursor;int ok,i;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    cod1PerlightSnapshot_t *saved=NULL;
    qboolean unsupported;
#endif
    if(!body || !worldSource || !cod1Bsp59MapActive ||
       r_vertexLight->integer || r_lightmap->integer ||
       shader.lightmapIndex<0 || shader.lightmapIndex>=tr.numLightmaps ||
       !tr.lightmaps[shader.lightmapIndex])return qfalse;
    memset(&caps,0,sizeof(caps));
    /* Native requires tests the effective GL capability. The projection
     * validates ordinary storage separately against its two-bundle limit;
     * lowering this operand would incorrectly exclude selected 3-unit passes. */
    caps.units=R_Cod1NativeTextureUnits();
    caps.extension=R_Cod1StypeExtension;
    projected=(char*)malloc(COD1_ST_BODY);if(!projected)return qfalse;
    if(!Cod1PL_Prepare(body,&caps,projected,COD1_ST_BODY,&result)) {
        free(projected);
        /* The optional first attempt must leave unsupported historical
         * source shapes for their existing compiler. A supported source
         * which fails image/backend resolution still owns its failure. */
        if(beforeLegacy)return qfalse;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
        if(R_Cod1SelectedPerlight(body,&caps)) {
            ri.Printf(PRINT_WARNING,"CoD1 R62 perLight static preflight: %s line=%d reason=%s\n",shader.name,result.line,result.reason);
            R_Cod1MaterialErrorStages();
            shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PARSE_FAILED;
            return qtrue;
        }
#endif
        return qfalse;
    }
#if !defined(COD1_STAGE_BACKEND_INFRA) || !COD1_STAGE_BACKEND_INFRA
    if(result.multiplyIdentityMask) {free(projected);return qfalse;}
#endif
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    /* Validate both ordinary and retained stages before image lookup. */
    if(!R_Cod1PerlightPreflight(body,&caps)) {
        if(beforeLegacy) {free(projected);return qfalse;}
        free(projected);R_Cod1MaterialErrorStages();
        shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PARSE_FAILED;
        return qtrue;
    }
#endif
    memset(&auth,0,sizeof(auth));
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(beforeLegacy) {
        saved=(cod1PerlightSnapshot_t*)malloc(sizeof(*saved));
        if(!saved) {free(projected);return qfalse;}
        saved->shader=shader;memcpy(saved->stages,stages,sizeof(stages));
        for(i=0;i<MAX_SHADER_STAGES;++i) {
            memcpy(saved->mods[i][0],texMods[i],sizeof(texMods[i]));
            memcpy(saved->mods[i][1],cod1Bundle1TexMods[i],sizeof(texMods[i]));
            memcpy(saved->mods[i][2],cod1Bundle2TexMods[i],sizeof(texMods[i]));
        }
    }
#endif
    for(i=0;i<result.stages && i<8;++i)auth.bundleAuth[i]=result.bundleAuth[i];
    R_Cod1SetBundleAuth(&auth,1);cod1MultiplyIdentityStages=result.multiplyIdentityMask;
    cursor=projected;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    ok=ParseShaderForSource(&cursor,worldSource);
#else
    ok=ParseShader(&cursor);
#endif
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    unsupported=cod1MultiplyIdentityUnsupported;
#endif
    R_Cod1ClearBundleAuth();
    free(projected);
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(!ok && beforeLegacy && unsupported) {
        /* Unsupported native image codecs are outside this decoded RGBA
         * contract. Restore the complete untouched compiler scratch
         * before the existing source-shaped legacy path gets its attempt. */
        shader=saved->shader;memcpy(stages,saved->stages,sizeof(stages));
        for(i=0;i<MAX_SHADER_STAGES;++i) {
            memcpy(texMods[i],saved->mods[i][0],sizeof(texMods[i]));
            memcpy(cod1Bundle1TexMods[i],saved->mods[i][1],sizeof(texMods[i]));
            memcpy(cod1Bundle2TexMods[i],saved->mods[i][2],sizeof(texMods[i]));
        }
        ri.Printf(PRINT_ALL,"CoD1 R65 perLight: %s image multiplier codec unsupported; legacy source compiler retained\n",shader.name);
        free(saved);return qfalse;
    }
    free(saved);
    if(ok)ok=R_Cod1CompilePerlightRuntime(body,&caps);
#endif
    if(!ok) {
        /* Recognized source failed image/backend resolution. Never publish
         * earlier passes or fall back to a guessed first-image shader. */
        R_Cod1MaterialErrorStages();
        shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PARSE_FAILED;
        return qtrue;
    }
    shader.cod1NativeSurfaceFlags=result.nativeSurfaceFlags;
    if(result.nativeSurfaceFlags&0x4u)shader.surfaceFlags|=SURF_SKY;
    if(result.nativeSurfaceFlags&0x10u)shader.surfaceFlags|=SURF_NOIMPACT;
    if(result.nativeSurfaceFlags&0x20u)shader.surfaceFlags|=SURF_NOMARKS;
    if(result.nativeSurfaceFlags&0x20000u)shader.surfaceFlags|=SURF_NODLIGHT;
    shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PERLIGHT_PARTIAL;
    shader.explicitlyDefined=qtrue;
    ri.Printf(PRINT_ALL,"CoD1 R60 perlight projection: material=%s hash=%08x status=partial_perlight selected=%08x excluded=%08x emitted=%08x omitted=%08x staticStages=%d omittedStages=%d nativeSurface=%08x ignoredSurface=%d runtimeHints=%d\n",
        shader.name,result.hash,result.selected,result.excluded,result.emitted,result.omitted,
        result.stages,result.omittedStages,result.nativeSurfaceFlags,result.ignoredSurfaceParms,result.ignoredRuntimeHints);
    return qtrue;
}
static void R_Cod1MaterialErrorStages(void) {
    int i;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    shader.cod1PerlightRuntime=NULL;
    cod1PerlightCompiling=qfalse;
#endif
    memset(stages,0,sizeof(stages));
    R_Cod1ClearBundleAuth();
    for(i=0;i<MAX_SHADER_STAGES;i++) {
        stages[i].bundle[0].texMods=texMods[i];
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
        stages[i].bundle[1].texMods=cod1Bundle1TexMods[i];
        stages[i].bundle[2].texMods=cod1Bundle2TexMods[i];
#endif
    }
    /* Explicit failure marker, never claim an unsupported material is an
     * ordinary diffuse surface. No partially parsed pass is published. */
    shader.defaultShader=qtrue;shader.multitextureEnv=0;
    shader.cullType=CT_FRONT_SIDED;shader.polygonOffset=qfalse;shader.cod1PolygonOffsetMode=0;shader.isSky=qfalse;
    shader.sort=SS_OPAQUE;shader.noMipMaps=qfalse;shader.noPicMip=qfalse;
    shader.numDeforms=0;shader.contentFlags=0;shader.surfaceFlags=0;
    stages[0].active=qtrue;stages[0].bundle[0].image[0]=tr.defaultImage;
    stages[0].rgbGen=CGEN_IDENTITY;stages[0].alphaGen=AGEN_IDENTITY;
    stages[0].stateBits=GLS_DEFAULT;
}
/* CoD world scripts need the same bounded selected-stage validation as a
 * type. Legacy two-bundle scripts without requires retain their old route.
 * Projection is private and a selected unsupported command fails atomically. */
static qboolean R_Cod1CompileRequiredSource(const char *body,qboolean worldSource) {
    cod1MaterialProbe_t *ir;cod1MpCaps_t caps;cod1StResult_t result;
    char *projected=NULL,*isolated=NULL,*cursor;int i,needed=0,ok=0;
    if(!body || !worldSource || !cod1Bsp59MapActive)return qfalse;
    memset(&caps,0,sizeof(caps));memset(&result,0,sizeof(result));
    caps.units=R_Cod1NativeTextureUnits();
    caps.extension=R_Cod1StypeExtension;
    ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));if(!ir)return qfalse;
    if(!Cod1MP_Parse(body,&caps,ir)) {free(ir);return qfalse;}
    for(i=0;i<ir->numStages;++i)
        if(ir->stage[i].numRequires || ir->stage[i].numBundles>2)needed=1;
    if(!needed) {free(ir);return qfalse;}
    /* FindShader returns a pointer into the whole combined file. Copy only
     * the parsed balanced body; later definitions are not this material. */
    if(ir->bytes>=COD1_ST_BODY) {
        free(ir);Cod1ST_Why(&result,0,"source body exceeds limit");goto done;
    }
    isolated=(char*)malloc(ir->bytes+1);
    if(!isolated) {free(ir);Cod1ST_Why(&result,0,"allocation failed");goto done;}
    memcpy(isolated,body,ir->bytes);isolated[ir->bytes]=0;free(ir);
    projected=(char*)malloc(COD1_ST_BODY);
    if(!projected) {Cod1ST_Why(&result,0,"allocation failed");goto done;}
    if(!Cod1ST_PrepareAuth(isolated,shader.name,&caps,projected,COD1_ST_BODY,1,&result))goto done;
    cursor=projected;R_Cod1SetBundleAuth(&result,1);
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    ok=ParseShaderForSource(&cursor,worldSource);
#else
    ok=ParseShader(&cursor);
#endif
    R_Cod1ClearBundleAuth();
    if(!ok)Cod1ST_Why(&result,0,"backend parse/image resolution failed");
done:
    R_Cod1ClearBundleAuth();free(projected);free(isolated);
    if(!ok) {
        R_Cod1MaterialErrorStages();shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PARSE_FAILED;
    } else {
        shader.cod1NativeSurfaceFlags=result.nativeSurfaceFlags;
        if(result.nativeSurfaceFlags&0x4u)shader.surfaceFlags|=SURF_SKY;
        if(result.nativeSurfaceFlags&0x10u)shader.surfaceFlags|=SURF_NOIMPACT;
        if(result.nativeSurfaceFlags&0x20u)shader.surfaceFlags|=SURF_NOMARKS;
        if(result.nativeSurfaceFlags&0x20000u)shader.surfaceFlags|=SURF_NODLIGHT;
        shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_IR;shader.explicitlyDefined=qtrue;
    }
    ri.Printf(ok?PRINT_ALL:PRINT_WARNING,
        "CoD1 R61 required source: material=%s hash=%08x status=%s selected=%08x excluded=%08x stages=%d line=%d reason=%s nativeSurface=%08x ignoredSurface=%d runtimeHints=%d\n",
        shader.name,result.hash,ok?"subset-compiled":"unsupported",result.selected,result.excluded,
        result.stages,result.line,ok?"bounded source selection":result.reason,
        result.nativeSurfaceFlags,result.ignoredSurfaceParms,result.ignoredRuntimeHints);
    return qtrue;
}
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
#include "tr_cod1_water_compile.h"
#endif
/* Returns a complete private candidate or a cached explicit failure. No
 * previously published shader or source template is mutated. */
static shader_t *R_Cod1ShaderType(qboolean mipRawImage,qboolean worldSource) {
    cod1StResult_t result;cod1StUsage_t usage;
    cod1MpCaps_t caps;char *raw=NULL,*expanded=NULL,*cursor;
    int size=0,ok=0;const char *why="type IO unavailable";
    memset(&result,0,sizeof(result));memset(&caps,0,sizeof(caps));
    shader.cod1ShaderType=qtrue;shader.cod1ShaderTypeLightmapKey=shader.lightmapIndex;
    shader.cod1MaterialRoute=COD1_MATERIAL_STYPE_FAILED;
    /* LM_BY_VERTEX is a port storage convention, not a native namespace.
     * Its model use is made explicit by R_FindCod1StaticModelShader. */
    if(shader.cod1StaticModelSource || shader.lightmapIndex==LIGHTMAP_NONE) usage=COD1_ST_MODEL;
    else if(shader.lightmapIndex==LIGHTMAP_2D) usage=COD1_ST_2D;
    else if(worldSource || shader.lightmapIndex>=0 || shader.lightmapIndex==LIGHTMAP_WHITEIMAGE) usage=COD1_ST_WORLD;
    else { why="ambiguous vertex-lit type usage";goto done; }
    if(Cod1ST_Path(shader.name,usage,result.path,sizeof(result.path))!=1) {
        why="invalid type path";goto done;
    }
    Q_strncpyz(shader.cod1ShaderTypePath,result.path,sizeof(shader.cod1ShaderTypePath));
    if(!ri.FS_ReadFile || !ri.FS_FreeFile) goto done;
    size=ri.FS_ReadFile(result.path,(void**)&raw);
    if(!raw || size<=0) { why="missing/empty type source";goto done; }
    if(size>=(int)COD1_ST_BODY) { why="type source exceeds limit";goto done; }
    /* FS_ReadFile supplies one terminating NUL. Reject embedded NULs so a
     * truncated prefix can never be reported as the entire authored type. */
    if(memchr(raw,0,(size_t)size)) { why="embedded NUL in type source";goto done; }
    expanded=(char*)malloc(COD1_ST_BODY);
    if(!expanded) { why="expanded allocation failed";goto done; }
    caps.units=R_Cod1NativeTextureUnits();
    caps.extension=R_Cod1StypeExtension;
    if(!Cod1ST_Prepare(raw,shader.name,&caps,expanded,COD1_ST_BODY,&result)) {
        why=result.reason;goto done;
    }
    shader.cod1ShaderTypeHash=result.hash;
    shader.cod1NativeSurfaceFlags=result.nativeSurfaceFlags;
    /* Only renderer-consumed attributes are translated into RTCW flag space.
     * Physical surface tags remain native metadata; BSP/collision flags are
     * not overwritten by a runtime material script. */
    if(result.nativeSurfaceFlags&0x4u)shader.surfaceFlags|=SURF_SKY;
    if(result.nativeSurfaceFlags&0x10u)shader.surfaceFlags|=SURF_NOIMPACT;
    if(result.nativeSurfaceFlags&0x20u)shader.surfaceFlags|=SURF_NOMARKS;
    if(result.nativeSurfaceFlags&0x20000u)shader.surfaceFlags|=SURF_NODLIGHT;
    shader.noMipMaps=!mipRawImage;shader.noPicMip=!mipRawImage;
    cursor=expanded;
    R_Cod1SetBundleAuth(&result,0);
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    ok=ParseShaderForSource(&cursor,worldSource);
#else
    ok=ParseShader(&cursor);
#endif
    R_Cod1ClearBundleAuth();
    if(!ok) { why="backend parse/image resolution failed";goto done; }
    shader.cod1MaterialRoute=COD1_MATERIAL_STYPE_SUBSET;
    shader.explicitlyDefined=qtrue;
    why="supported fixed-function subset; native pixel parity unverified";
done:
    R_Cod1ClearBundleAuth();
    if(!ok)R_Cod1MaterialErrorStages();
    shader.cod1ShaderTypeHash=result.hash;
    ri.Printf(ok?PRINT_ALL:PRINT_WARNING,
        "CoD1 R48 shader type: material=%s source=%s hash=%08x status=%s selected=%08x excluded=%08x stages=%d line=%d reason=%s bakedVertexBridge=%d nativeSurface=%08x ignoredSurface=%d runtimeHints=%d\n",
        shader.name,result.path[0]?result.path:"<invalid>",result.hash,
        ok?"subset-compiled":"unsupported",result.selected,result.excluded,result.stages,
        result.line,why,shader.cod1StaticModelSource,result.nativeSurfaceFlags,result.ignoredSurfaceParms,result.ignoredRuntimeHints);
    if(raw)ri.FS_FreeFile(raw);
    free(expanded);
    return FinishShader();
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static shader_t *R_FindCachedShaderForSource( const char *name, int lightmapIndex,
		int hash, qboolean cod1Bsp59WorldMaterialSource );
static qhandle_t R_RegisterShaderFromImageForSource( const char *name, int lightmapIndex,
		image_t *image, qboolean mipRawImage, qboolean cod1Bsp59WorldMaterialSource );
#endif
static shader_t *R_FindShaderForSource( const char *name, int lightmapIndex,
		qboolean mipRawImage, qboolean cod1Bsp59WorldMaterialSource,
        qboolean staticModelSource ) {
	char strippedName[MAX_QPATH];
	char fileName[MAX_QPATH];
	int i, hash;
	char        *shaderText;
	image_t     *image;
	shader_t    *sh;

	if ( !name || name[0] == 0 || strlen(name)>=MAX_QPATH ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	COM_StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
#if COD1_NATIVE_LIGHTMAP_PACK
	if ( cod1PackedWorldShaders && lightmapIndex >= 0 && !strchr(strippedName,'@') && !staticModelSource ) {
		for ( sh=hashTable[hash]; sh; sh=sh->next ) {
			if ( sh->lightmapIndex != lightmapIndex ||
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
			     sh->cod1Bsp59WorldMaterialSource != cod1Bsp59WorldMaterialSource ||
#endif
			     Q_stricmp( sh->name, strippedName ) ) continue;
			return sh;
		}
	}
#endif
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
        if (sh->cod1StaticModelSource!=staticModelSource)continue;
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
        if(sh->cod1WaterProgramSource && sh->cod1WaterProgramLightmapKey!=lightmapIndex)continue;
#endif
        if (strchr(strippedName,'@')) {
            if (!sh->cod1ShaderType || sh->cod1ShaderTypeLightmapKey!=lightmapIndex)continue;
        }
		// index by name
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
		if ( sh->cod1Bsp59WorldMaterialSource != cod1Bsp59WorldMaterialSource ) {
			continue;
		}
#endif

        if(sh->cod1ShaderType) {
            if(!Q_stricmp(sh->name,strippedName))return sh;
            continue;
        }
		// Ridah, modified this so we don't keep trying to load an invalid lightmap shader
/*
		if ( sh->lightmapIndex == lightmapIndex &&
			!Q_stricmp(sh->name, strippedName)) {
			// match found
			return sh;
		}
*/
		if ( ( ( sh->lightmapIndex == lightmapIndex ) ||
			( sh->lightmapIndex < 0 && lightmapIndex >= 0
#if COD1_NATIVE_LIGHTMAP_PACK
			  && ( !cod1PackedWorldShaders ||
			       R_Cod1PackedLightmapFreeMatch( sh, lightmapIndex ) )
#endif
			) ) &&
			 !Q_stricmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// Ridah, check the cache
    /* R47 type candidates must not inherit older raw-image backup variants. */
    sh=NULL;
    if(!staticModelSource && !strchr(strippedName,'@')) {
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	sh = R_FindCachedShaderForSource( strippedName, lightmapIndex, hash,
		cod1Bsp59WorldMaterialSource );
#else
	sh = R_FindCachedShader( strippedName, lightmapIndex, hash );
#endif
	}
	if ( sh ) {
		return sh;
	}
	// done.

	// clear the global shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
    R_Cod1ClearBundleAuth();
	Q_strncpyz( shader.name, strippedName, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
    shader.cod1StaticModelSource=staticModelSource;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	shader.cod1Bsp59WorldMaterialSource = cod1Bsp59WorldMaterialSource;
#endif
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
		stages[i].bundle[1].texMods = cod1Bundle1TexMods[i];
        stages[i].bundle[2].texMods = cod1Bundle2TexMods[i];
#endif
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

    /* Native 0x4f197e-0x4f19c5: @ dispatch precedes script-name lookup. */
    if(strchr(strippedName,'@'))return R_Cod1ShaderType(mipRawImage,cod1Bsp59WorldMaterialSource);

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		char *shaderTextStart = shaderText;
		if ( cod1Bsp59MapActive ) {
			shader.cod1HasScriptSource = qtrue;
			shader.cod1ScriptOffset = (unsigned)( shaderTextStart - s_shaderText );
		}
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}
		#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
		R_Cod1ProbeMaterial( strippedName, shaderTextStart );
		#endif
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
        if(R_Cod1CompilePerlightStatic(shaderTextStart,cod1Bsp59WorldMaterialSource,qtrue)) {
            sh=FinishShader();return sh;
        }
#endif
		if ( cod1Bsp59MapActive && R_Cod1CompileWindowGlow2( shaderTextStart ) ) {
			sh = FinishShader();
			#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
			R_Cod1ProbeCompiled(strippedName,shaderTextStart,lightmapIndex,sh,
				"opaque-glass-static-perlight-deferred");
			#endif
			return sh;
		}
        if(R_Cod1CompilePerlightStatic(shaderTextStart,cod1Bsp59WorldMaterialSource,qfalse)) {
            sh=FinishShader();return sh;
        }
		#if defined(COD1_MATERIAL_STAGE_COMPILER) && COD1_MATERIAL_STAGE_COMPILER
		if ( cod1Bsp59MapActive && R_Cod1CompileOneLightmappedStage( shaderTextStart ) ) {
			sh = FinishShader();
			#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
			R_Cod1ProbeCompiled(strippedName,shaderTextStart,lightmapIndex,sh,"ir-compiled");
			#endif
			return sh;
		}
		#endif
		#if defined(COD1_WATER_FIXED2_CANDIDATE) && COD1_WATER_FIXED2_CANDIDATE
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
        if(cod1Bsp59MapActive && R_Cod1CompileWaterRuntime(shaderTextStart)) {
            sh=FinishShader();return sh;
        }
#endif
		if ( cod1Bsp59MapActive && R_Cod1CompileWaterFixed2( shaderTextStart ) ) {
			sh = FinishShader();
			ri.Printf( PRINT_ALL, "CoD1 fixed2 material: %s\n", strippedName );
			return sh;
		}
		#endif

        if(R_Cod1CompileRequiredSource(shaderTextStart,cod1Bsp59WorldMaterialSource)) {
            sh=FinishShader();return sh;
        }
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
		if ( !ParseShaderForSource( &shaderText, cod1Bsp59WorldMaterialSource ) ) {
#else
		if ( !ParseShader( &shaderText ) ) {
#endif
			/* R47: reject incomplete scripts; no first-image salvage. */
            R_Cod1MaterialErrorStages();
            shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_PARSE_FAILED;
		} else if ( cod1Bsp59MapActive ) {
			shader.cod1MaterialRoute = COD1_MATERIAL_SCRIPT_LEGACY;
		}
		sh = FinishShader();
		#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
		R_Cod1ProbeCompiled(strippedName,shaderTextStart,lightmapIndex,sh,
			shader.defaultShader?"default-after-parse-failure":"explicit");
		#endif
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	/*
	 * RTCW's raw-image fallback historically assumes an extensionless shader
	 * means .tga.  Call of Duty 1's pre-rendered UI/font atlases use the same
	 * extensionless shader names in fontImage_*.dat, but the actual pages are
	 * DDS (for example fonts/fontImage_0_16.dds).
	 *
	 * CoDA already resolves resources as: exact path -> stem.dds -> stem.tga.
	 * Mirror that compatibility rule here rather than teaching the UI/font
	 * code about DDS filenames.  This keeps CoD's shaderName strings intact.
	 */
	Q_strncpyz( fileName, name, sizeof( fileName ) );
	COM_DefaultExtension( fileName, sizeof( fileName ), ".tga" );
	image = R_FindImageFile( fileName, mipRawImage, mipRawImage, (mipRawImage?GL_REPEAT:GL_CLAMP) );
	if ( !image ) {
		image = R_CoD1FindImageCandidate( strippedName, mipRawImage,
			(mipRawImage?GL_REPEAT:GL_CLAMP) );
	}
	if ( !image ) {
		ri.Printf( PRINT_DEVELOPER, "Couldn't find image for shader %s\n", name );
		shader.defaultShader = qtrue;
		if ( cod1Bsp59MapActive ) shader.cod1MaterialRoute = COD1_MATERIAL_MISSING_IMAGE;
		return FinishShader();
	}

    if ( cod1Bsp59MapActive ) shader.cod1MaterialRoute = COD1_MATERIAL_IMPLICIT_IMAGE;


	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

shader_t *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage ) {
	return R_FindShaderForSource( name, lightmapIndex, mipRawImage, qfalse, qfalse );
}
shader_t *R_FindCod1StaticModelShader( const char *name, qboolean mipRawImage ) {
    return R_FindShaderForSource( name, LIGHTMAP_BY_VERTEX, mipRawImage, qfalse, qtrue );
}
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
shader_t *R_FindCod1Bsp59WorldMaterialShader( const char *name, int lightmapIndex,
		qboolean mipRawImage ) {
    shader_t *result;
    result=R_FindShaderForSource(name,lightmapIndex,mipRawImage,qtrue,qfalse);
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    /* Cached world materials must replay their captured name after a level
     * reset. Do not load a sunfile while parser scratch arrays are active. */
    if(result && result->cod1SunFileSeen)
        Q_strncpyz(tr.cod1SunFile,result->cod1SunFile,sizeof(tr.cod1SunFile));
#endif
    return result;
}
#endif


#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static qhandle_t R_RegisterShaderFromImageForSource( const char *name, int lightmapIndex,
		image_t *image, qboolean mipRawImage, qboolean cod1Bsp59WorldMaterialSource ) {
#else
qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage ) {
#endif
	int i, hash;
	shader_t    *sh;

	hash = generateHashValue( name );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
        if(sh->cod1StaticModelSource || sh->cod1ShaderType)continue;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
		if ( sh->cod1Bsp59WorldMaterialSource != cod1Bsp59WorldMaterialSource ) {
			continue;
		}
#endif
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
			 // index by name
			 !Q_stricmp( sh->name, name ) ) {
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
    R_Cod1ClearBundleAuth();
	Q_strncpyz( shader.name, name, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
	shader.cod1Bsp59WorldMaterialSource = cod1Bsp59WorldMaterialSource;
#endif
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
		stages[i].bundle[1].texMods = cod1Bundle1TexMods[i];
        stages[i].bundle[2].texMods = cod1Bundle2TexMods[i];
#endif
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex,
		image_t *image, qboolean mipRawImage ) {
	return R_RegisterShaderFromImageForSource( name, lightmapIndex, image, mipRawImage, qfalse );
}
#endif


/*
====================
RE_RegisterShaderLightMap

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader ); // bk: FIXME name
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void    R_ShaderList_f( void ) {
	int i;
	int count;
	shader_t    *shader;

	ri.Printf( PRINT_ALL, "-----------------------\n" );

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if ( shader->lightmapIndex >= 0 ) {
			ri.Printf( PRINT_ALL, "L " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}
		if ( shader->multitextureEnv == GL_ADD ) {
			ri.Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri.Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri.Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri.Printf( PRINT_ALL, "      " );
		}
		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture ) {
			ri.Printf( PRINT_ALL, "lmmt" );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture ) {
			ri.Printf( PRINT_ALL, "vlt " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf( PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name );
		} else {
			ri.Printf( PRINT_ALL,  ": %s\n", shader->name );
		}
		count++;
	}
	ri.Printf( PRINT_ALL, "%i total shaders\n", count );
#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
	ri.Printf(PRINT_ALL,"CoD1 material probe: %u unique source/name variants, %u repeated registrations, %u overflowed entries\n",
		cod1MpUnique,cod1MpDuplicates,cod1MpOverflow);
	ri.Printf(PRINT_ALL,"CoD1 material result: %u unique source/name/LM variants, %u duplicates, %u overflowed, %u reentrant suppressed\n",
		cod1MpCompiledUnique,cod1MpCompiledDuplicates,cod1MpCompiledOverflow,cod1MpCompiledReentrant);
#endif
	ri.Printf( PRINT_ALL, "------------------\n" );
}

// Ridah, optimized shader loading

#define MAX_SHADER_STRING_POINTERS  100000
shaderStringPointer_t shaderStringPointerList[MAX_SHADER_STRING_POINTERS];

/*
====================
BuildShaderChecksumLookup
====================
*/
static void BuildShaderChecksumLookup( void ) {
	char *p = s_shaderText, *pOld;
	char *token;
	unsigned short int checksum;
	int numShaderStringPointers = 0;

	// initialize the checksums
	memset( shaderChecksumLookup, 0, sizeof( shaderChecksumLookup ) );

	if ( !p ) {
		return;
	}

	// loop for all labels
	while ( 1 ) {

		pOld = p;

		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, "{" ) ) {
			// skip braced section
			/* SkipBracedSection expects to consume the opening brace itself.
			 * Otherwise stage texture names can enter the shader-name index. */
			p = pOld;
			SkipBracedSection( &p );
			continue;
		}

		// get it's checksum
		checksum = generateHashValue( token );

		// if it's not currently used
		if ( !shaderChecksumLookup[checksum].pStr ) {
			shaderChecksumLookup[checksum].pStr = pOld;
		} else {
			// create a new list item
			shaderStringPointer_t *newStrPtr;

			if ( numShaderStringPointers >= MAX_SHADER_STRING_POINTERS ) {
				ri.Error( ERR_DROP, "MAX_SHADER_STRING_POINTERS exceeded, too many shaders" );
			}

			newStrPtr = &shaderStringPointerList[numShaderStringPointers++]; //ri.Hunk_Alloc( sizeof( shaderStringPointer_t ), h_low );
			newStrPtr->pStr = pOld;
			newStrPtr->next = shaderChecksumLookup[checksum].next;
			shaderChecksumLookup[checksum].next = newStrPtr;
		}
	}
}
// done.


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define MAX_SHADER_FILES    4096
static void ScanAndLoadShaderFiles( void ) {
    static const char *dirs[] = { "fxshaders", "scripts" };
    char *buffers[MAX_SHADER_FILES];
    char **files, *cursor;
    int count=0, fxCount=0, num, d, i, n;
    long sum=0;
    /* CoD keeps particle blend/alpha declarations outside scripts/.
     * Reverse concatenation below leaves scripts overrides ahead of fxshaders,
     * and preserves the former ordering within the scripts directory. */
    s_shaderText=NULL;
    memset(shaderChecksumLookup,0,sizeof(shaderChecksumLookup));
#if defined(COD1_MATERIAL_PROBE_DIAGNOSTICS) && COD1_MATERIAL_PROBE_DIAGNOSTICS
    memset(cod1MpSeen,0,sizeof(cod1MpSeen));
    cod1MpUnique=cod1MpDuplicates=cod1MpOverflow=0;
    memset(cod1MpCompiledSeen,0,sizeof(cod1MpCompiledSeen));
    cod1MpCompiledUnique=cod1MpCompiledDuplicates=cod1MpCompiledOverflow=0;
    cod1MpCompiledReentrant=0;cod1MpCompiledLogging=0;
#endif
    ri.Cvar_Get("r_cod1MuzzleQuads", "0", CVAR_ROM);
    ri.Cvar_Set("r_cod1MuzzleQuads", "1");
    ri.Cvar_Get("r_cod1EfxShaders", "0", CVAR_ROM);
    ri.Cvar_Set("r_cod1EfxShaders", "0");
    memset(buffers,0,sizeof(buffers));
    for(d=0;d<2;++d) {
        files=ri.FS_ListFiles(dirs[d], ".shader", &num);
        if(!files) continue;
        for(i=0;i<num;++i) {
            char filename[MAX_QPATH]; char *data=NULL;
            if(count>=MAX_SHADER_FILES) { ri.Printf(PRINT_WARNING,"WARNING: shader file limit reached\n");break; }
            if(strlen(dirs[d])+strlen(files[i])+2>sizeof(filename)) {
                ri.Printf(PRINT_WARNING,"WARNING: shader filename too long\n");continue;
            }
            Com_sprintf(filename,sizeof(filename),"%s/%s",dirs[d],files[i]);
            n=ri.FS_ReadFile(filename,(void **)&data);
            if(n<=0||!data||n>16*1024*1024||sum+n+2>64*1024*1024) {
                if(data)ri.FS_FreeFile(data);
                ri.Printf(PRINT_WARNING,"WARNING: invalid/oversized shader file %s\n",filename);continue;
            }
            ri.Printf(PRINT_ALL,"...loading '%s'\n",filename);
            buffers[count++]=data;sum+=n+1;
            if(d==0)++fxCount;
        }
        ri.FS_FreeFileList(files);
    }
    if(!count) { ri.Printf(PRINT_WARNING,"WARNING: no shader files found\n");return; }
    s_shaderText=ri.Hunk_Alloc(sum+1,h_low);cursor=s_shaderText;
    for(i=count-1;i>=0;--i) {
        size_t len=strlen(buffers[i]);
        *cursor++='\n';memcpy(cursor,buffers[i],len);cursor+=len;
        ri.FS_FreeFile(buffers[i]);
    }
    *cursor=0;
    ri.Cvar_Set("r_cod1EfxShaders",fxCount ? "1" : "0");
    ri.Printf(PRINT_ALL,"CoD1 NATIVE_EFX_R1 renderer: %d fxshaders files, %d total shader files\n",fxCount,count);
    BuildShaderChecksumLookup();
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
    R_Cod1ClearBundleAuth();

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void ) {
//	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
	tr.spotFlareShader = R_FindShader( "spotLight", LIGHTMAP_NONE, qtrue );
//	tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, qtrue );	//----(SA)	let sky shader set this
	tr.sunflareShader[0] = R_FindShader( "sunflare1", LIGHTMAP_NONE, qtrue );
	tr.dlightShader = R_FindShader( "dlightshader", LIGHTMAP_NONE, qtrue );
}

//=============================================================================
// Ridah, shader caching
static int numBackupShaders = 0;
static shader_t *backupShaders[MAX_SHADERS];
static shader_t *backupHashTable[FILE_HASH_SIZE];

/*
===============
R_CacheShaderAlloc
===============
*/
void *R_CacheShaderAlloc( int size ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//return malloc( size );
		return malloc( size );
	} else {
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheShaderFree
===============
*/
void R_CacheShaderFree( void *ptr ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//free( ptr );
		free( ptr );
	}
}

/*
===============
R_PurgeShaders
===============
*/
void R_PurgeShaders( int count ) {
	int i, j, c, b;
	shader_t **sh;
	static int lastPurged = 0;

	if ( !numBackupShaders ) {
		lastPurged = 0;
		return;
	}

	// find the first shader still in memory
	c = 0;
	sh = (shader_t **)&backupShaders;
	for ( i = lastPurged; i < numBackupShaders; i++, sh++ ) {
		if ( *sh ) {
			// free all memory associated with this shader
			for ( j = 0 ; j < ( *sh )->numUnfoggedPasses ; j++ ) {
				if ( !( *sh )->stages[j] ) {
					break;
				}
				for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
					if ( ( *sh )->stages[j]->bundle[b].texMods ) {
						R_CacheShaderFree( ( *sh )->stages[j]->bundle[b].texMods );
					}
				}
				R_CacheShaderFree( ( *sh )->stages[j] );
			}
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
            if((*sh)->cod1PerlightRuntime) {
                cod1PerlightRuntime_t *program=(*sh)->cod1PerlightRuntime;
                for(j=0;j<program->numStages;++j)for(b=0;b<NUM_TEXTURE_BUNDLES;++b)
                    if(program->stage[j].bundle[b].texMods)
                        R_CacheShaderFree(program->stage[j].bundle[b].texMods);
                R_CacheShaderFree(program);
            }
#endif
			R_CacheShaderFree( *sh );
			*sh = NULL;

			if ( ++c >= count ) {
				lastPurged = i;
				return;
			}
		}
	}
	lastPurged = 0;
	numBackupShaders = 0;
}

/*
===============
R_BackupShaders
===============
*/
void R_BackupShaders( void ) {

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// copy each model in memory across to the backupModels
	memcpy( backupShaders, tr.shaders, sizeof( backupShaders ) );
	// now backup the hashTable
	memcpy( backupHashTable, hashTable, sizeof( hashTable ) );

	numBackupShaders = tr.numShaders;
}

/*
=================
R_RegisterShaderImages

  Make sure all images that belong to this shader remain valid
=================
*/
static qboolean R_RegisterShaderImages( shader_t *sh ) {
	int i,j,b;

	if ( sh->isSky ) {
		return qfalse;
	}

	for ( i = 0; i < sh->numUnfoggedPasses; i++ ) {
		if ( sh->stages[i] && sh->stages[i]->active ) {
			for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
				for ( j = 0; sh->stages[i]->bundle[b].image[j] && j < MAX_IMAGE_ANIMATIONS; j++ ) {
					if ( !R_TouchImage( sh->stages[i]->bundle[b].image[j] ) ) {
						return qfalse;
					}
				}
			}
		}
	}
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
    if(sh->cod1PerlightRuntime) {
        cod1PerlightRuntime_t *program=sh->cod1PerlightRuntime;
        for(i=0;i<program->numStages;++i)for(b=0;b<NUM_TEXTURE_BUNDLES;++b)
            for(j=0;j<MAX_IMAGE_ANIMATIONS && program->stage[i].bundle[b].image[j];++j)
                if(!R_TouchImage(program->stage[i].bundle[b].image[j]))return qfalse;
    }
#endif
	return qtrue;
}

/*
===============
R_FindCachedShader

  look for the given shader in the list of backupShaders
===============
*/
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
static shader_t *R_FindCachedShaderForSource( const char *name, int lightmapIndex,
		int hash, qboolean cod1Bsp59WorldMaterialSource ) {
#else
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash ) {
#endif
	shader_t *sh, *shPrev;
#if COD1_NATIVE_LIGHTMAP_PACK
	int matchPass=0;
#endif

	if ( !r_cacheShaders->integer ) {
		return NULL;
	}

	if ( !numBackupShaders ) {
		return NULL;
	}

	if ( !name ) {
		return NULL;
	}

#if COD1_NATIVE_LIGHTMAP_PACK
retryCanonical:
#endif
	sh = backupHashTable[hash];
	shPrev = NULL;
	while ( sh ) {
		if (
#if COD1_NATIVE_LIGHTMAP_PACK
			( ( matchPass == 0 && sh->lightmapIndex == lightmapIndex ) ||
			  ( matchPass == 1 && R_Cod1PackedLightmapFreeMatch( sh, lightmapIndex ) ) )
#else
			sh->lightmapIndex == lightmapIndex
#endif
			&& !sh->cod1StaticModelSource
			&& !Q_stricmp( sh->name, name )
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
			 && sh->cod1Bsp59WorldMaterialSource == cod1Bsp59WorldMaterialSource
#endif
			 ) {

#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
            if(cod1Bsp59WorldMaterialSource &&
               (sh->cod1MaterialRoute==COD1_MATERIAL_SCRIPT_PERLIGHT_PARTIAL ||
                sh->cod1MaterialRoute==COD1_MATERIAL_SCRIPT_IR ||
                sh->cod1MaterialRoute==COD1_MATERIAL_SCRIPT_PARSE_FAILED))return NULL;
#endif
#if defined(COD1_WATER_RUNTIME_R50) && COD1_WATER_RUNTIME_R50
            if(sh->cod1WaterProgramSource)return NULL;
#endif
			// make sure the images stay valid
			if ( !R_RegisterShaderImages( sh ) ) {
				return NULL;
			}

			// this is the one, so move this shader into the current list

			if ( !shPrev ) {
				backupHashTable[hash] = sh->next;
			} else {
				shPrev->next = sh->next;
			}

			sh->next = hashTable[hash];
			hashTable[hash] = sh;

			backupShaders[sh->index] = NULL;    // make sure we don't try and free it

			// set the index up, and add it to the current list
			tr.shaders[ tr.numShaders ] = sh;
			sh->index = tr.numShaders;

			tr.sortedShaders[ tr.numShaders ] = sh;
			sh->sortedIndex = tr.numShaders;

			tr.numShaders++;

			SortNewShader();    // make sure it renders in the right order
			if ( cod1Bsp59MapActive ) sh->cod1MaterialFromBackup=qtrue;

			return sh;
		}

		shPrev = sh;
		sh = sh->next;
	}

#if COD1_NATIVE_LIGHTMAP_PACK
	if ( matchPass == 0 && cod1PackedWorldShaders && lightmapIndex >= 0 ) {
		matchPass=1;
		goto retryCanonical;
	}
#endif

	return NULL;
}
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash ) {
	return R_FindCachedShaderForSource( name, lightmapIndex, hash, qfalse );
}
#endif

/*
===============
R_LoadCacheShaders
===============
*/
void R_LoadCacheShaders( void ) {
	int len;
	byte *buf;
	char    *token, *pString;
	char name[MAX_QPATH];

	if ( !r_cacheShaders->integer ) {
		return;
	}

	// don't load the cache list in between level loads, only on startup, or after a vid_restart
	if ( numBackupShaders > 0 ) {
		return;
	}

	len = ri.FS_ReadFile( "shader.cache", NULL );

	if ( len <= 0 ) {
		return;
	}

	buf = (byte *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "shader.cache", (void **)&buf );
	pString = (char*)buf;   //DAJ added (char*)

	while ( ( token = COM_ParseExt( &pString, qtrue ) ) != NULL && token[0] ) {
		Q_strncpyz( name, token, sizeof( name ) );
		RE_RegisterModel( name );
	}

	ri.Hunk_FreeTempMemory( buf );
}
// done.
//=============================================================================

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {
    ri.Printf(PRINT_ALL,"CoD1 R51 material map: shared axis qualifiers; authored sampler cache; water lightmap input\n");
    ri.Printf(PRINT_ALL,"CoD1 R53 materials: shared map noopt; eager image policy; static precalc retained\n");

	glfogNum = FOG_NONE;
	ri.Cvar_Set( "r_waterFogColor", "0" );  // clear fog
	ri.Cvar_Set( "r_mapFogColor", "0" );        //
	ri.Cvar_Set( "r_savegameFogColor", "0" );

	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	memset( hashTable, 0, sizeof( hashTable ) );
	deferLoad = qfalse;

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();

	// Ridah
	R_LoadCacheShaders();
}
