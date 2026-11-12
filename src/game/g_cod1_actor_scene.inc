/* R197 bounded visual scene command. No AI constructor or saved entity. */
void Cmd_Cod1ActorServer_f( gentity_t *player ) {
	char mode[MAX_TOKEN_CHARS], distanceArg[MAX_TOKEN_CHARS], *end;
	vec3_t eye, forward, to, from, down, facing, angles;
	trace_t wall, ground;
	gentity_t *actor;
	double distance = 140.0;
	float length;
	int argc = trap_Argc();
    int gesture = 0;
    char *modelAlias=COD1_ACTOR_MODEL_ALIAS;

	if ( g_gametype.integer != GT_SINGLE_PLAYER || !CheatsOk( player ) ) return;
	if ( argc < 2 || argc > 3 ) goto usage;
	trap_Argv( 1, mode, sizeof( mode ) );
	if ( !Q_stricmp( mode, "0" ) && argc == 2 ) {
		while ( ( actor = G_Find( NULL, FOFS( classname ), COD1_ACTOR_NET_CLASSNAME ) ) )
			G_FreeEntity( actor );
		trap_SendServerCommand( player - g_entities, "print \"CoD1 server actor: off\\n\"" );
		return;
	}
	if(!Q_stricmp(mode,"foley")||!Q_stricmp(mode,"gesture")){
        gesture=!Q_stricmp(mode,"gesture");
        if(!G_Cod1MapEntities())goto usage;
        modelAlias=COD1_ACTOR_FOLEY_ALIAS;
    }else if ( Q_stricmp( mode, "1" ) ) goto usage;
	if ( argc == 3 ) {
		trap_Argv( 2, distanceArg, sizeof( distanceArg ) );
		distance = strtod( distanceArg, &end );
		if ( end == distanceArg || *end || !( distance >= 80.0 && distance <= 512.0 ) )
			goto usage;
	}
	if ( level.intermissiontime || ( player->client->ps.eFlags & EF_VIEWING_CAMERA ) ) {
		trap_SendServerCommand( player - g_entities, "print \"CoD1 server actor: leave intermission/camera first\\n\"" );
		return;
	}
	AngleVectors( player->client->ps.viewangles, forward, NULL, NULL );
	forward[2] = 0;
	length = VectorLength( forward );
	if ( !( length > 0.001f && length < 1000.f ) ) return;
	VectorScale( forward, 1.f / length, forward );
	VectorCopy( player->client->ps.origin, eye );
	eye[2] += player->client->ps.viewheight;
	VectorMA( eye, (float)distance, forward, to );
	trap_Trace( &wall, eye, NULL, NULL, to, player->s.number, MASK_SOLID );
	if ( wall.startsolid || wall.allsolid || !( wall.fraction >= 1.f ) ) {
		trap_SendServerCommand( player - g_entities, "print \"CoD1 server actor: wall ahead\\n\"" );
		return;
	}
	VectorCopy( to, from );
	VectorCopy( to, down );
	from[2] = eye[2] + 96.f;
	down[2] = eye[2] - 384.f;
	trap_Trace( &ground, from, NULL, NULL, down, player->s.number, MASK_SOLID );
	if ( ground.startsolid || ground.allsolid ||
	     !( ground.fraction >= 0.f && ground.fraction < 1.f ) ||
	     !( ground.plane.normal[2] >= 0.7f ) ||
	     !( ground.endpos[2] > -100000.f && ground.endpos[2] < 100000.f ) ) {
		trap_SendServerCommand( player - g_entities, "print \"CoD1 server actor: no usable floor\\n\"" );
		return;
	}
	actor = G_Find( NULL, FOFS( classname ), COD1_ACTOR_NET_CLASSNAME );
	if ( !actor ) actor = G_Spawn();
	actor->classname = COD1_ACTOR_NET_CLASSNAME;
	actor->s.eType = ET_GENERAL;
	actor->s.density = COD1_ACTOR_NET_MARKER;
	actor->s.modelindex = G_ModelIndex( modelAlias );
	if(!actor->s.modelindex){G_FreeEntity(actor);return;}
    actor->s.time = level.time;
    actor->s.frame = gesture?COD1_ACTOR_FOLEY_GESTURE_FRAME:0;
	actor->r.contents = 0; /* visible snapshot, no solid body or hitbox */
	G_SetOrigin( actor, ground.endpos );
	VectorScale( forward, -1.f, facing );
	vectoangles( facing, angles );
	angles[PITCH] = angles[ROLL] = 0;
	G_SetAngle( actor, angles );
	trap_LinkEntity( actor );
    if(gesture)G_Printf("[R197] FOLEY_SEQUENCE_PLACED entity=%d 1800ms breath / gesture / 900ms fades / 9000ms cycle\n",actor->s.number);
    else if(!Q_stricmp(mode,"foley"))G_Printf("[R197] FOLEY_PLACED entity=%d model=%s clip=training_foley_breathing_loop visual-only\n",actor->s.number,modelAlias);
	trap_SendServerCommand( player - g_entities, "print \"CoD1 server actor: placed\\n\"" );
	return;
usage:
	trap_SendServerCommand( player - g_entities,
	                        "print \"Usage: cod1_actor_server 1|foley|gesture [distance 80..512] or cod1_actor_server 0\\n\"" );
}

