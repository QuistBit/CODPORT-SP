/* R76: isolated, opt-in, native-DLL GSC test. NOT original map scripting.
 * Calls are no-ops when COD1_GSC_TEST_WIP is disabled. No game/save/protocol struct is extended. */
#ifndef G_COD1_GSC_TEST_H
#define G_COD1_GSC_TEST_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include <stdint.h>
#include <stddef.h>
#include "gsc_host/g_cod1_campaign_features47.h"
/* Typed feature read: live legacy or explicitly admitted map-owned lane. */
int G_Cod1GscFeature47(c1cp47Feature feature);
struct gentity_s;
/* DATA receipt for the exact current early compiled Session; no entry permit. */
uint64_t G_Cod1GscTestEarlyProgramIdentity(uint64_t epoch,uint64_t receipt);
void G_Cod1GscCampaignMapProfile(void);
void G_Cod1GscCampaignBegin47(int restart);
void G_Cod1GscCampaignObserve47(const char *probedMap);
void G_Cod1GscCampaignRetire47(void);
int G_Cod1GscTestPrepareBeforeBsp( int levelTime );
int G_Cod1GscTestPreparePlacedResources(void);
struct cod1ActorBspConstructionOwner_s;
int G_Cod1GscTestConstructBspActor(const void *,void *,char *,size_t);
void G_Cod1GscTestInitAfterMapBootstrap( int levelTime );
int G_Cod1GscTestGraphBeforePlayer(int);
int G_Cod1GscTestGraphCommand(const char *);
void G_Cod1GscSoundDurationReply(int clientNum);
void G_Cod1GscLoopSoundReply(int clientNum);
void G_Cod1GscLoopSoundResync(int clientNum);
void G_Cod1GscLoopSoundFrame(void);
void G_Cod1GscVoiceReply(int clientNum);
void G_Cod1LookAtRunFrame(void);
void G_Cod1GscTestRunFrame( int levelTime );
int G_Cod1GscTestRunFrameSlice(int levelTime,int (*expired)(void));
void G_Cod1GscTestShutdown( void );
void G_Cod1GscTestBeforeLoad( void );
/* Unsupported VM state must never be accepted as an RTCW raw save. */
int G_Cod1GscTestBlocksRawSave(void);
/* Selection is also queried during map spawn, before the VM is created. */
int G_Cod1GscTestEntityEventsEnabled( void );
/* Pure mode-5/6/7/8/9/10/11/12 runtime gate; no trap calls in the earlier modes. */
int G_Cod1GscTestUseSelectionActive( void );
/* Constructor mode must still match the cvar across selector callbacks. */
int G_Cod1GscTestUseSelectionModeCurrent( int mode );
/* Marks current matching waits ready; never executes a VM continuation here. */
int G_Cod1GscTestEntityEvent( struct gentity_s *owner, const char *event );
/* Contact/use also keep the initiating player alive across selector traps. */
int G_Cod1GscTestEntityTouchEvent( struct gentity_s *owner, struct gentity_s *player );
int G_Cod1GscTestEntityUseEvent( struct gentity_s *owner, struct gentity_s *player );
/* Called only after the real ClientBegin/ClientSpawn has completed. */
void G_Cod1GscTestClientSpawned( struct gentity_s *player );
#else
#define G_Cod1GscCampaignMapProfile() ((void)0)
#define G_Cod1GscCampaignBegin47(restart) ((void)0)
#define G_Cod1GscCampaignObserve47(map) ((void)0)
#define G_Cod1GscCampaignRetire47() ((void)0)
#define G_Cod1GscTestPrepareBeforeBsp(t) (1)
#define G_Cod1GscTestGraphBeforePlayer(n) (1)
#define G_Cod1GscTestGraphCommand(n) (0)
#define G_Cod1GscTestInitAfterMapBootstrap(t) ((void)0)
#define G_Cod1GscSoundDurationReply(n) ((void)0)
#define G_Cod1GscLoopSoundReply(n) ((void)0)
#define G_Cod1GscLoopSoundResync(n) ((void)0)
#define G_Cod1GscLoopSoundFrame() ((void)0)
#define G_Cod1GscVoiceReply(n) ((void)0)
#define G_Cod1GscTestPreparePlacedResources() (1)
#define G_Cod1GscTestRunFrame(t) ((void)0)
#define G_Cod1GscTestRunFrameSlice(t, expired) 1
#define G_Cod1GscTestShutdown() ((void)0)
#define G_Cod1GscTestBeforeLoad() ((void)0)
#define G_Cod1GscTestBlocksRawSave() (0)
#define G_Cod1GscTestEntityEventsEnabled() (0)
#define G_Cod1GscTestUseSelectionActive() (0)
#define G_Cod1GscTestUseSelectionModeCurrent(mode) (0)
#define G_Cod1GscTestEntityEvent(owner,event) (0)
#define G_Cod1GscTestEntityTouchEvent(owner,player) (0)
#define G_Cod1GscTestEntityUseEvent(owner,player) (0)
#define G_Cod1GscTestClientSpawned(player) ((void)0)
#endif
#endif
