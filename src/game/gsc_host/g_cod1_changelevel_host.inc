/* Native GAME359B0 -> 1E790. Engine calls occur outside VM advancement.
 * One/two argument, nonpersistent normal-map domain; gamefinished/third fade
 * argument remain explicit unsupported domains. No victory marker or timer. */
static int GscChangeMapName(const char *s){
 size_t i,n;if(!s||(n=strlen(s))==0||n>=MAX_QPATH||!Q_stricmp(s,"gamefinished"))return 0;
 for(i=0;i<n;i++){unsigned c=(unsigned char)s[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return 0;}
 return 1;
}
static int GscChangeOwned(const GscChangeLevel *p){
 GscSessionSnapshot s;
 return p&&p->phase&&p->session&&p->session==gscTestSession&&p->id==gscTestId&&p->epoch==gscTestMission.epoch&&
   gscTestMission.owned&&p->map&&p->map==G_Cod1EntityMapEpoch()&&!gscTestStopRequested&&!level.restarted&&
   G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&GscTestModuleReadCurrent()&&
   G_Cod1EntitySameAllocation(&p->player)&&p->player.slot==0&&g_entities[0].inuse&&g_entities[0].client==&level.clients[0]&&
   g_entities[0].client->pers.connected==CON_CONNECTED&&
   gsc_session_snapshot(p->session,&s)&&s.session_id==p->id&&!s.error[0]&&
   (s.state==GSC_SESSION_RUNNING||s.state==GSC_SESSION_COMPLETED)&&!memcmp(p,&gscChangeLevel,sizeof(*p));
}
static int GscChangeBegin(const char *target){
 GscChangeLevel p;
 if(!GscChangeMapName(target)||gscTestFinaleSelected||level.time<0||level.time>INT_MAX-1100)return 0;
 if(gscChangeLevel.phase){if(gscChangeLevel.phase!=1||!GscChangeOwned(&gscChangeLevel))return 0;p=gscChangeLevel;}
 else{memset(&p,0,sizeof(p));p.session=gscTestSession;p.id=gscTestId;p.epoch=gscTestMission.epoch;p.map=G_Cod1EntityMapEpoch();p.phase=1;
  if(!G_Cod1EntityCapture(GscTestMissionPlayer(),&p.player))return 0;}
 strcpy(p.target,target);p.last=level.time;gscChangeLevel=p;return GscChangeOwned(&p);
}
static void GscChangeCancel(void){
 GscChangeLevel p=gscChangeLevel;int owned=p.phase==2&&GscChangeOwned(&p);
 memset(&gscChangeLevel,0,sizeof(gscChangeLevel));
 if(owned&&trap_Cvar_VariableIntegerValue("g_reloading")==4&&p.map==G_Cod1EntityMapEpoch()){
  trap_Cvar_Set("g_reloading","0");trap_SendServerCommand(-1,va("scr_fade 0 %d 0",level.time));trap_SendServerCommand(-1,"snd_fade 1 100");
 }
}
static int GscChangeFrame(void){
 GscChangeLevel p=gscChangeLevel;int now=level.time,reload,cheats;char command[256];
 if(!p.phase||p.phase==3)return 1;
 if(now<p.last||!GscChangeOwned(&p))return 0;
 reload=trap_Cvar_VariableIntegerValue("g_reloading");if(!GscChangeOwned(&p))return 0;
 if(p.phase==1){
  /* Consumer repeats native health/reload gate; it consumes the request even
   * if these changed since the producer. */
  gscTestMission.requested=0;
  if(g_entities[0].health<=0||reload){memset(&gscChangeLevel,0,sizeof(gscChangeLevel));return 1;}
  if(now>INT_MAX-1100)return 0;p.phase=2;p.at=p.last=now;p.deadline=now+1000;gscChangeLevel=p;
  trap_SendServerCommand(-1,va("scr_fade 1 %d 750",now+250));if(!GscChangeOwned(&p))return 0;
  trap_SendServerCommand(-1,"snd_fade 0 1000");if(!GscChangeOwned(&p))return 0;
  trap_Cvar_Set("g_reloading","4");if(!GscChangeOwned(&p))return 0;
  G_Printf("[Campaign] CHANGELEVEL_FADE target=%s tick=%d deadline=%d\n",p.target,now,p.deadline);return 1;
 }
 if(reload!=4)return 0;if(now<=p.deadline)return 1;
 cheats=trap_Cvar_VariableIntegerValue("sv_cheats");if(!GscChangeOwned(&p))return 0;
 trap_Cvar_Set("g_reloading","2");if(!GscChangeOwned(&p))return 0;
 /* Map entry selection is configuration for the NEXT load. Its script comes
  * from VFS unchanged and must independently pass ordinary load/admission. */
 trap_Cvar_Set("g_cod1GscTestScript",va("maps/%s.gsc",p.target));if(!GscChangeOwned(&p))return 0;
 snprintf(command,sizeof(command),"%s %s\n",cheats?"spdevmap":"spmap",p.target);
 p.phase=3;p.last=now;gscChangeLevel=p;
 trap_SendConsoleCommand(EXEC_APPEND,command);
 if(!GscChangeOwned(&p))return 0;
 G_Printf("[Campaign] CHANGELEVEL_QUEUED target=%s tick=%d\n",p.target,now);return 1;
}
