/* Actual R45D1 fence; demonstrate a stronger mutation model than the
 * public provider contract. This is NOT a reachable GAME exploit claim. */
#define main R45D1FixtureMain
#include "opt_r45d_face_callback_test.c"
#undef main

static void change(C1GARecord *r,unsigned kind){
 switch(kind){
 case 0:r->nodes[17].cycle++;break;
 case 1:r->notify->pending[23]++;break;
 case 2:r->names[0].text[0]^=1;break;
 default:assert(0);
 }
}
int main(void){
 unsigned writable,kind;Control q={0};
 for(writable=0;writable<2;writable++)for(kind=0;kind<3;kind++){
  C1GARecord parent,independent;C1FaceFrameCall*c;
  setup();assert(ActorGenericClone(&parent,cod1GenericActors));
  c=ActorFaceFrameAcquire(&input,100,current,&q,(int)writable);assert(c);
  assert(ActorFaceFrameFence(c));independent=c->baseline;
  /* Only live changes: both independent and borrowed comparisons reject. */
  change(cod1GenericActors,kind);assert(!ActorFaceFrameFence(c));
  c->baseline=parent;assert(!ActorFaceFrameFence(c));c->baseline=independent;
  /* Both live and hypothetical parent change together, without revision. */
  change(&parent,kind);assert(!ActorFaceFrameFence(c));
  c->baseline=parent;assert(ActorFaceFrameFence(c));c->baseline=independent;
  /* Restore ownership before unwind: this is a predicate experiment,
   * not a proposed borrowing implementation. */
  ActorFaceFrameLeave(c);ActorGenericFreeOwned(&parent);destroy();
 }
 puts("R45D3: 6 independent-vs-shared fence counterexamples reproduced");
 return 0;
}
