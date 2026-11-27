#ifndef COD1_PHASE_PROFILE46_H
#define COD1_PHASE_PROFILE46_H
/* Capture labels before the operation; discard cross-frame/epoch samples. */
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
typedef struct {uint64_t start,epoch;int now,frame;unsigned owner;} C1PhaseProfile46;
static C1PhaseProfile46 ActorPhaseProfileStart46(const GscEntityRef*ref,int now){
 C1PhaseProfile46 t={0};
 t.start=ActorProfileStart31();
 if(t.start){t.epoch=cod1EntityEpoch;t.now=now;t.frame=level.framenum;t.owner=ref?ref->slot:C1AP31_OWNERS;}
 return t;
}
static void ActorPhaseProfileEnd46(unsigned stage,C1PhaseProfile46 t,int ok){
 if(t.start&&t.epoch==cod1EntityEpoch&&t.now==level.time&&t.frame==level.framenum)
  ActorProfileFinish31(stage,t.start,t.epoch,t.now,t.owner,ok,0,0,0,0);
}
#endif
#endif
