#ifndef G_COD1_NAVIGATION_ACTOR_INPUT_H
#define G_COD1_NAVIGATION_ACTOR_INPUT_H
/* Read-only current input projection. This does not certify a retained path
 * or transfer ownership from the existing movement implementation. */
typedef struct {cod1ActorLease_t lease;int category,mode,now;uint64_t selection,revision;unsigned char physics[264];float portVelocity[3],gravity;} cod1NavigationActorInput_t;
int G_Cod1NavigationActorInput(const GscEntityRef*,int,cod1NavigationActorInput_t*);
#endif
