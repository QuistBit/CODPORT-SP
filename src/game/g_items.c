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

/*
 * name:		g_items.c
 *
 * desc:		Items are any object that a player can touch to gain some effect.
 *				Pickup will return the number of seconds until they should respawn.
 *				all items should pop when dropped in lava or slime.
 *				Respawnable items don't actually go away when picked up, they are
 *				just made invisible and untouchable.  This allows them to ride
 *				movers and respawn apropriately.
 *
*/

#include "g_local.h"
#include "g_cod1_dropped_item.h"
#include "bg_cod1_inventory.h"
#include "g_cod1_health_fence.inc"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_gsc_test.h"

static int G_Cod1PickupFiniteBounds(const gentity_t *item) {
	int i;
	for (i = 0; i < 3; ++i) {
		if (!(item->r.absmin[i] >= -3.402823466e38f && item->r.absmin[i] <= 3.402823466e38f) ||
			!(item->r.absmax[i] >= -3.402823466e38f && item->r.absmax[i] <= 3.402823466e38f) ||
			item->r.absmin[i] > item->r.absmax[i]) return 0;
	}
	return 1;
}

typedef struct {
	vec3_t mins, maxs;
	int linked;
} Cod1PickupBounds;

static void G_Cod1PickupCaptureBounds(const gentity_t *item, Cod1PickupBounds *bounds) {
	VectorCopy(item->r.absmin, bounds->mins);
	VectorCopy(item->r.absmax, bounds->maxs);
	bounds->linked = item->r.linked;
}

static int G_Cod1PickupSameBounds(const gentity_t *item, const Cod1PickupBounds *bounds) {
	int i;
	if (item->r.linked != bounds->linked) return 0;
	for (i = 0; i < 3; ++i)
		if (item->r.absmin[i] != bounds->mins[i] || item->r.absmax[i] != bounds->maxs[i]) return 0;
	return 1;
}
#endif



#define RESPAWN_SP          -1
#define RESPAWN_KEY         4
#define RESPAWN_ARMOR       25
#define RESPAWN_TEAM_WEAPON 30
#define RESPAWN_HEALTH      35
#define RESPAWN_AMMO        40
#define RESPAWN_HOLDABLE    60
#define RESPAWN_MEGAHEALTH  120
#define RESPAWN_POWERUP     120
#define RESPAWN_PARTIAL     998     // for multi-stage ammo/health
#define RESPAWN_PARTIAL_DONE 999    // for multi-stage ammo/health

qboolean G_Cod1NativeHealthItem( const gitem_t *item ) {
	return G_Cod1MapEntities() && item && item->giType == IT_HEALTH && item->classname &&
		( !Q_stricmp( item->classname, "item_health_small" ) ||
		  !Q_stricmp( item->classname, "item_health" ) ||
		  !Q_stricmp( item->classname, "item_health_large" ) );
}

/* Native gamex86 0x2001d270..0x2001d6d6 examines nearby item bbox centers:
 * eye-to-center <=72, forward cosine >=.76, then occlusion.  Use the same
 * candidate for the server hint and +activate, including empty hands.
 * Health also uses this path when a passive map trigger masks the item. */
static gentity_t *G_Cod1AimedPickupItem( gentity_t *player, int itemType,
	float *candidateScore ) {
	vec3_t eye, forward, right, up, mins, maxs;
	vec3_t reach = { 72.0f, 72.0f, 72.0f };
	int indices[MAX_GENTITIES], count, i;
	float bestScore = 1.0e30f;
	gentity_t *best = NULL;
	playerState_t *ps;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	Cod1PickupBounds bestBounds;
	int checkedGeometry = 0;
#endif

	if ( candidateScore ) {
		*candidateScore = bestScore;
	}
	if ( !player || !player->client || g_gametype.integer != GT_SINGLE_PLAYER ||
		 player->health <= 0 || player->client->sess.sessionTeam == TEAM_SPECTATOR ||
		 ( player->client->ps.pm_flags & PMF_LIMBO ) || player->client->ps.aiChar ) {
		return NULL;
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if (!G_Cod1UseSelectionReady(player)) return NULL;
#endif
	ps = &player->client->ps;
	AngleVectors( ps->viewangles, forward, right, up );
	CalcMuzzlePointForActivate( player, forward, right, up, eye );
	VectorSubtract( eye, reach, mins );
	VectorAdd( eye, reach, maxs );
	count = trap_EntitiesInBox( mins, maxs, indices, MAX_GENTITIES );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if (!G_Cod1UseSelectionReady(player)) return NULL;
	checkedGeometry = G_Cod1GscTestUseSelectionActive();
	if (checkedGeometry) {
		if (count < 0 || count > MAX_GENTITIES) { G_Cod1UseSelectionReject(); return NULL; }
		for (i = 0; i < count; ++i)
			if (indices[i] < 0 || indices[i] >= level.num_entities || indices[i] >= MAX_GENTITIES) { G_Cod1UseSelectionReject(); return NULL; }
	}
#endif
	for ( i = 0; i < count; i++ ) {
		gentity_t *item = &g_entities[indices[i]];
		vec3_t center, direction;
		trace_t tr;
		float distance, cosine, score;
		int weapon = WP_NONE;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
		Cod1PickupBounds queryBounds;
#endif

		if ( !item->inuse || item->s.eType != ET_ITEM || !item->item || !item->touch ||
			 ( item->item->giType != IT_WEAPON &&
			   !G_Cod1NativeHealthItem( item->item ) ) ||
			 ( itemType && item->item->giType != itemType ) ||
			 !( item->r.contents & CONTENTS_TRIGGER ) ||
			 ( item->flags & FL_NODRAW ) ||
			 item->s.modelindex <= 0 || item->s.modelindex >= bg_numItems ) {
			continue;
		}
		if ( item->item->giType == IT_WEAPON ) {
			weapon = item->item->giTag;
			if ( !CW_WeaponDef( weapon ) ||
				 ( item->cod1DroppedAmmo && item->cod1DropOwner == player->s.number &&
				   level.time < item->cod1DropOwnerUntil ) ||
				 ( !COM_BitCheck( ps->weapons, weapon ) &&
				   !CW_PickupSlot( ps, weapon, qtrue ) ) ) {
				continue;
			}
		}
		/* The hint and USE candidate must agree with Touch_Item's shared
		 * eligibility gate.  An aid kit at full health is not a USE target. */
		if ( !BG_CanItemBeGrabbed( &item->s, ps ) ) {
			continue;
		}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
		if (G_Cod1GscTestUseSelectionActive() && !G_Cod1PickupFiniteBounds(item)) { G_Cod1UseSelectionReject(); return NULL; }
#endif
		VectorAdd( item->r.absmin, item->r.absmax, center );
		VectorScale( center, 0.5f, center );
		VectorSubtract( center, eye, direction );
		distance = VectorNormalize( direction );
		if ( distance > 72.0f || distance <= 0.0f ) {
			continue;
		}
		cosine = DotProduct( direction, forward );
		if ( cosine < 0.76f ) {
			continue;
		}
		/* Original 0x2001d51e..0x2001d5a7 weights distance and angle.
		 * The port currently adapts CoD weapons and the three aid kits. */
		score = distance + 144.0f * ( 1.0f - ( cosine - 0.76f ) / 0.24f );
		if ( score >= bestScore ) {
			continue;
		}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
		if (checkedGeometry) G_Cod1PickupCaptureBounds(item, &queryBounds);
#endif
		/* Training's two M1A1 carbines sit inside playerclip volumes. These
		 * block player movement but not sight of the weapons on the table.
		 * Keep solid world and actors in this visibility trace. */
		trap_Trace( &tr, eye, NULL, NULL, center, player->s.number,
			CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
		if (!G_Cod1UseSelectionReady(player)) return NULL;
		/* A same-allocation move during this or a later candidate's LOS
		 * invalidates the point, visibility and score just calculated. */
		if (checkedGeometry &&
			(!G_Cod1PickupSameBounds(item, &queryBounds) ||
			 (best && !G_Cod1PickupSameBounds(best, &bestBounds)))) {
			G_Cod1UseSelectionReject(); return NULL;
		}
		if (G_Cod1GscTestUseSelectionActive() &&
			(!(tr.fraction >= 0.0f && tr.fraction <= 1.0f) ||
			 tr.entityNum < 0 || tr.entityNum >= MAX_GENTITIES ||
			 (tr.entityNum != ENTITYNUM_WORLD && tr.entityNum != ENTITYNUM_NONE &&
			  (tr.entityNum >= level.num_entities || !g_entities[tr.entityNum].inuse ||
			   g_entities[tr.entityNum].s.number != tr.entityNum)) ||
			 !(tr.plane.normal[0] >= -3.402823466e38f && tr.plane.normal[0] <= 3.402823466e38f) ||
			 !(tr.plane.normal[1] >= -3.402823466e38f && tr.plane.normal[1] <= 3.402823466e38f) ||
			 !(tr.plane.normal[2] >= -3.402823466e38f && tr.plane.normal[2] <= 3.402823466e38f) ||
			 !(tr.endpos[0] >= -3.402823466e38f && tr.endpos[0] <= 3.402823466e38f) ||
			 !(tr.endpos[1] >= -3.402823466e38f && tr.endpos[1] <= 3.402823466e38f) ||
			 !(tr.endpos[2] >= -3.402823466e38f && tr.endpos[2] <= 3.402823466e38f))) { G_Cod1UseSelectionReject(); return NULL; }
#endif
		if ( tr.startsolid || ( tr.fraction < 1.0f && tr.entityNum != item->s.number ) ) {
			continue;
		}
		bestScore = score;
		best = item;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
		if (checkedGeometry) bestBounds = queryBounds;
#endif
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if (checkedGeometry && best && !G_Cod1PickupSameBounds(best, &bestBounds)) {
		G_Cod1UseSelectionReject(); return NULL;
	}
#endif
	if ( candidateScore ) {
		*candidateScore = bestScore;
	}
	return best;
}

gentity_t *G_Cod1AimedPickupCandidate( gentity_t *player, float *score ) {
	return G_Cod1AimedPickupItem( player, 0, score );
}

gentity_t *G_Cod1AimedWeaponItem( gentity_t *player ) {
	return G_Cod1AimedPickupItem( player, IT_WEAPON, NULL );
}

/* The training carbine and aid-kit tables are enclosed by trigger_multiple
 * brushes.  They fire when touched and are not direct USE targets, although
 * the point trace (which includes CONTENTS_TRIGGER) can hit them first.
 * Only bypass these passive volumes; doors, actors and other items keep their
 * normal direct interaction priority. The candidate's own visibility trace
 * still rejects solid occlusion. */
qboolean G_Cod1AimedWeaponLookTraceAllows( const trace_t *lookTrace ) {
	gentity_t *hit;
	if ( !lookTrace ) {
		return qfalse;
	}
	if ( lookTrace->fraction >= 1.0f ||
		 lookTrace->entityNum == ENTITYNUM_WORLD ||
		 lookTrace->entityNum == ENTITYNUM_NONE ) {
		return qtrue;
	}
	if ( lookTrace->entityNum < 0 || lookTrace->entityNum >= MAX_GENTITIES ) {
		return qfalse;
	}
	hit = &g_entities[lookTrace->entityNum];
	if ( hit->s.eType == ET_ITEM && hit->item &&
		 ( G_Cod1NativeHealthItem( hit->item ) ||
		   ( hit->item->giType == IT_WEAPON && CW_WeaponDef( hit->item->giTag ) ) ) ) {
		return qtrue;
	}
	if ( hit->classname && !Q_stricmp( hit->classname, "trigger_multiple" ) &&
		 ( hit->r.contents & CONTENTS_TRIGGER ) ) {
		return qtrue;
	}
	return qfalse;
}

/* The unified native selector already chose this item by distance, angle and
 * visibility.  Applying it must not query again or let the old point trace
 * veto a valid item behind a passive trigger. */
qboolean G_Cod1ApplyAimedPickupCandidate( gentity_t *player, gentity_t *item,
	qboolean hint ) {
	trace_t itemTrace;
	int pickupPreference;
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	GscEntityRef pickupPlayer, pickupItem;
	int checkedPlayer = 0;
#endif
	if ( !player || !player->client || g_gametype.integer != GT_SINGLE_PLAYER ||
		 player->health <= 0 || player->client->sess.sessionTeam == TEAM_SPECTATOR ||
		 ( player->client->ps.pm_flags & PMF_LIMBO ) || player->client->ps.aiChar ) {
		return qfalse;
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if (!G_Cod1UseSelectionReady(player)) return qfalse;
#endif
	if ( !item || !item->inuse || item->s.eType != ET_ITEM || !item->item ||
		 !item->touch || !( item->r.contents & CONTENTS_TRIGGER ) ||
		 ( item->flags & FL_NODRAW ) || item->s.modelindex <= 0 ||
		 item->s.modelindex >= bg_numItems ||
		 ( item->item->giType != IT_WEAPON && !G_Cod1NativeHealthItem( item->item ) ) ||
		 !BG_CanItemBeGrabbed( &item->s, &player->client->ps ) ) {
		return qfalse;
	}
	if ( item->item->giType == IT_WEAPON ) {
		int weapon = item->item->giTag;
		if ( !CW_WeaponDef( weapon ) ||
			 ( item->cod1DroppedAmmo && item->cod1DropOwner == player->s.number &&
			   level.time < item->cod1DropOwnerUntil ) ||
			 ( !COM_BitCheck( player->client->ps.weapons, weapon ) &&
			   !CW_PickupSlot( &player->client->ps, weapon, qtrue ) ) ) {
			return qfalse;
		}
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if (G_Cod1GscTestUseSelectionActive()) {
		if (!G_Cod1EntityCapture(player, &pickupPlayer) ||
			!G_Cod1EntityCapture(item, &pickupItem) ||
			!G_Cod1UseSelectionStable(player) ||
			!G_Cod1EntitySameAllocation(&pickupPlayer) ||
			!G_Cod1EntitySameAllocation(&pickupItem)) return qfalse;
		checkedPlayer = 1;
	}
#endif
	if ( hint ) {
		if ( item->s.modelindex >= 256 ) {
			return qfalse;
		}
		if ( item->item->giType == IT_HEALTH ) {
			player->client->ps.serverCursorHint = HINT_HEALTH;
		} else {
			player->client->ps.serverCursorHint =
				COM_BitCheck( player->client->ps.weapons, item->item->giTag ) ?
				HINT_AMMO : HINT_WEAPON;
		}
		player->client->ps.serverCursorHintVal = item->s.modelindex;
		return qtrue;
	}
	memset( &itemTrace, 0, sizeof( itemTrace ) );
	pickupPreference = player->client->pers.autoActivate;
	player->client->pers.autoActivate = PICKUP_FORCE;
	item->active = qtrue;
	item->touch( item, player, &itemTrace );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	/* An accepted pickup may retire the item; restore only the initiating player. */
	if (checkedPlayer && (!G_Cod1EntitySameAllocation(&pickupPlayer) || !player->client)) return qtrue;
#endif
	player->client->pers.autoActivate = pickupPreference;
	return qtrue;
}

qboolean G_Cod1ApplyAimedWeaponHint( gentity_t *player ) {
	return G_Cod1ApplyAimedPickupCandidate( player,
		G_Cod1AimedWeaponItem( player ), qtrue );
}

qboolean G_Cod1ApplyAimedPickupHint( gentity_t *player ) {
	return G_Cod1ApplyAimedPickupCandidate( player,
		G_Cod1AimedPickupItem( player, 0, NULL ), qtrue );
}

/* The legacy direct door, actor and item priorities remain in these wrappers.
 * The unified native selector applies its chosen candidate directly. */
static qboolean G_Cod1TryAimedPickupUseType( gentity_t *player,
	const trace_t *lookTrace, int itemType ) {
	if ( !player || !player->client || !lookTrace ||
		 g_gametype.integer != GT_SINGLE_PLAYER || player->client->ps.aiChar ||
		 !G_Cod1AimedWeaponLookTraceAllows( lookTrace ) ) {
		return qfalse;
	}
	return G_Cod1ApplyAimedPickupCandidate( player,
		G_Cod1AimedPickupItem( player, itemType, NULL ), qfalse );
}

qboolean G_Cod1TryAimedWeaponUse( gentity_t *player, const trace_t *lookTrace ) {
	return G_Cod1TryAimedPickupUseType( player, lookTrace, IT_WEAPON );
}

qboolean G_Cod1TryAimedPickupUse( gentity_t *player, const trace_t *lookTrace ) {
	return G_Cod1TryAimedPickupUseType( player, lookTrace, 0 );
}


//======================================================================

int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int quantity;
	int i;
	gclient_t   *client;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {

		// some powerups are time based on how long the powerup is /used/
		// rather than timed from when the player picks it up.
		if ( ent->item->giTag == PW_NOFATIGUE ) {
		} else {
			// round timing to seconds to make multiple powerup timers
			// count in sync
			other->client->ps.powerups[ent->item->giTag] = level.time - ( level.time % 1000 );
		}
	}

	// if an amount was specified in the ent, use it
	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;


	// brandy also gives a little health (10)
	if ( ent->item->giTag == PW_NOFATIGUE ) {
		if ( Q_stricmp( ent->item->classname, "item_stamina_brandy" ) == 0 ) {
			other->health += 10;
			if ( other->health > other->client->ps.stats[STAT_MAX_HEALTH] ) {
				other->health = other->client->ps.stats[STAT_MAX_HEALTH];
			}
			other->client->ps.stats[STAT_HEALTH] = other->health;
		}

		// cap stamina
		if ( other->client->ps.powerups[PW_NOFATIGUE] > 60000 ) {
			other->client->ps.powerups[PW_NOFATIGUE] = 60000;
		}
	}


	// Ridah, not in single player
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		// done.
		// give any nearby players a "denied" anti-reward
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			vec3_t delta;
			float len;
			vec3_t forward;
			trace_t tr;

			client = &level.clients[i];
			if ( client == other->client ) {
				continue;
			}
			if ( client->pers.connected == CON_DISCONNECTED ) {
				continue;
			}
			if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
				continue;
			}

			// if too far away, no sound
			VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
			len = VectorNormalize( delta );
			if ( len > 192 ) {
				continue;
			}

			// if not facing, no sound
			AngleVectors( client->ps.viewangles, forward, NULL, NULL );
			if ( DotProduct( delta, forward ) < 0.4 ) {
				continue;
			}

			// if not line of sight, no sound
			trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
			if ( tr.fraction != 1.0 ) {
				continue;
			}

			// anti-reward
			client->ps.persistant[PERS_REWARD_COUNT]++;
			client->ps.persistant[PERS_REWARD] = REWARD_DENIED;
		}
		// Ridah
	}
	// done.

	if ( ent->s.density == 2 ) {   // multi-stage health first stage
		return RESPAWN_PARTIAL;
	} else if ( ent->s.density == 1 ) {    // last stage, leave the plate
		return RESPAWN_PARTIAL_DONE;
	}

	// single player has no respawns	(SA)
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}
	}

	return RESPAWN_POWERUP;
}

//----(SA) Wolf keys
//======================================================================
int Pickup_Key( gentity_t *ent, gentity_t *other ) {
	other->client->ps.stats[STAT_KEYS] |= ( 1 << ent->item->giTag );
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}
	}

	return RESPAWN_KEY;
}



/*
==============
Pickup_Clipboard
==============
*/
int Pickup_Clipboard( gentity_t *ent, gentity_t *other ) {

	if ( ent->spawnflags & 4 ) {
		return 0;   // leave in world

	}
	return -1;
}


/*
==============
Pickup_Treasure
==============
*/
int Pickup_Treasure( gentity_t *ent, gentity_t *other ) {
	gentity_t *player = AICast_FindEntityForName( "player" );
	player->numTreasureFound++;
	G_SendMissionStats();
	return RESPAWN_SP;  // no respawn
}


/*
==============
UseHoldableItem
	server side handling of holdable item use
==============
*/
void UseHoldableItem( gentity_t *ent, int item ) {
	switch ( item ) {
	case HI_WINE:           // 1921 Chateu Lafite - gives 25 pts health up to max health
		ent->health += 25;
		if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		break;

	case HI_STAMINA:        // restores fatigue bar and sets "nofatigue" for a time period (currently forced to 60 sec)
		//----(SA)	NOTE:	currently only gives free nofatigue time, doesn't reset fatigue bar.
		//					(this is because I'd like the restore to be visually gradual (on the HUD item representing
		//					current status of your fatigue) rather than snapping back to 'full')
		ent->client->ps.powerups[PW_NOFATIGUE] = 60000;
		break;

	case HI_BOOK1:
	case HI_BOOK2:
	case HI_BOOK3:
		G_AddEvent( ent, EV_POPUPBOOK, ( item - HI_BOOK1 ) + 1 );
		break;
	}
}


//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {
	gitem_t *item;

//	item = BG_FindItemForHoldable(ent->item);
	item = ent->item;

	if ( item->gameskillnumber[0] ) {  // if the item specifies an amount, give it
		other->client->ps.holdable[item->giTag] += item->gameskillnumber[0];
	} else {
		other->client->ps.holdable[item->giTag] += 1;   // add default of 1

	}
	other->client->ps.holding = item->giTag;

	other->client->ps.stats[STAT_HOLDABLE_ITEM] |= ( 1 << ent->item->giTag );   //----(SA)	added

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}
	}

	return RESPAWN_HOLDABLE;
}


//======================================================================

/*
==============
Fill_Clip
	push reserve ammo into available space in the clip
==============
*/
void Fill_Clip( playerState_t *ps, int weapon ) {
	int inclip, maxclip, ammomove;
	int ammoweap = BG_PlayerAmmoIndex(ps,weapon);

	if ( weapon < WP_LUGER || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( g_dmflags.integer & DF_NO_WEAPRELOAD ) {
		return;
	}

	inclip  = ps->ammoclip[BG_FindClipForWeapon( weapon )];
	maxclip = CW_MaxClip(ps,weapon);

	ammomove = maxclip - inclip;    // max amount that can be moved into the clip
    if(!ps->aiChar && CW_WeaponDef(weapon) && ammomove<0)ammomove=0;

	// cap move amount if it's more than you've got in reserve
	if ( ammomove > ps->ammo[ammoweap] ) {
		ammomove = ps->ammo[ammoweap];
	}

	if ( ammomove ) {
		if ( !ps->aiChar || ps->ammo[ammoweap] < 999 ) {  // RF, dont take ammo away if they need unlimited supplies
			ps->ammo[ammoweap] -= ammomove;
		}
		ps->ammoclip[BG_FindClipForWeapon( weapon )] += ammomove;
	}
}

/*
==============
Add_Ammo
	Try to always add ammo here unless you have specific needs
	(like the AI "infinite ammo" where they get below 900 and force back up to 999)

	fillClip will push the ammo straight through into the clip and leave the rest in reserve
==============
*/
void Add_Ammo( gentity_t *ent, int weapon, int count, qboolean fillClip ) {
	int ammoweap = BG_PlayerAmmoIndex(&ent->client->ps,weapon);
	qboolean noPack = qfalse;       // no extra ammo in your 'pack'

	ent->client->ps.ammo[ammoweap] += count;

	switch ( ammoweap ) {
		// some weaps load straight into the 'clip' since they have no storage outside the clip

	case WP_GRENADE_LAUNCHER:       // make sure if he picks up a grenade that he get's the "launcher" too
	case WP_GRENADE_PINEAPPLE:
	case WP_MK1:
	case WP_RGD33:
	case WP_DYNAMITE:
		COM_BitSet( ent->client->ps.weapons, ammoweap );

	case WP_TESLA:
	case WP_FLAMETHROWER:
		noPack = qtrue;
		break;
	default:
		break;
	}

    if(!ent->client->ps.aiChar && CW_Number(CW_WeaponDef(weapon),"clipOnly"))noPack=qtrue;

	if ( fillClip || noPack ) {
		Fill_Clip( &ent->client->ps, weapon );
	}

	if ( ent->aiCharacter ) {
		noPack = qfalse;    // let AI's deal with their own clip/ammo handling

	}
	// cap to max ammo
	if ( noPack ) {
		ent->client->ps.ammo[ammoweap] = 0;
	} else {
		if ( ent->client->ps.ammo[ammoweap] > CW_MaxAmmo(&ent->client->ps,weapon) ) {
			ent->client->ps.ammo[ammoweap] = CW_MaxAmmo(&ent->client->ps,weapon);
		}

		if ( count >= 999 && (ent->client->ps.aiChar || !CW_WeaponDef(weapon)) ) { // 'really, give /all/'
			ent->client->ps.ammo[ammoweap] = count;
		}
	}

    if(!ent->client->ps.aiChar && CW_WeaponDef(weapon)) {
        int clip=BG_FindClipForWeapon(weapon),maximum=CW_MaxClip(&ent->client->ps,weapon);
        if(ent->client->ps.ammoclip[clip]>maximum)ent->client->ps.ammoclip[clip]=maximum;
    } else {
	if ( ent->client->ps.ammoclip[ammoweap] > ammoTable[ammoweap].maxclip ) {
		ent->client->ps.ammoclip[ammoweap] = ammoTable[ammoweap].maxclip;
	}
    }

}


/*
==============
Pickup_Ammo
==============
*/
int Pickup_Ammo( gentity_t *ent, gentity_t *other ) {
	int quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		// quantity = ent->item->quantity;

		quantity = ent->item->gameskillnumber[g_gameskill.integer];

		// FIXME just for now
		if ( !quantity ) {
			quantity = ent->item->quantity;
		}
	}

	Add_Ammo( other, ent->item->giTag, quantity, qfalse );   //----(SA)	modified

	// single player has no respawns	(SA)
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}
	}

	return RESPAWN_AMMO;
}

//======================================================================


/* A dropped CoD weapon retains two distinct ammunition quantities. Zero is
 * literal, unlike RTCW's count==0 default/random ammunition convention. */
static gentity_t *G_Cod1DropInventoryWeapon(gentity_t *other,gentity_t *pickup,int weapon) {
    playerState_t *ps=&other->client->ps;
    gitem_t *item=BG_FindItemForWeapon(weapon);
    gentity_t *drop;int clip=BG_FindClipForWeapon(weapon),ammo=BG_PlayerAmmoIndex(ps,weapon),i;
    if(!item)return NULL;
    /* A slot exchange inherits the world placement of the item taken.
     * The inventory bookkeeping below still owns the old clip and reserve. */
    drop=LaunchItem(item,pickup->r.currentOrigin,vec3_origin);
    if(!drop)return NULL;
    G_SetOrigin(drop,pickup->r.currentOrigin);
    G_SetAngle(drop,pickup->r.currentAngles);
    drop->s.groundEntityNum=pickup->s.groundEntityNum;
    drop->s.otherEntityNum2=pickup->s.otherEntityNum2;
    drop->s.eFlags &= ~EF_BOUNCE_HALF;
    VectorCopy(pickup->r.mins,drop->r.mins);
    VectorCopy(pickup->r.maxs,drop->r.maxs);
    trap_LinkEntity(drop);
    if(g_debugMove.integer)G_Printf("[Training exchange] take=%d old=%d source=%d placed=%d xyz=%.3f %.3f %.3f item=%.3f %.3f %.3f presentation=%d/%d stationary=%d\n",
        pickup->item->giTag,weapon,pickup->s.number,drop->s.number,pickup->r.currentOrigin[0],pickup->r.currentOrigin[1],pickup->r.currentOrigin[2],
        drop->r.currentOrigin[0],drop->r.currentOrigin[1],drop->r.currentOrigin[2],pickup->s.otherEntityNum2,drop->s.otherEntityNum2,drop->s.pos.trType==TR_STATIONARY);
    drop->cod1DroppedAmmo=qtrue;
    drop->cod1DropClip=ps->ammoclip[clip];drop->cod1DropReserve=ps->ammo[ammo];
    drop->cod1DropOwner=other->s.number;drop->cod1DropOwnerUntil=level.time+500;
    ps->ammoclip[clip]=0;ps->ammo[ammo]=0;
    COM_BitClear(ps->weapons,weapon);
    for(i=1;i<CW_SLOT_COUNT;i++)if(ps->cod1Slots[i]==weapon)ps->cod1Slots[i]=0;
    return drop;
}
static int G_Cod1PickupWeapon(gentity_t *ent,gentity_t *other) {
    playerState_t *ps=&other->client->ps;
    int weapon=ent->item->giTag,slot,old,owned,clip,ammo,quantity;
    CW_SyncSlots(ps);owned=COM_BitCheck(ps->weapons,weapon);
    if(!owned && CW_WeaponDef(ps->weapon) && !COM_BitCheck(ps->weapons,ps->weapon))return 0;
    if(ent->cod1DroppedAmmo && ent->cod1DropOwner==other->s.number && level.time<ent->cod1DropOwnerUntil)return 0;
    slot=CW_PickupSlot(ps,weapon,other->client->pers.autoActivate==PICKUP_FORCE);
    if(!slot && !owned)return 0;
    old=slot?ps->cod1Slots[slot]:0;
    if(!owned && old && old!=weapon && !(CW_SlotStackable(old)&&CW_SlotStackable(weapon))) {
        if(!G_Cod1DropInventoryWeapon(other,ent,old))return 0;
    }
    COM_BitSet(ps->weapons,weapon);
    if(slot && (!old || !CW_SlotStackable(weapon)))ps->cod1Slots[slot]=weapon;
    clip=BG_FindClipForWeapon(weapon);ammo=BG_PlayerAmmoIndex(ps,weapon);
    if(ent->cod1DroppedAmmo) {
        if(!owned) {
            ps->ammoclip[clip]=ent->cod1DropClip;
            ps->ammo[ammo]=ent->cod1DropReserve;
        } else Add_Ammo(other,weapon,ent->cod1DropClip+ent->cod1DropReserve,qfalse);
    } else {
        quantity=ent->count<0?0:ent->count;
        if(!ent->count)quantity=(int)CW_Number(CW_WeaponDef(weapon),"startAmmo");
        Add_Ammo(other,weapon,quantity,!owned);
    }
    CW_SyncSlots(ps);
    /* A forced slot replacement can drop the selected gun before another
     * client command arrives. Do not expose an unowned current weapon to
     * GSC in that intervening server frame. The next genuine PM command
     * raises the newly selected owned weapon through the normal path. */
    if(!owned && old && old!=weapon && ps->weapon==old && !COM_BitCheck(ps->weapons,old)) {
        ps->weapon=WP_NONE;ps->weaponstate=WEAPON_READY;
        ps->weaponTime=ps->weaponDelay=ps->grenadeTimeLeft=ps->weapAnimTimer=0;
        ps->stats[STAT_COD1_RELOAD]=ps->stats[STAT_COD1_SWITCH_TARGET]=0;
        ps->stats[STAT_COD1_ADS]=ps->stats[STAT_COD1_ADS_REMAINDER]=0;
    }
    if(g_gametype.integer==GT_SINGLE_PLAYER && !(ent->spawnflags&8))return RESPAWN_SP;
    return g_gametype.integer==GT_TEAM?g_weaponTeamRespawn.integer:g_weaponRespawn.integer;
}

int Pickup_Weapon( gentity_t *ent, gentity_t *other ) {
	int quantity;
	qboolean alreadyHave = qfalse;
	int weapon;

	weapon = ent->item->giTag;
    if(!other->client->ps.aiChar && CW_WeaponDef(weapon))return G_Cod1PickupWeapon(ent,other);

	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
//----(SA) modified
//			quantity = ent->item->quantity;
//			quantity = (random() * (ent->item->quantity - 1)) + 1;	// giving 1-<item default count>
			quantity = ( random() * ( ammoTable[weapon].maxclip - 4 ) ) + 4;    // giving 4-<item default count>

		}

		// dropped items and teamplay weapons always have full ammo
//		if ( ! (ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM ) {
//			// respawning rules
//			// drop the quantity if the already have over the minimum
//			if ( other->client->ps.ammo[ BG_FindAmmoForWeapon(weapon)] < quantity ) {
//				quantity = quantity - other->client->ps.ammo[ BG_FindAmmoForWeapon(weapon)];
//			} else {
//				quantity = 1;		// only add a single shot
//			}
//		}
//----(SA) end
	}


	//----(SA)	added
	// check for special colt->akimbo add (if you've got a colt already, add the second now)
	if ( weapon == WP_COLT && (other->client->ps.aiChar || !CW_WeaponDef(weapon)) ) {
		if ( COM_BitCheck( other->client->ps.weapons, WP_COLT ) ) {
			weapon = WP_AKIMBO;
		}
	}
//----(SA)	end

	// check if player already had the weapon
	alreadyHave = COM_BitCheck( other->client->ps.weapons, weapon );

	// add the weapon
	COM_BitSet( other->client->ps.weapons, weapon );

//----(SA)	added
	// snooper/garand
	if ( weapon == WP_SNOOPERSCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_GARAND );
	} else if ( weapon == WP_GARAND && (other->client->ps.aiChar || !CW_WeaponDef(weapon)) ) {
		COM_BitSet( other->client->ps.weapons, WP_SNOOPERSCOPE );
	}
	// fg42/scope
	else if ( weapon == WP_FG42SCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_FG42 );
	} else if ( weapon == WP_FG42 && (other->client->ps.aiChar || !CW_WeaponDef(weapon)) ) {
		COM_BitSet( other->client->ps.weapons, WP_FG42SCOPE );
	} else if ( weapon == WP_SNIPERRIFLE ) {
		COM_BitSet( other->client->ps.weapons, WP_MAUSER );
	}

//----(SA)	end

	Add_Ammo( other, weapon, quantity, !alreadyHave );

//----(SA) no hook
//	if (weapon == WP_GRAPPLING_HOOK)
//		other->client->ps.ammo[BG_FindAmmoForWeapon(weapon)] = -1; // unlimited ammo

	// single player has no respawns	(SA)
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}
	}

//	// team deathmatch has slow weapon respawns
//	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
//		return RESPAWN_TEAM_WEAPON;
//	}

	if ( g_gametype.integer == GT_TEAM ) {
		return g_weaponTeamRespawn.integer;
	}

	return g_weaponRespawn.integer;
}


//======================================================================

int Pickup_Health( gentity_t *ent, gentity_t *other ) {
	int max;
	int quantity = 0;

	/* In CoD SP the three aid kits give a percentage of maximum health:
	 * 10/25/50, or the entity's count override.  The native code adjusts
	 * integer truncation so the resulting health percentage matches its
	 * requested percentage.  RTCW's food and wall boxes keep their own
	 * multi-stage and skill-dependent rules below. */
	if ( g_gametype.integer == GT_SINGLE_PLAYER &&
		 G_Cod1NativeHealthItem( ent->item ) ) {
		int maxHealth = other->client->ps.stats[STAT_MAX_HEALTH];
		int currentPercent, targetPercent, resultPercent;
		int result;
		/* The shared item table retains RTCW's small-health quantity 5:
		 * changing it would also alter RTCW overhealth rules.  Native CoD
		 * instead defines small/medium/large as 10/25/50 percent. */
		if ( ent->count ) {
			quantity = ent->count;
		} else if ( !Q_stricmp( ent->item->classname, "item_health_small" ) ) {
			quantity = 10;
		} else if ( !Q_stricmp( ent->item->classname, "item_health_large" ) ) {
			quantity = 50;
		} else {
			quantity = 25;
		}
		if ( maxHealth <= 0 ) {
			return RESPAWN_SP;
		}
		result = other->health + maxHealth * quantity / 100;
		currentPercent = other->health * 100 / maxHealth;
		if ( currentPercent < 1 ) {
			currentPercent = 1;
		} else if ( currentPercent > 100 ) {
			currentPercent = 100;
		}
		targetPercent = currentPercent + quantity;
		if ( targetPercent < 1 ) {
			targetPercent = 1;
		} else if ( targetPercent > 100 ) {
			targetPercent = 100;
		}
		resultPercent = result * 100 / maxHealth;
		if ( resultPercent < 1 ) {
			resultPercent = 1;
		} else if ( resultPercent > 100 ) {
			resultPercent = 100;
		}
		if ( resultPercent != targetPercent ) {
			result = maxHealth * targetPercent / 100;
		}
		if ( result > maxHealth ) {
			result = maxHealth;
		}
		other->health = result;
		other->client->ps.stats[STAT_HEALTH] = result;
		return RESPAWN_SP;
	}

	// small and mega healths will go over the max
	if ( ent->item->quantity != 5 && ent->item->quantity != 100  ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		if ( ent->s.density ) {    // multi-stage health
			if ( ent->s.density == 2 ) {       // first stage (it counts down)
				quantity = ent->item->gameskillnumber[g_gameskill.integer];
			} else if ( ent->s.density == 1 )      { // second stage
				quantity = ent->item->quantity;
			}
		} else {
			quantity = ent->item->gameskillnumber[g_gameskill.integer];
		}
	}

	other->health += quantity;

	if ( other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->s.density == 2 ) {   // multi-stage health first stage
		return RESPAWN_PARTIAL;
	} else if ( ent->s.density == 1 ) {    // last stage, leave the plate
		return RESPAWN_PARTIAL_DONE;
	}

	// single player has no respawns	(SA)
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return RESPAWN_SP;
	}

	if ( ent->item->giTag == 100 ) {        // mega health respawns slow
		return RESPAWN_MEGAHEALTH;
	}

	return RESPAWN_HEALTH;
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
//	if ( other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
//		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	if ( other->client->ps.stats[STAT_ARMOR] > 100 ) {
		other->client->ps.stats[STAT_ARMOR] = 100;
	}

	// single player has no respawns	(SA)
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return RESPAWN_SP;
	}

	return RESPAWN_ARMOR;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
	// randomly select from teamed entities
	if ( ent->team ) {
		gentity_t   *master;
		int count;
		int choice;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster" );
		}
		master = ent->teammaster;

		for ( count = 0, ent = master; ent; ent = ent->teamchain, count++ )
			;

		choice = rand() % count;

		for ( count = 0, ent = master; count < choice; ent = ent->teamchain, count++ )
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.eFlags &= ~EF_NODRAW;
	ent->flags &= ~FL_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity( ent );

/*
	if ( ent->item->giType == IT_POWERUP && g_gametype.integer != GT_SINGLE_PLAYER) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}
*/

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}


/*
==============
Touch_Item
	if other->client->pers.autoActivate == PICKUP_ACTIVATE	(0), he will pick up items only when using +activate
	if other->client->pers.autoActivate == PICKUP_TOUCH		(1), he will pickup items when touched
	if other->client->pers.autoActivate == PICKUP_FORCE		(2), he will pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)
==============
*/
qboolean G_ItemAutoTouchAllowed( gentity_t *ent, gentity_t *other ) {
	/* CoD map GSCs can disable automatic touch via setautopickup(false),
	 * which the port cannot execute yet.  Keep weapons for the aimed Use
	 * interaction until that script flag exists, without changing automatic
	 * ammo/health pickup.  Cmd_Activate_f sets PICKUP_FORCE explicitly. */
	if ( !ent || !other || !other->client ||
		 other->client->pers.autoActivate == PICKUP_ACTIVATE ) {
		return qfalse;
	}
	if ( g_gametype.integer == GT_SINGLE_PLAYER && ent->item &&
		 ent->item->giType == IT_WEAPON && CW_WeaponDef( ent->item->giTag ) &&
		 !other->client->ps.aiChar &&
		 other->client->pers.autoActivate != PICKUP_FORCE ) {
		return qfalse;
	}
	return qtrue;
}

void Touch_Item_Auto( gentity_t *ent, gentity_t *other, trace_t *trace ) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef cod1DropTouchRef;int cod1DropTouchOwned=G_Cod1DroppedItemPickupBegin(ent,other,&cod1DropTouchRef);
    if(cod1DropTouchOwned<0)return;
#endif
	if ( !G_ItemAutoTouchAllowed( ent, other ) ) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
        if(cod1DropTouchOwned)G_Cod1DroppedItemPickupEnd(&cod1DropTouchRef);
#endif
		return;
	}

	ent->active = qtrue;
	Touch_Item( ent, other, trace );

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(cod1DropTouchOwned&&!G_Cod1DroppedItemPickupPlayerAlive(&cod1DropTouchRef,other)){G_Cod1DroppedItemPickupEnd(&cod1DropTouchRef);return;}
#endif
	if ( other->client->pers.autoActivate == PICKUP_FORCE ) {      // autoactivate probably forced by the "Cmd_Activate_f()" function
		other->client->pers.autoActivate = PICKUP_ACTIVATE;     // so reset it.
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(cod1DropTouchOwned)G_Cod1DroppedItemPickupEnd(&cod1DropTouchRef);
#endif

}


/*
===============
Touch_Item
===============
*/
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace ) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef cod1DropTouchRef;int cod1DropTouchOwned=G_Cod1DroppedItemPickupOwner(ent,&cod1DropTouchRef);
    if(cod1DropTouchOwned<0)return;
#endif
	int respawn;
	int makenoise = EV_ITEM_PICKUP;
	int healthBefore;

	// only activated items can be picked up
	if ( !ent->active ) {
		return;
	} else {
		// need to set active to false if player is maxed out
		ent->active = qfalse;
	}

	if ( !other->client ) {
		return;
	}
	if ( other->health < 1 ) {
		return;     // dead people can't pickup

	}
	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( &ent->s, &other->client->ps ) ) {
		return;
	}
	healthBefore = other->client->ps.stats[STAT_HEALTH];

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(cod1DropTouchOwned&&!G_Cod1DroppedItemPickupCurrent(&cod1DropTouchRef))return;
#endif

	// call the item-specific pickup function
	switch ( ent->item->giType ) {
	case IT_WEAPON:
        if(!other->client->ps.aiChar && CW_WeaponDef(ent->item->giTag) &&
           COM_BitCheck(other->client->ps.weapons,ent->item->giTag))
            makenoise=EV_COD1_WEAPON_AMMO_PICKUP;
		respawn = Pickup_Weapon( ent, other );
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo( ent, other );
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor( ent, other );
		break;
	case IT_HEALTH:
		respawn = Pickup_Health( ent, other );
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup( ent, other );
		break;
	case IT_TEAM:
		respawn = Pickup_Team( ent, other );
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable( ent, other );
		break;
	case IT_KEY:
		respawn = Pickup_Key( ent, other );
		break;
	case IT_TREASURE:
		respawn = Pickup_Treasure( ent, other );
		break;
	case IT_CLIPBOARD:
		respawn = Pickup_Clipboard( ent, other );
		// send the event to the client to request that the UI draw a popup
		// (specified by the configstring in ent->s.density).
		G_AddEvent( other, EV_POPUP, ent->s.density );
		if ( ent->key ) {
			G_AddEvent( other, EV_GIVEPAGE, ent->key );
		}
		break;
	default:
		return;
	}

	if ( !respawn ) {
		return;
	}

	/* The retail game emits game-message commands alongside pickup events.
	 * Keep its health amount accurate after skill-dependent pickup rules, and
	 * send ammo's pickup name through the same status channel. */
	if ( g_gametype.integer == GT_SINGLE_PLAYER && ent->item->giType == IT_HEALTH &&
		 other->client->ps.stats[STAT_HEALTH] > healthBefore ) {
		trap_SendServerCommand( other->s.number,
			va( "gm \"GAME_PICKUP_HEALTH%c%d\"", 21,
				other->client->ps.stats[STAT_HEALTH] - healthBefore ) );
	} else if ( g_gametype.integer == GT_SINGLE_PLAYER && ent->item->giType == IT_AMMO &&
				ent->item->pickup_name && ent->item->pickup_name[0] ) {
		trap_SendServerCommand( other->s.number,
			va( "gm \"GAME_PICKUP_AMMO%c%s\"", 20, ent->item->pickup_name ) );
	}

	// play sounds
	if ( ent->noise_index ) {
		// (SA) a sound was specified in the entity, so play that sound
		// (this G_AddEvent) and send the pickup as "EV_ITEM_PICKUP_QUIET"
		// so it doesn't make the default pickup sound when the pickup event is recieved
		makenoise = EV_ITEM_PICKUP_QUIET;
		G_AddEvent( other, EV_GENERAL_SOUND, ent->noise_index );
	}


	// send the pickup event
	if ( other->client->pers.predictItemPickup ) {
		G_AddPredictableEvent( other, makenoise, ent->s.modelindex );
	} else {
		G_AddEvent( other, makenoise, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM ) {
		// (SA) probably need to check for IT_KEY here too... (coop?)
		gentity_t   *te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
		te->s.eventParm = ent->s.modelindex;
		te->r.svFlags |= SVF_BROADCAST;

		// (SA) set if we want this to only go to the pickup client
//		te->r.svFlags |= SVF_SINGLECLIENT;
//		te->r.singleClient = other->s.number;

	}

#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    GscEntityRef placedWeapon;
    int retirePlaced = respawn == RESPAWN_SP && ent->s.eType == ET_ITEM &&
        ent->item && ent->item->giType == IT_WEAPON && CW_WeaponDef(ent->item->giTag) &&
        !(ent->flags & FL_DROPPED_ITEM) && !ent->wait && !ent->targetname && !ent->target &&
        G_Cod1MapEntities() && G_Cod1GscTestCarbineActive() &&
        G_Cod1EntityCapture(ent, &placedWeapon);
#endif
	// fire item targets
	G_UseTargets( ent, other );
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    /* Original2001B845: completed -1 pickup frees immediately. This selected
     * branch consumes only the captured dropped allocation, not RTCW items. */
    if(cod1DropTouchOwned&&G_Cod1DroppedItemPickupCommit(&cod1DropTouchRef,respawn))return;
#endif

#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    /* The retail SP pickup result -1 retires the picked allocation. Script
     * precacheItem authorizes a future spawn, independent of this pickup. */
    if (retirePlaced) {
        if (G_Cod1EntitySameAllocation(&placedWeapon)) G_FreeEntity(ent);
        return;
    }
    if (G_Cod1GscTestCarbineActive() && ent->item && ent->item->giType==IT_WEAPON &&
        G_Cod1GscTestCarbineSpawnAllowed(ent->classname) && respawn<0 && !ent->wait &&
        !ent->targetname && !ent->target) {
        G_FreeEntity(ent);return;
    }
#endif

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->flags |= FL_NODRAW;
		ent->r.svFlags |= SVF_NOCLIENT; // (SA) commented back in.
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// wait of -2 will respawn but not be available for pickup anymore
	// (partial use things that leave a spent modle (ex. plate for turkey)
	if ( respawn == RESPAWN_PARTIAL_DONE ) {
		ent->s.density = ( 1 << 9 );    // (10 bits of data transmission for density)
		ent->active = qtrue;        // re-activate
		trap_LinkEntity( ent );
		return;
	}

	if ( respawn == RESPAWN_PARTIAL ) {    // multi-stage health
		ent->s.density--;
		if ( ent->s.density ) {        // still not completely used up ( (SA) this will change to == 0 and stage 1 will be a destroyable item (plate/etc.) )
			ent->active = qtrue;        // re-activate
			trap_LinkEntity( ent );
			return;
		}
	}


	// non zero wait overrides respawn time
	if ( ent->wait ) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random ) {
		respawn += crandom() * ent->random;
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->flags |= FL_NODRAW;
	//ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if ( respawn <= 0 ) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity( ent );
}


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t   *dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
//	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item	//----(SA)	commented out since I'm using modelindex2 for model indexing now
	dropped->s.otherEntityNum2 = 1;     // DHM - Nerve :: this is taking modelindex2's place for signaling a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
//	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
//	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	VectorSet( dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );            //----(SA)	so items sit on the ground
	VectorSet( dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );  //----(SA)	so items sit on the ground
	dropped->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;

	dropped->touch = Touch_Item_Auto;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;

	// (SA) TODO: FIXME: don't do this right now.  bug needs to be found.
//	if(item->giType == IT_WEAPON)
//		dropped->s.eFlags |= EF_SPINNING;	// spin the weapon as it flies from the dead player.  it will stop when it hits the ground


	if ( item->giType == IT_TEAM ) { // Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
	} else { // auto-remove after 30 seconds
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity( dropped );

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean novelocity ) {
	vec3_t velocity;
	vec3_t angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;  // always forward

	if ( novelocity ) {
		VectorClear( velocity );
	} else
	{
		AngleVectors( angles, velocity, NULL, NULL );
		VectorScale( velocity, 150, velocity );
		velocity[2] += 200 + crandom() * 50;
	}

	return LaunchItem( item, ent->s.pos.trBase, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t tr;
	vec3_t dest;
	vec3_t maxs;

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef healthRef;gentity_t healthBefore;
    int healthOwned=G_Cod1PathHealthFinishStart(ent,&healthRef);
    if(healthOwned<0)return;
#endif

	if ( ent->spawnflags & 1 ) { // suspended
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
	} else
	{
		// Rafael
		// had to modify this so that items would spawn in shelves
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
		maxs[2] /= 2;
	}

	ent->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;
	ent->touch = Touch_Item_Auto;
	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;        // store item number in modelindex

	ent->s.otherEntityNum2 = 0;     // DHM - Nerve :: takes modelindex2's place in signaling a dropped item
//----(SA)	we don't use this (yet, anyway) so I'm taking it so you can specify a model for treasure items and clipboards
//	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item
	if ( ent->model ) {
		C1_HEALTH_BEFORE;
        {int model=G_ModelIndex(ent->model);C1_HEALTH_AFTER;ent->s.modelindex2=model;}
	}


	// if clipboard, add the menu name string to the client's configstrings
	if ( ent->item->giType == IT_CLIPBOARD ) {
		if ( !ent->message ) {
			ent->s.density = G_FindConfigstringIndex( "clip_test", CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		} else {
			ent->s.density = G_FindConfigstringIndex( ent->message, CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		}

		ent->touch = Touch_Item;    // no auto-pickup, only activate
	} else if ( ent->item->giType == IT_HOLDABLE )      {
		if ( ent->item->giTag >= HI_BOOK1 && ent->item->giTag <= HI_BOOK3 ) {
			G_FindConfigstringIndex( va( "hbook%d", ent->item->giTag - HI_BOOK1 ), CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		}
//		ent->touch = Touch_Item;	// no auto-pickup, only activate
	}


	// using an item causes it to respawn
	ent->use = Use_Item;

//----(SA) moved this up so it happens for suspended items too (and made it a function)
	G_SetAngle( ent, ent->s.angles );

	if ( ent->spawnflags & 1 ) {    // suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {

		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		C1_HEALTH_BEFORE;
		trap_Trace( &tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
        C1_HEALTH_AFTER;

		if ( tr.startsolid ) {
			vec3_t temp;

			VectorCopy( ent->s.origin, temp );
			temp[2] -= ITEM_RADIUS;

			VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
			C1_HEALTH_BEFORE;
		trap_Trace( &tr, temp, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
        C1_HEALTH_AFTER;
		}

		if ( tr.startsolid ) {
			C1_HEALTH_BEFORE;
			G_Printf( "FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos( ent->s.origin ) );
            C1_HEALTH_AFTER;
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	if ( ent->spawnflags & 2 ) {      // spin
		ent->s.eFlags |= EF_SPINNING;
	}


	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->flags |= FL_NODRAW;
		//ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// health/ammo can potentially be multi-stage (multiple use)
	if ( ent->item->giType == IT_HEALTH || ent->item->giType == IT_AMMO || ent->item->giType == IT_POWERUP ) {
		int i;

		// having alternate models defined in bg_misc.c for a health or ammo item specify it as "multi-stage"
		// TTimo: left-hand operand of comma expression has no effect
		// was:
		// for(i=0;i<4,ent->item->world_model[i];i++) {}
		i = 0;
		while ( i < 4 && ent->item->world_model[i] )
			i++;

		ent->s.density = i - 1;   // store number of stages in 'density' for client (most will have '1')
	}

	// powerups don't spawn in for a while
	if ( ent->item->giType == IT_POWERUP && g_gametype.integer != GT_SINGLE_PLAYER ) {
		float respawn;

		respawn = 45 + crandom() * 15;
		ent->flags |= FL_NODRAW;
		//ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}

    C1_HEALTH_BEFORE;
    trap_LinkEntity(ent);
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(healthOwned){
        if(!Cod1HealthLinkFence(ent,&healthRef,&healthBefore))return;
        G_Cod1PathHealthFinished(ent,&healthRef);
    }
#endif
}


qboolean itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {
	if ( g_gametype.integer == GT_CTF ) {
		gitem_t *item;

		// make sure we actually have two flags...
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Error( "No team_CTF_redflag in map" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Error( "No team_CTF_blueflag in map" );
		}
	}
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	memset( itemRegistered, 0, sizeof( itemRegistered ) );

	// players always start with the base weapon
	// (SA) Nope, not any more...

//----(SA)	this will be determined by the level or starting position, or the savegame
//			but for now, re-register the MP40 automatically
//	RegisterItem( BG_FindItemForWeapon( WP_MP40 ) );
	RegisterItem( BG_FindItem( "Med Health" ) );           // NERVE - SMF - this is so med packs properly display
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char string[MAX_ITEMS + 1];
	int i;
	int count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

	if ( trap_Cvar_VariableIntegerValue( "g_gametype" ) != GT_SINGLE_PLAYER ) {
		G_Printf( "%i items registered\n", count );
	}
	trap_SetConfigstring( CS_ITEMS, string );
}


/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
int G_Cod1SpawnTrainingWeapon(const char *name,const float origin[3],int32_t flags,GscEntityRef *out,char *error,size_t size) {
    GscEntityRef ref;gentity_t *e;gitem_t *item=NULL;int i;vec3_t position;
    if(!name||!origin||!out||flags||G_Cod1EntityStorageProtected(origin,sizeof(vec3_t))||
       G_Cod1EntityStorageProtected(out,sizeof(*out))||G_Cod1EntityStorageProtected(error,size)||
       (size&&(!error||(uintptr_t)error>UINTPTR_MAX-size)))return 0;
    /* Owned inputs are copied before the first admission callback. */
    if(!strcmp(name,"weapon_m1carbine"))name="weapon_m1carbine";
    else if(!strcmp(name,"weapon_springfield"))name="weapon_springfield";
    else if(!strcmp(name,"weapon_thompson"))name="weapon_thompson";else return 0;
    VectorCopy(origin,position);
    for(i=0;i<3;i++)if(!isfinite(position[i]))return 0;
    if(level.spawning||level.restarted||!G_Cod1GscTestCarbineSpawnAllowed(name))return 0;
    for(i=1;i<bg_numItems;i++)if(bg_itemlist[i].classname&&!strcmp(bg_itemlist[i].classname,name)&&bg_itemlist[i].giType==IT_WEAPON&&CW_WeaponDef(bg_itemlist[i].giTag)){item=&bg_itemlist[i];break;}
    if(!item||(level.num_entities>=ENTITYNUM_MAX_NORMAL&&!G_EntitiesFree()))return 0;
    e=G_Spawn();if(!G_Cod1EntityCapture(e,&ref))return 0;e->classname=G_NewString(name);VectorCopy(position,e->s.origin);
    /* With empty authored vars, no model/noise/clipboard branch can call an
     * engine service here. Restore the parser view before Finish's trace. */
    {int vars=level.numSpawnVars;level.numSpawnVars=0;G_SpawnItem(e,item);level.numSpawnVars=vars;}
    if(!G_Cod1EntitySameAllocation(&ref)||!G_Cod1GscTestCarbineSpawnAllowed(name))goto failed;
    e->nextthink=0;e->think=NULL;FinishSpawningItem(e);
    if(!G_Cod1EntitySameAllocation(&ref)||!e->r.linked||!e->touch||!G_Cod1GscTestCarbineSpawnAllowed(name))goto failed;
    *out=ref;return 1;
failed:
    if(G_Cod1EntitySameAllocation(&ref))G_FreeEntity(&g_entities[ref.slot]);
    if(error&&size)snprintf(error,size,"original native carbine item construction lost current registration");return 0;
}
#endif
void G_SpawnItem( gentity_t *ent, gitem_t *item ) {
	char    *noise;
	int page;

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    GscEntityRef healthRef;gentity_t healthBefore;
    int healthOwned=G_Cod1PathHealthStart(ent,item,&healthRef);
#endif

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	RegisterItem( item );
	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	if ( G_SpawnString( "noise", 0, &noise ) ) {
		C1_HEALTH_BEFORE;
        {int sound=G_SoundIndex(noise);C1_HEALTH_AFTER;ent->noise_index=sound;}
	}

	ent->physicsBounce = 0.50;      // items are bouncy

	if ( ent->model ) {
		C1_HEALTH_BEFORE;
        {int model=G_ModelIndex(ent->model);C1_HEALTH_AFTER;ent->s.modelindex2=model;}
	}

	if ( item->giType == IT_CLIPBOARD ) {
		if ( G_SpawnInt( "notebookpage", "1", &page ) ) {
			ent->key = page;
		}
	}

	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );
	}

	if ( item->giType == IT_TREASURE ) {
		level.numTreasure++;
		G_SendMissionStats();
	}
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    if(healthOwned)G_Cod1PathHealthPending(ent,&healthRef);
#endif
}



/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;    // make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

/*
=================
G_RunItemProp
=================
*/

void G_RunItemProp( gentity_t *ent, vec3_t origin ) {
	gentity_t   *traceEnt;
	trace_t trace;
	gentity_t   *owner;
	vec3_t start;
	vec3_t end;

	owner = &g_entities[ent->r.ownerNum];

	VectorCopy( ent->r.currentOrigin, start );
	start[2] += 1;

	VectorCopy( origin, end );
	end[2] += 1;

	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, end,
				ent->r.ownerNum, MASK_SHOT );

	traceEnt = &g_entities[ trace.entityNum ];

	if ( traceEnt && traceEnt->takedamage && traceEnt != ent ) {
		ent->enemy = traceEnt;
	}

	if ( owner->client && trace.startsolid && traceEnt != owner && traceEnt != ent /* && !traceEnt->active*/ ) {

		ent->takedamage = qfalse;
		ent->die( ent, ent, NULL, 10, 0 );
		Prop_Break_Sound( ent );

		return;
	} else if ( trace.surfaceFlags & SURF_NOIMPACT )    {
		ent->takedamage = qfalse;

		Props_Chair_Skyboxtouch( ent );

		return;
	}
}

/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
	if(G_Cod1ScriptLaunchFrame(ent))return;
	if(G_Cod1DroppedItemFrame(ent))return;
#endif
	vec3_t origin;
	trace_t tr;
	int contents;
	int mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY || ent->s.pos.trType == TR_GRAVITY_PAUSED ) { //----(SA)
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID | CONTENTS_MISSILECLIP;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
				ent->r.ownerNum, mask );

	if ( ent->isProp && ent->takedamage ) {
		G_RunItemProp( ent, origin );
	}

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent ); // FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if ( ent->item && ent->item->giType == IT_TEAM ) {
			Team_FreeEntity( ent );
		} else {
			G_FreeEntity( ent );
		}
		return;
	}

	G_BounceItem( ent, &tr );
}
