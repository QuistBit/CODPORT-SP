#include "../g_cod1_navigation_actor_input.h"
int G_Cod1NavigationActorInput(const GscEntityRef*ref,int now,cod1NavigationActorInput_t*out){
 cod1NavigationActorInput_t v;cod1ActorLease_t lease;const cod1ActorRecord_t*a;
 if(!ref||!out||now!=level.time||!ActorGoalBuffers(ref,sizeof(*ref),out,sizeof(*out),NULL,0)||!ActorGoalLease(ref,&lease)||!ActorOrdinaryClockOwner(cod1GenericActors+lease.actorIndex))return 0;
 a=cod1Actors+lease.actorIndex;memset(&v,0,sizeof(v));v.lease=lease;v.now=now;v.category=a->scriptedController.currentCategory;v.mode=a->nativeMode264;
 v.selection=a->scriptedController.selectionSerial;v.revision=a->scriptedController.revision;v.gravity=g_gravity.value;
 memcpy(v.physics,a->defaults.lifeWords,264);memcpy(v.portVelocity,a->motion.velocity,12);*out=v;return 1;
}
