/* R200 diagnostic spawn only. No player setorigin/native compatibility is
 * inferred. Exact selected script/map and a real clear ground trace are required.
 * The normal ClientSpawn path still initializes input, health and view state. */
static void Cod1R200FixtureSpawn(gentity_t *ent,vec3_t origin,vec3_t angles) {
    char map[MAX_QPATH],script[MAX_QPATH];
    vec3_t candidate,below;
    trace_t ground;
    if(!ent || ent!=&g_entities[0] || !ent->client ||
       (ent->r.svFlags&(SVF_CASTAI|SVF_BOT)) ||
       ent->client->sess.sessionTeam==TEAM_SPECTATOR ||
       g_gametype.integer!=GT_SINGLE_PLAYER || !G_Cod1MapEntities() ||
       trap_Cvar_VariableIntegerValue("g_cod1GscTest")!=1 ||
       trap_Cvar_VariableIntegerValue("g_cod1GscEntityTest")!=43 ||
       trap_Cvar_VariableIntegerValue("savegame_loading")!=0)return;
    trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
    trap_Cvar_VariableStringBuffer("g_cod1GscTestScript",script,sizeof(script));
    if(Q_stricmp(map,"training") ||
       strcmp(script,"scripts/gsc_tests/r200_execution.gsc"))return;
    VectorSet(candidate,4650.f,-7050.f,-64.f);
    VectorCopy(candidate,below);below[2]-=256.f;
    trap_TraceCapsule(&ground,candidate,ent->r.mins,ent->r.maxs,below,
                      ent->s.number,MASK_PLAYERSOLID);
    if(ground.startsolid || ground.allsolid || ground.fraction>=1.f ||
       ground.plane.normal[2]<.7f) {
        G_Printf("[R200] FIXTURE_SPAWN_REFUSED original spawn retained\n");
        return;
    }
    VectorCopy(ground.endpos,origin);origin[2]+=1.f;
    VectorSet(angles,7.f,167.f,0.f);
    G_Printf("[R200] FIXTURE_SPAWN xyz=%.3f,%.3f,%.3f yaw=%.1f\n",
             origin[0],origin[1],origin[2],angles[1]);
}
