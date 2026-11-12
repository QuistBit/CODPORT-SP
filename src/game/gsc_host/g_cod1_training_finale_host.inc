/* R229 Training-only mission consumer. Old diagnostic profiles never enter.
 * Engine services are deferred until after Session advancement, never inside
 * the native callback. Original victory menu owns pause/Next Level behavior. */
static int GscTestFinaleAdmission(const Cod1MissionTransition *s,int now){
 gentity_t *p;
 if(!s||!gscTestFinaleSelected||!s->phase||now!=level.time||
    memcmp(s,&gscTestFinale,sizeof(*s))||
    !GscTestMissionCurrent(s->session,s->epoch))return 0;
 p=GscTestMissionPlayer();
 if(!p||!G_Cod1EntitySameAllocation(&gscTestFinalePlayer)||gscTestFinalePlayer.slot!=0||
    !p->inuse||!p->client||p->health<=0||
    p->client->pers.connected!=CON_CONNECTED||p->client->noclip||
    p->client->sess.sessionTeam==TEAM_SPECTATOR||
    trap_Cvar_VariableIntegerValue("g_cod1GscTrainingFinale")!=1)return 0;
 return now==level.time&&!memcmp(s,&gscTestFinale,sizeof(*s))&&
        GscTestMissionCurrent(s->session,s->epoch);
}
static void GscTestFailureCancel(void) {
    Cod1MissionTransition s=gscTestFinale;
    char quote[MAX_STRING_CHARS];
    if (s.phase < COD1_FAILURE_PENDING) return;
    Cod1MissionTransitionReset(&gscTestFinale);
    if (!gscTestFinaleSelected || s.session!=gscTestId || s.epoch!=gscTestMission.epoch ||
        !gscTestMapEpoch || gscTestMapEpoch!=G_Cod1EntityMapEpoch() ||
        !G_Cod1EntitySameAllocation(&gscTestFinalePlayer)) return;
    trap_Cvar_VariableStringBuffer("ui_deadquote",quote,sizeof(quote));
    if (!strcmp(quote,"@TRAINING_SHOT_MOODY")) trap_Cvar_Set("ui_deadquote","");
    if (s.phase > COD1_FAILURE_PENDING && trap_Cvar_VariableIntegerValue("g_reloading")==RELOAD_SAVEGAME) {
        trap_Cvar_Set("g_reloading","0");
        trap_SendServerCommand(-1,va("scr_fade 0 %d 0",level.time));
        trap_SendServerCommand(-1,"snd_fade 1 100");
    }
}

static int GscTestFailureFrame(Cod1MissionTransition s, int now, int reload) {
    if (!gscTestMission.failed || now<s.last || !GscTestFinaleAdmission(&s,now)) return 0;
    if (s.phase == COD1_FAILURE_PENDING) {
        if (reload || level.reloadDelayTime || level.reloadPauseTime) return 0;
        gscTestFinale.phase=COD1_FAILURE_FADING;
        gscTestFinale.last=now;
        s=gscTestFinale;
        trap_SendServerCommand(-1,va("scr_fade 1 %d 2000",gscTestMission.fade_start));
        if (!GscTestFinaleAdmission(&s,now)) return 0;
        trap_Cvar_Set("g_reloading","1");
        if (!GscTestFinaleAdmission(&s,now)) return 0;
        trap_SendServerCommand(-1,"snd_fade 0 2000");
        return GscTestFinaleAdmission(&s,now);
    }
    if (reload!=RELOAD_SAVEGAME || level.reloadDelayTime || level.reloadPauseTime) return 0;
    if (s.phase==COD1_FAILURE_ENDED || now<=s.deadline) return 1;
    if (s.phase==COD1_FAILURE_SCREEN && trap_Cvar_VariableIntegerValue("cl_paused")) return 1;
    if (!GscTestFinaleAdmission(&s,now) || now>INT_MAX-50) return 0;
    gscTestFinale.last=now;
    gscTestFinale.phase=s.phase==COD1_FAILURE_FADING ? COD1_FAILURE_SCREEN : COD1_FAILURE_ENDED;
    gscTestFinale.deadline=now+50;
    s=gscTestFinale;
    trap_SendServerCommand(-1,"opendeadscreen");
    return GscTestFinaleAdmission(&s,now);
}
static void GscTestFinaleCancel(void){
 Cod1MissionTransition s=gscTestFinale;
 if(s.phase>=COD1_FAILURE_PENDING){GscTestFailureCancel();return;}
 /* Retire first; callbacks cannot dispatch this record again. */
 Cod1MissionTransitionReset(&gscTestFinale);
 if(gscTestFinaleSelected&&s.session==gscTestId&&s.epoch==gscTestMission.epoch&&s.phase>1&&s.phase<4&&
    gscTestMapEpoch&&gscTestMapEpoch==G_Cod1EntityMapEpoch()&&
    trap_Cvar_VariableIntegerValue("g_reloading")==RELOAD_NEXTMAP_WAITING&&
    s.session==gscTestId&&s.epoch==gscTestMission.epoch&&
    gscTestMapEpoch==G_Cod1EntityMapEpoch()){
  trap_Cvar_Set("g_reloading","0");
  trap_SendServerCommand(-1,va("scr_fade 0 %d 0",level.time));
  trap_SendServerCommand(-1,"snd_fade 1 100");
 }
}
static int GscTestFinaleFrame(char *error,size_t capacity){
 Cod1MissionTransition s;int now=level.time,effect=0,reload;
 if(!gscTestFinaleSelected||!gscTestFinale.phase)return 1;
 s=gscTestFinale;if(!GscTestFinaleAdmission(&s,now))goto refuse;
 reload=trap_Cvar_VariableIntegerValue("g_reloading");
 if(!GscTestFinaleAdmission(&s,now))goto refuse;
 if(s.phase>=COD1_FAILURE_PENDING){
  if(!GscTestFailureFrame(s,now,reload))goto refuse;
  return 1;
 }
 if(s.phase==1){if(reload||level.reloadDelayTime)goto refuse;}
 else if(s.phase<4&&reload!=RELOAD_NEXTMAP_WAITING)goto refuse;
 if(s.phase==3){
  int paused=trap_Cvar_VariableIntegerValue("cl_paused");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  if(paused)return 1; /* Also makes controlled tests model native UI pause. */
 }
 if(s.phase==1){
  if(now>INT_MAX-1100)goto refuse;
  gscTestFinale.at=now;gscTestFinale.last=now;gscTestFinale.deadline=now+1000;
 }
 if(!Cod1MissionTransitionTick(&gscTestFinale,s.session,s.epoch,now,&effect))goto refuse;
 s=gscTestFinale;
 if(effect==COD1_MISSION_FADE){
  trap_SendServerCommand(-1,va("scr_fade 1 %d 750",s.at+250));
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  trap_SendServerCommand(-1,"snd_fade 0 1000");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  trap_Cvar_Set("g_reloading","4");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  G_Printf("[R229] ORIGINAL_MISSION_FADE_STARTED\n");
 }else if(effect==COD1_MISSION_VICTORY){
  trap_SendServerCommand(-1,"openvictoryscreen");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  G_Printf("[R229] ORIGINAL_VICTORY_SCREEN_REQUESTED\n");
 }else if(effect==COD1_MISSION_NEXT_MAP){
  int cheats=trap_Cvar_VariableIntegerValue("sv_cheats");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  trap_Cvar_Set("g_reloading","2");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  /* Only the native Training target; neither script text nor arbitrary map
   * strings are composed into a console command. This does not admit us_intro GSC. */
  trap_SendConsoleCommand(EXEC_APPEND,cheats?"spdevmap us_intro\n":"spmap us_intro\n");
  if(!GscTestFinaleAdmission(&s,now))goto refuse;
  G_Printf("[R229] ORIGINAL_NEXT_MAP_QUEUED us_intro\n");
 }
 return 1;
refuse:
 GscTestError(error,capacity,"finale ownership/player/reload state changed");return 0;
}
