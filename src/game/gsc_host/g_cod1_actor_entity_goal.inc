/* Public Sentient method 0x20074520. The empty-path direct-position branch
 * uses the observer's five native words for the chosen Sentient. A repeated
 * target skips 0x20073810, including path release and position refresh. */
static int ActorSetEntityGoal(const GscActorGoalRequest *input,
                             const GscActorGoalRequest *request,
                             int now,char *error,size_t capacity,
                             int (*current)(void *),void *context) {
    static const unsigned oppositeTeam[5]={0,2,1,0,0};
    cod1ActorLease_t owner,observed;
    cod1ActorRecord_t *actor=NULL;
    cod1SentientRecord_t *sentient;
    const gentity_t *target;
    const uint32_t *observation=NULL;
    const float *position;
    unsigned index;
    int accepted=0;

    cod1ActorGoalBusy=1;
    cod1ActorGoalCancelled=0;
    if(current(context)!=1 || cod1ActorGoalCancelled || now!=level.time ||
       level.spawning || level.restarted || memcmp(input,request,sizeof(*input)) ||
       !G_Cod1EntitySameAllocation(&request->entity) ||
       !G_Cod1EntitySameAllocation(&request->target))goto done;
    index=cod1SentientByEntity[request->entity.slot];
    if(!index || index>COD1_SENTIENT_CAPACITY ||
       !ActorPerceptionMemberLease(cod1Sentients+index-1,&owner) ||
       !ActorRefEqual(&request->entity,&owner.entity)){
        EntityError(error,capacity,"setgoalentity requires a Sentient receiver");
        goto done;
    }
    sentient=cod1Sentients+owner.sentientIndex;
    if(ActorRefEqual(&sentient->rawGoal.entityGoal,&request->target)){
        sentient->defaults.cell0020=-1.0f;
        accepted=1;goto done;
    }
    if(sentient->rawGoal.node0040 || sentient->rawGoal.node004c){
        EntityError(error,capacity,"setgoalentity requires native node claim release");
        goto done;
    }
    target=g_entities+request->target.slot;
    if(owner.kind==COD1_ACTOR_LEASE_ACTOR){
        if(!ActorGoalLease(&request->entity,&owner))goto done;
        actor=cod1Actors+owner.actorIndex;
        if((int16_t)actor->defaults.pathWords[(0x6f4-0x374)/4] ||
           (int16_t)(actor->defaults.pathWords[(0x6f8-0x374)/4]>>16) ||
           actor->motion.busy || actor->goal.assigned){
            EntityError(error,capacity,"setgoalentity requires active route release");
            goto done;
        }
        index=cod1SentientByEntity[request->target.slot];
        if(index){
            const Cod1ObservationTarget *binding;
            if(index>COD1_SENTIENT_CAPACITY ||
               !ActorPerceptionMemberLease(cod1Sentients+index-1,&observed) ||
               !ActorRefEqual(&observed.entity,&request->target))goto done;
            if(sentient->defaults.team>=5)goto done;
            if(oppositeTeam[sentient->defaults.team]!=cod1Sentients[index-1].defaults.team &&
               actor->defaults.allocation.followmin<=actor->defaults.allocation.followmax){
                EntityError(error,capacity,"setgoalentity requires native follow-node selection");
                goto done;
            }
            binding=actor->observationTargets+index-1;
            if(binding->serial && (binding->serial!=observed.serial ||
               !ActorRefEqual(&binding->owner,&observed.entity)))goto done;
            observation=actor->observations.row[index-1].word;
        }
    }
    /* 73B1D reads the target client. With no client 73B3D reads the
     * receiver entity origin, including when the target is another actor. */
    position=target->client?target->client->ps.origin:g_entities[owner.entity.slot].r.currentOrigin;
    if(!isfinite(position[0]) || !isfinite(position[1]) || !isfinite(position[2]))goto done;
    if(actor){
        actor->defaults.pathWords[(0x6f4-0x374)/4]=0;
        actor->defaults.cell0774=0;
        actor->rawGoalCell07e4=0;
        if(observation){
            actor->rawGoalCell07e4=observation[0];
            memcpy(actor->goalObservationTail,observation+1,sizeof(actor->goalObservationTail));
        }
        actor->stationaryGoal=0;
    }
    sentient->rawGoal.entityGoal=request->target;
    memcpy(sentient->rawGoal.position,position,12);
    sentient->defaults.cell0020=-1.0f;
    accepted=1;
done:
    cod1ActorGoalBusy=0;
    return accepted;
}
