static int cod1PlacedTargetBusy,cod1PlacedTargetCancelled;
/* Original736A0 supported empty-path/claim-free target prefix. Numeric
 * discriminator precedes named entity lookup; node fallback is published DATA.
 * Unsupported sentient observation/search/cover lanes refuse BEFORE stores.
 * That refusal is port admission, not an assertion native makes no stores. */
static int ActorPlacedTargetCurrent(const cod1ActorLease_t*l){
 const cod1ActorRecord_t*a;
 if(cod1PlacedTargetCancelled||!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||!ActorLeaseValid(l)||
    l->kind!=COD1_ACTOR_LEASE_ACTOR||!G_Cod1ActorBootstrapPostinitCurrent(l))return 0;
 a=cod1Actors+l->actorIndex;
 return a->positionPostinit&&!a->placedTargetApplied&&!a->numericTargetApplied&&a->alive&&!a->initialized&&a->nativeDataInitialized&&
  !memcmp(a->nativeData.entity.origin,g_entities[l->entity.slot].r.currentOrigin,12)&&
  !memcmp(a->nativeData.entity.angles,g_entities[l->entity.slot].r.currentAngles,12)&&
  a->spatial.linked&&!a->spatial.busy&&!a->dobj.busy&&!a->modelBusy&&!a->modelMethodBusy&&!a->motion.active&&
  !a->goal.assigned&&!a->criticalBusy&&!a->scriptStateBusy;
}
static int ActorTargetFirst(const char*text,int classname,GscEntityRef*out){
 unsigned i;for(i=0;i<(unsigned)level.num_entities&&i<MAX_GENTITIES;i++){
  gentity_t*e=g_entities+i;const char*value=classname?e->classname:e->targetname;
  if(e->inuse&&value&&!strcmp(text,value))return G_Cod1EntityCapture(e,out)?2:0;
 }return 1;
}
static int ActorPlacedPostinitTarget(const GscActorBootstrapServices*input,const cod1ActorLease_t*inputLease,char*error,size_t cap){
 GscActorBootstrapServices api;cod1ActorLease_t l;cod1ActorRecord_t*a,actor;cod1SentientRecord_t*s,sentient;
 GscActorBootstrapNodeTarget node;GscEntityRef entity={0};C1PostinitPosition v;char text[128]={0};const char*target;
 float goal[3];size_t n;unsigned i;uint32_t cursor;int kind=0,found,repeat=0,changed=0,named=0,nodeUsed=0,clientTarget=0;
 if(!input||!inputLease||ActorGoalPrivate(input,sizeof(*input))||ActorGoalPrivate(inputLease,sizeof(*inputLease))||
    ActorGoalPrivate(error,cap)||ActorOverlap(error,cap,input,sizeof(*input))||!ActorArguments(inputLease,sizeof(*inputLease),NULL,0,error,cap))return 0;
 api=*input;l=*inputLease;
 if(!api.current||!api.node_target||!api.storage_separate||!api.storage_separate(api.context,input,sizeof(*input))||
    !api.storage_separate(api.context,inputLease,sizeof(*inputLease))||!api.storage_separate(api.context,error,cap)||
    api.current(api.context)!=1||!ActorPlacedTargetCurrent(&l))return 0;
 a=cod1Actors+l.actorIndex;s=cod1Sentients+l.sentientIndex;actor=*a;sentient=*s;target=g_entities[l.entity.slot].target;
 if(target){for(n=0;n<sizeof(text)&&target[n];n++);if(n==sizeof(text))return 0;memcpy(text,target,n+1);}
 if(target){
  if(C1TargetDigit((unsigned char)text[0])||text[0]=='-'||text[0]=='.'){
   if(!C1TargetParse(text,goal)){EntityError(error,cap,"placed numeric target is outside supported finite decimal contract");return 0;}kind=1;
  }else{
   named=1;found=ActorTargetFirst(text,0,&entity);if(!found)return 0;
   if(found==2){
    if(g_entities[entity.slot].classname&&!strcmp(g_entities[entity.slot].classname,"info_player_deathmatch")){
     found=ActorTargetFirst("player",1,&entity);if(found!=2){EntityError(error,cap,"placed target player substitution has no current player");return 0;}}
    if(!EntityValidate(NULL,&entity))return 0;
    /* Independent exact allocation identity, never a node token/address. */
    repeat=ActorRefEqual(&s->rawGoal.entityGoal,&entity);kind=2;
    if(!repeat){
     if(cod1SentientByEntity[entity.slot]){EntityError(error,cap,"placed entity target observation/search contract remains open");return 0;}
     clientTarget=g_entities[entity.slot].client!=NULL;
     memcpy(goal,clientTarget?g_entities[entity.slot].client->ps.origin:g_entities[l.entity.slot].r.currentOrigin,12);
    }
   }else{
    memset(&node,0,sizeof(node));if(!api.node_target(api.context,&l.entity,text,&node,error,cap))return 0;
    if(node.status!=2||node.type>1){EntityError(error,cap,"placed named target absent or requires unsupported cover/claim tail");return 0;}
    memcpy(goal,node.origin,12);kind=1;nodeUsed=1;
   }
  }
 }
 if(api.current(api.context)!=1)return 0;
 if(nodeUsed){GscActorBootstrapNodeTarget last={0};
  /* The last external call revalidates the independent graph after all
   * other provider callbacks. Only pure local fences follow it. */
  if(!api.node_target(api.context,&l.entity,text,&last,error,cap)||memcmp(&last,&node,sizeof(node)))return 0;}
 if(!ActorPlacedTargetCurrent(&l)||memcmp(a,&actor,sizeof(actor))||memcmp(s,&sentient,sizeof(sentient))||
    (!!target)!=(g_entities[l.entity.slot].target!=NULL)||(target&&strcmp(text,g_entities[l.entity.slot].target)))return 0;
 if(named){GscEntityRef current={0};int match=ActorTargetFirst(text,0,&current);
  if(match==2&&g_entities[current.slot].classname&&!strcmp(g_entities[current.slot].classname,"info_player_deathmatch"))match=ActorTargetFirst("player",1,&current);
  if(kind==2?(match!=2||!ActorRefEqual(&entity,&current)):match!=1)return 0;}
 if(kind==2&&(!EntityValidate(NULL,&entity)||(!repeat&&cod1SentientByEntity[entity.slot])))return 0;
 if(kind==2&&!repeat&&(clientTarget!=(g_entities[entity.slot].client!=NULL)||
    (clientTarget&&memcmp(goal,g_entities[entity.slot].client->ps.origin,12))))return 0;
 if((s->rawGoal.entityGoal.epoch||s->rawGoal.entityGoal.slot||s->rawGoal.entityGoal.generation)&&!EntityValidate(NULL,&s->rawGoal.entityGoal))return 0;
 memset(&v,0,sizeof(v));memcpy(v.position,g_entities[l.entity.slot].r.currentOrigin,12);memcpy(v.goal,s->rawGoal.position,12);
 v.radius=s->defaults.goalradius;v.cell20=s->defaults.cell0020;v.node8=s->rawGoal.entityGoal.epoch?1:0;
 v.node40=s->rawGoal.node0040;v.node4c=s->rawGoal.node004c;v.path6f4=a->defaults.pathWords[(0x6f4-0x374)/4];
 cursor=a->defaults.pathWords[(0x6f8-0x374)/4];v.cursor6fa=(int16_t)(cursor>>16);
 v.cell774=a->defaults.cell0774;v.cell7e4=a->rawGoalCell07e4;v.cell78f=a->rawGoalCell078f;
 if(!s->enemyKnown||s->enemy.epoch||s->enemy.slot||s->enemy.generation||v.node40||v.node4c||(int16_t)v.path6f4||v.cursor6fa)return 0;
 if(kind==1){
  for(i=0;i<3;i++)if(v.goal[i]!=goal[i])changed=1;
  if(!C1PostinitTargetApply(&v,goal))return 0;
 }else if(kind==2&&!repeat){
  if(!C1PostinitEntityTargetApply(&v,goal,0))return 0;
 }
 /* No callbacks/failures below; metadata is allocation-owned, not readiness. */
 if(kind==1||kind==2&&!repeat){
  memcpy(s->rawGoal.position,v.goal,12);s->defaults.cell0020=v.cell20;
  if(kind==2)s->rawGoal.entityGoal=entity;else if(changed)memset(&s->rawGoal.entityGoal,0,sizeof(s->rawGoal.entityGoal));
  a->defaults.pathWords[(0x6f4-0x374)/4]=v.path6f4;a->defaults.cell0774=v.cell774;
  a->rawGoalCell07e4=v.cell7e4;a->rawGoalCell078f=v.cell78f;
 }
 a->placedTargetApplied=1;return 1;
}
int G_Cod1ActorPlacedPostinitTarget(const GscActorBootstrapServices*api,const cod1ActorLease_t*l,char*error,size_t cap){
 int ok;
 if(cod1PlacedTargetBusy){cod1PlacedTargetCancelled=1;return 0;}
 if(ActorOverlap(api,api?sizeof(*api):0,&cod1PlacedTargetBusy,sizeof(cod1PlacedTargetBusy))||
    ActorOverlap(api,api?sizeof(*api):0,&cod1PlacedTargetCancelled,sizeof(cod1PlacedTargetCancelled))||
    ActorOverlap(l,l?sizeof(*l):0,&cod1PlacedTargetBusy,sizeof(cod1PlacedTargetBusy))||
    ActorOverlap(l,l?sizeof(*l):0,&cod1PlacedTargetCancelled,sizeof(cod1PlacedTargetCancelled))||
    ActorOverlap(error,cap,&cod1PlacedTargetBusy,sizeof(cod1PlacedTargetBusy))||
    ActorOverlap(error,cap,&cod1PlacedTargetCancelled,sizeof(cod1PlacedTargetCancelled)))return 0;
 cod1PlacedTargetBusy=1;cod1PlacedTargetCancelled=0;
 ok=ActorPlacedPostinitTarget(api,l,error,cap);
 cod1PlacedTargetBusy=0;return ok&&!cod1PlacedTargetCancelled;
}
