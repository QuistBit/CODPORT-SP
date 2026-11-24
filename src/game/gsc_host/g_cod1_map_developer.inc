/* Capture once at map program construction, matching the original compiler
 * policy. The bounded host also rejects later configuration changes. */
static struct {uint64_t token,epoch;GscSession*session;int enabled;} gscMapDeveloper;
static uint64_t gscMapDeveloperSerial;
static void GscMapDeveloperReset(void){memset(&gscMapDeveloper,0,sizeof(gscMapDeveloper));}
static int GscMapDeveloperCurrent(uint64_t token){
 return token&&token==gscMapDeveloper.token&&gscMapDeveloper.session==gscTestSession&&
  gscMapDeveloper.epoch==G_Cod1EntityMapEpoch()&&GscCampaignProgramCurrent(gscMapDeveloper.epoch)&&
  GscTestModuleReadCurrent()&&trap_Cvar_VariableIntegerValue("developer_script")==gscMapDeveloper.enabled;
}
static int GscMapDeveloperSelect(char*error,size_t size){
 GscDeveloperScriptSelection selection;int enabled=trap_Cvar_VariableIntegerValue("developer_script");
 if(gscMapDeveloper.token||gscMapDeveloperSerial==UINT64_MAX||(enabled!=0&&enabled!=1)||!gscTestSession||!GscCampaignProgramCurrent(G_Cod1EntityMapEpoch())||!GscTestModuleReadCurrent())return 0;
 memset(&selection,0,sizeof(selection));selection.enabled=enabled;
 gscMapDeveloper.token=++gscMapDeveloperSerial;gscMapDeveloper.epoch=G_Cod1EntityMapEpoch();gscMapDeveloper.session=gscTestSession;gscMapDeveloper.enabled=enabled;
 selection.certificate.receipt=gscMapDeveloper.token;selection.certificate.current=GscMapDeveloperCurrent;
 if(!gsc_session_select_map_developer_script(gscTestSession,&selection,error,size)){GscMapDeveloperReset();return 0;}
 return GscMapDeveloperCurrent(selection.certificate.receipt);
}
