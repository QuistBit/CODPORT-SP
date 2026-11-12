#ifndef G_COD1_ACTOR_SCRIPTED_CONTROLLER_H
#define G_COD1_ACTOR_SCRIPTED_CONTROLLER_H
/* WORK ONLY. Public API typedefs are now genuinely present in S243's
 * canonical gsc_fixture.h. Put the metadata typedef before the Actor record
 * member, avoiding a second owner/current stack or duplicate shared types. */
#include "cod1_gsc/gsc_fixture.h"
typedef struct {
 uint64_t actorSerial,revision,runtime,catalog,initTask,selectionSerial,lastSelectionFrame;
 int initialized,entered,invalid,entered_ms;
 int timer28,timer2c,timer34,base_entered_ms;
 int force250,currentSource,currentCategory,resumed4;
} cod1ActorScriptedController_t;
enum {C1CP_NONE=0,C1CP_CLOCK_SELECT=1,C1CP_RECORD_UPDATE=2,C1CP_FRAME_ENTER=3};
/* Internal prototypes must precede provider implementations. */
static int ActorControllerGenericSourceEnter(const GscEntityRef*,uint64_t,uint64_t,int,const void*,size_t,int(*)(void*),void*,const void*,int);
static int ActorControllerGenericSourceCurrent(void*);
static void*ActorControllerGenericSourceContext(void);
static int ActorControllerGenericSourceFinish(int,int);
static int ActorControllerAnimationPrivate(const void*,size_t);
static int ActorControllerAnimationMutationAllowed(void);
static int ActorControllerAnimationGenericAllowed(void);
static int ActorControllerAnimationFrameEnter(const GscEntityRef*,int,int(*)(void*),void*);
static void ActorControllerAnimationCancelOwner(const GscEntityRef*);
static void ActorControllerAnimationReset(void);
static int ActorControllerProviderEnter(int,const GscEntityRef*,int,int(*)(void*),void*);
static int ActorControllerProviderArmFrame(int,const GscEntityRef*,int,int(*)(void*),void*);
static int ActorControllerForeignMutationAllowed(void);
static int ActorControllerPrivate(const void*,size_t);
static void ActorControllerFresh(uint32_t,uint64_t);
static void ActorControllerRetire(uint32_t,const GscEntityRef*);
static void ActorControllerReset(void);
const GscActorScriptedControllerHost *G_Cod1ActorScriptedControllerHost(void);
int G_Cod1ActorSelectWithCleanup(void*,const GscActorControllerRequest*,GscActorControllerSelection*,char*,size_t,int(*)(void*),void*,const GscMainReplacementService*);
#endif
