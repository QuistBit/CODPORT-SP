/* R145: an explicit static ammo packet consumer. Native item indices never
 * enter RTCW's ET_ITEM/bg_itemlist path. This selected packet lane renders
 * the declared XModel at its copied pose; pickup/physics are still separate. */
#include "../game/bg_cod1_ammo_entity.h"

/* A marked ET_GENERAL belongs to this lane even when validation fails. Do
 * not fall back to the generic model path for stale or malformed packets. */
static qboolean CG_Cod1AmmoEntity( centity_t *cent ) {
	cod1AmmoEntityView_t view;
	cod1ItemDescriptor_t descriptor;
	refEntity_t ent;
	const char *modelName;
	qhandle_t handle;
	if ( cent->currentState.eType != ET_GENERAL ||
	     cent->currentState.density != COD1_AMMO_NET_MARKER ) {
		return qfalse;
	}
	if ( cgs.gametype != GT_SINGLE_PLAYER || !cgs.cod1MapEntities ||
	     !CAN_ReadPacket( &cent->currentState, &view ) ||
	     !CI_Descriptor( view.itemIndex, &descriptor ) ) {
		return qtrue;
	}
	modelName = CG_ConfigString( CS_MODELS + view.networkModelSlot );
	handle = cgs.gameModels[view.networkModelSlot];
	if ( !modelName || Q_stricmp( modelName, descriptor.models[0] ) || handle <= 0 ) {
		return qtrue;
	}
	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_MODEL;
	ent.hModel = handle;
	VectorCopy( view.origin, ent.origin );
	VectorCopy( view.origin, ent.oldorigin );
	AnglesToAxis( view.angles, ent.axis );
	trap_R_AddRefEntityToScene( &ent );
	cent->refEnt = ent;
	return qtrue;
}
