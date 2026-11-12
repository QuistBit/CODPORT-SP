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
 * name:		g_weapon.c
 *
 * desc:		perform the server side effects of a weapon firing
 *
*/


#include "g_local.h"
#include "bg_cod1_fire_event.h"
#include "bg_cod1_combat.h"
#include "bg_cod1_inventory.h"
#include "bg_cod1_lean.h"
#include "bg_cod1_impact.h"
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
#include "g_cod1_entity_host.h"
#endif

static float s_quadFactor;
static vec3_t forward, right, up;
static vec3_t muzzleEffect;
static vec3_t muzzleTrace;


// forward dec
void weapon_zombiespit( gentity_t *ent );

void Bullet_Fire( gentity_t *ent, float spread, int damage );
void Bullet_Fire_Extended( gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, float spread, int damage, int recursion );

int G_GetWeaponDamage( int weapon ); // JPW

/*
======================================================================

KNIFE/GAUNTLET (NOTE: gauntlet is now the Zombie melee)

======================================================================
*/

#define KNIFE_DIST 48

/*
==============
Weapon_Knife
==============
*/
void Weapon_Knife( gentity_t *ent ) {
	trace_t tr;
	gentity_t   *traceEnt, *tent;
	int damage, mod;
//	vec3_t		pforward, eforward;

	vec3_t end;

	mod = MOD_KNIFE;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( ent, ent->s.weapon, forward, right, up, muzzleTrace );
	VectorMA( muzzleTrace, KNIFE_DIST, forward, end );
	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return;
	}

	if ( tr.entityNum >= MAX_CLIENTS ) {   // world brush or non-player entity (no blood)
		tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
	} else {                            // other player
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
	}

	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm = DirToByte( tr.plane.normal );
	tent->s.weapon = ent->s.weapon;

	if ( tr.entityNum == ENTITYNUM_WORLD ) { // don't worry about doing any damage
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !( traceEnt->takedamage ) ) {
		return;
	}

	// RF, no knife damage for big guys
	switch ( traceEnt->aiCharacter ) {
	case AICHAR_PROTOSOLDIER:
	case AICHAR_SUPERSOLDIER:
	case AICHAR_HEINRICH:
		return;
	}

	damage = G_GetWeaponDamage( ent->s.weapon ); // JPW		// default knife damage for frontal attacks

	if ( traceEnt->client ) {
		if ( ent->client->ps.serverCursorHint == HINT_KNIFE ) {
//		AngleVectors (ent->client->ps.viewangles,		pforward, NULL, NULL);
//		AngleVectors (traceEnt->client->ps.viewangles,	eforward, NULL, NULL);

			// (SA) TODO: neutralize pitch (so only yaw is considered)
//		if(DotProduct( eforward, pforward ) > 0.9f)	{	// from behind

			// if relaxed, the strike is almost assured a kill
			// if not relaxed, but still from behind, it does 10x damage (50)

// (SA) commented out right now as the ai's state always checks here as 'combat'

//			if(ent->s.aiState == AISTATE_RELAXED) {
			damage = 100;       // enough to drop a 'normal' (100 health) human with one jab
			mod = MOD_KNIFE_STEALTH;
//			} else {
//				damage *= 10;
//			}
//----(SA)	end
		}
	}

	G_Damage( traceEnt, ent, ent, vec3_origin, tr.endpos, ( damage + rand() % 5 ) * s_quadFactor, 0, mod );
}

/* HANDHELD_FINAL_R1: WeaponDef-driven CoD melee. The preserved CoD1 cgame
 * proves input/timing/damage fields, but the original SP world trace range was
 * not part of that recovered block. Keep this range isolated as a compatibility
 * constant rather than pretending the RTCW knife path is native behavior. */
#define COD1_MELEE_RANGE 64.0f
void G_Cod1MeleeAttack( gentity_t *ent, int definitionIndex ) {
    const cwDef_t *d;trace_t tr;gentity_t *hit;vec3_t f,r,u,start,end;int damage;
    if(!ent || !ent->client || ent->health<=0 || definitionIndex<0)return;
    d=CW_Get(definitionIndex);
    if(!d || CW_FamilyRoot(definitionIndex)!=CW_WeaponRoot(ent->client->ps.weapon))return;
    damage=(int)CW_Number(d,"meleeDamage");if(damage<=0)return;
    AngleVectors(ent->client->ps.viewangles,f,r,u);
    CalcMuzzlePointForActivate(ent,f,r,u,start);
    VectorMA(start,COD1_MELEE_RANGE,f,end);
    trap_Trace(&tr,start,NULL,NULL,end,ent->s.number,MASK_SHOT);
    if((tr.surfaceFlags&SURF_NOIMPACT) || tr.fraction==1.0f || tr.entityNum==ENTITYNUM_WORLD)return;
    hit=&g_entities[tr.entityNum];if(!hit->takedamage)return;
    G_Damage(hit,ent,ent,f,tr.endpos,damage,0,MOD_KNIFE);
}

// JPW NERVE
/*
======================
  Weapon_Class_Special
	class-specific in multiplayer
======================
*/
// JPW NERVE
void Weapon_Medic( gentity_t *ent ) {
	gitem_t *item;
	gentity_t *ent2;
	vec3_t velocity, org, offset;
	vec3_t angles;

	trace_t tr;
	gentity_t   *traceEnt;
//	int			mod = MOD_KNIFE, healamt, headshot;
	int healamt, headshot;

	vec3_t end;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePointForActivate( ent, forward, right, up, muzzleTrace );
	VectorMA( muzzleTrace, 30, forward, end );           // CH_ACTIVATE_DIST
	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.fraction < 1.0 ) {
		traceEnt = &g_entities[ tr.entityNum ];
		if ( traceEnt->client != NULL ) {
			if ( ( traceEnt->client->ps.pm_type == PM_DEAD ) && ( traceEnt->client->sess.sessionTeam == ent->client->sess.sessionTeam ) ) {
				if ( level.time - ent->client->ps.classWeaponTime > g_medicChargeTime.integer ) {
					ent->client->ps.classWeaponTime = level.time - g_medicChargeTime.integer;
				}
				ent->client->ps.classWeaponTime += 125;
				traceEnt->client->medicHealAmt++;
				if ( ent->client->ps.classWeaponTime > level.time ) { // heal the dude
/*
					traceEnt->health = 80;
					traceEnt->s.weapon = WP_KNIFE;
					traceEnt->r.maxs[2] = 0;
					traceEnt->client->ps.pm_type = PM_NORMAL;
					traceEnt->client->ps.torsoAnim = traceEnt->client->oldAnimTorso;
					traceEnt->client->ps.legsAnim = traceEnt->client->oldAnimLegs;
					traceEnt->die = player_die;
					traceEnt->client->respawnTime = level.time + 1700;
*/
// copy some stuff out that we'll wanna restore
					VectorCopy( traceEnt->client->ps.origin, org );
					healamt = traceEnt->client->medicHealAmt;
					headshot = traceEnt->client->ps.eFlags & EF_HEADSHOT;

					ClientSpawn( traceEnt );
					if ( healamt > 80 ) {
						healamt = 80;
					}
					if ( healamt < 10 ) {
						healamt = 10;
					}
					if ( headshot ) {
						traceEnt->client->ps.eFlags |= EF_HEADSHOT;
					}
					traceEnt->health = healamt;
					VectorCopy( org,traceEnt->s.origin );
					VectorCopy( org,traceEnt->r.currentOrigin );
					VectorCopy( org,traceEnt->client->ps.origin );
				}
			}
		}
	} else { // throw out health pack
//		if (level.time - ent->client->ps.classWeaponTime >= g_medicChargeTime.integer) {
		if ( level.time - ent->client->ps.classWeaponTime >= g_medicChargeTime.integer * 0.25f ) {
			if ( level.time - ent->client->ps.classWeaponTime > g_medicChargeTime.integer ) {
				ent->client->ps.classWeaponTime = level.time - g_medicChargeTime.integer;
			}
			ent->client->ps.classWeaponTime += g_medicChargeTime.integer * 0.25;

//			ent->client->ps.classWeaponTime = level.time;
//			if (ent->client->ps.classWeaponTime > level.time)
//				ent->client->ps.classWeaponTime = level.time;
			item = BG_FindItem( "Med Health" );
			VectorCopy( ent->client->ps.viewangles, angles );
			angles[PITCH] = 0;  // always forward
			AngleVectors( angles, velocity, NULL, NULL );
			VectorScale( velocity, 75, offset );
			VectorScale( velocity, 50, velocity );
			velocity[2] += 50 + crandom() * 50;

			VectorAdd( ent->client->ps.origin,offset,org );
			ent2 = LaunchItem( item, org, velocity );
		}
	}
}
// jpw

// DHM - Nerve
void Weapon_Engineer( gentity_t *ent ) {
	trace_t tr;
	gentity_t   *traceEnt;
//	int			mod = MOD_KNIFE;

	vec3_t end;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePointForActivate( ent, forward, right, up, muzzleTrace );
	VectorMA( muzzleTrace, 96, forward, end );           // CH_ACTIVATE_DIST
	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return;
	}

	if ( tr.entityNum == ENTITYNUM_NONE || tr.entityNum == ENTITYNUM_WORLD ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];
	if ( traceEnt->methodOfDeath == MOD_DYNAMITE ) {

		traceEnt->health += 3;
		if ( traceEnt->health >= 248 ) {
			traceEnt->health = 255;
			// Need some kind of event/announcement here

			Add_Ammo( ent, WP_DYNAMITE, 1, qtrue );

			traceEnt->think = G_FreeEntity;
			traceEnt->nextthink = level.time + FRAMETIME;
// JPW NERVE
			if ( ent->client->sess.sessionTeam == TEAM_RED ) {
				trap_SendServerCommand( -1, "cp \"Axis engineer disarmed a det charge!\n\"" );
			} else {
				trap_SendServerCommand( -1, "cp \"Allied engineer disarmed a det charge!\n\"" );
			}
// jpw
		}
	} else if ( !traceEnt->takedamage && !Q_stricmp( traceEnt->classname, "misc_mg42" ) )       {
		// "Ammo" for this weapon is time based
		if ( ent->client->ps.classWeaponTime + g_engineerChargeTime.integer < level.time ) {
			ent->client->ps.classWeaponTime = level.time - g_engineerChargeTime.integer;
		}
		ent->client->ps.classWeaponTime += 150;

		if ( ent->client->ps.classWeaponTime > level.time ) {
			ent->client->ps.classWeaponTime = level.time;
			return;     // Out of "ammo"
		}

		if ( traceEnt->health >= 255 ) {
			traceEnt->s.frame = 0;

			if ( traceEnt->mg42BaseEnt > 0 ) {
				g_entities[ traceEnt->mg42BaseEnt ].health = 100;
				g_entities[ traceEnt->mg42BaseEnt ].takedamage = qtrue;
				traceEnt->health = 0;
			} else {
				traceEnt->health = 100;
			}

			traceEnt->takedamage = qtrue;

			trap_SendServerCommand( ent - g_entities, "cp \"You have repaired the MG42!\n\"" );
		} else {
			traceEnt->health += 3;
		}
	}
}


// JPW NERVE -- launch airstrike as line of bombs mostly-perpendicular to line of grenade travel
// (close air support should *always* drop parallel to friendly lines, tho accidents do happen)
void G_ExplodeMissile( gentity_t *ent );
#define NUMBOMBS 10
#define BOMBSPREAD 150
extern void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message );
void weapon_callAirStrike( gentity_t *ent ) {
	int i;
	vec3_t bombaxis, lookaxis, pos, bomboffset, fallaxis;
	gentity_t *bomb;
	trace_t tr;
	float traceheight, bottomtraceheight;

	VectorCopy( ent->s.pos.trBase,bomboffset );
	bomboffset[2] += 4096;

	// turn off smoke grenade
	ent->think = G_ExplodeMissile;
	ent->nextthink = level.time + 1000 + NUMBOMBS * 100 + crandom() * 50; // 3000 offset is for aircraft flyby

	trap_Trace( &tr, ent->s.pos.trBase, NULL, NULL, bomboffset, ent->s.number, MASK_SHOT );
	if ( ( tr.fraction < 1.0 ) && ( !( tr.surfaceFlags & SURF_SKY ) ) ) {
		G_SayTo( ent->parent, ent->parent, 2, COLOR_YELLOW, "Pilot: ", "Can't see target, aborting bomb run" );
		return;
	}

	VectorCopy( tr.endpos, bomboffset );
	traceheight = bomboffset[2];
	bottomtraceheight = traceheight - 8192;

	VectorSubtract( ent->s.pos.trBase,ent->parent->client->ps.origin,lookaxis );
	lookaxis[2] = 0;
	VectorNormalize( lookaxis );
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = crandom(); // generate either up or down vector,
	VectorNormalize( pos ); // which adds randomness to pass direction below
	RotatePointAroundVector( bombaxis,pos,lookaxis,90 + crandom() * 30 ); // munge the axis line a bit so it's not totally perpendicular
	VectorNormalize( bombaxis );

	VectorCopy( bombaxis,pos );
	VectorScale( pos,(float)( -0.5f * BOMBSPREAD * NUMBOMBS ),pos );
	VectorAdd( ent->s.pos.trBase, pos, pos ); // first bomb position
	VectorScale( bombaxis,BOMBSPREAD,bombaxis ); // bomb drop direction offset

// add an aircraft (looks suspiciously like a rocket right now) (but doesn't work)
/*
	bomb = G_Spawn();
	bomb->nextthink = level.time + 26000;
	bomb->think = G_ExplodeMissile;
	bomb->s.eType		= ET_MISSILE;
	bomb->r.svFlags		= SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
	bomb->s.weapon		= WP_GRENADE_LAUNCHER; // might wanna change this
	bomb->r.ownerNum	= ent->s.number;
	bomb->parent		= ent->parent;
	bomb->damage		= 400; // maybe should un-hard-code these?
	bomb->splashDamage  = 400;
	bomb->classname		= "fighterbomber";
	bomb->splashRadius			= 400;
	bomb->methodOfDeath			= MOD_DYNAMITE; // FIXME add MOD for air strike
	bomb->splashMethodOfDeath	= MOD_DYNAMITE_SPLASH;
	bomb->clipmask = MASK_MISSILESHOT;
	bomb->s.pos.trType = TR_STATIONARY; // TR_LINEAR;
	bomb->s.pos.trTime = level.time;
	VectorCopy(ent->s.pos.trBase, bomb->s.pos.trBase);
	bomb->s.pos.trBase[2] += 200;
	bomb->s.modelindex = G_ModelIndex( "models/mapobjects/vehicles/m109.md3" );
*/
	for ( i = 0; i < NUMBOMBS; i++ ) {
		bomb = G_Spawn();
		bomb->nextthink = level.time + i * 100 + crandom() * 50 + 1000; // 1000 for aircraft flyby, other term for tumble stagger
		bomb->think = G_ExplodeMissile;
		bomb->s.eType       = ET_MISSILE;
		bomb->r.svFlags     = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
		bomb->s.weapon      = WP_GRENADE_LAUNCHER; // might wanna change this
		bomb->r.ownerNum    = ent->s.number;
		bomb->parent        = ent->parent;
		bomb->damage        = 400; // maybe should un-hard-code these?
		bomb->splashDamage  = 400;
		bomb->classname             = "air strike";
		bomb->splashRadius          = 400;
		bomb->methodOfDeath         = MOD_AIRSTRIKE;
		bomb->splashMethodOfDeath   = MOD_AIRSTRIKE;
		bomb->clipmask = MASK_MISSILESHOT;
		bomb->s.pos.trType = TR_STATIONARY; // was TR_GRAVITY,  might wanna go back to this and drop from height
		bomb->s.pos.trTime = level.time;        // move a bit on the very first frame
		bomboffset[0] = crandom() * 0.5 * BOMBSPREAD;
		bomboffset[1] = crandom() * 0.5 * BOMBSPREAD;
		bomboffset[2] = 0;
		VectorAdd( pos,bomboffset,bomb->s.pos.trBase );

		VectorCopy( bomb->s.pos.trBase,bomboffset ); // make sure bombs fall "on top of" nonuniform scenery
		bomboffset[2] = traceheight;

		VectorCopy( bomboffset, fallaxis );
		fallaxis[2] = bottomtraceheight;

		trap_Trace( &tr, bomboffset, NULL, NULL, fallaxis, ent->s.number, MASK_SHOT );
		if ( tr.fraction != 1.0 ) {
			VectorCopy( tr.endpos,bomb->s.pos.trBase );
		}

		bomb->s.pos.trDelta[0] = 0; // might need to change this
		bomb->s.pos.trDelta[1] = 0;
		bomb->s.pos.trDelta[2] = 0;
		SnapVector( bomb->s.pos.trDelta );          // save net bandwidth
		VectorCopy( bomb->s.pos.trBase, bomb->r.currentOrigin );

		// move pos for next bomb
		VectorAdd( pos,bombaxis,pos );
	}
}

gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void Weapon_Class_Special( gentity_t *ent ) {

	switch ( ent->client->ps.stats[STAT_PLAYER_CLASS] ) {
	case PC_SOLDIER:
		G_Printf( "shooting soldier\n" );
		break;
	case PC_MEDIC:
		Weapon_Medic( ent );
		break;
	case PC_ENGINEER:
		//G_Printf("shooting engineer\n");
		//ent->client->ps.classWeaponTime = level.time;
		Weapon_Engineer( ent );
		break;
	case PC_LT:
		if ( level.time - ent->client->ps.classWeaponTime > g_LTChargeTime.integer ) {
			weapon_grenadelauncher_fire( ent,WP_GRENADE_SMOKE );
			ent->client->ps.classWeaponTime = level.time;
		}
		break;
	}
}
// jpw

/*
==============
Weapon_Gauntlet
==============
*/
void Weapon_Gauntlet( gentity_t *ent ) {
	trace_t *tr;
	tr = CheckMeleeAttack( ent, 32, qfalse );
	if ( tr ) {
		G_Damage( &g_entities[tr->entityNum], ent, ent, vec3_origin, tr->endpos,
				  ( 10 + rand() % 5 ) * s_quadFactor, 0, MOD_GAUNTLET );
	}
}

/*
===============
CheckMeleeAttack
	using 'isTest' to return hits to world surfaces
===============
*/
trace_t *CheckMeleeAttack( gentity_t *ent, float dist, qboolean isTest ) {
	static trace_t tr;
	vec3_t end;
	gentity_t   *tent;
	gentity_t   *traceEnt;

	// set aiming directions
	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePoint( ent, WP_GAUNTLET, forward, right, up, muzzleTrace );

	VectorMA( muzzleTrace, dist, forward, end );

	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return NULL;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return NULL;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

//----(SA)	added
	if ( isTest ) {
		return &tr;
	}
//----(SA)

	if ( !traceEnt->takedamage ) {
		return NULL;
	}

	if ( ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}

	return &tr;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/

// (SA) modified so it doesn't have trouble with negative locations (quadrant problems)
//			(this was causing some problems with bullet marks appearing since snapping
//			too far off the target surface causes the the distance between the transmitted impact
//			point and the actual hit surface larger than the mark radius.  (so nothing shows) )

void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
//			v[i] = (int)v[i];
			v[i] = floor( v[i] );
		} else {
//			v[i] = (int)v[i] + 1;
			v[i] = ceil( v[i] );
		}
	}
}

// JPW
// mechanism allows different weapon damage for single/multiplayer; we want "balanced" weapons
// in multiplayer but don't want to alter the existing single-player damage items that have already
// been changed
//
// KLUDGE/FIXME: also modded #defines below to become macros that call this fn for minimal impact elsewhere
//
int G_GetWeaponDamage( int weapon ) {
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		switch ( weapon ) {
		case WP_LUGER:
		case WP_SILENCER: return 6;
		case WP_COLT: return 8;
		case WP_AKIMBO: return 8;       //----(SA)	added
		case WP_VENOM: return 12;       // 15  ----(SA)	slight modify for DM
		case WP_MP40: return 6;
		case WP_THOMPSON: return 8;
		case WP_STEN: return 10;
		case WP_FG42SCOPE:
		case WP_FG42: return 15;
		case WP_MAUSER: return 20;
		case WP_GARAND: return 25;
		case WP_SNIPERRIFLE: return 55;
		case WP_SNOOPERSCOPE: return 25;
		case WP_NONE: return 0;
		case WP_KNIFE: return 5;
		case WP_GRENADE_LAUNCHER: return 100;
		case WP_GRENADE_PINEAPPLE: return 80;
		case WP_DYNAMITE: return 400;
		case WP_PANZERFAUST: return 200;        // (SA) was 100
		case WP_MORTAR: return 100;
		case WP_FLAMETHROWER:     // FIXME -- not used in single player yet
		case WP_TESLA:
		case WP_GAUNTLET:
		case WP_SNIPER:
		default:    return 1;
		}
	} else { // multiplayer damage
		switch ( weapon ) {
		case WP_LUGER:
		case WP_SILENCER: return 14;
		case WP_COLT: return 18;
		case WP_AKIMBO: return 18;      //----(SA)	added
		case WP_VENOM: return 20;
		case WP_MP40: return 14;
		case WP_THOMPSON: return 18;
		case WP_STEN: return 14;
		case WP_FG42SCOPE:
		case WP_FG42: return 15;
		case WP_MAUSER: return 25;
		case WP_GARAND: return 25;
		case WP_SNIPERRIFLE: return 80;
		case WP_SNOOPERSCOPE: return 75;
		case WP_NONE: return 0;
		case WP_KNIFE: return 10;
		case WP_GRENADE_SMOKE: return 100;
		case WP_GRENADE_LAUNCHER: return 200;
		case WP_GRENADE_PINEAPPLE: return 200;
		case WP_DYNAMITE: return 600;
		case WP_PANZERFAUST: return 400;
		case WP_MORTAR: return 100;
		case WP_FLAMETHROWER: return 1;
		case WP_TESLA:
		case WP_GAUNTLET:
		case WP_SNIPER:
		default:    return 1;
		}
	}
}
// JPW - this chunk appears to not be used, right?
/*
#define MACHINEGUN_SPREAD	200
#define	MACHINEGUN_DAMAGE	G_GetWeaponDamage(WP_MACHINEGUN) // JPW
#define	MACHINEGUN_TEAM_DAMAGE	G_GetWeaponDamage(WP_MACHINEGUN) // JPW		// wimpier MG in teamplay
*/
// jpw

// RF, wrote this so we can dynamically switch between old and new values while testing g_userAim
float G_GetWeaponSpread( int weapon ) {
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {   // JPW NERVE -- don't affect SP game
		if ( g_userAim.integer ) {
			// these should be higher since they become erratic if aiming is out
			switch ( weapon ) {
			case WP_LUGER:      return 600;
			case WP_SILENCER:   return 900;
			case WP_COLT:       return 700;
			case WP_AKIMBO:     return 700; //----(SA)	added
			case WP_VENOM:      return 1000;
			case WP_MP40:       return 1000;
			case WP_FG42SCOPE:  return 300;
			case WP_FG42:       return 800;
			case WP_THOMPSON:   return 1200;
			case WP_STEN:       return 1200;
			case WP_MAUSER:     return 400;
			case WP_GARAND:     return 500;
			case WP_SNIPERRIFLE:    return 300;
			case WP_SNOOPERSCOPE:   return 300;
			}
		} else {    // old values
			switch ( weapon ) {
			case WP_LUGER:      return 25;
			case WP_SILENCER:   return 150;
			case WP_COLT:       return 30;
			case WP_AKIMBO:     return 30;      //----(SA)	added
			case WP_VENOM:      return 200;
			case WP_MP40:       return 200;
			case WP_FG42SCOPE:  return 10;
			case WP_FG42:       return 150;
			case WP_THOMPSON:   return 250;
			case WP_STEN:       return 300;
			case WP_MAUSER:     return 15;
			case WP_GARAND:     return 25;
			case WP_SNIPERRIFLE:    return 10;
			case WP_SNOOPERSCOPE:   return 10;
			}
		}
	} else { // JPW NERVE but in multiplayer...  new spreads and don't look at g_userAim
		switch ( weapon ) {
		case WP_LUGER: return 600;
		case WP_SILENCER: return 900;
		case WP_COLT: return 800;
		case WP_AKIMBO: return 800;         //----(SA)added
		case WP_VENOM: return 600;
		case WP_MP40: return 400;
		case WP_FG42SCOPE:
		case WP_FG42:   return 500;
		case WP_THOMPSON: return 600;
		case WP_STEN: return 200;
		case WP_MAUSER: return 700;
		case WP_GARAND: return 600;
		case WP_SNIPERRIFLE: return 700;         // was 300
		case WP_SNOOPERSCOPE: return 700;
		}
	}
	G_Printf( "shouldn't ever get here (weapon %d)\n",weapon );
	// jpw
	return 0;   // shouldn't get here
}

#define LUGER_SPREAD    G_GetWeaponSpread( WP_LUGER )
#define LUGER_DAMAGE    G_GetWeaponDamage( WP_LUGER ) // JPW
#define SILENCER_SPREAD G_GetWeaponSpread( WP_SILENCER )
#define COLT_SPREAD     G_GetWeaponSpread( WP_COLT )
#define COLT_DAMAGE     G_GetWeaponDamage( WP_COLT ) // JPW

#define VENOM_SPREAD    G_GetWeaponSpread( WP_VENOM )
#define VENOM_DAMAGE    G_GetWeaponDamage( WP_VENOM ) // JPW

#define MP40_SPREAD     G_GetWeaponSpread( WP_MP40 )
#define MP40_DAMAGE     G_GetWeaponDamage( WP_MP40 ) // JPW
#define THOMPSON_SPREAD G_GetWeaponSpread( WP_THOMPSON )
#define THOMPSON_DAMAGE G_GetWeaponDamage( WP_THOMPSON ) // JPW
#define STEN_SPREAD     G_GetWeaponSpread( WP_STEN )
#define STEN_DAMAGE     G_GetWeaponDamage( WP_STEN ) // JPW
#define FG42_SPREAD     G_GetWeaponSpread( WP_FG42 )
#define FG42_DAMAGE     G_GetWeaponDamage( WP_FG42 ) // JPW

#define MAUSER_SPREAD   G_GetWeaponSpread( WP_MAUSER )
#define MAUSER_DAMAGE   G_GetWeaponDamage( WP_MAUSER ) // JPW
#define GARAND_SPREAD   G_GetWeaponSpread( WP_GARAND )
#define GARAND_DAMAGE   G_GetWeaponDamage( WP_GARAND ) // JPW

#define SNIPER_SPREAD   G_GetWeaponSpread( WP_SNIPERRIFLE )
#define SNIPER_DAMAGE   G_GetWeaponDamage( WP_SNIPERRIFLE ) // JPW

#define SNOOPER_SPREAD  G_GetWeaponSpread( WP_SNOOPERSCOPE )
#define SNOOPER_DAMAGE  G_GetWeaponDamage( WP_SNOOPERSCOPE ) // JPW

/*
==============
EmitterCheck
	see if a new particle emitter should be created at the bullet impact point
==============
*/
void EmitterCheck( gentity_t *ent, gentity_t *attacker, trace_t *tr ) {
	gentity_t *tent;
	vec3_t origin;

	if ( !ent->emitNum ) { // no emitters left for this entity.
		return;
	}

	VectorCopy( tr->endpos, origin );
	SnapVectorTowards( tr->endpos, attacker->s.origin ); // make sure it's out of the wall


	// why were these stricmp's?...
	if ( ent->s.eType == ET_EXPLOSIVE ) {
	} else if ( ent->s.eType == ET_LEAKY ) {

		tent = G_TempEntity( origin, EV_EMITTER );
		VectorCopy( origin, tent->s.origin );
		tent->s.time = ent->emitTime;
		tent->s.density = ent->emitPressure;    // 'pressure'
		tent->s.teamNum = ent->emitID;          // 'type'
		VectorCopy( tr->plane.normal, tent->s.origin2 );
	}

	ent->emitNum--;
}



/*
==============
Bullet_Endpos
	find target end position for bullet trace based on entities weapon and accuracy
==============
*/
void Bullet_Endpos( gentity_t *ent, float spread, vec3_t *end ) {
	float r, u;
	qboolean randSpread = qtrue;
	int dist = 8192;

	r = crandom() * spread;
	u = crandom() * spread;
    if(ent->client && !(ent->r.svFlags&SVF_CASTAI) && CW_Active(&ent->client->ps)) {
        int saved=ent->client->ps.stats[STAT_COD1_SHOT_CONE];
        float cone=saved>0?(saved-1)*.001f:CW_SpreadDegrees(&ent->client->ps);
        float radius=(float)tan(cone*M_PI/180.0)*dist;
        float angle=random()*2*M_PI, distance=random()*radius;
        r=(float)cos(angle)*distance;u=(float)sin(angle)*distance;
    }

	// Ridah, if this is an AI shooting, apply their accuracy
	if ( ent->r.svFlags & SVF_CASTAI ) {
		float accuracy;
		accuracy = ( 1.0 - AICast_GetAccuracy( ent->s.number ) ) * AICAST_AIM_SPREAD;
		r += crandom() * accuracy;
		u += crandom() * ( accuracy * 1.25 );
	} else {
		if ( ent->s.weapon == WP_SNOOPERSCOPE || ent->s.weapon == WP_SNIPERRIFLE || ent->s.weapon == WP_FG42SCOPE ) {
//		if(ent->s.weapon == WP_SNOOPERSCOPE || ent->s.weapon == WP_SNIPERRIFLE) {
			// aim dir already accounted for sway of scoped weapons in CalcMuzzlePoints()
			dist *= 2;
			randSpread = qfalse;
		}
	}

	VectorMA( muzzleTrace, dist, forward, *end );

	if ( randSpread ) {
		VectorMA( *end, r, right, *end );
		VectorMA( *end, u, up, *end );
	}
}

/*
==============
Bullet_Fire
==============
*/
void Bullet_Fire( gentity_t *ent, float spread, int damage ) {
	vec3_t end;

	Bullet_Endpos( ent, spread, &end );
	/* PATCH62: base damage is metadata; armor, hit locations and falloff still
     * use the existing RTCW damage backend. */
    if(ent->client && CW_Active(&ent->client->ps))damage=(int)CW_Number(CW_PSDef(&ent->client->ps),"damage");
    Bullet_Fire_Extended( ent, ent, muzzleTrace, end, spread, damage, 0 );
}


/*
==============
Bullet_Fire_Extended
	A modified Bullet_Fire with more parameters.
	The original Bullet_Fire still passes through here and functions as it always has.

	uses for this include shooting through entities (windows, doors, other players, etc.) and reflecting bullets
==============
*/
void Bullet_Fire_Extended( gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, float spread, int damage, int recursion ) {
	trace_t tr;
    vec3_t cod1Contact;
	gentity_t   *tent;
	gentity_t   *traceEnt;
	int dflags = 0;         // flag if source==attacker, meaning it wasn't shot directly, but was reflected went through an entity that allows bullets to pass through
	qboolean reflectBullet = qfalse;

	// RF, abort if too many recursions.. there must be a real solution for this, but for now this is the safest
	// fix I can find
	if ( recursion > 12 ) {
		return;
	}

	damage *= s_quadFactor;

	if ( source != attacker ) {
		dflags = DAMAGE_PASSTHRU;
	}

	// (SA) changed so player could shoot his own dynamite.
	// (SA) whoops, but that broke bullets going through explosives...
	trap_Trace( &tr, start, NULL, NULL, end, source->s.number, MASK_SHOT );
    VectorCopy(tr.endpos,cod1Contact); /* before legacy SnapVectorTowards */
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(!recursion&&attacker&&attacker->client&&CW_Active(&attacker->client->ps)&&G_Cod1GscTestCarbineActive()) {
        GscEntityRef owner,sourceOwner;int serverTime=level.time;
        if(!G_Cod1EntityCapture(attacker,&owner)||!G_Cod1EntityCapture(source,&sourceOwner))return;
        G_Cod1BulletDamageTriggers(start,tr.endpos,attacker,damage);
        if(!G_Cod1EntitySameAllocation(&owner)||!G_Cod1EntitySameAllocation(&sourceOwner)||
           level.time!=serverTime||!G_Cod1GscTestCarbineActive())return;
    }
#endif
//	trap_Trace (&tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SHOT);

	// DHM - Nerve :: only in single player
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		AICast_ProcessBullet( attacker, start, tr.endpos );
	}

	// bullet debugging using Q3A's railtrail
	if ( g_debugBullets.integer & 1 ) {
		tent = G_TempEntity( start, EV_RAILTRAIL );
		VectorCopy( tr.endpos, tent->s.origin2 );
		tent->s.otherEntityNum2 = attacker->s.number;
	}



	traceEnt = &g_entities[ tr.entityNum ];

	EmitterCheck( traceEnt, attacker, &tr );

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, start );

	// should we reflect this bullet?
	if ( traceEnt->flags & FL_DEFENSE_GUARD ) {
		reflectBullet = qtrue;
	} else if ( traceEnt->flags & FL_DEFENSE_CROUCH ) {
		if ( rand() % 3 < 2 ) {
			reflectBullet = qtrue;
		}
	}

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client && !reflectBullet ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		if ( LogAccuracyHit( traceEnt, attacker ) ) {
			attacker->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}

//----(SA)	added
		if ( g_debugBullets.integer >= 2 ) {   // show hit player bb
			gentity_t *bboxEnt;
			vec3_t b1, b2;
			VectorCopy( traceEnt->r.currentOrigin, b1 );
			VectorCopy( traceEnt->r.currentOrigin, b2 );
			VectorAdd( b1, traceEnt->r.mins, b1 );
			VectorAdd( b2, traceEnt->r.maxs, b2 );
			bboxEnt = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, bboxEnt->s.origin2 );
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}
//----(SA)	end

	} else if ( traceEnt->takedamage && traceEnt->s.eType == ET_BAT ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( g_debugBullets.integer <= -2 ) {  // show hit thing bb
			gentity_t *bboxEnt;
			vec3_t b1, b2;
			VectorCopy( traceEnt->r.currentOrigin, b1 );
			VectorCopy( traceEnt->r.currentOrigin, b2 );
			VectorAdd( b1, traceEnt->r.mins, b1 );
			VectorAdd( b2, traceEnt->r.maxs, b2 );
			bboxEnt = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, bboxEnt->s.origin2 );
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}

		if ( reflectBullet ) {
			// reflect off sheild
			VectorSubtract( tr.endpos, traceEnt->r.currentOrigin, reflect );
			VectorNormalize( reflect );
			VectorMA( traceEnt->r.currentOrigin, 15, reflect, reflect );
			tent = G_TempEntity( reflect, EV_BULLET_HIT_WALL );
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
		}

		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );

		if ( reflectBullet ) {
			tent->s.otherEntityNum2 = traceEnt->s.number;   // force sparks
		} else {
			tent->s.otherEntityNum2 = ENTITYNUM_NONE;
		}
        /* CoD bullet reactions also apply to inline brush models (doors,
         * script_brushmodel, etc.). R9 restores their material-bearing point
         * traces. Keep actors/shield reflections on the legacy path. Marks on
         * inline models are deferred until mover-relative decals exist: native
         * FX and sound still use the exact world-space contact and normal. */
        if(!reflectBullet && attacker->client && CW_Active(&attacker->client->ps) &&
            (tr.entityNum==ENTITYNUM_WORLD || tr.entityNum==ENTITYNUM_NONE || traceEnt->r.bmodel)) {
            const cwDef_t *d=CW_PSDef(&attacker->client->ps);
            if(d && !strcmp(CW_String(d,"weaponType"),"bullet")) {
                CI_WriteEvent(&tent->s,&tr,cod1Contact,start,end,
                    attacker->client->ps.weapon,CW_Number(d,"rifleBullet")!=0);
                if(traceEnt->r.bmodel && tr.entityNum!=ENTITYNUM_WORLD && tr.entityNum!=ENTITYNUM_NONE) {
                    tent->s.time2|=CI_NOMARKS;
                    tent->s.otherEntityNum2=tr.entityNum;
                }
            }
        }
		// done.
	}
	tent->s.otherEntityNum = attacker->s.number;

	if ( traceEnt->takedamage ) {
		qboolean reflectBool = qfalse;
		vec3_t trDir;

		if ( reflectBullet ) {
			// if we are facing the direction the bullet came from, then reflect it
			AngleVectors( traceEnt->s.apos.trBase, trDir, NULL, NULL );
			if ( DotProduct( forward, trDir ) < 0.6 ) {
				reflectBool = qtrue;
			}
		}

		if ( reflectBool ) {
			vec3_t reflect_end;
			// reflect this bullet
			G_AddEvent( traceEnt, EV_GENERAL_SOUND, level.bulletRicochetSound );
			CalcMuzzlePoints( traceEnt, traceEnt->s.weapon );

//----(SA)	modified to use extended version so attacker would pass through
//			Bullet_Fire( traceEnt, 1000, damage );
			Bullet_Endpos( traceEnt, 2800, &reflect_end );    // make it inaccurate
			Bullet_Fire_Extended( traceEnt, attacker, muzzleTrace, reflect_end, spread, damage, recursion + 1 );
//----(SA)	end

		} else {
			int oldHealth;

			// Ridah, don't hurt team-mates
			// DHM - Nerve :: Only in single player
			if ( attacker->client && traceEnt->client && g_gametype.integer == GT_SINGLE_PLAYER && ( traceEnt->r.svFlags & SVF_CASTAI ) && ( attacker->r.svFlags & SVF_CASTAI ) && AICast_SameTeam( AICast_GetCastState( attacker->s.number ), traceEnt->s.number ) ) {
				// AI's don't hurt members of their own team
				return;
			}
			// done.

			oldHealth = traceEnt->health;
			G_Damage( traceEnt, attacker, attacker, forward, tr.endpos, damage, dflags, ammoTable[attacker->s.weapon].mod );

			// allow bullets to "pass through" func_explosives if they break by taking another simultanious shot
			// start new bullet at position this hit and continue to the end position (ignoring shot-through ent in next trace)
			// spread = 0 as this is an extension of an already spread shot (so just go straight through)
			if ( Q_stricmp( traceEnt->classname, "func_explosive" ) == 0 ) {
				if ( traceEnt->health <= 0 ) {
					Bullet_Fire_Extended( traceEnt, attacker, tr.endpos, end, 0, damage, recursion + 1 );
				}
			} else if ( traceEnt->client ) {
				if ( traceEnt->health <= 0 ) {
					Bullet_Fire_Extended( traceEnt, attacker, tr.endpos, end, 0, damage / 2, recursion + 1 ); // halve the damage each player it goes through
				}
			}
		}
	}
}



/*
======================================================================

GRENADE LAUNCHER

  700 has been the standard direction multiplier in fire_grenade()

======================================================================
*/
extern void G_ExplodeMissilePoisonGas( gentity_t *ent );

gentity_t *weapon_grenadelauncher_fire( gentity_t *ent, int grenType ) {
	gentity_t   *m, *te; // JPW NERVE
	float upangle = 0;                  //	start with level throwing and adjust based on angle
	vec3_t tosspos;
	qboolean underhand = 0;

	if ( ( ent->s.apos.trBase[0] > 0 ) && ( grenType != WP_GRENADE_SMOKE ) ) { // JPW NERVE -- smoke grenades always overhand
		underhand = qtrue;
	}

	if ( underhand ) {
		forward[2] = 0;                 //	start the toss level for underhand
	} else {
		forward[2] += 0.2;              //	extra vertical velocity for overhand

	}
	VectorNormalize( forward );         //	make sure forward is normalized

	upangle = -( ent->s.apos.trBase[0] ); //	this will give between	-90 / 90
	upangle = min( upangle, 50 );
	upangle = max( upangle, -50 );        //	now clamped to			-50 / 50	(don't allow firing straight up/down)
	upangle = upangle / 100.0f;           //						   -0.5 / 0.5
	upangle += 0.5f;                    //						    0.0 / 1.0

	if ( upangle < .1 ) {
		upangle = .1;
	}

	// pineapples are not thrown as far as mashers
	if ( grenType == WP_GRENADE_LAUNCHER ) {
		upangle *= 800;     //									    0.0 / 800.0
	} else if ( grenType == WP_GRENADE_PINEAPPLE )                                {
// JPW NERVE
		if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
			upangle *= 800;
		} else {
// jpw
			upangle *= 600;     //									    0.0 / 600.0
		}
	}
// JPW NERVE
	else if ( grenType == WP_GRENADE_SMOKE ) { // smoke grenades *really* get chucked
		upangle *= 800;
	}
// jpw
	else {      // WP_DYNAMITE
		upangle *= 400;     //										0.0 / 100.0

	}
	/*
	if(ent->aiCharacter)
	{
		VectorScale(forward, 700, forward);				//----(SA)	700 is the default grenade throw they are already used to
		m = fire_grenade (ent, muzzleTrace, forward);	//----(SA)	temp to make AI's throw grenades at their actual target
	}
	else
	*/



	{
		VectorCopy( muzzleEffect, tosspos );
		if ( underhand ) {
			VectorMA( muzzleEffect, 15, forward, tosspos );   // move a little bit more away from the player (so underhand tosses don't get caught on nearby lips)
			tosspos[2] -= 24;   // lower origin for the underhand throw
			upangle *= 1.3;     // a little more force to counter the lower position / lack of additional lift
		}

		VectorScale( forward, upangle, forward );


		{
			// check for valid start spot (so you don't throw through or get stuck in a wall)
			trace_t tr;
			vec3_t viewpos;

			VectorCopy( ent->s.pos.trBase, viewpos );
			viewpos[2] += ent->client->ps.viewheight;

			trap_Trace( &tr, viewpos, NULL, NULL, tosspos, ent->s.number, MASK_SHOT );
			if ( tr.fraction < 1 ) {   // oops, bad launch spot
				VectorCopy( tr.endpos, tosspos );
			}
		}


		m = fire_grenade( ent, tosspos, forward, grenType );
	}


	//m->damage *= s_quadFactor;
	m->damage = 0;  // Ridah, grenade's don't explode on contact
	m->splashDamage *= s_quadFactor;

	if ( ent->aiCharacter == AICHAR_VENOM ) { // poison gas grenade
		m->think = G_ExplodeMissilePoisonGas;
		m->s.density = 1;
	}

// JPW NERVE
	if ( grenType == WP_GRENADE_SMOKE ) {
		if ( ent->client->sess.sessionTeam == TEAM_RED ) { // store team so we can generate red or blue smoke
			m->s.otherEntityNum2 = 1;
		} else {
			m->s.otherEntityNum2 = 0;
		}
		m->nextthink = level.time + 4000;
		m->think = weapon_callAirStrike;

		te = G_TempEntity( m->s.pos.trBase, EV_GLOBAL_SOUND );
		te->s.eventParm = G_SoundIndex( "sound/scenaric/forest/me109_flight.wav" );
//		te->r.svFlags |= SVF_BROADCAST | SVF_USE_CURRENT_ORIGIN;
	}
// jpw

	//----(SA)	adjust for movement of character.  TODO: Probably comment in later, but only for forward/back not strafing
//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics

	// let the AI know which grenade it has fired
	ent->grenadeFired = m->s.number;

	// Ridah, return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return m;
}

/*
=====================
Zombie spit
=====================
*/
void weapon_zombiespit( gentity_t *ent ) {
	gentity_t *m;

//RF, HARD disable
	return;

	m = fire_zombiespit( ent, muzzleTrace, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	if ( m ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "sound/Loogie/spit.wav" ) );
	}


}

/*
=====================
Zombie spirit
=====================
*/
void weapon_zombiespirit( gentity_t *ent, gentity_t *missile ) {
	gentity_t *m;

	m = fire_zombiespirit( ent, missile, muzzleTrace, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	if ( m ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "zombieAttackPlayer" ) );
	}

}

//----(SA)	modified this entire "venom" section
/*
============================================================================

VENOM GUN TRACING

============================================================================
*/
#define DEFAULT_VENOM_COUNT 10
#define DEFAULT_VENOM_SPREAD 20
#define DEFAULT_VENOM_DAMAGE 15

qboolean VenomPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t tr;
	int damage;
	gentity_t       *traceEnt;

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	// send bullet impact
	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( traceEnt->takedamage ) {
		damage = DEFAULT_VENOM_DAMAGE * s_quadFactor;

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_VENOM );
		if ( LogAccuracyHit( traceEnt, ent ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

// this should match CG_VenomPattern
void VenomPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;
	int oldScore;
	qboolean hitClient = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	oldScore = ent->client->ps.persistant[PERS_SCORE];

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_VENOM_COUNT ; i++ ) {
		r = Q_crandom( &seed ) * DEFAULT_VENOM_SPREAD;
		u = Q_crandom( &seed ) * DEFAULT_VENOM_SPREAD;
		VectorMA( origin, 8192, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );
		if ( VenomPellet( origin, end, ent ) && !hitClient ) {
			hitClient = qtrue;
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
}



/*
==============
weapon_venom_fire
==============
*/
void weapon_venom_fire( gentity_t *ent, qboolean fullmode, float aimSpreadScale ) {
	gentity_t       *tent;

	if ( fullmode ) {
		tent = G_TempEntity( muzzleTrace, EV_VENOMFULL );
	} else {
		tent = G_TempEntity( muzzleTrace, EV_VENOM );
	}

	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() & 255;       // seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	if ( fullmode ) {
		VenomPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
	} else
	{
		int dam;
		dam = VENOM_DAMAGE;
		if ( ent->aiCharacter ) {  // venom guys are /vicious/
			dam *= 0.5f;
		}
		Bullet_Fire( ent, VENOM_SPREAD * aimSpreadScale, dam );
	}
}





/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire( gentity_t *ent, float aimSpreadScale ) {
//	trace_t		tr;
	float r, u;
	vec3_t dir, launchpos;     //, viewpos, wallDir;
	gentity_t   *m;

	// get a little bit of randomness and apply it back to the direction
	if ( !ent->aiCharacter ) {
		r = crandom() * aimSpreadScale;
		u = crandom() * aimSpreadScale;

		VectorScale( forward, 16, dir );
		VectorMA( dir, r, right, dir );
		VectorMA( dir, u, up, dir );
		VectorNormalize( dir );

		VectorCopy( muzzleEffect, launchpos );

		// check for valid start spot (so you don't lose it in a wall)
		// (doesn't ever happen)
//		VectorCopy( ent->s.pos.trBase, viewpos );
//		viewpos[2] += ent->client->ps.viewheight;
//		trap_Trace (&tr, viewpos, NULL, NULL, muzzleEffect, ent->s.number, MASK_SHOT);
//		if(tr.fraction < 1) {	// oops, bad launch spot
///			VectorCopy(tr.endpos, launchpos);
//			VectorSubtract(tr.endpos, viewpos, wallDir);
//			VectorNormalize(wallDir);
//			VectorMA(tr.endpos, -5, wallDir, launchpos);
//		}

		m = fire_rocket( ent, launchpos, dir );

		// add kick-back
		VectorMA( ent->client->ps.velocity, -64, forward, ent->client->ps.velocity );

	} else {
		m = fire_rocket( ent, muzzleEffect, forward );
	}

	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}




/*
==============
AddLean
	add leaning offset
==============
*/
void AddLean( gentity_t *ent, vec3_t point ) {
	if ( ent->client ) {
        if(BG_Cod1LeanActive(&ent->client->ps)) {
            COD1_LeanOffset(point,ent->client->ps.viewangles[YAW],ent->client->ps.leanf,16,20);
        } else if ( ent->client->ps.leanf ) {
			vec3_t right;
			AngleVectors( ent->client->ps.viewangles, NULL, right, NULL );
			VectorMA( point, ent->client->ps.leanf, right, point );
		}
	}
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if ( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if ( !target->client ) {
		return qfalse;
	}

	if ( !attacker->client ) {
		return qfalse;
	}

	if ( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint( gentity_t *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->r.currentOrigin, muzzlePoint );
	muzzlePoint[2] += BG_Cod1LeanActive(&ent->client->ps)?BG_Cod1LeanEyeHeight(&ent->client->ps):ent->client->ps.viewheight;
	// Ridah, this puts the start point outside the bounding box, isn't necessary
//	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// done.

	// Ridah, offset for more realistic firing from actual gun position
	//----(SA) modified
	switch ( weapon )  // Ridah, changed this so I can predict weapons
	{
	case WP_PANZERFAUST:
//			VectorMA( muzzlePoint, 14, right, muzzlePoint );	//----(SA)	new first person rl position
		VectorMA( muzzlePoint, 10, right, muzzlePoint );        //----(SA)	new first person rl position
		VectorMA( muzzlePoint, -10, up, muzzlePoint );
		break;
//		case WP_ROCKET_LAUNCHER:
//			VectorMA( muzzlePoint, 14, right, muzzlePoint );	//----(SA)	new first person rl position
//			break;
	case WP_DYNAMITE:
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		VectorMA( muzzlePoint, 20, right, muzzlePoint );
		break;
	case WP_AKIMBO:     // left side rather than right
		VectorMA( muzzlePoint, -6, right, muzzlePoint );
		VectorMA( muzzlePoint, -4, up, muzzlePoint );
	default:
		VectorMA( muzzlePoint, 6, right, muzzlePoint );
		VectorMA( muzzlePoint, -4, up, muzzlePoint );
		break;
	}

	// done.

	// (SA) actually, this is sort of moot right now since
	// you're not allowed to fire when leaning.  Leave in
	// in case we decide to enable some lean-firing.
	AddLean( ent, muzzlePoint );

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

// Rafael - for activate
void CalcMuzzlePointForActivate( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {

	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += BG_Cod1LeanActive(&ent->client->ps)?BG_Cod1LeanEyeHeight(&ent->client->ps):ent->client->ps.viewheight;

	AddLean( ent, muzzlePoint );

	// snap to integer coordinates for more efficient network bandwidth usage
//	SnapVector( muzzlePoint );
	// (SA) /\ only used server-side, so leaving the accuracy in is fine (and means things that show a cursorhint will be hit when activated)
	//			(there were differing views of activatable stuff between cursorhint and activatable)
}
// done.

// Ridah
void CalcMuzzlePoints( gentity_t *ent, int weapon ) {
	vec3_t viewang;

	VectorCopy( ent->client->ps.viewangles, viewang );
    /* PATCH56: native game 0x20049646 uses command weapon pitch/yaw,
     * while roll still comes from playerState. Never alter movement angles. */
    if(ent->client->pers.cmd.cod1AimValid && CW_WeaponDef(weapon) &&
       !ent->client->ps.aiChar && !(ent->r.svFlags&SVF_CASTAI) &&
       (ent->client->pers.cmd.wbuttons&WBUTTON_COD1_STANCE)) {
        float p=ent->client->pers.cmd.cod1WeaponAngles[0];
        float y=ent->client->pers.cmd.cod1WeaponAngles[1];
        if(p>=-360 && p<=360 && y>=-360 && y<=360) {viewang[PITCH]=p;viewang[YAW]=y;}
    }

	if ( !( ent->r.svFlags & SVF_CASTAI ) ) {   // non ai's take into account scoped weapon 'sway' (just another way aimspread is visualized/utilized)
		float spreadfrac, phase;

		if ( weapon == WP_SNIPERRIFLE || weapon == WP_SNOOPERSCOPE || weapon == WP_FG42SCOPE ) {
			spreadfrac = ent->client->currentAimSpreadScale;

			// rotate 'forward' vector by the sway
			phase = level.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
			viewang[PITCH] += ZOOM_PITCH_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE );

			phase = level.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
			viewang[YAW] += ZOOM_YAW_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_YAW_MIN_AMPLITUDE );
		}
	}


	// set aiming directions
	AngleVectors( viewang, forward, right, up );

//----(SA)	modified the muzzle stuff so that weapons that need to fire down a perfect trace
//			straight out of the camera (SP5, Mauser right now) can have that accuracy, but
//			weapons that need an offset effect (bazooka/grenade/etc.) can still look like
//			they came out of the weap.
	CalcMuzzlePointForActivate( ent, forward, right, up, muzzleTrace );
	CalcMuzzlePoint( ent, weapon, forward, right, up, muzzleEffect );
}

/*
====================
G_Cod1FireWeapon

Player-only CoD dispatch.  CW_Active() deliberately excludes AI, so the
legacy switch below remains available to RTCW actors and future NPC work.
For CoD hitscan weapons Bullet_Endpos() consumes STAT_COD1_SHOT_CONE and
Bullet_Fire() consumes WeaponDef damage, making the old per-weapon RTCW
spread/damage cases redundant for the player path.
====================
*/
static qboolean G_Cod1FireWeapon( gentity_t *ent, int shotWeapon ) {
    const cwDef_t *d;
    const char *type;

    if ( !ent->client || ent->client->ps.aiChar || !CW_WeaponDef(shotWeapon) ) {
        return qfalse;
    }

    d = shotWeapon==ent->client->ps.weapon ? CW_PSDef( &ent->client->ps ) : CW_WeaponDef(shotWeapon);
    if ( !d ) {
        return qfalse;
    }
    type = CW_String( d, "weaponType" );

    if ( !strcmp( type, "grenade" ) || !strcmp( type, "projectile" ) ) {
        gentity_t *m;
        vec3_t velocity, start, end;
        trace_t launchTrace;
        int grenade = !strcmp( type, "grenade" );

        VectorCopy( muzzleTrace, start );
        if ( !grenade ) {
            VectorMA( start, 16, forward, end );
            trap_Trace( &launchTrace, start, NULL, NULL, end, ent->s.number, MASK_MISSILESHOT );
            VectorCopy( launchTrace.endpos, start );
        }
        VectorScale( forward, (float)CW_Number( d, "projectileSpeed" ), velocity );
        if ( grenade ) {
            vec3_t throwDirection;
            float inherited;
            velocity[2] += (float)CW_Number( d, "projectileSpeedUp" );
            VectorCopy( velocity, throwDirection );
            VectorNormalize( throwDirection );
            inherited = DotProduct( ent->client->ps.velocity, throwDirection );
            VectorMA( velocity, inherited, throwDirection, velocity );
            ent->client->ps.grenadeTimeLeft = (int)CW_Number( d, "fuseTime" );
            m = fire_grenade( ent, start, velocity, WP_GRENADE_PINEAPPLE );
        } else {
            m = fire_rocket( ent, start, forward );
        }
        m->s.weapon = shotWeapon;
        m->damage = (int)CW_Number( d, "damage" );
        m->splashDamage = (int)CW_Number( d, "explosionInnerDamage" );
        m->splashRadius = (float)CW_Number( d, "explosionRadius" );
        m->cod1ExplosionOuterDamage = (int)CW_Number( d, "explosionOuterDamage" );
        m->cod1ExplosionType = !Q_stricmp( CW_String( d, "projExplosionType" ), "rocket" ) ? 1 :
            !Q_stricmp( CW_String( d, "projExplosionType" ), "molotov" ) ? 2 : 0;
        m->cod1ImpactExplode = (qboolean)( CW_Number( d, "projImpactExplode" ) != 0 );
        VectorCopy( velocity, m->s.pos.trDelta );
        if ( grenade ) {
            m->s.pos.trTime = level.time;
        }
        trap_LinkEntity( m );
        if ( !grenade ) {
            AICast_RecordWeaponFire( ent );
        }
        return qtrue;
    }

    if ( !strcmp( d->name, "stalingrad_ammo" ) ) {
        return qtrue;
    }

    if ( !strcmp( type, "bullet" ) ) {
        /* The spread argument is intentionally neutral: Bullet_Endpos()
         * replaces it with the native CoD cone whenever CW_Active() is true. */
        Bullet_Fire( ent, 0.0f, (int)CW_Number( d, "damage" ) );
        if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
            AICast_RecordWeaponFire( ent );
        }
        return qtrue;
    }

    return qfalse;
}

/* R217: an already emitted handheld shot remains a shot from that weapon
 * after disposal, a queued switch, or several PmoveSingle steps in one cmd. */
qboolean G_Cod1FireWeaponEvent(gentity_t *ent,int event,int parameter) {
    int shotWeapon;
    if(!ent || !ent->client || (ent->r.svFlags&SVF_CASTAI))return qfalse;
    shotWeapon=CW_ProjectileFireEventWeapon(event,parameter,ent->client->ps.aiChar);
    if(shotWeapon==WP_NONE)return qfalse;
    if(ent->client->ps.persistant[PERS_HWEAPON_USE] && ent->active)return qtrue;
    s_quadFactor=ent->client->ps.powerups[PW_QUAD]?g_quadfactor.value:1;
    CalcMuzzlePoints(ent,shotWeapon);
    return G_Cod1FireWeapon(ent,shotWeapon);
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	float aimSpreadScale;
	vec3_t viewang;  // JPW NERVE

	// Rafael mg42
	//if (ent->active)
	//	return;
	if ( ent->client->ps.persistant[PERS_HWEAPON_USE] && ent->active ) {
		return;
	}

	if ( ent->client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
//----(SA)	removing old weapon references
//	if( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET ) {
//		ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;
//	}

	// Ridah, need to call this for AI prediction also
	CalcMuzzlePoints( ent, ent->s.weapon );

	/* CoD player weapons no longer enter the RTCW weapon switch below. */
	if ( G_Cod1FireWeapon( ent, ent->client->ps.weapon ) ) {
		return;
	}

	if ( g_userAim.integer ) {
		aimSpreadScale = ent->client->currentAimSpreadScale;
		// Ridah, add accuracy factor for AI
		if ( ent->aiCharacter ) {
			float aim_accuracy;
			aim_accuracy = AICast_GetAccuracy( ent->s.number );
			if ( aim_accuracy <= 0 ) {
				aim_accuracy = 0.0001;
			}
			aimSpreadScale = ( 1.0 - aim_accuracy ) * 2.0;
		} else {
			//	/maximum/ accuracy for player for a given weapon
			switch ( ent->s.weapon ) {
			case WP_LUGER:
			case WP_SILENCER:
			case WP_COLT:
			case WP_AKIMBO:
				aimSpreadScale += 0.4f;
				break;

			case WP_PANZERFAUST:
				aimSpreadScale += 0.3f;     // it's calculated a different way, so this keeps the accuracy never perfect, but never rediculously wild either
				break;

			default:
				aimSpreadScale += 0.15f;
				break;
			}

			if ( aimSpreadScale > 1 ) {
				aimSpreadScale = 1.0f;  // still cap at 1.0
			}
		}
	} else {
		aimSpreadScale = 1.0;
	}

	// fire the specific legacy/AI weapon
	switch ( ent->s.weapon ) {
	case WP_KNIFE:
		Weapon_Knife( ent );
		break;
// JPW NERVE
	case WP_CLASS_SPECIAL:
		Weapon_Class_Special( ent );
		break;
// jpw
		break;
	case WP_LUGER:
		Bullet_Fire( ent, LUGER_SPREAD * aimSpreadScale, LUGER_DAMAGE );
		break;
	case WP_SILENCER:
		Bullet_Fire( ent, SILENCER_SPREAD * aimSpreadScale, LUGER_DAMAGE );
		break;
	case WP_AKIMBO: //----(SA)	added
	case WP_COLT:
		Bullet_Fire( ent, COLT_SPREAD * aimSpreadScale, COLT_DAMAGE );
		break;
	case WP_VENOM:
		weapon_venom_fire( ent, qfalse, aimSpreadScale );
		break;
	case WP_SNIPERRIFLE:
		Bullet_Fire( ent, SNIPER_SPREAD * aimSpreadScale, SNIPER_DAMAGE );
// JPW NERVE -- added muzzle flip in multiplayer
		if ( !ent->aiCharacter ) {
//		if (g_gametype.integer != GT_SINGLE_PLAYER) {
			VectorCopy( ent->client->ps.viewangles,viewang );
//			viewang[PITCH] -= 6; // handled in clientthink instead
			ent->client->sniperRifleMuzzleYaw = crandom() * 0.5; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = 0.8f;
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
// jpw
		break;
	case WP_SNOOPERSCOPE:
		Bullet_Fire( ent, SNOOPER_SPREAD * aimSpreadScale, SNOOPER_DAMAGE );
// JPW NERVE -- added muzzle flip in multiplayer
		if ( !ent->aiCharacter ) {
//		if (g_gametype.integer != GT_SINGLE_PLAYER) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * 0.5; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = 0.9f;
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
// jpw
		break;
	case WP_MAUSER:
		Bullet_Fire( ent, MAUSER_SPREAD * aimSpreadScale, MAUSER_DAMAGE );
		break;
	case WP_GARAND:
		Bullet_Fire( ent, GARAND_SPREAD * aimSpreadScale, GARAND_DAMAGE );
		break;
//----(SA)	added
	case WP_FG42SCOPE:
		if ( !ent->aiCharacter ) {
//		if (g_gametype.integer != GT_SINGLE_PLAYER) {
			VectorCopy( ent->client->ps.viewangles,viewang );
//			ent->client->sniperRifleMuzzleYaw = crandom()*0.04; // used in clientthink
			ent->client->sniperRifleMuzzleYaw = 0;
			ent->client->sniperRifleMuzzlePitch = 0.07f;
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
	case WP_FG42:
		Bullet_Fire( ent, FG42_SPREAD * aimSpreadScale, FG42_DAMAGE );
		break;
//----(SA)	end
	case WP_STEN:
		Bullet_Fire( ent, STEN_SPREAD * aimSpreadScale, STEN_DAMAGE );
		break;
	case WP_MP40:
		Bullet_Fire( ent, MP40_SPREAD * aimSpreadScale, MP40_DAMAGE );
		break;
    case WP_MP44:case WP_PPSH:case WP_BAR:case WP_BREN:case WP_M1CARBINE:
    case WP_KAR98K:case WP_ENFIELD:case WP_MOSIN:case WP_KAR98KSCOPE:case WP_MOSINSCOPE:case WP_SPRINGFIELD:
        Bullet_Fire(ent,THOMPSON_SPREAD*aimSpreadScale,(int)CW_Number(CW_PSDef(&ent->client->ps),"damage"));
        break;
	case WP_THOMPSON:
		Bullet_Fire( ent, THOMPSON_SPREAD * aimSpreadScale, THOMPSON_DAMAGE );
		break;
	case WP_PANZERFAUST:
		ent->client->ps.classWeaponTime = level.time; // JPW NERVE
		Weapon_RocketLauncher_Fire( ent, aimSpreadScale );
		break;
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
		// weapon_grenadelauncher_fire( ent, ent->s.weapon );
		//RF- disabled this since it's broken (do we still want it?)
		if ( ent->s.weapon == WP_DYNAMITE ) {
			ent->client->ps.classWeaponTime = level.time; // JPW NERVE
		}
		weapon_grenadelauncher_fire( ent, ent->s.weapon );
		break;
	case WP_FLAMETHROWER:
		break;
	case WP_TESLA:
		// push the player back a bit
		if ( !ent->aiCharacter ) {
			vec3_t forward, vangle;
			VectorCopy( ent->client->ps.viewangles, vangle );
			vangle[PITCH] = 0;  // nullify pitch so you can't lightning jump
			AngleVectors( vangle, forward, NULL, NULL );
			// make it less if in the air
			if ( ent->s.groundEntityNum == ENTITYNUM_NONE ) {
				VectorMA( ent->client->ps.velocity, -32, forward, ent->client->ps.velocity );
			} else {
				VectorMA( ent->client->ps.velocity, -100, forward, ent->client->ps.velocity );
			}
		}
		break;
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;

	case WP_MONSTER_ATTACK1:
		switch ( ent->aiCharacter ) {
		case AICHAR_WARZOMBIE:
			break;
		case AICHAR_ZOMBIE:
			// temp just to show it works
			// G_Printf("ptoo\n");
			weapon_zombiespit( ent );
			break;
		default:
			//G_Printf( "FireWeapon: unknown ai weapon: %s attack1\n", ent->classname );
			// ??? bug ???
			break;
		}

	case WP_MORTAR:
		break;

	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}

	// Ridah
	// DHM - Nerve :: Only in single player
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		AICast_RecordWeaponFire( ent );
	}
}
