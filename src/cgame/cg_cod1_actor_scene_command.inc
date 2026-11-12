void CG_Cod1ActorServer_f( void ) {
	char mode[8], distance[32], command[64], *end;
	double value;
	int argc = trap_Argc();
	if ( argc < 2 || argc > 3 ) goto usage;
	if(strlen(CG_Argv(1))>=sizeof(mode))goto usage;
	Q_strncpyz( mode, CG_Argv( 1 ), sizeof( mode ) );
	if ( !Q_stricmp( mode, "0" ) && argc == 2 ) {
		trap_SendClientCommand( "cod1_actor_server 0" );
		return;
	}
	if ( Q_stricmp( mode, "1" ) && Q_stricmp(mode,"foley") && Q_stricmp(mode,"gesture") ) goto usage;
	if ( argc == 3 ) {
		const char *arg = CG_Argv( 2 );
		if ( strlen( arg ) >= sizeof( distance ) ) goto usage;
		Q_strncpyz( distance, arg, sizeof( distance ) );
		value = strtod( distance, &end );
		if ( end == distance || *end || !( value >= 80.0 && value <= 512.0 ) ) goto usage;
		Com_sprintf( command, sizeof( command ), "cod1_actor_server %s %s", mode, distance );
	} else {
		Com_sprintf(command,sizeof(command),"cod1_actor_server %s",mode);
	}
	CG_Cod1ActorDebugReset();
	trap_SendConsoleCommand( "cod1_actor_preview 0\n" );
	trap_SendClientCommand( command );
	return;
usage:
	CG_Printf( "Usage: cod1_actor_server 1|foley|gesture [distance 80..512] or cod1_actor_server 0\n" );
}

