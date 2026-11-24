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

// sv_game.c -- interface to the game dll

#include "server.h"

#include "../qcommon/vm_local.h"
#include "sv_cod1_navigation_owner.inc"
#include "../game/bg_cod1_keybinding.h"
extern qboolean CL_Cod1KeyBindingQuery(cod1KeyBindingQuery_t *);
static qboolean SV_Cod1KeyBindingQuery(int low,int high) {
    uint64_t raw;uintptr_t pointer;
    if(!gvm||!gvm->dllHandle||!gvm->entryPoint||currentVM!=gvm||
       sv.state!=SS_GAME||sv.restarting||!sv_gametype||sv_gametype->integer!=GT_SINGLE_PLAYER||
       (sv_reloading&&sv_reloading->integer))return qfalse;
    raw=(uint64_t)(uint32_t)low|((uint64_t)(uint32_t)high<<32);pointer=(uintptr_t)raw;
    if((uint64_t)pointer!=raw||!pointer)return qfalse;
    return CL_Cod1KeyBindingQuery((cod1KeyBindingQuery_t *)pointer);
}


#include "../game/botlib.h"
#include "sv_cod1_ambient_module.inc"

botlib_export_t *botlib_export;

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SV_NumForGentity( sharedEntity_t *ent ) {
	int num;

	num = ( (byte *)ent - (byte *)sv.gentities ) / sv.gentitySize;

	return num;
}

sharedEntity_t *SV_GentityNum( int num ) {
	sharedEntity_t *ent;

	ent = ( sharedEntity_t * )( (byte *)sv.gentities + sv.gentitySize * ( num ) );

	return ent;
}

playerState_t *SV_GameClientNum( int num ) {
	playerState_t   *ps;

	ps = ( playerState_t * )( (byte *)sv.gameClients + sv.gameClientSize * ( num ) );

	return ps;
}

svEntity_t  *SV_SvEntityForGentity( sharedEntity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t h;
	vec3_t mins, maxs;

	if ( !name ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if ( name[0] != '*' ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}


	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;       // we don't know exactly what is in the brushes

	SV_LinkEntity( ent );       // FIXME: remove
}



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	int area1, area2;
	byte    *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );
	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		return qfalse;
	}
	if ( !CM_AreasConnected( area1, area2 ) ) {
		return qfalse;      // a door blocks sight
	}
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	int area1, area2;
	byte    *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t  *svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean    SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, const int capsule ) {
	const float *origin, *angles;
	clipHandle_t ch;
	trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs,
							ch, -1, origin, angles, capsule );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
						playerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

//==============================================

static int  FloatAsInt( float f ) {
	int temp;

	*(float *)&temp = f;

	return temp;
}

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
//rcg010207 - see my comments in VM_DllSyscall(), in qcommon/vm.c ...
#if ( ( defined __linux__ ) && ( defined __powerpc__ ) )
#define VMA( x ) ( (void *) args[x] )
#else
#define VMA( x ) VM_ArgPtr( args[x] )
#endif

#define VMF( x )  ( (float *)args )[x]

/* R113 paired private query; no shared/network layout change. */
int SV_Cod1SpatialPointTrace(const cod1SpatialTraceQuery_t *query, trace_t *output);
int SV_Cod1SpatialBoxTrace(const cod1SpatialTraceQuery_t *query, trace_t *output);

/* Original syscall149 resets entity token cursor to the current CM entity string.
 * This port exposes the selected prepass handoff only once, after EOF, during
 * GAME_INIT. No map/Actor/program execution certificate is created here. */
static int sv_cod1EntityParseInit,sv_cod1EntityParseRewound;
static int SV_Cod1EntityParseRewind(void){
 char *base;
 if(!sv_cod1EntityParseInit||sv_cod1EntityParseRewound||sv.state!=SS_LOADING||sv.entityParsePoint)return 0;
 base=CM_EntityString();if(!base||!base[0])return 0;
 sv.entityParsePoint=base;sv_cod1EntityParseRewound=1;return 1;
}

int SV_GameSystemCalls( int *args ) {
	switch ( args[0] ) {
	case G_PRINT:
		Com_Printf( "%s", VMA( 1 ) );
		return 0;
	case G_ERROR:
		Com_Error( ERR_DROP, "%s", VMA( 1 ) );
		return 0;
	case G_ENDGAME:
		Com_Error( ERR_ENDGAME, "endgame" );  // no message, no error print
		return 0;
	case G_MILLISECONDS:
		return Sys_Milliseconds();
	case G_CVAR_REGISTER:
		SV_Cod1AmbientRegisterGameCvar( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[4] );
		return 0;
	case G_CVAR_UPDATE:
		Cvar_Update( VMA( 1 ) );
		return 0;
	case G_CVAR_SET:
		SV_Cod1AmbientSetGameCvar( (const char *)VMA( 1 ), (const char *)VMA( 2 ) );
		return 0;
	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue( (const char *)VMA( 1 ) );
	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer( VMA( 1 ), VMA( 2 ), args[3] );
		return 0;
	case G_ARGC:
		return Cmd_Argc();
	case G_ARGV:
		Cmd_ArgvBuffer( args[1], VMA( 2 ), args[3] );
		return 0;
	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText( args[1], VMA( 2 ) );
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode( VMA( 1 ), VMA( 2 ), args[3] );
	case G_FS_READ:
		return FS_Read( VMA( 1 ), args[2], args[3] );
	case G_FS_WRITE:
		return FS_Write( VMA( 1 ), args[2], args[3] );
	case G_FS_RENAME:
		FS_Rename( VMA( 1 ), VMA( 2 ) );
		return 0;
	case G_FS_FCLOSE_FILE:
		FS_FCloseFile( args[1] );
		return 0;
	case G_FS_COPY_FILE:
		FS_CopyFileOS( VMA( 1 ), VMA( 2 ) );    //DAJ
		return 0;
	case G_FS_GETFILELIST:
		return FS_GetFileList( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[4] );

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData( VMA( 1 ), args[2], args[3], VMA( 4 ), args[5] );
		return 0;
	case G_DROP_CLIENT:
		SV_GameDropClient( args[1], VMA( 2 ) );
		return 0;
	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand( args[1], VMA( 2 ) );
		return 0;
	case G_COD1_TRY_SEND_SERVER_COMMAND:
		return SV_Cod1TrySendServerCommand( args[1], VMA( 2 ) );
	case G_COD1_LOCALIZED_ACQUIRE:
		return SV_Cod1LocalizedAcquire(VMA(1),args[2]);
	case G_COD1_NAVIGATION_RESOURCE:
		return CM_Cod1NavigationResource(VMA(1),VMA(2),VMA(3),(uint32_t)args[4]);
	case G_COD1_LOCALIZED_RELEASE:
		return SV_Cod1LocalizedRelease(VMA(1));
	case G_COD1_LOCALIZED_STARTUP:
		return SV_Cod1LocalizedStartup(VMA(1));
	case G_COD1_TRY_SET_LOCALIZED_CONFIGSTRING:
		return SV_Cod1TrySetLocalizedConfigstring( args[1], VMA( 2 ), VMA( 3 ), VMA( 4 ) );
	case G_COD1_TRY_SET_HUD_MARKER:
		return SV_Cod1TrySetHudMarker( VMA( 1 ), VMA( 2 ) );
	case G_COD1_CVAR_VALUE:
		{
			union { float f; int i; } result;
			result.f = Cvar_VariableValue( VMA( 1 ) );
			return result.i;
		}
	case G_COD1_KEYBINDING_QUERY:
        return SV_Cod1KeyBindingQuery(args[1],args[2]);
	case G_LINKENTITY:
		SV_LinkEntity( VMA( 1 ) );
		return 0;
	case G_UNLINKENTITY:
		SV_UnlinkEntity( VMA( 1 ) );
		return 0;
	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[4] );
	case G_ENTITY_CONTACT:
		return SV_EntityContact( VMA( 1 ), VMA( 2 ), VMA( 3 ), /* int capsule */ qfalse );
	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact( VMA( 1 ), VMA( 2 ), VMA( 3 ), /* int capsule */ qtrue );
	case G_TRACE:
		SV_Trace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[6], args[7], /* int capsule */ qfalse );
		return 0;
	case G_TRACE_USE_ENTITY:
		SV_TraceUseEntity(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;
	case G_COD1_SPATIAL_CAPSULE_TRACE:
		return SV_Cod1SpatialCapsuleTrace(VMA(1), VMA(2));
	case G_COD1_SPATIAL_BOX_TRACE:
		return SV_Cod1SpatialBoxTrace(VMA(1), VMA(2));
	case G_COD1_SPATIAL_POINT_TRACE:
		return SV_Cod1SpatialPointTrace(VMA(1), VMA(2));
	case G_COD1_SPATIAL_AREA:
		return SV_Cod1SpatialArea(VMA(1), VMA(2), args[3], VMA(4));
	case G_COD1_SPATIAL_SIGHT:
		return SV_Cod1SpatialSight(VMA(1), VMA(2));
	case G_COD1_SPATIAL_ACTOR_LINK:
		return SV_Cod1SpatialActorLink(VMA(1));
	case G_COD1_SPATIAL_ACTOR_UNLINK:
		return SV_Cod1SpatialActorUnlink(VMA(1));
	case G_COD1_PATH_SIGHT:
		return SV_Cod1PathSight(VMA(1),VMA(2));
	case G_COD1_PATH_WORLD_SIGHT:
		return SV_Cod1PathWorldSight(VMA(1),VMA(2));
	case G_COD1_SPATIAL_ACTOR_CONTENTS:
		return SV_Cod1SpatialActorContents(VMA(1));
	case G_COD1_SPATIAL_SCENE_RESET:
		return SV_Cod1SpatialSceneReset(VMA(1));
	case G_COD1_DOBJ_BUILD:
		return SV_Cod1DObjBuild(VMA(1), VMA(2));
	case G_COD1_DOBJ_FREE:
		return SV_Cod1DObjFree(VMA(1));
	case G_COD1_DOBJ_SCENE_RESET:
		return SV_Cod1DObjSceneReset(VMA(1));
	case G_COD1_DOBJ_QUERY:
		return SV_Cod1DObjQuery(VMA(1), VMA(2));
	case G_COD1_RESOURCE_MODEL_PRECACHE:
		return SV_Cod1ModelPrecache(VMA(1));
	case G_COD1_MODEL_BOUNDS:
		return SV_Cod1ModelBounds(VMA(1),VMA(2));
	case G_COD1_MODEL_PART_NAME:
		return SV_Cod1ModelPartName(VMA(1),args[2],VMA(3));
	case G_COD1_MODEL_RESOURCE_REFERENCE:
		return SV_Cod1ModelResourceReference(VMA(1), VMA(2));
	case G_COD1_ACTOR_MODEL_APPLY:
		return SV_Cod1ActorModelApply(VMA(1), VMA(2));
	case G_COD1_ACTOR_REGISTERED_MODELS_APPLY:
		return SV_Cod1ActorRegisteredModelsApply(VMA(1), VMA(2));
	case G_TRACECAPSULE:
		SV_Trace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[6], args[7], /* int capsule */ qtrue );
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents( VMA( 1 ), args[2] );
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel( VMA( 1 ), VMA( 2 ) );
		return 0;
	case G_IN_PVS:
		return SV_inPVS( VMA( 1 ), VMA( 2 ) );
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals( VMA( 1 ), VMA( 2 ) );

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring( args[1], VMA( 2 ) );
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring( args[1], VMA( 2 ), args[3] );
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo( args[1], VMA( 2 ) );
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo( args[1], VMA( 2 ), args[3] );
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo( VMA( 1 ), args[2] );
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState( VMA( 1 ), args[2] );
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected( args[1], args[2] );

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient( args[1] );
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd( args[1], VMA( 2 ) );
		return 0;
	case G_COD1_ENTITY_PARSE_REWIND:
		return SV_Cod1EntityParseRewind();
	case G_GET_ENTITY_TOKEN:
	{
		const char  *s;

		s = COM_Parse( &sv.entityParsePoint );
		Q_strncpyz( VMA( 1 ), s, args[2] );
		if ( !sv.entityParsePoint && !s[0] ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate( args[1], args[2], VMA( 3 ) );
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete( args[1] );
		return 0;
	case G_REAL_TIME:
		return Com_RealTime( VMA( 1 ) );
	case G_SNAPVECTOR:
		Sys_SnapVector( VMA( 1 ) );
		return 0;
	case G_GETTAG:
		return SV_GetTag( args[1], VMA( 2 ), VMA( 3 ) );

		//====================================

	case BOTLIB_SETUP:
		return SV_BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return botlib_export->BotLibVarSet( VMA( 1 ), VMA( 2 ) );
	case BOTLIB_LIBVAR_GET:
		return botlib_export->BotLibVarGet( VMA( 1 ), VMA( 2 ), args[3] );

	case BOTLIB_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( VMA( 1 ) );
	case BOTLIB_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( VMA( 1 ) );
	case BOTLIB_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case BOTLIB_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], VMA( 2 ) );
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], VMA( 2 ), VMA( 3 ) );

	case BOTLIB_START_FRAME:
		return botlib_export->BotLibStartFrame( VMF( 1 ) );
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap( VMA( 1 ) );
	case BOTLIB_UPDATENTITY:
		return botlib_export->BotLibUpdateEntity( args[1], VMA( 2 ) );
	case BOTLIB_TEST:
		return botlib_export->Test( args[1], VMA( 2 ), VMA( 3 ), VMA( 4 ) );

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity( args[1], args[2] );
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage( args[1], VMA( 2 ), args[3] );
	case BOTLIB_USER_COMMAND:
		SV_ClientThink( &svs.clients[args[1]], VMA( 2 ) );
		return 0;

	case BOTLIB_AAS_ENTITY_INFO:
		botlib_export->aas.AAS_EntityInfo( args[1], VMA( 2 ) );
		return 0;

	case BOTLIB_AAS_INITIALIZED:
		return botlib_export->aas.AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		botlib_export->aas.AAS_PresenceTypeBoundingBox( args[1], VMA( 2 ), VMA( 3 ) );
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt( botlib_export->aas.AAS_Time() );

		// Ridah
	case BOTLIB_AAS_SETCURRENTWORLD:
		botlib_export->aas.AAS_SetCurrentWorld( args[1] );
		return 0;
		// done.

	case BOTLIB_AAS_POINT_AREA_NUM:
		return botlib_export->aas.AAS_PointAreaNum( VMA( 1 ) );
	case BOTLIB_AAS_TRACE_AREAS:
		return botlib_export->aas.AAS_TraceAreas( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), args[5] );

	case BOTLIB_AAS_POINT_CONTENTS:
		return botlib_export->aas.AAS_PointContents( VMA( 1 ) );
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return botlib_export->aas.AAS_NextBSPEntity( args[1] );
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_ValueForBSPEpairKey( args[1], VMA( 2 ), VMA( 3 ), args[4] );
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_VectorForBSPEpairKey( args[1], VMA( 2 ), VMA( 3 ) );
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_FloatForBSPEpairKey( args[1], VMA( 2 ), VMA( 3 ) );
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_IntForBSPEpairKey( args[1], VMA( 2 ), VMA( 3 ) );

	case BOTLIB_AAS_AREA_REACHABILITY:
		return botlib_export->aas.AAS_AreaReachability( args[1] );

	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return botlib_export->aas.AAS_AreaTravelTimeToGoalArea( args[1], VMA( 2 ), args[3], args[4] );

	case BOTLIB_AAS_SWIMMING:
		return botlib_export->aas.AAS_Swimming( VMA( 1 ) );
	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return botlib_export->aas.AAS_PredictClientMovement( VMA( 1 ), args[2], VMA( 3 ), args[4], args[5],
															 VMA( 6 ), VMA( 7 ), args[8], args[9], VMF( 10 ), args[11], args[12], args[13] );

		// Ridah, route-tables
	case BOTLIB_AAS_RT_SHOWROUTE:
		botlib_export->aas.AAS_RT_ShowRoute( VMA( 1 ), args[2], args[3] );
		return 0;

	case BOTLIB_AAS_RT_GETHIDEPOS:
		return botlib_export->aas.AAS_RT_GetHidePos( VMA( 1 ), args[2], args[3], VMA( 4 ), args[5], args[6], VMA( 7 ) );

	case BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE:
		return botlib_export->aas.AAS_FindAttackSpotWithinRange( args[1], args[2], args[3], VMF( 4 ), args[5], VMA( 6 ) );

	case BOTLIB_AAS_GETROUTEFIRSTVISPOS:
		return botlib_export->aas.AAS_GetRouteFirstVisPos( VMA( 1 ), VMA( 2 ), args[3], VMA( 4 ) );

	case BOTLIB_AAS_SETAASBLOCKINGENTITY:
		botlib_export->aas.AAS_SetAASBlockingEntity( VMA( 1 ), VMA( 2 ), args[3] );
		return 0;
		// done.

	case BOTLIB_EA_SAY:
		botlib_export->ea.EA_Say( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_SAY_TEAM:
		botlib_export->ea.EA_SayTeam( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_USE_ITEM:
		botlib_export->ea.EA_UseItem( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_DROP_ITEM:
		botlib_export->ea.EA_DropItem( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_USE_INV:
		botlib_export->ea.EA_UseInv( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_DROP_INV:
		botlib_export->ea.EA_DropInv( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_EA_GESTURE:
		botlib_export->ea.EA_Gesture( args[1] );
		return 0;
	case BOTLIB_EA_COMMAND:
		botlib_export->ea.EA_Command( args[1], VMA( 2 ) );
		return 0;

	case BOTLIB_EA_SELECT_WEAPON:
		botlib_export->ea.EA_SelectWeapon( args[1], args[2] );
		return 0;
	case BOTLIB_EA_TALK:
		botlib_export->ea.EA_Talk( args[1] );
		return 0;
	case BOTLIB_EA_ATTACK:
		botlib_export->ea.EA_Attack( args[1] );
		return 0;
	case BOTLIB_EA_RELOAD:
		botlib_export->ea.EA_Reload( args[1] );
		return 0;
	case BOTLIB_EA_USE:
		botlib_export->ea.EA_Use( args[1] );
		return 0;
	case BOTLIB_EA_RESPAWN:
		botlib_export->ea.EA_Respawn( args[1] );
		return 0;
	case BOTLIB_EA_JUMP:
		botlib_export->ea.EA_Jump( args[1] );
		return 0;
	case BOTLIB_EA_DELAYED_JUMP:
		botlib_export->ea.EA_DelayedJump( args[1] );
		return 0;
	case BOTLIB_EA_CROUCH:
		botlib_export->ea.EA_Crouch( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_UP:
		botlib_export->ea.EA_MoveUp( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_DOWN:
		botlib_export->ea.EA_MoveDown( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_FORWARD:
		botlib_export->ea.EA_MoveForward( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_BACK:
		botlib_export->ea.EA_MoveBack( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_LEFT:
		botlib_export->ea.EA_MoveLeft( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_RIGHT:
		botlib_export->ea.EA_MoveRight( args[1] );
		return 0;
	case BOTLIB_EA_MOVE:
		botlib_export->ea.EA_Move( args[1], VMA( 2 ), VMF( 3 ) );
		return 0;
	case BOTLIB_EA_VIEW:
		botlib_export->ea.EA_View( args[1], VMA( 2 ) );
		return 0;

	case BOTLIB_EA_END_REGULAR:
		botlib_export->ea.EA_EndRegular( args[1], VMF( 2 ) );
		return 0;
	case BOTLIB_EA_GET_INPUT:
		botlib_export->ea.EA_GetInput( args[1], VMF( 2 ), VMA( 3 ) );
		return 0;
	case BOTLIB_EA_RESET_INPUT:
		botlib_export->ea.EA_ResetInput( args[1], VMA( 2 ) );
		return 0;

	case BOTLIB_AI_LOAD_CHARACTER:
		return botlib_export->ai.BotLoadCharacter( VMA( 1 ), args[2] );
	case BOTLIB_AI_FREE_CHARACTER:
		botlib_export->ai.BotFreeCharacter( args[1] );
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_Float( args[1], args[2] ) );
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_BFloat( args[1], args[2], VMF( 3 ), VMF( 4 ) ) );
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return botlib_export->ai.Characteristic_Integer( args[1], args[2] );
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return botlib_export->ai.Characteristic_BInteger( args[1], args[2], args[3], args[4] );
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		botlib_export->ai.Characteristic_String( args[1], args[2], VMA( 3 ), args[4] );
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return botlib_export->ai.BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		botlib_export->ai.BotFreeChatState( args[1] );
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		botlib_export->ai.BotQueueConsoleMessage( args[1], args[2], VMA( 3 ) );
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		botlib_export->ai.BotRemoveConsoleMessage( args[1], args[2] );
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNextConsoleMessage( args[1], VMA( 2 ) );
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNumConsoleMessages( args[1] );
	case BOTLIB_AI_INITIAL_CHAT:
		botlib_export->ai.BotInitialChat( args[1], VMA( 2 ), args[3], VMA( 4 ), VMA( 5 ), VMA( 6 ), VMA( 7 ), VMA( 8 ), VMA( 9 ), VMA( 10 ), VMA( 11 ) );
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return botlib_export->ai.BotNumInitialChats( args[1], VMA( 2 ) );
	case BOTLIB_AI_REPLY_CHAT:
		return botlib_export->ai.BotReplyChat( args[1], VMA( 2 ), args[3], args[4], VMA( 5 ), VMA( 6 ), VMA( 7 ), VMA( 8 ), VMA( 9 ), VMA( 10 ), VMA( 11 ), VMA( 12 ) );
	case BOTLIB_AI_CHAT_LENGTH:
		return botlib_export->ai.BotChatLength( args[1] );
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat( args[1], args[2], args[3] );
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		botlib_export->ai.BotGetChatMessage( args[1], VMA( 2 ), args[3] );
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return botlib_export->ai.StringContains( VMA( 1 ), VMA( 2 ), args[3] );
	case BOTLIB_AI_FIND_MATCH:
		return botlib_export->ai.BotFindMatch( VMA( 1 ), VMA( 2 ), args[3] );
	case BOTLIB_AI_MATCH_VARIABLE:
		botlib_export->ai.BotMatchVariable( VMA( 1 ), args[2], VMA( 3 ), args[4] );
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		botlib_export->ai.UnifyWhiteSpaces( VMA( 1 ) );
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		botlib_export->ai.BotReplaceSynonyms( VMA( 1 ), args[2] );
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return botlib_export->ai.BotLoadChatFile( args[1], VMA( 2 ), VMA( 3 ) );
	case BOTLIB_AI_SET_CHAT_GENDER:
		botlib_export->ai.BotSetChatGender( args[1], args[2] );
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		botlib_export->ai.BotSetChatName( args[1], VMA( 2 ) );
		return 0;

	case BOTLIB_AI_RESET_GOAL_STATE:
		botlib_export->ai.BotResetGoalState( args[1] );
		return 0;
	case BOTLIB_AI_RESET_AVOID_GOALS:
		botlib_export->ai.BotResetAvoidGoals( args[1] );
		return 0;
	case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		botlib_export->ai.BotRemoveFromAvoidGoals( args[1], args[2] );
		return 0;
	case BOTLIB_AI_PUSH_GOAL:
		botlib_export->ai.BotPushGoal( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_AI_POP_GOAL:
		botlib_export->ai.BotPopGoal( args[1] );
		return 0;
	case BOTLIB_AI_EMPTY_GOAL_STACK:
		botlib_export->ai.BotEmptyGoalStack( args[1] );
		return 0;
	case BOTLIB_AI_DUMP_AVOID_GOALS:
		botlib_export->ai.BotDumpAvoidGoals( args[1] );
		return 0;
	case BOTLIB_AI_DUMP_GOAL_STACK:
		botlib_export->ai.BotDumpGoalStack( args[1] );
		return 0;
	case BOTLIB_AI_GOAL_NAME:
		botlib_export->ai.BotGoalName( args[1], VMA( 2 ), args[3] );
		return 0;
	case BOTLIB_AI_GET_TOP_GOAL:
		return botlib_export->ai.BotGetTopGoal( args[1], VMA( 2 ) );
	case BOTLIB_AI_GET_SECOND_GOAL:
		return botlib_export->ai.BotGetSecondGoal( args[1], VMA( 2 ) );
	case BOTLIB_AI_CHOOSE_LTG_ITEM:
		return botlib_export->ai.BotChooseLTGItem( args[1], VMA( 2 ), VMA( 3 ), args[4] );
	case BOTLIB_AI_CHOOSE_NBG_ITEM:
		return botlib_export->ai.BotChooseNBGItem( args[1], VMA( 2 ), VMA( 3 ), args[4], VMA( 5 ), VMF( 6 ) );
	case BOTLIB_AI_TOUCHING_GOAL:
		return botlib_export->ai.BotTouchingGoal( VMA( 1 ), VMA( 2 ) );
	case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return botlib_export->ai.BotItemGoalInVisButNotVisible( args[1], VMA( 2 ), VMA( 3 ), VMA( 4 ) );
	case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return botlib_export->ai.BotGetLevelItemGoal( args[1], VMA( 2 ), VMA( 3 ) );
	case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return botlib_export->ai.BotGetNextCampSpotGoal( args[1], VMA( 2 ) );
	case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return botlib_export->ai.BotGetMapLocationGoal( VMA( 1 ), VMA( 2 ) );
	case BOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt( botlib_export->ai.BotAvoidGoalTime( args[1], args[2] ) );
	case BOTLIB_AI_INIT_LEVEL_ITEMS:
		botlib_export->ai.BotInitLevelItems();
		return 0;
	case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
		botlib_export->ai.BotUpdateEntityItems();
		return 0;
	case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return botlib_export->ai.BotLoadItemWeights( args[1], VMA( 2 ) );
	case BOTLIB_AI_FREE_ITEM_WEIGHTS:
		botlib_export->ai.BotFreeItemWeights( args[1] );
		return 0;
	case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotInterbreedGoalFuzzyLogic( args[1], args[2], args[3] );
		return 0;
	case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotSaveGoalFuzzyLogic( args[1], VMA( 2 ) );
		return 0;
	case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotMutateGoalFuzzyLogic( args[1], VMF( 2 ) );
		return 0;
	case BOTLIB_AI_ALLOC_GOAL_STATE:
		return botlib_export->ai.BotAllocGoalState( args[1] );
	case BOTLIB_AI_FREE_GOAL_STATE:
		botlib_export->ai.BotFreeGoalState( args[1] );
		return 0;

	case BOTLIB_AI_RESET_MOVE_STATE:
		botlib_export->ai.BotResetMoveState( args[1] );
		return 0;
	case BOTLIB_AI_MOVE_TO_GOAL:
		botlib_export->ai.BotMoveToGoal( VMA( 1 ), args[2], VMA( 3 ), args[4] );
		return 0;
	case BOTLIB_AI_MOVE_IN_DIRECTION:
		return botlib_export->ai.BotMoveInDirection( args[1], VMA( 2 ), VMF( 3 ), args[4] );
	case BOTLIB_AI_RESET_AVOID_REACH:
		botlib_export->ai.BotResetAvoidReach( args[1] );
		return 0;
	case BOTLIB_AI_RESET_LAST_AVOID_REACH:
		botlib_export->ai.BotResetLastAvoidReach( args[1] );
		return 0;
	case BOTLIB_AI_REACHABILITY_AREA:
		return botlib_export->ai.BotReachabilityArea( VMA( 1 ), args[2] );
	case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return botlib_export->ai.BotMovementViewTarget( args[1], VMA( 2 ), args[3], VMF( 4 ), VMA( 5 ) );
	case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return botlib_export->ai.BotPredictVisiblePosition( VMA( 1 ), args[2], VMA( 3 ), args[4], VMA( 5 ) );
	case BOTLIB_AI_ALLOC_MOVE_STATE:
		return botlib_export->ai.BotAllocMoveState();
	case BOTLIB_AI_FREE_MOVE_STATE:
		botlib_export->ai.BotFreeMoveState( args[1] );
		return 0;
	case BOTLIB_AI_INIT_MOVE_STATE:
		botlib_export->ai.BotInitMoveState( args[1], VMA( 2 ) );
		return 0;
		// Ridah
	case BOTLIB_AI_INIT_AVOID_REACH:
		botlib_export->ai.BotInitAvoidReach( args[1] );
		return 0;
		// done.

	case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return botlib_export->ai.BotChooseBestFightWeapon( args[1], VMA( 2 ) );
	case BOTLIB_AI_GET_WEAPON_INFO:
		botlib_export->ai.BotGetWeaponInfo( args[1], args[2], VMA( 3 ) );
		return 0;
	case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return botlib_export->ai.BotLoadWeaponWeights( args[1], VMA( 2 ) );
	case BOTLIB_AI_ALLOC_WEAPON_STATE:
		return botlib_export->ai.BotAllocWeaponState();
	case BOTLIB_AI_FREE_WEAPON_STATE:
		botlib_export->ai.BotFreeWeaponState( args[1] );
		return 0;
	case BOTLIB_AI_RESET_WEAPON_STATE:
		botlib_export->ai.BotResetWeaponState( args[1] );
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return botlib_export->ai.GeneticParentsAndChildSelection( args[1], VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ) );

	case TRAP_MEMSET:
		memset( VMA( 1 ), args[2], args[3] );
		return 0;

	case TRAP_MEMCPY:
		memcpy( VMA( 1 ), VMA( 2 ), args[3] );
		return 0;

	case TRAP_STRNCPY:
		return (int)strncpy( VMA( 1 ), VMA( 2 ), args[3] );

	case TRAP_SIN:
		return FloatAsInt( sin( VMF( 1 ) ) );

	case TRAP_COS:
		return FloatAsInt( cos( VMF( 1 ) ) );

	case TRAP_ATAN2:
		return FloatAsInt( atan2( VMF( 1 ), VMF( 2 ) ) );

	case TRAP_SQRT:
		return FloatAsInt( sqrt( VMF( 1 ) ) );

	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply( VMA( 1 ), VMA( 2 ), VMA( 3 ) );
		return 0;

	case TRAP_ANGLEVECTORS:
		AngleVectors( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ) );
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector( VMA( 1 ), VMA( 2 ) );
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt( floor( VMF( 1 ) ) );

	case TRAP_CEIL:
		return FloatAsInt( ceil( VMF( 1 ) ) );


	default:
		Com_Error( ERR_DROP, "Bad game system trap: %i", args[0] );
	}
	return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
/* Client records survive a map change, but their entity pointers belong to
 * the current game VM. A bootstrap server command can inspect svFlags before
 * clients reconnect, so discard those references before unloading the VM.
 * SV_SpawnServer can apply a new maxclients cvar before this VM shuts down,
 * while reallocating the client array only afterwards. Keep this VM's count,
 * captured when its allocation was established, for every clear. */
static int sv_gameEntityClientCount;

static void SV_ClearGameEntityReferences( void ) {
	int i;
	for ( i = 0 ; i < sv_gameEntityClientCount ; i++ ) {
		svs.clients[i].gentity = NULL;
	}
}

void SV_ShutdownGameProgs( void ) {
	if ( !com_errorEntered ) {
		SV_FinishFrameSlice();
	}
	SV_ResetFrameSlices();
	if ( !gvm ) {
		SV_Cod1PrecacheClearScene();
		return;
	}
	SV_Cod1LocalizedEndGameModule();
	VM_Call( gvm, GAME_SHUTDOWN, qfalse );
	SV_Cod1PrecacheClearScene();
	SV_ClearGameEntityReferences();
	VM_Free( gvm );
	gvm = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM( qboolean restart ) {
	/* GAME_INIT may now emit authored fog through SV_SendServerCommand.
	 * Clearing only afterwards leaves the previous map's entities reachable
	 * while that command filters clients, including native DLL reloads. */
	sv_gameEntityClientCount = sv_maxclients->integer;
	SV_ClearGameEntityReferences();
	if (VM_Call(gvm, GAME_COD1_STATE_ABI, sizeof(playerState_t), sizeof(usercmd_t)) != COD1_STATE_ABI) {
		Com_Error(ERR_DROP, "PATCH42: rebuild and copy qagamex86.dll together with WolfSP.exe");
	}


	/* ABI checked; register before any GAME_INIT resource/registry intake. */
	SV_Cod1NavigationBeginGameModule();

	/* A fast restart retains the server world sectors while the native GAME
	 * module replaces its entity array. Early BSP constructors can trace before
	 * every old slot is allocated/relinked, so retire the previous spatial
	 * scene before GAME_INIT. Preserve network baselines and snapshot metadata.
	 * No pointer into the retired GAME array is dereferenced here. */
	if ( restart && CM_HasV59() ) {
		int slot;
		SV_ClearWorld();
		for ( slot = 0; slot < MAX_GENTITIES; ++slot ) {
			sv.svEntities[slot].worldSector = NULL;
			sv.svEntities[slot].nextEntityInWorldSector = NULL;
		}
	}

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// use the current msec count for a random seed
	// init for this gamestate
	SV_Cod1AmbientBeginGameModule();
	SV_Cod1LocalizedBeginGameModule();
	sv_cod1EntityParseInit=1;sv_cod1EntityParseRewound=0;
	VM_Call( gvm, GAME_INIT, svs.time, Com_Milliseconds(), restart );
	sv_cod1EntityParseInit=0;
	SV_SetFrameSliceSupport( gvm->dllHandle && gvm->entryPoint &&
		VM_Call( gvm, GAME_FRAME_SLICE_API ) == COD1_FRAME_SLICE_API );

	// Keep the existing post-init reset. Reconnecting clients receive their
	// current game entities later, after this initialization has completed.
	SV_ClearGameEntityReferences();
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs( void ) {
	SV_FinishFrameSlice();
	if ( !gvm ) {
		return;
	}
	SV_Cod1LocalizedEndGameModule();
	SV_ResetFrameSlices();
	VM_Call( gvm, GAME_SHUTDOWN, qtrue );
	SV_ClearGameEntityReferences();

	// do a restart instead of a free
	gvm = VM_Restart( gvm );
	if ( !gvm ) { // bk001212 - as done below
		Com_Error( ERR_FATAL, "VM_Restart on game failed" );
	}

	SV_InitGameVM( qtrue );
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs( void ) {
	cvar_t  *var;
	//FIXME these are temp while I make bots run in vm
	extern int bot_enable;

	var = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	if ( var ) {
		bot_enable = var->integer;
	} else {
		bot_enable = 0;
	}

	// load the dll or bytecode
	gvm = VM_Create( "qagame", SV_GameSystemCalls, Cvar_VariableValue( "vm_game" ) );
	if ( !gvm ) {
		Com_Error( ERR_FATAL, "VM_Create on game failed" );
	}

	SV_InitGameVM( qfalse );
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}

	return VM_Call( gvm, GAME_CONSOLE_COMMAND );
}


/*
====================
SV_SendMoveSpeedsToGame
====================
*/
void SV_SendMoveSpeedsToGame( int entnum, char *text ) {
	if ( !gvm ) {
		return;
	}
	VM_Call( gvm, GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT, entnum, text );
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag( int clientNum, char *tagname, orientation_t *or );

qboolean SV_GetTag( int clientNum, char *tagname, orientation_t *or ) {
	if ( com_dedicated->integer ) {
		return qfalse;
	}

	return CL_GetTag( clientNum, tagname, or );
}

/*
===================
SV_GetModelInfo

  request this modelinfo from the game
===================
*/
qboolean SV_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo ) {
	return VM_Call( gvm, GAME_GETMODELINFO, clientNum, modelName, modelInfo );
}
