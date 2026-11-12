/* Actual selected current root/tag producer for the owned mount physics.
 * The sample callback is read-only and rejects any changed animation/pose. */
typedef struct {int used;cod1TurretMountFitSample_t sample;C1TARecord animation;c1tpStaticPose_t pose;float controls[3];} C1TFitProducer;
static C1TFitProducer cod1TurretFitProducers[C1T_CAPACITY];
static uint64_t cod1TurretFitProducerRevision,cod1TurretMountedBusyEpoch;
static int TurretMountedPrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1TurretMountedBusyEpoch,sizeof(cod1TurretMountedBusyEpoch))||ActorOverlap(p,n,cod1TurretFitProducers,sizeof(cod1TurretFitProducers))||ActorOverlap(p,n,&cod1TurretFitProducerRevision,sizeof(cod1TurretFitProducerRevision));}
static void TurretMountedReset(void){memset(cod1TurretFitProducers,0,sizeof(cod1TurretFitProducers));cod1TurretFitProducerRevision=cod1TurretMountedBusyEpoch=0;}
static void TurretMountedRetire(const GscEntityRef*actor){int i;if(!actor)return;for(i=0;i<C1T_CAPACITY;i++)if(ActorRefEqual(&cod1TurretFitProducers[i].sample.actor,actor))memset(&cod1TurretFitProducers[i],0,sizeof(C1TFitProducer));}
static int TurretMountedTag(const C1TRecord*r,const char*name,float out[16]){
 unsigned i;float axis[3][3];int k;
 for(i=0;i<cod1TurretModel.skeleton.boneCount;i++)if(!strcmp(cod1TurretModel.skeleton.boneName[i],name))break;
 if(i>=r->pose.bones||!C1TP_Axis(r->pose.q[i],axis))return 0;
 memset(out,0,64);for(k=0;k<3;k++){memcpy(out+k*4,axis[k],12);out[12+k]=r->pose.p[i][k];}return 1;
}
static int TurretMountedSample(void*unused,const GscEntityRef*gun,cod1TurretMountFitSample_t*out){
 C1TRecord*r;C1TFitProducer*p;GscEntityRef actor;int i;(void)unused;
 if(!gun||!out||!ActorGoalBuffers(gun,sizeof(*gun),out,sizeof(*out),NULL,0)||(r=TurretFind(gun))==NULL)return 0;
 i=(int)(r-cod1Turrets);p=&cod1TurretFitProducers[i];
 if(!p->used||p->sample.tick!=level.time||!ActorRefEqual(gun,&p->sample.gun)||!TurretMountOwnerCurrent(r,level.time,&actor)||!ActorRefEqual(&actor,&p->sample.actor)||
    !TurretAnimationLive(&cod1TurretAnimations[i],r,level.time)||memcmp(&cod1TurretAnimations[i],&p->animation,sizeof(p->animation))||r->poseTick!=level.time||
    memcmp(&r->pose,&p->pose,sizeof(p->pose))||memcmp(r->controls,p->controls,12))return 0;
 *out=p->sample;return 1;
}
static int TurretMountedFrameCore(const GscEntityRef*gun,int now,int(*current)(void*),void*context){
 C1TRecord*r;C1TFitProducer p;C1TARecord beforeAnimation;c1tpStaticPose_t beforePose;float beforeControls[3];c1att14Tree_t before[2],after[2];c1att14Selection_t selected;
 cod1ActorLease_t lease;GscEntityRef actor;cod1TurretMountFitSample_t existing;float desired,yaw;int index,which;
 if(!gun||!current||!ActorGoalBuffers(gun,sizeof(*gun),NULL,0,NULL,0)||now!=level.time||current(context)!=1||(r=TurretFind(gun))==NULL||
    !TurretMountOwnerCurrent(r,now,&actor)||!TurretMountLease(&actor,&lease)||!TurretPrepare(r,now))return 0;
 index=(int)(r-cod1Turrets);
 if(TurretMountedSample(NULL,gun,&existing))return G_Cod1TurretMountFit(gun,now,TurretMountedSample,current,context);
 if(TurretAnimationActorRead(r,now,before,&which)!=1||which<0||which>1||cod1TurretFitProducerRevision==UINT64_MAX)return 0;
 beforeAnimation=cod1TurretAnimations[index];beforePose=r->pose;memcpy(beforeControls,r->controls,12);
 memcpy(after,before,sizeof(after));memset(&p,0,sizeof(p));p.sample.gun=*gun;p.sample.actor=actor;p.sample.tick=now;
 if(!TurretMountedTag(r,"tag_weapon",p.sample.tagWeapon)||!TurretMountedTag(r,"tag_aim",p.sample.tagAim))return 0;
 desired=cod1Actors[lease.actorIndex].turretMount.relativeOrigin[2]-p.sample.tagWeapon[14];
 yaw=C1TM_Degrees(atan2((double)p.sample.tagWeapon[1],(double)p.sample.tagWeapon[0]));
 if(!C1ATT14_Select(&after[which],yaw,cod1TurretWeapon.animHorRotateIncrement,desired,&selected))return 0;
 p.sample.endpoint=selected.endpoint;p.sample.endpointRootZ=selected.rowHeight[selected.lowerRow];
 memcpy(p.sample.rootRotation,selected.root.q2,8);memcpy(p.sample.rootTranslation,selected.root.translation,12);
 if(current(context)!=1||now!=level.time||!TurretMountOwnerCurrent(r,now,&actor)||!ActorRefEqual(&actor,&p.sample.actor)||
    !TurretAnimationLive(&cod1TurretAnimations[index],r,now)||memcmp(&cod1TurretAnimations[index],&beforeAnimation,sizeof(beforeAnimation))||
    memcmp(&r->pose,&beforePose,sizeof(beforePose))||memcmp(r->controls,beforeControls,12)||!TurretAnimationActorCommit(r,now,before,after))return 0;
 p.sample.revision=++cod1TurretFitProducerRevision;p.animation=cod1TurretAnimations[index];p.pose=r->pose;memcpy(p.controls,r->controls,12);p.used=1;cod1TurretFitProducers[index]=p;
 return G_Cod1TurretMountFit(gun,now,TurretMountedSample,current,context);
}

typedef struct {const GscEntityRef*input;GscEntityRef saved;int now;int(*current)(void*);void*context;} C1TMountedCall;
static int TurretMountedCallerCurrent(void*opaque){C1TMountedCall*c=opaque;
 if(!c||memcmp(c->input,&c->saved,sizeof(c->saved))||!G_Cod1EntitySameAllocation(&c->saved)||!ActorGoalBuffers(c->input,sizeof(*c->input),NULL,0,NULL,0)||c->now!=level.time||c->current(c->context)!=1)return 0;
 return !memcmp(c->input,&c->saved,sizeof(c->saved))&&G_Cod1EntitySameAllocation(&c->saved)&&ActorGoalBuffers(c->input,sizeof(*c->input),NULL,0,NULL,0)&&c->now==level.time&&cod1EntityReady&&!level.spawning&&!level.restarted;
}
int G_Cod1TurretMountedFrame(const GscEntityRef*gun,int now,int(*current)(void*),void*context){
 uint64_t epoch=cod1EntityEpoch;int ok;C1TMountedCall call;
 if(!epoch||!gun||!current||!ActorGoalBuffers(gun,sizeof(*gun),NULL,0,NULL,0)||cod1TurretMountedBusyEpoch||!cod1EntityReady||level.spawning||level.restarted||now!=level.time)return 0;
 call.input=gun;call.saved=*gun;call.now=now;call.current=current;call.context=context;
 cod1TurretMountedBusyEpoch=epoch;ok=TurretMountedFrameCore(&call.saved,now,TurretMountedCallerCurrent,&call);
 if(ok)ok=TurretActorPublish(&call.saved,now,TurretMountedCallerCurrent,&call);
 if(epoch!=cod1EntityEpoch||!cod1EntityReady||level.spawning||level.restarted||now!=level.time)ok=0;
 if(cod1TurretMountedBusyEpoch==epoch)cod1TurretMountedBusyEpoch=0;return ok;
}
