#include "../../qcommon/cod1_actor_source_pose14.h"
static int ActorSourcePoseSample(const GscEntityRef*input,int now,c1asp14Packet_t*out,int(*current)(void*),void*ctx){
 C1FaceFrameCall*c=NULL;c1asp14Packet_t packet;c1asp14Decoded_t decoded;unsigned i;int ok=0,jumper=0;uint64_t jumpRevision=0;
 if(!input||!out||!current||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),NULL,0)||ActorGenericPrivate(out,sizeof(*out)))return 0;
 c=ActorFaceFrameAcquire(input,now,current,ctx,0);if(!c)return 0;
 jumper=TrainingJumperAllocation(input);if(jumper)jumpRevision=cod1JumperResources.revision;
 if(!c->frame.scripted||c->saved.tick!=now||
    !(jumper?ActorFaceFrameClosure(&c->baseline):ActorLocomotionClosure(&c->baseline)))goto done;
 if(!C1ASP14_CollectDecoded(cod1GenericDefinitions,c->baseline.nodes,C1GA_COUNT,&packet,&decoded))goto done;
 for(i=0;i<decoded.leaves;i++){
  unsigned resource=decoded.resource[i],source=c1asp14Source[resource];
  if(strcmp(cod1GenericNames[source].name,c1asp14Name[resource]))goto done;
  if(source==892){if(!jumper||!ActorJumperReady(c->epoch)||!ActorJumperValid(cod1JumperResources.root,cod1JumperResources.body))goto done;}
  else if(!ActorLocomotionBody(source)||!ActorLocomotionRoot(source))goto done;
 }
 if(!ActorPronePosePacket(c->actor,now,packet.controller))goto done;
 if(current(ctx)!=1||!ActorFaceFrameFence(c)||
    (jumper&&(!TrainingJumperAllocation(input)||!ActorJumperReady(c->epoch)||cod1JumperResources.revision!=jumpRevision)))goto done;
 *out=packet;ok=1;
done:ActorFaceFrameLeave(c);return ok;
}

/* Extend the verified run profile only when the actual body subtree has a
 * reachable current leaf. An absent idle producer is not a source-pose proof.
 * Invalid/unknown source trees refuse; they do not select the old profile. */
static int ActorSourcePoseSelection(const C1GARecord*r){
 c1asp14Packet_t p;unsigned i,n;
 if(ActorGeneralClockActive(r))return 1;
 if(!r||!r->nodes||!ActorLocomotionReady(cod1EntityEpoch)||strcmp(cod1GenericNames[5].name,"body")||
    !C1ASP14_Collect(cod1GenericDefinitions,r->nodes,C1GA_COUNT,&p))return -1;
 for(i=1;i<p.count;i++)if(!c1asp14Definition[p.source[i]].children){
  n=p.source[i];while(n&&n!=5)n=c1asp14Definition[n].parent;if(n==5)return 1;
 }
 return 0;
}

/* The owned original run tree now drives the exit pose too. Movement and
 * its current graph path retain their separate lifetime/assignment owner. */
static int ActorJumperSourcePoseSelected(const C1GARecord*r){
 const cod1ActorRecord_t*a;const c1atl14Node_t*n;
 if(!r||!r->nodes||!TrainingJumperAllocation(&r->lease.entity)||
    memcmp(&r->lease,&trainingJumperOwner,sizeof(r->lease)))return -1;
 a=cod1Actors+r->lease.actorIndex;n=r->nodes+892;
 if(!C1ASP14_Unit(n->currentWeight)||!C1ASP14_Unit(n->goalWeight)||!C1ASP14_Unit(n->phase))return -1;
 if(a->motion.active){
  if(a->motion.busy||a->motion.model||!a->goal.assigned||a->motion.assignment!=a->goal.assignment||
     a->motion.route.phase!=2||!TrainingJumperExitNode(&r->lease.entity,a->goal.assignment,&a->goal.node)||
     !ActorLocomotionActive(r)||!ActorLocomotionClosure(r))return -1;
  return 1;
 }
 if(a->goal.assigned&&!TrainingJumperExitNode(&r->lease.entity,a->goal.assignment,&a->goal.node))return -1;
 if(!n->currentWeight||!n->goalWeight)return 0;
 return 1;
}
