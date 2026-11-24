/* GAME39E20/39E30 ->39D20 ->39580/38E80. Relative yaw/roll target is built
 * from the current angles before evaluating an interrupted trajectory.
 * Existing ScriptMotionFrame advances the real phases and rotatedone. */
static int ScriptRotationPlan(const GscYawMotionRequest *q,const gentity_t *entity,ScriptMotionAxis *next){
    vec3_t start,base,target,delta,decelDirection;
    float seconds[3],speed;
    double denominator,length;
    int i,j,axis=q->method==COD1_ROTATE_TO?-1:q->method==502?YAW:ROLL;vec3_t direction;
    if(!q->operandsValid||(axis<0?!YawVector(q->velocity,36000.0f):!YawFinite(q->yaw))||!YawFinite(q->seconds)||!YawFinite(q->accel)||!YawFinite(q->decel)||
       q->seconds<=0||q->accel<0||q->decel<0||(double)q->accel+q->decel>q->seconds||
       (double)q->seconds*1000>INT_MAX-level.time||!YawVector(entity->r.currentAngles,10000000.0f))return 0;
    VectorCopy(entity->r.currentAngles,target);
    if(axis<0){
        if(!YawVector(target,36000.0f))return 0;
        for(j=0;j<3;j++){float d=(float)(q->velocity[j]-target[j]);while(d>180.0f)d=(float)(d-360.0f);while(d< -180.0f)d=(float)(d+360.0f);target[j]=(float)(target[j]+d);}
    }else target[axis]=(float)(target[axis]+q->yaw);
    if(!YawVector(target,10000000.0f))return 0;
    VectorSubtract(target,entity->r.currentAngles,delta);
    VectorCopy(entity->r.currentAngles,start);
    if(entity->s.apos.trType!=TR_STATIONARY)BG_EvaluateTrajectory(&entity->s.apos,level.time,start);
    if(!YawVector(start,10000000.0f))return 0;
    VectorCopy(start,base);VectorCopy(target,next->endpoint);
    seconds[0]=q->accel;seconds[1]=(float)((double)q->seconds-q->accel-q->decel);seconds[2]=q->decel;
    denominator=2.0*q->seconds-q->accel-q->decel;
    length=axis<0?sqrt((double)delta[0]*delta[0]+(double)delta[1]*delta[1]+(double)delta[2]*delta[2]):sqrt((double)delta[axis]*delta[axis]);
    for(j=0;j<3;j++)direction[j]=length?(float)(delta[j]/length):0;
    speed=(float)(2.0*length/denominator);
    if(!YawFinite(speed))return 0;
    for(i=0;i<3;i++){
        trajectory_t *phase;int duration;
        if(seconds[i]==0)continue;
        duration=(int)((double)seconds[i]*1000.0);if(duration<1)return 0;
        VectorClear(decelDirection);
        if(i==2&&next->phaseCount){
            double magnitude;
            VectorSubtract(target,base,decelDirection);
            magnitude=sqrt((double)decelDirection[0]*decelDirection[0]+(double)decelDirection[1]*decelDirection[1]+(double)decelDirection[2]*decelDirection[2]);
            if(magnitude)for(j=0;j<3;j++)decelDirection[j]=(float)(decelDirection[j]/magnitude);
        }
        phase=&next->phases[next->phaseCount++];phase->trType=i==0?TR_ACCELERATE:i==1?TR_LINEAR_STOP:TR_DECCELERATE;
        phase->trTime=level.time;phase->trDuration=duration;
        for(j=0;j<3;j++){
            float velocity=(i==2&&next->phaseCount>1)?decelDirection[j]*speed:axis<0?direction[j]*speed:j==axis?(delta[axis]>0?speed:delta[axis]<0?-speed:0):0;
            phase->trBase[j]=base[j];
            if(!q->accel&&!q->decel)phase->trDelta[j]=(float)((double)delta[j]*1000.0/duration);
            else phase->trDelta[j]=i==1?(float)((double)velocity*seconds[i]*1000.0/duration):velocity;
            if(!YawFinite(phase->trDelta[j]))return 0;
        }
        BG_EvaluateTrajectory(phase,level.time+duration,base);
    }
    if(!next->phaseCount)return 0;
    next->owner=q->entity;next->trajectory=next->phases[0];return 1;
}
static int ScriptRotationInvoke(const GscYawMotionRequest *input,int (*current)(void *),void *context){
    GscYawMotionRequest request=*input;ScriptMotionAxis next={0},axisBefore;
    gentity_t before,*e;uint64_t epoch=cod1EntityEpoch;int now=level.time,count=level.num_entities,linked;
    if(request.now_ms<0||!YawMap()||!ActorOwnerAllowed(&request.entity,0))return 0;
    e=g_entities+request.entity.slot;before=*e;axisBefore=scriptMotion[request.entity.slot][1];
    if(!e->classname||(strcmp(e->classname,"script_origin")&&strcmp(e->classname,"script_model")&&strcmp(e->classname,"script_brushmodel")))return 0;
    linked=ScriptMotionParent(&request.entity);
    if(!linked&&!ScriptRotationPlan(&request,&before,&next))return 0;
    if(current(context)!=1||!cod1YawGuard.active||cod1YawGuard.cancelled||!YawMap()||
       cod1EntityEpoch!=epoch||level.time!=now||level.num_entities!=count||
       memcmp(input,&request,sizeof(request))||!ActorOwnerAllowed(&request.entity,0)||
       memcmp(e,&before,sizeof(before))||memcmp(&axisBefore,&scriptMotion[request.entity.slot][1],sizeof(axisBefore))||
       ScriptMotionParent(&request.entity)!=linked)return 0;
    if(linked)return 1;
    scriptMotion[request.entity.slot][1]=next;e->s.apos=next.trajectory;
    trap_LinkEntity(e);
    return current(context)==1&&cod1YawGuard.active&&!cod1YawGuard.cancelled&&YawMap()&&
        level.time==now&&cod1EntityEpoch==epoch&&level.num_entities==count&&
        !memcmp(input,&request,sizeof(request))&&ActorOwnerAllowed(&request.entity,0)&&
        e->classname==before.classname&&ScriptMotionParent(&request.entity)==linked&&
        !memcmp(&e->s.apos,&next.trajectory,sizeof(next.trajectory))&&
        !memcmp(&scriptMotion[request.entity.slot][1],&next,sizeof(next));
}
