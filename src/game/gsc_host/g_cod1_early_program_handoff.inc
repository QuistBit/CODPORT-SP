/* Map declaration and selected configuration only; no runtime certificate. */
static const unsigned gscEarlyKeys[]={
 C1CP47_F_ENABLE,
 C1CP47_F_MODE,
 C1CP47_F_ACTOR_PREFIX,
 C1CP47_F_AI_ARRAYS,
 C1CP47_F_DELETE,
 C1CP47_F_TURRET,
 C1CP47_F_SOUND,
 C1CP47_F_SOUND_NOTIFY,
 C1CP47_F_VOICE,
 C1CP47_F_CARBINE,
 C1CP47_F_FINALE,
 C1CP47_F_GRENADES,
 C1CP47_F_OPENING,
 C1CP47_K_DEVELOPER,
 C1CP47_K_SAVE,
 C1CP47_K_MAP,
 C1CP47_K_ROOT,
 C1CP47_F_BSP_CONSTRUCT,
 C1CP47_F_GRAPH_STARTUP,
 C1CP47_F_CLIENT_STARTUP,
 C1CP47_F_NATIVE_THREADS,
 C1CP47_F_PLACED_INIT,
 C1CP47_F_PLACED_STATE,
 C1CP47_F_PLACED_COHORT
};
static struct {
 uint64_t epoch,receipt,id;void*user;int(*current)(void*,uint64_t,uint64_t);
 int attempted,prepared,consumed,nodeSelected,busy,cancelled;
 char values[sizeof(gscEarlyKeys)/sizeof(gscEarlyKeys[0])][MAX_CVAR_VALUE_STRING],path[MAX_CVAR_VALUE_STRING];
} gscTestEarlyProgram;
static int GscTestEarlySelected(void){
 return G_Cod1GscFeature47(C1CP47_F_ENABLE)==1&&
  GscTestEntityModeValue()==43&&G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1;
}
static int GscTestEarlyOwner(void){
 return gscTestEarlyProgram.attempted&&gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch()&&
  !gscTestEarlyProgram.cancelled&&gscTestEarlyProgram.current&&
  gscTestEarlyProgram.current(gscTestEarlyProgram.user,gscTestEarlyProgram.epoch,gscTestEarlyProgram.receipt)==1;
}
static int GscTestEarlyCurrent(void){
 char value[MAX_CVAR_VALUE_STRING];unsigned i;int ok=1;
 if(gscTestEarlyProgram.busy){gscTestEarlyProgram.cancelled=1;return 0;}
 if(!GscTestEarlyOwner())return 0;
 gscTestEarlyProgram.busy=1;
 for(i=0;i<sizeof(gscEarlyKeys)/sizeof(gscEarlyKeys[0]);i++){
  memset(value,0,sizeof(value));GscConfigString47(gscEarlyKeys[i],value,sizeof(value));
  if(!memchr(value,0,sizeof(value))||strcmp(value,gscTestEarlyProgram.values[i])||!GscTestEarlyOwner()){ok=0;break;}
 }
 if(!ok)gscTestEarlyProgram.cancelled=1;
 gscTestEarlyProgram.busy=0;return ok;
}
static int GscTestEarlyCapture(void){
 unsigned i;
 if(!GscTestEarlyOwner()||gscTestEarlyProgram.busy)return 0;
 gscTestEarlyProgram.busy=1;
 for(i=0;i<sizeof(gscEarlyKeys)/sizeof(gscEarlyKeys[0]);i++){
  GscConfigString47(gscEarlyKeys[i],gscTestEarlyProgram.values[i],sizeof(gscTestEarlyProgram.values[i]));
  if(!memchr(gscTestEarlyProgram.values[i],0,sizeof(gscTestEarlyProgram.values[i]))||!GscTestEarlyOwner()){
   gscTestEarlyProgram.cancelled=1;gscTestEarlyProgram.busy=0;return 0;
  }
 }
 gscTestEarlyProgram.busy=0;return GscTestEarlyCurrent();
}

static int GscTestEarlyConsume(const GscSessionSnapshot*s,uint64_t id,int spawning,int restarted){
 if(!s||!gscTestEarlyProgram.prepared||gscTestEarlyProgram.consumed||spawning||restarted||
    !GscTestEarlyCurrent()||s->state!=GSC_SESSION_COMPILED||s->busy||!s->has_program||(s->has_runtime&&G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)!=1)||
    s->session_id!=gscTestEarlyProgram.id||id!=gscTestEarlyProgram.id||!id){
  gscTestEarlyProgram.cancelled=1;return 0;
 }
 gscTestEarlyProgram.consumed=1;return 1;
}

uint64_t G_Cod1GscTestEarlyProgramIdentity(uint64_t epoch,uint64_t receipt){
 uint64_t program;
 if(!gscTestEarlyProgram.prepared||gscTestEarlyProgram.consumed||!level.spawning||level.restarted||
    epoch!=gscTestEarlyProgram.epoch||receipt!=gscTestEarlyProgram.receipt||
    !gscTestSession||gscTestId!=gscTestEarlyProgram.id||!GscTestEarlyCurrent())return 0;
 program=gsc_session_prepared_program_identity(gscTestSession,gscTestId);
 return program&&GscTestEarlyOwner()?program:0;
}
static int GscTestEarlyPending(void){
 return gscTestEarlyProgram.attempted&&gscTestEarlyProgram.prepared&&!gscTestEarlyProgram.consumed&&
  gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch();
}

static int GscTestScriptRootsCurrent(void*user,uint64_t epoch,uint64_t receipt){
 return user==&gscTestEarlyProgram&&epoch==gscTestEarlyProgram.epoch&&receipt==gscTestEarlyProgram.receipt&&GscTestEarlyCurrent();
}

/* Compilation resources belong to the captured declaration lifetime. During
 * BSP construction the program has been prepared but not consumed; afterward
 * both the early receipt and the ready physical world remain mandatory. */
typedef struct {
 unsigned char early[sizeof(gscTestEarlyProgram)];GscSession*session;uint64_t epoch,id;
 int preparing,spawning,now,count;
} GscCampaignProgramSeal;
static int GscCampaignProgramSealed(const GscCampaignProgramSeal*c){
 return c->epoch==G_Cod1EntityMapEpoch()&&g_gametype.integer==GT_SINGLE_PLAYER&&!level.restarted&&!gscTestStopRequested&&
  c->preparing==gscTestPreparingEarly&&c->spawning==level.spawning&&c->now==level.time&&c->count==level.num_entities&&c->session==gscTestSession&&c->id==gscTestId&&
  !memcmp(c->early,&gscTestEarlyProgram,sizeof(c->early));
}
static int GscCampaignProgramCurrent(uint64_t epoch){
 GscCampaignProgramSeal seal;int ok=0;
 if(!epoch||epoch!=G_Cod1EntityMapEpoch()||g_gametype.integer!=GT_SINGLE_PLAYER||level.restarted||gscTestStopRequested)return 0;
 memcpy(seal.early,&gscTestEarlyProgram,sizeof(seal.early));seal.session=gscTestSession;seal.id=gscTestId;seal.epoch=epoch;
 seal.preparing=gscTestPreparingEarly;seal.spawning=level.spawning;seal.now=level.time;seal.count=level.num_entities;
 if(gscTestEarlyProgram.attempted){
  /* A retired declaration cannot fall back to an unrelated ordinary map. */
  if(gscTestEarlyProgram.epoch!=epoch||!GscTestEarlyCurrent()||!GscCampaignProgramSealed(&seal))return 0;
  if(seal.spawning)ok=!gscTestEarlyProgram.consumed&&
   (seal.preparing||(gscTestEarlyProgram.prepared&&seal.session&&seal.id==gscTestEarlyProgram.id));
  else ok=gscTestEarlyProgram.prepared&&gscTestEarlyProgram.consumed&&GscVehicleMapCurrent(epoch)&&GscCampaignProgramSealed(&seal)&&GscTestEarlyCurrent();
 }else ok=!seal.preparing&&GscVehicleMapCurrent(epoch);
 return ok&&GscCampaignProgramSealed(&seal);
}
