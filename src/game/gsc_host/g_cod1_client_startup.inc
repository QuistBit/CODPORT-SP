/* Opt-in port orchestration: prepare while loading, start exactly once after
 * a real ClientBegin has completed. Native GAME export2/3 precede export7;
 * the original GSC body and its startup/no-wait gate remain unchanged. */
typedef struct {
 GscSession*session;uint64_t id,epoch,serial,string_incarnation;
 int phase,ready,cancelled;GscEntityRef player;gclient_t*client;cod1PlayerLifetime_t completed_spawn;
 char path[MAX_QPATH],owner[128];
} GscTestClientStartup;
static GscTestClientStartup gscTestClientStartup;
static void GscTestStartPrepared(int);
static void GscTestDrop(void);
static void GscTestLeave(void);
/* Map/script pairing is rechecked at every startup lease boundary. */
static int GscTestBriefingSelection(void){
 char map[MAX_QPATH],path[MAX_QPATH];
 trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
 trap_Cvar_VariableStringBuffer("g_cod1GscTestScript",path,sizeof(path));
 return !gscTestStopRequested&&((!strcmp(map,"us_intro")&&!strcmp(path,"maps/us_intro.gsc"))||(!strcmp(map,"pathfinder")&&!strcmp(path,"maps/pathfinder.gsc")));
}
static int GscTestClientStartupOptions(void){
 if(trap_Cvar_VariableIntegerValue("g_cod1GscGraphStartup")==1)return GscTestGraphClientOptions();
 static const char*names[]={"g_cod1GscClientStartup","g_cod1GscTest","g_cod1GscEntityTest","savegame_loading","g_cod1GscActorPrefix","g_cod1GscAiArrays","g_cod1GscDeleteTransactions","g_cod1GscTrainingGrenades","g_cod1GscTrainingCarbine","g_cod1GscR232Turret","g_cod1GscSoundNotify","g_cod1GscStreamedVoice","g_cod1GscTrainingFinale"};
 static const int values[]={1,1,43,0,0,0,0,0,0,0,0,0,0};unsigned i;int briefing=GscTestBriefingSelection();char map[MAX_QPATH];
 trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
 for(i=0;i<sizeof(values)/sizeof(values[0]);i++)if(gscTestStopRequested||trap_Cvar_VariableIntegerValue(names[i])!=((briefing&&(i==10||i==11||((i==5||i==6)&&!strcmp(map,"pathfinder"))))?1:values[i])||gscTestStopRequested)return 0;
 return 1;
}
static int GscTestClientStartupIdentity(const GscTestClientStartup*p){
 GscSessionSnapshot s;
 return p&&p->phase&&p->serial&&p->serial==gscTestClientStartup.serial&&
  p->session&&p->session==gscTestSession&&p->session==gscTestClientStartup.session&&
  p->id&&p->id==gscTestId&&p->id==gscTestClientStartup.id&&
  p->epoch&&p->epoch==G_Cod1EntityMapEpoch()&&p->epoch==gscTestClientStartup.epoch&&
  !p->cancelled&&!gscTestClientStartup.cancelled&&!level.restarted&&gscTestEntityMode==43&&
  G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&
  gsc_session_snapshot(p->session,&s)&&s.session_id==p->id&&!s.error[0]&&
  (s.state==GSC_SESSION_PREPARED||s.state==GSC_SESSION_RUNNING||s.state==GSC_SESSION_COMPLETED);
}
/* Historical completion equality and current startup-player selection are
 * separate. This fence is pure and safe AFTER the last engine callback. */
static int GscTestClientStartupPlayerCurrentPure(const GscTestClientStartup*p){
 if(!p)return 0;if(!p->ready)return 1;
 return G_Cod1PlayerLifetimeSameCompleted(&p->completed_spawn)&&
  p->completed_spawn.entity.epoch==p->player.epoch&&
  p->completed_spawn.entity.generation==p->player.generation&&
  p->completed_spawn.entity.slot==p->player.slot&&p->player.slot==0&&
  G_Cod1EntitySameAllocation(&p->player)&&g_entities[0].client==p->client&&
  GscTestQuakePlayerCurrent(&p->player)&&g_entities[0].r.linked&&g_entities[0].s.eType==ET_PLAYER;
}
static int GscTestClientStartupCurrent(const GscTestClientStartup*p,int prepared){
 GscSessionSnapshot s;char path[MAX_QPATH];
 if(!GscTestClientStartupIdentity(p)||!GscTestClientStartupOptions()||!GscTestClientStartupIdentity(p))return 0;
 memset(path,0,sizeof(path));trap_Cvar_VariableStringBuffer("g_cod1GscTestScript",path,sizeof(path));
 if(!GscTestClientStartupIdentity(p)||!memchr(path,0,sizeof(path))||strcmp(path,p->path))return 0;
 if(prepared&&(!gsc_session_snapshot(p->session,&s)||s.busy||s.state!=GSC_SESSION_PREPARED))return 0;
 if(!GscTestClientStartupPlayerCurrentPure(p))return 0;
 return GscTestClientStartupIdentity(p);
}
static void GscTestClientStartupRequest(const GscTestClientStartup*p,int operation,cod1LocalizedStartup_t*q){
 memset(q,0,sizeof(*q));q->version=COD1_LOCALIZED_STARTUP_VERSION;q->bytes=sizeof(*q);q->operation=operation;
 q->session_id=p->id;q->map_epoch=p->epoch;q->player_generation=p->ready?p->player.generation:0;strcpy(q->owner,p->owner);
}
static void GscTestClientStartupCancel(void){
 GscTestClientStartup p=gscTestClientStartup;cod1LocalizedStartup_t q;
 if(!p.phase)return;gscTestClientStartup.cancelled=1;
 GscTestClientStartupRequest(&p,COD1_LOCALIZED_STARTUP_CANCEL,&q);(void)trap_Cod1LocalizedStartup(&q);
}
static void GscTestClientStartupReset(void){uint64_t serial=gscTestClientStartup.serial;memset(&gscTestClientStartup,0,sizeof(gscTestClientStartup));gscTestClientStartup.serial=serial;}
static int GscTestClientStartupArm(const char*path,char*error,size_t cap){
 GscTestClientStartup p;cod1LocalizedStartup_t q;
 if(gscTestClientStartup.phase||gscTestClientStartup.serial==UINT64_MAX||!path||strlen(path)>=MAX_QPATH||
    !GscTestClientStartupOptions()||!GscTestPrecacheStringBegin(error,cap))return 0;
 memset(&p,0,sizeof(p));p.session=gscTestSession;p.id=gscTestId;p.epoch=G_Cod1EntityMapEpoch();
 p.serial=gscTestClientStartup.serial+1;p.phase=1;p.string_incarnation=gscTestPrecacheString.incarnation;
 strcpy(p.path,path);strcpy(p.owner,gscTestPrecacheString.marker);gscTestClientStartup=p;
 if(!GscTestClientStartupCurrent(&p,1))return 0;
 GscTestClientStartupRequest(&p,COD1_LOCALIZED_STARTUP_ARM,&q);
 return trap_Cod1LocalizedStartup(&q)&&GscTestClientStartupCurrent(&p,1);
}
void G_Cod1GscTestClientSpawned(struct gentity_s*entity){
 GscTestGraphClientSpawned(entity);
 GscTestClientStartup p=gscTestClientStartup;GscEntityRef player;cod1PlayerLifetime_t completed;
 if(!p.phase||entity!=&g_entities[0])return;
 if(gscTestBusy||p.phase!=1||!GscTestClientStartupIdentity(&p)||!G_Cod1EntityCapture(entity,&player)||
    !GscTestQuakePlayerCurrent(&player)||!entity->r.linked||entity->s.eType!=ET_PLAYER||
    !G_Cod1PlayerLifetimeRead(&player,&completed)){gscTestClientStartup.cancelled=1;return;}
 if(p.ready){if(memcmp(&p.player,&player,sizeof(player))||p.client!=entity->client||
    p.completed_spawn.completion!=completed.completion||!G_Cod1PlayerLifetimeSameCompleted(&p.completed_spawn))gscTestClientStartup.cancelled=1;return;}
 gscTestClientStartup.player=player;gscTestClientStartup.client=entity->client;
 gscTestClientStartup.completed_spawn=completed;gscTestClientStartup.ready=1;
}
static int GscTestClientStartupRetime(char*error,size_t cap){
 GscTestClientStartup p=gscTestClientStartup;GscTestPrecacheStringScene strings=gscTestPrecacheString;
 if(p.phase!=2||!p.ready||!GscTestClientStartupCurrent(&p,1)||
    strings.incarnation!=p.string_incarnation||strcmp(strings.marker,p.owner)||
    !GscTestPrecacheStringMarker(&strings,0,0)||!GscTestClientStartupCurrent(&p,1)){
  GscTestError(error,cap,"prepared client startup localized owner retired");return 0;
 }
 gscTestPrecacheString.host_time=level.time;return 1;
}
static int GscTestClientStartupBegin(cod1LocalizedStartup_t*q){
 GscTestClientStartup p=gscTestClientStartup;
 if(p.phase!=2||!p.ready||!GscTestClientStartupCurrent(&p,1))return 0;
 GscTestClientStartupRequest(&p,COD1_LOCALIZED_STARTUP_BEGIN,q);
 if(!trap_Cod1LocalizedStartup(q))return 0;
 if(!GscTestClientStartupCurrent(&p,1)){q->operation=COD1_LOCALIZED_STARTUP_END;(void)trap_Cod1LocalizedStartup(q);return 0;}
 return 1;
}
/* Return1 while this invocation consumes/holds the pending startup. */
static int GscTestClientStartupFrame(int levelTime){
 GscTestClientStartup p=gscTestClientStartup;
 if(!p.phase)return 0;
 if(!GscTestClientStartupCurrent(&p,p.phase==1)){
  G_Printf("[GSC R104] client startup owner/config/player retired\n");GscTestDrop();GscTestLeave();return 1;
 }
 if(p.phase!=1)return 0;
 if(!p.ready){GscTestLeave();return 1;}
 gscTestClientStartup.phase=2;GscTestStartPrepared(levelTime);return 1;
}
