/* Isolated GSC boundary in the unified BSP build. The legacy scalar/fog
 * session remains the default. R84 optionally enables checked entity values,
 * lookup, spawn and nonsolid positioning through the engine host. R85 mode 2
 * additionally enables checked model assignment and origin/angles properties.
 * Keep this independent of water, materials, actor/UI and RTCW .script.
 * Native DLL only; below game/ to stay OUT of root *.c save-function scan.
 *
 * Enable the build AND set g_cod1GscTest=1 before loading a fresh SP map.
 * Earlier modes admit explicitly selected scripts/gsc_tests/ GSC files.
 * R95 mode 10 also admits selected maps/ scripts and their bounded modules.
 * Turning off stops the session; changing a path/enabling needs a fresh map.
 * Entity mode requires g_cod1GscEntityTest=1 before a fresh map; access mode
 * uses value 2; R86 entity events use value 3; R88 script state uses value 4.
 * R89 mode 5 adds the shared brush USE selector and authoritative hint.
 * R91 mode 6 adds metadata, visibility and admitted trigger positioning.
 * R92 mode 7 adds bounded statement control flow and scalar mutation.
 * R93 mode 8 adds bounded VM arrays and checked entity collections.
 * R94 mode 9 adds resumable script expressions and typed function results.
 * R95 mode 10 adds canonical, bounded module dependency loading.
 * R96 mode 11 adds typed event payloads and local waittill output variables.
 * R97 mode 12 additionally accepts guarded native combat notifications.
 * R107 mode 14 adds canonical function names and checked mounted-alias music.
 * R108 mode 15 adds original initialization-only shader/model precache.
 * R109 mode 16 adds map-owned mission objective records.
 * R110 mode 17 additionally adds separately gated console output.
 * R111 mode 18 additionally adds bounded normal/bold game HUD messages.
 * R112 mode 19 adds map-owned diagnostic mission request intent only.
 * R113 mode 20 additionally selects checked nonactor script radius damage.
 * R114 mode 21 adds a paired, map-owned script earthquake command channel.
 * R115 mode 22 adds typed loadfx/playfx and a separate FX resource scene.
 * R116 mode 23 adds separately selected REGISTER/SET typed setcvar phases.
 * R117 mode 24 adds startup-only server shellshock name registration.
 * R118 mode 25 adds a real paired client world-text renderer (port policy).
 * R119 mode 26 appends a paired world-line renderer, optional integer zero only.
 * R120 mode 27 appends a separate owned ambience stream.
 * R121 mode 28 appends source-backed native item precache resources/state.
 * R123 mode 29 appends the health/getter-backed pending saveGame request queue.
 * R130 mode 30 selects the cumulative tag-FX/native-query/startup-localized
 * string constructor. Tag-FX uses the genuine topology provider and pending
 * effect lane; current-pose rendering/client admission remain unavailable.
 * R131 mode 31 adds a genuine complete copied getspawnerarray query.
 * R133 mode 32 adds only literal-int32 randomint call-site capability.
 * R134 mode 33 adds only exact-dyadic literal randomfloat call-site capability.
 * R135 mode 34 adds copied literal getkeybinding/current CGAME cache queries.
 * R136 mode 35 additionally selects proved computed-int32 randomint operands.
 * Separate constructors retain
 * the earlier modes' admitted capabilities.
 * No original mission discovery, full Entity API or VM save/restore. */
#include "../g_cod1_gsc_test.h"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "../g_cod1_gsc_combat.h"
#if defined(Q3_VM)
#error R76 requires a native game DLL, not a QVM
#endif
#if !defined(COD1_DIRECT_FOG_WIP) || !COD1_DIRECT_FOG_WIP
#error R76 requires the existing direct-fog game/cgame/renderer build profile
#endif
#include "../g_local.h"
#include "../g_cod1_entity_host.h"
#include "g_cod1_frame_profile35.h"
#include "g_cod1_campaign_config47.h"
#include "g_cod1_campaign_lane47.h"
#include "../g_cod1_player_lifetime.h"
#include "../bg_cod1_direct_fog.h"
#include "../cod1_gsc/gsc_session.h"
#include "../g_cod1_node_host.h"
#include "../g_cod1_vehicle_nodes.h"
#include "../g_cod1_spawner_pose.h"
#include "../g_cod1_actor.h"
#include "../g_cod1_gsc_event_payload.h"
#include <stdlib.h>
#include <string.h>

#ifndef COD1_GSC_VFS_CALLOC
#define COD1_GSC_VFS_CALLOC calloc
#endif
#ifndef COD1_GSC_VFS_FREE
#define COD1_GSC_VFS_FREE free
#endif
#define GSC_TEST_ROOT "scripts/gsc_tests/"
#define GSC_TEST_DEFAULT GSC_TEST_ROOT "r76_fog.gsc"

static GscSession *gscTestSession;
/* Failed unpublished factory cleanup is unresolved even after NULL return. */
static int gscTestRawSaveUnresolved;
static void GscTestDestroyUnpublished(GscSession*session){
    if(session&&!gsc_session_destroy(session,NULL,0))gscTestRawSaveUnresolved=1;
}
static uint64_t gscTestId;
static uint64_t gscTestMapEpoch; /* modes30/31 bootstrap lease; older modes do not read it */
static uint64_t gscTestActorPrefixEpoch; /* selected owner only; never adopts a later map */
static int gscTestBusy, gscTestStopRequested, gscTestCompleted;
static vmCvar_t gscTestEnabled, gscTestScript, gscTestEntities, gscTestAiArrays, gscTestActorPrefix, gscTestDeleteTransactions;
static int gscTestEntityMode;
static int gscTestDeleteTransactionsSelected;
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
static int GscTestGrenadeStorageProtected(const void *,size_t);
static int GscMapItemStorageProtected(const void *,size_t);
static int GscTestCarbineStorageProtected(const void *,size_t);
static int GscTestTurretStorageProtected(const void *,size_t);
#endif

/* Native profile belongs to one map; the old lane retains each exact live
 * read. No runtime cvar toggle can switch an existing Session's lane. */
static c1cp47Owner gscCampaignOwner47;
static int GscCampaignLane47(void){
 if(!gscCampaignOwner47.native)return -1;
 return C1CP47_Lane(&gscCampaignOwner47,G_Cod1EntityMapEpoch(),
  g_gametype.integer==GT_SINGLE_PLAYER,G_Cod1MapEntities(),level.restarted);
}
int G_Cod1GscFeature47(c1cp47Feature feature){
 return C1CP47_LaneInteger(GscCampaignLane47(),gscCampaignOwner47.profile,feature,trap_Cvar_VariableIntegerValue);
}
static void GscConfigString47(unsigned key,char *out,int cap){
 C1CP47_LaneString(C1CP47_MapOwnedKey(key)?GscCampaignLane47():-1,
  gscCampaignOwner47.profile,key,out,cap,trap_Cvar_VariableStringBuffer);
}
/* Live map/save validation at native init and frame admission, not on
 * each immutable feature read. Do not retire a newly created epoch. */
static int GscCampaignNativeLive47(void){
 uint64_t epoch;char map[MAX_QPATH];int save;
 if(!gscCampaignOwner47.native)return 1;
 epoch=gscCampaignOwner47.epoch;
 if(GscCampaignLane47()!=1)goto revoked;
 memset(map,0,sizeof(map));trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
 if(GscCampaignLane47()!=1||gscCampaignOwner47.epoch!=epoch)goto revoked;
 save=trap_Cvar_VariableIntegerValue("savegame_loading");
 if(gscCampaignOwner47.epoch==epoch&&GscCampaignLane47()==1&&memchr(map,0,sizeof(map))&&
    C1CP47_OwnerCurrent(&gscCampaignOwner47,epoch,map,g_gametype.integer==GT_SINGLE_PLAYER,
      G_Cod1MapEntities(),level.restarted,save))return 1;
 revoked:
 if(gscCampaignOwner47.epoch==epoch&&gscCampaignOwner47.native)C1CP47_OwnerRetire(&gscCampaignOwner47);
 return 0;
}
static int GscConfigInteger47(unsigned key){
 if(key<C1CP47_F_COUNT)return G_Cod1GscFeature47((c1cp47Feature)key);
 return key==C1CP47_K_SAVE?trap_Cvar_VariableIntegerValue("savegame_loading"):0;
}

/* R47I: normal fresh SP map admission is automatic; explicit legacy
 * settings still select the historical diagnostic lane. */
void G_Cod1GscCampaignRetire47(void){C1CP47_OwnerRetire(&gscCampaignOwner47);}
void G_Cod1GscCampaignBegin47(int restart){
 (void)C1CP47_OwnerArm(&gscCampaignOwner47,G_Cod1EntityMapEpoch(),restart);
}
static int GscCampaignPending47(uint64_t epoch){
 return epoch==G_Cod1EntityMapEpoch()&&C1CP47_OwnerPending(&gscCampaignOwner47,epoch);
}
void G_Cod1GscCampaignObserve47(const char *probed){
 uint64_t epoch=G_Cod1EntityMapEpoch();
 char value[MAX_CVAR_VALUE_STRING],live[MAX_QPATH];
 unsigned i;int save,dedicated,requested,legacy=0;
 const char *conflict=NULL;
 if(!GscCampaignPending47(epoch))return;
 /* The engine returns an empty string for an unregistered cvar. An
  * integer read would collapse absence and an explicit diagnostic opt-out. */
 memset(value,0,sizeof(value));
 trap_Cvar_VariableStringBuffer("g_cod1CampaignProfile",value,sizeof(value));
 if(!GscCampaignPending47(epoch))return;
 requested=memchr(value,0,sizeof(value))!=NULL&&C1CP47_NativeRequested(value);
 if(!probed||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||
    level.restarted||gscTestSession||gscTestBusy||
    (strcmp(probed,"training")&&strcmp(probed,"us_intro"))){
  (void)C1CP47_OwnerAdmit(&gscCampaignOwner47,epoch,NULL,NULL,0,0,0,0,0,0,1);
  if(requested==1&&(value[0]||(probed&&(!strcmp(probed,"training")||!strcmp(probed,"us_intro")))))
   G_Printf("[Campaign R47I] native profile not admitted: map/format/SP or Session boundary\n");
  return;
 }
 /* Explicit legacy selections win. Read raw values once at admission,
  * never normalize a diagnostic setting or write a cvar. */
 for(i=0;i<C1CP47_F_COUNT;i++){
  memset(value,0,sizeof(value));
  C1CP47_ReadLegacyString(i,value,sizeof(value),trap_Cvar_VariableStringBuffer);
  if(!GscCampaignPending47(epoch))return;
  if(!memchr(value,0,sizeof(value))||!C1CP47_DefaultFeatureString(value)){legacy=1;conflict=C1CP47_KeyName(i);break;}
 }
 if(!legacy){
  memset(value,0,sizeof(value));
  C1CP47_ReadLegacyString(C1CP47_K_ROOT,value,sizeof(value),trap_Cvar_VariableStringBuffer);
  if(!GscCampaignPending47(epoch))return;
  legacy=!memchr(value,0,sizeof(value))||!C1CP47_DefaultRootString(value);
  if(legacy)conflict=C1CP47_KeyName(C1CP47_K_ROOT);
 }
 dedicated=trap_Cvar_VariableIntegerValue("dedicated");
 if(!GscCampaignPending47(epoch))return;
 save=trap_Cvar_VariableIntegerValue("savegame_loading");
 if(!GscCampaignPending47(epoch))return;
 memset(live,0,sizeof(live));
 trap_Cvar_VariableStringBuffer("mapname",live,sizeof(live));
 if(!GscCampaignPending47(epoch))return;
 if(C1CP47_OwnerAdmit(&gscCampaignOwner47,epoch,probed,memchr(live,0,sizeof(live))?live:NULL,
    g_gametype.integer==GT_SINGLE_PLAYER,G_Cod1MapEntities(),level.restarted,
    save,dedicated,legacy,gscTestSession!=NULL||gscTestBusy)&&
    C1CP47_OwnerCurrent(&gscCampaignOwner47,epoch,live,g_gametype.integer==GT_SINGLE_PLAYER,G_Cod1MapEntities(),level.restarted,save)){
  if(C1CP47_OwnerActivate(&gscCampaignOwner47,epoch,live,g_gametype.integer==GT_SINGLE_PLAYER,
       G_Cod1MapEntities(),level.restarted,save,requested))
   G_Printf("[Campaign R47E] native map profile=%s epoch=%llu\n",live,(unsigned long long)epoch);
  else G_DPrintf("[Campaign R47E] prospective profile map=%s epoch=%llu (not activated)\n",live,(unsigned long long)epoch);
 }else if(requested==1){
  if(conflict)G_Printf("[Campaign R47I] native profile not admitted: legacy configuration %s\n",conflict);
  else G_Printf("[Campaign R47I] native profile not admitted: live map/save/dedicated or Session boundary\n");
 }
}

static int GscTestEntityModeValue( void ) {
    int mode = G_Cod1GscFeature47(C1CP47_F_MODE);
    return mode >= 1 && mode<=47 ? mode : 0;
}

static int GscTestEventMode( int mode ) {
    return mode >= 3 && mode<=43;
}

static int GscTestEventModeEnabled( int mode ) {
    /* Brush spawn queries this gate before a session exists. After creation,
     * crossing the new controls boundary must stop direct TOUCH/USE as well
     * as selection. Retain the older modes' gate and cvar call sequence. */
    return GscTestEventMode(mode) && (!gscTestSession ||
        (mode < 6 && gscTestEntityMode < 6) || mode == gscTestEntityMode);
}

/* Pure cached gate: old modes take no new cvar or collision callbacks. */
static int GscTestEarlyPending(void);
int G_Cod1GscTestUseSelectionActive( void ) {
    return gscTestSession && !gscTestCompleted && !gscTestBusy && !GscTestEarlyPending() &&
        !gscTestStopRequested && gscTestEntityMode >= 5 && gscTestEntityMode<=43 &&
        ((gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))) || (gscTestMapEpoch && gscTestMapEpoch==G_Cod1EntityMapEpoch()));
}

/* Compare with the constructor captured at map bootstrap. A cvar change
 * between modes 5 through 31 must not lend a session new capabilities. */
int G_Cod1GscTestUseSelectionModeCurrent( int mode ) {
    return G_Cod1GscTestUseSelectionActive() && mode == gscTestEntityMode;
}

int G_Cod1GscTestEntityEventsEnabled( void ) {
    return G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER &&
        G_Cod1GscFeature47(C1CP47_F_ENABLE) == 1 &&
        GscTestEventModeEnabled(GscTestEntityModeValue()) &&
        trap_Cvar_VariableIntegerValue("savegame_loading") == 0 &&
        ((gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))) || (gscTestMapEpoch && gscTestMapEpoch==G_Cod1EntityMapEpoch()));
}

static void GscTestError( char *error, size_t capacity, const char *message ) {
    if (error && capacity) {
        size_t length = strlen(message);
        if (length >= capacity) length = capacity - 1;
        memcpy(error, message, length); error[length] = '\0';
    }
}

/* Only the new constructor performs multiple reads. Check its captured mode
 * at read/effect boundaries; earlier constructors retain their trap order.
 * Bounded repeated reads are not an atomic snapshot of separate cvars. */
static int GscTestEarlyCurrent(void);
static int gscTestPreparingEarly;
#include "../cod1_gsc/gsc_training_readonly.h"
static int GscTestModuleReadCurrent( void ) {
    if ( gscTestPreparingEarly && !GscTestEarlyCurrent() ) {
        return 0;
    }
    if ( gscTestStopRequested || gscTestEntityMode < 10 || gscTestEntityMode > 47 ) {
        return 0;
    }
    if ( gscTestEntityMode >= 30 &&
         ( !gscTestMapEpoch || gscTestMapEpoch != G_Cod1EntityMapEpoch() ) ) {
        return 0;
    }
    /* These three engine queries are synchronous pure reads. Each invocation
     * of this predicate, including after a host callback, reads live values. */
    return G_Cod1GscFeature47(C1CP47_F_ENABLE) == 1 &&
           GscTestEntityModeValue() == gscTestEntityMode &&
           trap_Cvar_VariableIntegerValue( "savegame_loading" ) == 0;
}

#include "g_cod1_music_host.inc"
#include "g_cod1_resource_host.inc"
#include "g_cod1_objective_host.inc"
#include "g_cod1_console_host.inc"
#include "g_cod1_hud_host.inc"
#include "g_cod1_mission_host.inc"
#include "g_cod1_blast_events.inc"
#include "g_cod1_blast_host.inc"
#include "g_cod1_quake_host.inc"
#include "g_cod1_hud_elements_host.inc"
#include "../g_cod1_r200_collision.h"
#include "g_cod1_r200_collision_host.inc"
#include "g_cod1_script_sound_notify_host.inc"
#include "g_cod1_vehicle_host.inc"
static int gscVehicleFramesSelected;
static int GscCampaignProgramCurrent(uint64_t epoch);
static int GscVehicleMapCurrent(uint64_t epoch){return epoch&&epoch==G_Cod1EntityMapEpoch()&&g_gametype.integer==GT_SINGLE_PLAYER&&!level.spawning&&!level.restarted&&G_Cod1MapEntities()&&G_Cod1EntityCollectionReady(epoch,level.num_entities);}
#include "g_cod1_loop_sound_host.inc"
#include "g_cod1_loop_delivery_host.inc"
#include "g_cod1_fx_host.inc"
#include "g_cod1_loop_fx_host.inc"
#include "g_cod1_cvar_host.inc"
#include "g_cod1_shellshock_host.inc"
#include "g_cod1_debug_text_host.inc"
#include "g_cod1_debug_line_host.inc"
#include "g_cod1_ambient_host.inc"
#include "g_cod1_item_precache_host.inc"
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
#define COD1_REGISTRATION_EXTRA_STORAGE(p,n) (GscMapItemStorageProtected(p,n)||GscTestGrenadeStorageProtected(p,n)||GscTestCarbineStorageProtected(p,n)||GscTestTurretStorageProtected(p,n))
#endif
#include "g_cod1_weapon_registration.inc"
#include "g_cod1_registered_clip_ammo.inc"
#include "g_cod1_save_game_host.inc"
#include "g_cod1_tag_fx_host.inc"
#include "g_cod1_native_queries_host.inc"
#include "g_cod1_precache_string_host.inc"
#include "g_cod1_spawner_array_host.inc"
#include "g_cod1_entity_query_host.inc"
#include "../cod1_gsc/gsc_randomint_payload.inc"
#include "g_cod1_randomint_host.inc"
#include "../cod1_gsc/gsc_randomfloat_payload.inc"
#include "g_cod1_randomfloat_host.inc"
#include "../cod1_gsc/gsc_keybinding_payload.inc"
#include "g_cod1_keybinding_host.inc"

/* Keep all earlier constructor selections and their trap order intact.
 * Mode30 is an explicit cumulative constructor; every host remains fenced by
 * its existing map/session/time owner and the exact captured mode. */
static GscSession *GscTestCreateMode30(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode30 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    return gsc_session_create_with_precache_string(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,error,capacity);
}

/* Explicit new constructor; mode30 body above remains byte-identical. */
static GscSession *GscTestCreateMode31(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode31 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;
    return gsc_session_create_with_spawner_arrays(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,error,capacity);
}

/* R133 highest constructor; no RNG initialization or reseed. */
static GscSession *GscTestCreateMode32(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode32 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;
    return gsc_session_create_with_randomint(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,error,capacity);
}
static GscSession *GscTestCreateMode33(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode33 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;
    return gsc_session_create_with_randomfloat(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,error,capacity);
}
static GscSession *GscTestCreateMode34(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode34 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    return gsc_session_create_with_keybinding(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,error,capacity);
}
static GscSession *GscTestCreateMode35(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode35 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    return gsc_session_create_with_computed_randomint(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,error,capacity);
}

static GscSession *GscTestCreateMode36(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode36 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    return gsc_session_create_with_structs(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,error,capacity);
}

static GscSession *GscTestCreateMode37(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionEntityQueryHost entity_queries={0};
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode37 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    entity_queries.by_number=GscTestEntityByNumber;entity_queries.collect_team=GscTestSpawnerTeamArrayCollect;
    return gsc_session_create_with_entity_queries(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,&entity_queries,error,capacity);
}

static GscSession *GscTestCreateMode38(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionEntityQueryHost entity_queries={0};
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode38 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    entity_queries.by_number=GscTestEntityByNumber;entity_queries.collect_team=GscTestSpawnerTeamArrayCollect;
    return gsc_session_create_with_dynamic_randomint(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,&entity_queries,error,capacity);
}

static GscSession *GscTestCreateMode39(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSessionEntityQueryHost entity_queries={0};
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!GscTestObjectiveBegin() || !GscTestMissionBegin()) {
        GscTestError(error,capacity,"mode39 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    entity_queries.by_number=GscTestEntityByNumber;entity_queries.collect_team=GscTestSpawnerTeamArrayCollect;
    return gsc_session_create_with_numeric_randomint(host,
        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,&entity_queries,error,capacity);
}

#include "g_cod1_spawner_pose_test_host.inc"

static int TrainingFactoryPrepare(void *unused,const GscActorSpawnRequest *q,GscActorSpawnTicket *ticket,int *created,char *error,size_t size){
 return G_Cod1ActorFactoryPrepareSession(gscTestSession,gscTestId,q,ticket,created,error,size);
}
static int TrainingFactoryFinish(void *unused,const GscActorSpawnTicket *ticket,int published,char *error,size_t size){
 return G_Cod1ActorFactoryFinishSession(gscTestSession,gscTestId,ticket,published,error,size);
}
static GscSession *GscTestCreateMode40(const GscSessionHost *host,
    char *error,size_t capacity) {
    GscSession *session;GscScriptSoundHost sound;
    GscActorSpawnHost factory={NULL,TrainingFactoryPrepare,TrainingFactoryFinish};
    int rigs=gscTestEntityMode==43&&G_Cod1GscFeature47(C1CP47_F_RIGS)==1;
    GscSessionEntityQueryHost entity_queries={0};
    GscSessionMusicHost music={0};GscSessionResourceHost resources={0};
    GscSessionObjectiveHost objectives={0};GscSessionConsoleHost console={0};
    GscSessionHudHost hud={0};GscSessionMissionHost mission={0};
    GscSessionBlastHost blast={0};GscSessionQuakeHost quake={0};
    GscSessionFxHost fx={0};GscSessionCvarHost cvars={0};
    GscSessionShellshockHost shellshock={0};GscSessionDebugTextHost debug_text={0};
    GscSessionDebugLineHost debug_line={0};GscSessionAmbientHost ambient={0};
    GscSessionPrecacheItemHost precache_item={0};GscSessionSaveGameHost save_game={0};
    GscSessionTagFxHost tag_fx={0};GscSessionNativeQueryHost native_queries={0};
    GscSessionPrecacheStringHost precache_string={0};
    GscSessionSpawnerArrayHost spawner_arrays={0};GscSessionRandomIntHost randomint={0};GscSessionRandomFloatHost randomfloat={0};GscSessionKeyBindingHost keybinding={0};
    if(!gscTestPreparingEarly && gscTestEntityMode!=44 && gscTestEntityMode!=45 && gscTestEntityMode!=46 && gscTestEntityMode!=47 && (!GscTestObjectiveBegin() || !GscTestMissionBegin())) {
        GscTestError(error,capacity,"mode40 objective/mission map context unavailable");return NULL;
    }
    music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
    resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
    objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
    objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
    objectives.string_nomessage=GscTestObjectiveStringNoMessage;
    objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
    objectives.ring=GscTestObjectiveRing;
    console.query_gate=GscTestConsoleGate;console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
    hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
    mission.query_gate=GscTestMissionGate;mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
    blast.radius=GscTestBlastRadius;quake.emit=GscTestQuakeEmit;
    fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;cvars.publish=GscTestCvarPublish;
    shellshock.publish=GscTestShellshockPublish;debug_text.publish=GscTestDebugTextPublish;
    debug_line.publish=GscTestDebugLinePublish;ambient.publish=GscTestAmbientPublish;
    precache_item.publish=GscTestItemPublish;save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
    tag_fx.query=GscTestTagFxQuery;tag_fx.publish=GscTestTagFxPublish;
    native_queries.query=GscTestNativeQuery;
    precache_string.startup=GscTestPrecacheStringStartup;precache_string.publish=GscTestPrecacheStringPublish;
    spawner_arrays.collect_all=GscTestSpawnerArrayCollect;randomint.draw=GscTestRandomIntDraw;randomfloat.draw=GscTestRandomFloatDraw;keybinding.query=GscTestKeyBindingQuery;
    entity_queries.by_number=GscTestEntityByNumber;entity_queries.collect_team=GscTestSpawnerTeamArrayCollect;
    session=gsc_session_create_with_dynamic_randomfloat(host,
        (rigs||gscTestEntityMode==45)?G_Cod1SpawnerPoseEntityHost():G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
        gscTestEntityMode==45?GscTestSpawnerPoseControlHost():rigs?G_Cod1SpawnerPoseControlHost():G_Cod1EntityControlHost(),
        G_Cod1EntityCollectionHost(),G_Cod1EntityPredicateHost(),rigs?&factory:NULL,
        &music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,
        &shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,
        &tag_fx,&native_queries,&precache_string,&spawner_arrays,&randomint,&randomfloat,&keybinding,&entity_queries,error,capacity);
    if(session&&G_Cod1GscFeature47(C1CP47_F_SOUND)==1) {
        sound.user=NULL;sound.invoke=GscSoundInvoke;
        if(!gsc_session_select_script_sound(session,&sound,error,capacity)) {
            GscTestDestroyUnpublished(session);return NULL;
        }
        if(G_Cod1GscFeature47(C1CP47_F_SOUND_NOTIFY)==1){GscSoundNotifyHost h;h.user=NULL;h.invoke=GscSoundNotifyInvoke;
         if(!gsc_session_select_sound_notify(session,&h,error,capacity)){GscTestDestroyUnpublished(session);return NULL;}gscSoundNotifySelected=1;}
    }
    return session;
}

#include "g_cod1_start_origin_test_host.inc"
#include "g_cod1_animation_namespace_test_host.inc"
#include "g_cod1_map_start_pose.inc"
#include "g_cod1_map_animation_length.inc"
#include "g_cod1_bullet_trace.inc"
#include "g_cod1_map_developer.inc"
#include "g_cod1_map_animation_namespace.inc"
#include "g_cod1_namespace_start_origin_test_host.inc"
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
#include "g_cod1_training_grenade_host.inc"
#include "g_cod1_map_items.inc"
#include "g_cod1_turret_precache.inc"
#include "g_cod1_turret_factory.inc"
#include "g_cod1_training_carbine_host.inc"
#include "g_cod1_training_turret_host.inc"
#include "g_cod1_map_model_part.inc"
int G_Cod1GscTestGrenadeDamageActive(void){return GscTestGrenadeCurrent(NULL);}
int G_Cod1GscTestFollowupActive(void){return gscTestCarbine.followup&&GscTestCarbineCurrent(NULL);}
#define GSC_TEST_R216_PLAYER_HOST ((gscTestGrenades.selected||gscTestCarbine.selected)?G_Cod1GrenadePlayerNativeHost():G_Cod1PlayerNativeHost())
#define GSC_TEST_R232_SPATIAL_HOST (gscTestTurret.selected?GscTestTurretSpatialHost():G_Cod1SpatialNativeHost())
#define GSC_TEST_R233_NODE_HOST (gscTestTurret.selected?G_Cod1NodeRadiusQueryHost():G_Cod1NodeQueryHost())
#else
#define GSC_TEST_R216_PLAYER_HOST G_Cod1PlayerNativeHost()
#define GSC_TEST_R232_SPATIAL_HOST G_Cod1SpatialNativeHost()
#define GSC_TEST_R233_NODE_HOST G_Cod1NodeQueryHost()
#endif

/* Explicit controlled dedicated diagnostic; normal course admission is unchanged. */
static int GscTestTurretPoseProbePath(const char*p){return p&&!strcmp(p,"scripts/gsc_tests/r236_turret_pose_probe.gsc")&&trap_Cvar_VariableIntegerValue("g_cod1GscPoseTrace")==1&&trap_Cvar_VariableIntegerValue("dedicated")==1;}
/* Full original root uses the existing Training capability profile. The path
 * selects no substitute main and grants no completed-scene/AI certificate.
 * Fresh SP, training map, explicit profile flags and all owner gates below
 * remain required. */
#include "g_cod1_campaign_entry47.h"
static int GscTestOriginalTrainingPath(const char*p){return C1CP47_IsRoot(C1CP47_TRAINING,p);}
static int GscTestTurretEntryPath(const char*p){return p&&(GscTestOriginalTrainingPath(p)||!strcmp(p,"scripts/gsc_tests/r232_training_turret.gsc")||!strcmp(p,"scripts/gsc_tests/r234_training_turret.gsc")||!strcmp(p,"scripts/gsc_tests/r243_original_shuffle_demo.gsc")||GscTestTurretPoseProbePath(p));}

static int GscTestR198SourcePath(const char *path) {
    const char *p;size_t n;
    if(!path)return 0;n=strlen(path);
    if(n<5||n>=GSC_SESSION_PATH_CAP||strcmp(path+n-4,".gsc")||strstr(path,".."))return 0;
    if(strncmp(path,"scripts/gsc_tests/",17)&&strncmp(path,"maps/",5)&&
       strncmp(path,"animscripts/",12)&&strncmp(path,"character/",10)&&
       strncmp(path,"xmodelalias/",12)&&strncmp(path,"aitype/",7)&&strncmp(path,"mptype/",7))return 0;
    for(p=path;p<path+n-4;p++)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='_'||*p=='/'||*p=='-'))return 0;
    return 1;
}
static int GscTestRead( void *user, const char *path, GscSessionSource *out,
                       char *error, size_t capacity ) {
    fileHandle_t file = 0;
    int length;
    char *bytes;
    (void)user;
    memset(out, 0, sizeof(*out));
    /* gsc_session_load validates the entire canonical path first. Keep the
     * host/VFS root restriction independently at the actual read boundary. */
    if (!path || (strncmp(path, GSC_TEST_ROOT, sizeof(GSC_TEST_ROOT)-1) &&
        !((gscTestEntityMode >= 10 && gscTestEntityMode<=47) && !strncmp(path, "maps/", 5)) && !((gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))&&GscTestR198SourcePath(path)))) {
        GscTestError(error, capacity, (gscTestEntityMode >= 10 && gscTestEntityMode<=47) ?
            "module GSC must be under scripts/gsc_tests/ or maps/" :
            "test GSC must be under scripts/gsc_tests/");
        return 0;
    }
    if ((gscTestEntityMode >= 10 && gscTestEntityMode<=47) && !GscTestModuleReadCurrent()) {
        GscTestError(error, capacity, "module load mode changed before VFS read");
        return 0;
    }
    length = trap_FS_FOpenFile(path, &file, FS_READ);
    if (length <= 0 || !file || (unsigned)length > ((gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))?GSC_SESSION_EXTENDED_MAX_SOURCE:GSC_SESSION_MAX_SOURCE)) {
        if (file) trap_FS_FCloseFile(file);
        GscTestError(error, capacity, (gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))?"GSC missing, empty or above 1048576 bytes":"test GSC missing, empty or above 131072 bytes");
        return 0;
    }
    /* This engine's trap_FS_Read is VOID (sv_game discards FS_Read's result).
     * Do not invent a read count. Zero-fill and reject any remaining/internal
     * NUL before parsing, which also catches a short copy leaving a tail.
     * No promise is made about a corrupt FS implementation writing wrong bytes. */
    bytes = (char *)COD1_GSC_VFS_CALLOC((size_t)length + 1, 1);
    if (!bytes) {
        trap_FS_FCloseFile(file);
        GscTestError(error, capacity, "test GSC VFS allocation failed");
        return 0;
    }
    trap_FS_Read(bytes, length, file);
    trap_FS_FCloseFile(file);
    if ((gscTestEntityMode >= 10 && gscTestEntityMode<=47) && !GscTestModuleReadCurrent()) {
        COD1_GSC_VFS_FREE(bytes);
        GscTestError(error, capacity, "module load mode changed during VFS read");
        return 0;
    }
    if (memchr(bytes, '\0', (size_t)length)) {
        COD1_GSC_VFS_FREE(bytes);
        GscTestError(error, capacity, "test GSC contains NUL or incomplete VFS read");
        return 0;
    }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestTurret.selected){
        /* R232 whole-function admission consumes immutable complete original modules. */
    } else if(gscTestCarbine.selected){
        int selected=gscTestFinaleSelected?G_Cod1TrainingFinaleSourceSelect(path,bytes,(size_t)length):gscTestCarbine.opening?G_Cod1TrainingOpeningGameplaySourceSelect(path,bytes,(size_t)length):gscTestCarbine.followup?G_Cod1TrainingFollowupSourceSelect(path,bytes,(size_t)length):0;
        if(!selected&&gscTestCarbine.sequence)selected=G_Cod1TrainingSequenceSourceSelect(path,bytes,(size_t)length);
        if(!selected)selected=G_Cod1TrainingCarbineSourceSelect(path,bytes,(size_t)length);
        if(!selected&&gscTestCarbine.moody)selected=G_Cod1TrainingMoodySourceSelect(path,bytes,(size_t)length);
        if(selected<0){COD1_GSC_VFS_FREE(bytes);GscTestError(error,capacity,"original carbine source identity mismatch");return 0;}
    } else if(gscTestGrenades.selected) {
        int selected=G_Cod1TrainingGrenadeSourceSelect(path,bytes,(size_t)length,1);
        if(selected<0) {
            COD1_GSC_VFS_FREE(bytes);
            GscTestError(error,capacity,"original grenade source identity/profile mismatch");return 0;
        }
        if(selected)G_Printf("[R216] ORIGINAL_FUNCTION_SELECTION path=%s bytes=%d restock=1\n",path,length);
    }
#endif
    if((gscTestEntityMode==46 || gscTestEntityMode==47) && !strcmp(path,"maps/training_anim.gsc"))
        G_Printf("[R211] ORIGINAL_MODULE path=maps/training_anim.gsc bytes=%u fnv=%08x\n",
            (unsigned)length,GscTestAnimationNamespaceHash(bytes,(size_t)length));
    out->data = bytes; out->size = (size_t)length; out->token = bytes;
    return 1;
}
static void GscTestRelease( void *user, const GscSessionSource *source ) {
    (void)user;
    COD1_GSC_VFS_FREE(source->token);
}
static int GscTestFog( void *user, uint64_t id, int logicalTime,
                       const GscFogEvent *fog ) {
    cod1DirectFogEvent_t event;
    (void)user;
    if (gscTestStopRequested || id != gscTestId || !fog) return 0;
    if ((gscTestEntityMode >= 10 && gscTestEntityMode<=43) && !GscTestModuleReadCurrent()) return 0;
    event.nearDist = fog->near_dist;
    event.farDist = fog->far_dist;
    event.density = fog->density;
    event.color[0] = fog->red;
    event.color[1] = fog->green;
    event.color[2] = fog->blue;
    event.transitionMs = fog->transition_ms; /* NO second seconds/ms rounding. */
    /* The authoritative map gate lives in g_spawn.c. A v58/invalid/unprepared
     * map is refused there without touching CS_FOGVARS or sending a command. */
    if (!G_Cod1DirectFogEvent(&event)) return 0;
    G_Printf("[GSC R77] fog logical=%dms transition=%dms near=%.9g far=%.9g density=%.9g rgb=%.9g,%.9g,%.9g\n",
             logicalTime, event.transitionMs, (double)event.nearDist,
             (double)event.farDist, (double)event.density,
             (double)event.color[0], (double)event.color[1], (double)event.color[2]);
    return !gscTestStopRequested;
}
#include "g_cod1_training_finale_host.inc"
#include "g_cod1_changelevel_host.inc"
static void GscTestGraphClientSpawned(struct gentity_s*);
static int GscTestGraphClientOptions(void);
#include "g_cod1_client_startup.inc"
/* No callbacks or state transitions: all Session states remain unsaved.
 * Deferred destruction keeps the pointer/phase, including completed VMs. */
int G_Cod1GscTestBlocksRawSave(void){
    return gscTestSession!=NULL||gscTestBusy||gscTestPreparingEarly||gscTestClientStartup.phase!=0||gscTestRawSaveUnresolved;
}
static struct {uint64_t id,epoch,receipt,string_incarnation;char owner[128];int selected,attempted,published,probe,clientReady,started,cancelled;GscEntityRef player;gclient_t*client;cod1PlayerLifetime_t completed_spawn;} gscTestGraph;
/* The engine permits no game-world entry between slices. Only completed
 * operations are recorded here, all borrowed callbacks have unwound. */
enum { GSC_FRAME_BEGIN, GSC_FRAME_NATIVE, GSC_FRAME_POST, GSC_FRAME_ACTORS };
static struct {
    int stage,nativeStage,coursePaused,nextRunner,profile;
    unsigned profileElapsed;
    uint64_t generation,sessionId;
} gscFrameCursor;
static void GscTestDrop( void ) {
    memset(&gscFrameCursor,0,sizeof(gscFrameCursor));
    GscMapPoseReset();GscMapLengthReset();GscBulletTraceReset();GscMapDeveloperReset();
    gscVehicleFramesSelected=0;
    char error[GSC_SESSION_ERROR_CAP];
    GscTestClientStartupCancel();
    memset(&gscTestGraph,0,sizeof(gscTestGraph));
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestGrenadePrepared.transaction){cod1ItemPreparedReceipt_t prepared=gscTestGrenadePrepared;
        if(!G_Cod1ItemLifecyclePreparedAbort(&prepared,error,sizeof(error))){
            G_Printf("[GSC R111] prepared native item prefix cleanup deferred: %s\n",error);gscTestStopRequested=1;return;
        }
        memset(&gscTestGrenadePrepared,0,sizeof(gscTestGrenadePrepared));
    }
#endif
    if(!G_Cod1LoopSoundMapCleanup()||!GscTestLoopFxReset()){gscTestStopRequested=1;return;}
    GscTestFinaleCancel();GscChangeCancel();
    GscSoundNotifyReset();
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestTurret.dropSelected){if(!G_Cod1DroppedItemsStop(gscTestId)){G_Printf("R233 dropped-item cleanup refused\n");gscTestStopRequested=1;return;}gscTestTurret.dropSelected=0;}
    G_Cod1HintOwnersReset();
    GscMapItemReset();GscTestGrenadeReset();
    GscTestTurretReset();
    G_Cod1ActorMotionStop(gscTestCarbine.epoch);
    memset(&gscTestCarbine,0,sizeof(gscTestCarbine));
#endif
    if (!GscTestHudReset(error,sizeof(error))) {
        G_Printf("[GSC R200] HUD cleanup deferred: %s\n",error);
        gscTestStopRequested=1;return;
    }
    if(gscTestActorPrefixEpoch) {
        /* Retire the copied map certificate before freeing exact native
         * ownership. A deferred cleanup keeps this Session for retry. */
        if(!G_Cod1ActorBootstrapRetire(gscTestActorPrefixEpoch,error,sizeof(error))) {
            G_Printf("[GSC R203] actor prefix cleanup deferred: %s\n",error);
            gscTestStopRequested=1;return;
        }
        gscTestActorPrefixEpoch=0;
    }
    /* VM retirement preserves already published scene names, as FX/quake do.
     * True bootstrap/shutdown/save-load boundaries clear only exact ownership. */
    gscTestDeleteTransactionsSelected=0;
    G_Cod1PlayerNativeReset();
    GscTestSaveReset();
    GscTestTagFxReset();
    gscTestItemInitialization=0;
    gscTestShellshockInitialization=0;
    GscTestCvarReset();
    if (gscTestSession) {
        if (!gsc_session_destroy(gscTestSession, error, sizeof(error))) {
            /* Keep the pointer on refusal; never free an executing callback. */
            G_Printf("[GSC R77] cleanup refused: %s\n", error);
            gscTestStopRequested = 1;
            return;
        }
        gscTestSession = NULL;
    }
    GscTestBlastEventsReset();
    gscTestShellshockInitialization = 0;
    gscTestResourceInitialization = 0;
    gscTestItemInitialization = 0;
    memset(gscTestResourceValidated,0,sizeof(gscTestResourceValidated));
    gscTestId = 0; gscTestCompleted = 0; gscTestEntityMode = 0;gscTestMapEpoch=0;
    GscTestClientStartupReset();
}
static int GscTestEnter( void ) {
    if (gscTestBusy) { gscTestStopRequested = 1; return 0; }
    gscTestBusy = 1; gscTestStopRequested = 0; return 1;
}
static void GscTestLeave( void ) {
    if (gscTestStopRequested) GscTestDrop();
    gscTestBusy = 0;
}
static void GscTestCheckCompleted( void ) {
    GscSessionSnapshot snap;
    if (gscTestSession && !gscTestCompleted &&
        gsc_session_snapshot(gscTestSession, &snap) &&
        snap.state == GSC_SESSION_COMPLETED && !gscChangeLevel.phase) {
        if(!G_Cod1LoopSoundMapCleanup()||!GscTestLoopFxReset()){gscTestStopRequested=1;return;}
        if(gscTestEntityMode==43){char error[GSC_SESSION_ERROR_CAP];if(!GscTestHudReset(error,sizeof(error))){
            G_Printf("[GSC R200] completed HUD cleanup deferred: %s\n",error);gscTestStopRequested=1;return;
        }}
        if((gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))G_Cod1PlayerNativeReset();
        gscTestCompleted = 1;
        G_Printf("[GSC R77] completed: active=%d isdefined=%llu exp=%llu cull=%llu\n",
            snap.active_threads, snap.native.isdefined_callbacks,
            snap.fog.exp_callbacks, snap.fog.cull_callbacks);
    }
}
#include "g_cod1_early_program_handoff.inc"
int G_Cod1GscTestConstructBspActor(const void *owner,void *result,char*error,size_t size){int ok;
 if(!owner||!result||!gscTestSession||!level.spawning||!gscTestEarlyProgram.prepared||
    gscTestEarlyProgram.consumed||!GscTestEarlyCurrent()||!GscTestEnter())return 0;
 ok=G_Cod1ActorConstructBspPlaced(gscTestSession,gscTestId,owner,result,error,size);
 GscTestLeave();return ok;
}
/* The captured early owner remains mandatory while waiting across frames.
 * Publication consumes only the construction snapshot, never configuration,
 * Session or resource ownership. */
/* Loading validates only the captured selection. The selected graph/runtime
 * owner is established later, after real BSP construction and consumption;
 * all Arm/Current/BEGIN paths still require GraphClientOptions below. */
static int GscTestGraphClientSelection(void){
 return gscTestEntityMode==43&&g_gametype.integer==GT_SINGLE_PLAYER&&
  G_Cod1GscFeature47(C1CP47_F_ENABLE)==1&&
  G_Cod1GscFeature47(C1CP47_F_MODE)==43&&
  G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)==1&&
  G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1&&
  G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1&&
  G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1&&
  trap_Cvar_VariableIntegerValue("savegame_loading")==0;
}
static int GscTestGraphClientOptions(void){
 return gscTestGraph.selected&&!gscTestGraph.probe&&gscTestGraph.id==gscTestId&&
  gscTestGraph.epoch==G_Cod1EntityMapEpoch()&&gscTestEarlyProgram.consumed&&
  !gscTestStopRequested&&!level.restarted&&gscTestEntityMode==43&&
  G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)==1&&
  G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1&&
  G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1&&
  G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1&&
  GscTestEarlyCurrent()&&!gscTestStopRequested;
}
/* No traps/providers follow this last completed-client/session fence. */
static int GscTestGraphCompletedCurrentPure(int prepared){
 GscSessionSnapshot s;
 if(gscTestGraph.cancelled||gscTestStopRequested||level.restarted||level.spawning||
    !gscTestSession||gscTestGraph.id!=gscTestId||gscTestGraph.epoch!=G_Cod1EntityMapEpoch()||
    !gsc_session_snapshot(gscTestSession,&s)||s.session_id!=gscTestId||s.busy||s.error[0]||
    (prepared?(s.state!=GSC_SESSION_PREPARED||s.active_threads):
     (s.state!=GSC_SESSION_RUNNING&&s.state!=GSC_SESSION_COMPLETED)))return 0;
 if(gscTestGraph.probe)return 1;
 if(!GscTestClientStartupIdentity(&gscTestClientStartup)||
    !GscTestClientStartupPlayerCurrentPure(&gscTestClientStartup))return 0;
 if(!gscTestGraph.clientReady)return !gscTestClientStartup.ready;
 return gscTestClientStartup.ready&&gscTestClientStartup.client==gscTestGraph.client&&
  G_Cod1PlayerLifetimeSameCompleted(&gscTestGraph.completed_spawn)&&
  gscTestGraph.completed_spawn.entity.epoch==gscTestGraph.player.epoch&&
  gscTestGraph.completed_spawn.entity.generation==gscTestGraph.player.generation&&
  gscTestGraph.completed_spawn.entity.slot==gscTestGraph.player.slot&&
  gscTestClientStartup.completed_spawn.completion==gscTestGraph.completed_spawn.completion;
}
/* The original 14900 completes with the connected player's 134 bits
 * clear under 3000. The first 6FAD0 scan observes that same player before
 * map main. Bind this projection to both actual startup owners, only for
 * the fresh normal SP client at this specific graph phase. */
int G_Cod1StartupPlayerPath(const GscEntityRef *ref,cod1PlayerLifetime_t *out){
    const GscTestClientStartup *startup=&gscTestClientStartup;
    const cod1PlayerLifetime_t *completed=&gscTestGraph.completed_spawn;
    if(!ref||!out||ref->slot!=0||!gscTestGraph.selected||gscTestGraph.probe||
       !gscTestGraph.published||!gscTestGraph.started||!gscTestGraph.clientReady||
       gscTestGraph.cancelled||gscTestStopRequested||level.restarted||level.spawning||
       startup->phase!=2||!startup->ready||startup->cancelled||startup->session!=gscTestSession||
       startup->id!=gscTestId||gscTestGraph.id!=gscTestId||startup->epoch!=ref->epoch||
       gscTestGraph.epoch!=ref->epoch||completed->entity.slot!=ref->slot||
       completed->entity.epoch!=ref->epoch||completed->entity.generation!=ref->generation||
       completed->completion!=startup->completed_spawn.completion||
       startup->client!=gscTestGraph.client||g_entities[0].client!=startup->client||
       !GscTestClientStartupPlayerCurrentPure(startup)||
       g_entities[0].client->ps.pm_type!=PM_NORMAL||g_entities[0].health<=0||
       level.time<completed->completed_time||completed->completed_time<level.previousTime||
       !G_Cod1PlayerLifetimeSameCompleted(completed))return 0;
    *out=*completed;return 1;
}
static int GscTestGraphCurrent(int prepared){
 GscSessionSnapshot s;
 if(!gscTestGraph.selected||gscTestGraph.cancelled||!gscTestSession||gscTestGraph.id!=gscTestId||
    gscTestGraph.epoch!=G_Cod1EntityMapEpoch()||!gscTestEarlyProgram.consumed||
    gscTestStopRequested||level.restarted||level.spawning||gscTestEntityMode!=43||
    !G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||
    G_Cod1GscFeature47(C1CP47_F_ENABLE)!=1||
    G_Cod1GscFeature47(C1CP47_F_MODE)!=43||
    trap_Cvar_VariableIntegerValue("savegame_loading")!=0||
    G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)!=(gscTestGraph.probe?2:1)||
    G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)!=(gscTestGraph.probe?0:1)||
    (gscTestGraph.probe&&trap_Cvar_VariableIntegerValue("dedicated")!=1)||
    !GscTestEarlyCurrent()||!gsc_session_snapshot(gscTestSession,&s)||
    s.session_id!=gscTestId||s.error[0]||s.busy||
    (prepared?(s.state!=GSC_SESSION_PREPARED||s.active_threads):
     (s.state!=GSC_SESSION_RUNNING&&s.state!=GSC_SESSION_COMPLETED)))return 0;
 if(!gscTestGraph.probe&&!GscTestClientStartupCurrent(&gscTestClientStartup,prepared))return 0;
 if(!GscTestGraphCompletedCurrentPure(prepared))return 0;
 if(gscTestGraph.published&&!G_Cod1NodeGraphPublishedCurrent(gscTestSession,gscTestId,gscTestGraph.receipt))return 0;
 return GscTestEarlyCurrent()&&!gscTestStopRequested&&!level.restarted&&
  gscTestGraph.id==gscTestId&&gscTestGraph.epoch==G_Cod1EntityMapEpoch()&&
  GscTestGraphCompletedCurrentPure(prepared);
}
static int GscTestGraphPrepareItems(char*error,size_t cap){
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
 int ok;
 if(!gscTestGrenades.selected)return 1;
 if(!GscTestGraphCurrent(1)||gscTestGrenadePrepare.active)return 0;
 gscTestGrenadePrepare.session=gscTestSession;gscTestGrenadePrepare.id=gscTestId;
 gscTestGrenadePrepare.epoch=G_Cod1EntityMapEpoch();gscTestGrenadePrepare.active=1;
 ok=GscTestGrenadeBootstrap(error,cap);
 memset(&gscTestGrenadePrepare,0,sizeof(gscTestGrenadePrepare));
 if(!ok||!GscTestGraphCurrent(1))return 0;
 G_Printf("[GSC R110] original BSP items constructed before deferred legacy think; runtime actions remain held\n");
#endif
 return 1;
}
static int GscTestGraphExecuteItems(char*error,size_t cap){
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
 int ok;cod1ItemPreparedReceipt_t prepared=gscTestGrenadePrepared;
 if(!gscTestGrenadePrepared.transaction)return 1;
 if(!GscTestGraphCurrent(1)||gscTestGrenadePrepare.active)return 0;
 gscTestGrenadePrepare.session=gscTestSession;gscTestGrenadePrepare.id=gscTestId;
 gscTestGrenadePrepare.epoch=G_Cod1EntityMapEpoch();gscTestGrenadePrepare.active=1;
 ok=G_Cod1ItemLifecyclePreparedExecute(&prepared,error,cap);
 memset(&gscTestGrenadePrepare,0,sizeof(gscTestGrenadePrepare));
 return ok&&GscTestGraphCurrent(1);
#else
 return 1;
#endif
}
static int GscTestGraphProbeRetime(char*error,size_t cap){
 GscTestPrecacheStringScene p=gscTestPrecacheString;
 if(!gscTestGraph.probe||!GscTestGraphCurrent(1)||
    p.incarnation!=gscTestGraph.string_incarnation||strcmp(p.marker,gscTestGraph.owner)||
    !GscTestPrecacheStringMarker(&p,0,0)||!GscTestGraphCurrent(1)){
  GscTestError(error,cap,"graph probe localized owner retired");return 0;
 }
 /* No SERVER BEGIN or runtime publishing permission is manufactured by a
  * dedicated probe. Localized writes still need the real client scope. */
 gscTestPrecacheString.host_time=level.time;return 1;
}
static int GscTestGraphPendingFrame(int levelTime){
 if(!gscTestGraph.selected)return 0;
 if(!GscTestGraphCurrent(!gscTestGraph.started)){
  G_Printf("[GSC R110] pending/published graph owner or configuration retired\n");GscTestDrop();GscTestLeave();return 1;
 }
 if(gscTestGraph.started)return 0;
 if(!gscTestGraph.published||gscTestGraph.probe||!gscTestGraph.clientReady){GscTestLeave();return 1;}
 if(!G_Cod1EntitySameAllocation(&gscTestGraph.player)||gscTestGraph.player.slot!=0||
    g_entities[0].client!=gscTestGraph.client||gscTestGraph.client!=&g_clients[0]||
    gscTestGraph.client->pers.connected!=CON_CONNECTED||gscTestClientStartup.phase!=1||
    !G_Cod1PlayerLifetimeSameCompleted(&gscTestGraph.completed_spawn)){GscTestDrop();GscTestLeave();return 1;}
 /* Preserve the connected owner's Retime and native BEGIN/END transaction. */
 if(GscTestClientStartupFrame(levelTime))return 1;
 GscTestDrop();GscTestLeave();return 1;
}
static void GscTestInitPhase( int levelTime,int early ) {
    GscSessionHost host;
    int nodeProgramSelected=0;
    GscReadOnlyContext readonlyContext={0};
    char error[GSC_SESSION_ERROR_CAP], path[MAX_CVAR_VALUE_STRING];
    if (!GscTestEnter()) return;
    gscTestPreparingEarly=early;
    if(!early&&gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch()&&gscTestEarlyProgram.attempted){
        GscSessionSnapshot snapshot;
        if(!gscTestSession||!gsc_session_snapshot(gscTestSession,&snapshot)||
           !GscTestEarlyConsume(&snapshot,gscTestId,level.spawning,level.restarted)){
            G_Printf("[GSC R76] early program handoff refused\n");GscTestDrop();GscTestLeave();return;
        }
        nodeProgramSelected=gscTestEarlyProgram.nodeSelected;
        memcpy(path,gscTestEarlyProgram.path,sizeof(path));
        if(!GscTestObjectiveBegin()||!GscTestMissionBegin()){G_Printf("[GSC R76] late scene context refused\n");GscTestDrop();GscTestLeave();return;}
        goto admit_program;
    }
    if(!early&&GscTestEarlySelected()){G_Printf("[GSC R76] missing early program handoff\n");GscTestLeave();return;}
    GscTestPrecacheStringReset();
    GscTestTagFxReset();
    GscTestSaveReset();
    GscTurretPrecacheReset();GscTestItemReset();
    GscTestAmbientReset();
    GscTestDebugLineReset();
    GscTestDebugTextReset();
    GscTestShellshockReset();
    GscTestCvarReset();
    GscTestFxReset();
    GscLoopDeliveryReset();
    GscTestQuakeReset();
    GscTestBlastEventsReset();
    GscTestMissionReset();
    GscTestObjectiveReset();
    GscTestDrop(); /* also handles map_restart without an earlier shutdown */
    if (gscTestSession||!GscCampaignNativeLive47()) { GscTestLeave(); return; }
    /* Not archived: deliberately opt-in each test installation/session. */
    trap_Cvar_Register(&gscTestEnabled, "g_cod1GscTest", "0", 0);
    trap_Cvar_Register(&gscTestScript, "g_cod1GscTestScript", GSC_TEST_DEFAULT, 0);
    trap_Cvar_Register(&gscTestEntities, "g_cod1GscEntityTest", "0", 0);
    trap_Cvar_Register(NULL,"g_cod1GscClientStartup","0",0);
    /* Compiled only into the opt-in DLL. Also identifies disabled baseline
     * runs, without reading GSC or changing fog in that mode. */
    G_Printf("[GSC R216] build=R216 grenade pickup checkpoint enabled=%d (selected test, not full mission GSC)\n",
             G_Cod1GscFeature47(C1CP47_F_ENABLE));
    if (G_Cod1GscFeature47(C1CP47_F_ENABLE) != 1) {
        GscTestLeave(); return;
    }
    if (g_gametype.integer != GT_SINGLE_PLAYER ||
        trap_Cvar_VariableIntegerValue("savegame_loading") != 0) {
        G_Printf("[GSC R77] skipped: requires fresh SP map, not savegame load\n");
        GscTestLeave(); return;
    }
    if(early){
        /* Default registration precedes DATA capture; profile/path selection
         * follows capture and is fenced by that same immutable configuration. */
        trap_Cvar_Register(&gscTestAiArrays,"g_cod1GscAiArrays","0",0);
        trap_Cvar_Register(&gscTestActorPrefix,"g_cod1GscActorPrefix","0",0);
        trap_Cvar_Register(&gscTestDeleteTransactions,"g_cod1GscDeleteTransactions","0",0);
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
        trap_Cvar_Register(&gscTestTrainingGrenades,"g_cod1GscTrainingGrenades","0",0);
#endif
        if(!GscTestEarlyCapture()){GscTestDrop();GscTestLeave();return;}
    }
    memset(path, 0, sizeof(path));
    GscConfigString47(C1CP47_K_ROOT, path, sizeof(path));
    path[sizeof(path)-1] = '\0';
    memset(&host, 0, sizeof(host)); host.enabled = 1;
    host.read = GscTestRead; host.release = GscTestRelease; host.fog = GscTestFog;
    gscTestEntityMode = GscTestEntityModeValue();
    if(G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)!=0&&
       !(early&&G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1?
         (gscTestPreparingEarly&&level.spawning&&!level.restarted&&!gscTestStopRequested&&
          GscTestEarlyCurrent()&&GscTestGraphClientSelection()&&GscTestEarlyCurrent()&&
          !gscTestStopRequested&&gscTestPreparingEarly&&level.spawning&&!level.restarted):
         GscTestClientStartupOptions())){G_Printf("[GSC R104] client startup selection or captured owner refused\n");GscTestDrop();GscTestLeave();return;}
    if((!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path)))){
        char map[64];GscConfigString47(C1CP47_K_MAP,map,sizeof(map));map[sizeof(map)-1]=0;
        if(strcmp(map,"training")){G_Printf("[R229] Training profile held on next map; briefing GSC not selected\n");GscTestLeave();return;}
    }
    if(GscTestBriefingSelection())gscSoundStreamedSelected=COD1_SS_CAMPAIGN_VOICE_PROFILE;
    if(gscTestEntityMode==43) {
        if(!early){
            trap_Cvar_Register(&gscTestAiArrays,"g_cod1GscAiArrays","0",0);
            trap_Cvar_Register(&gscTestActorPrefix,"g_cod1GscActorPrefix","0",0);
            trap_Cvar_Register(&gscTestDeleteTransactions,"g_cod1GscDeleteTransactions","0",0);
        }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
        if(!early)trap_Cvar_Register(&gscTestTrainingGrenades,"g_cod1GscTrainingGrenades","0",0);
        if(G_Cod1GscFeature47(C1CP47_F_CARBINE)==1){
            if((strcmp(path,"scripts/gsc_tests/r222_training_carbine.gsc")&&strcmp(path,"scripts/gsc_tests/r223_training_moody.gsc")&&strcmp(path,"scripts/gsc_tests/r224_training_weapons.gsc")&&strcmp(path,"scripts/gsc_tests/r225_training_followup.gsc")&&strcmp(path,"scripts/gsc_tests/r228_training_opening.gsc")&&(strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")&&strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")&&!GscTestTurretEntryPath(path)))||
               G_Cod1GscFeature47(C1CP47_F_VOICE)!=((!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path)))||
               G_Cod1GscFeature47(C1CP47_F_FINALE)!=((!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path))))||
               G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)!=1||
               G_Cod1GscFeature47(C1CP47_F_AI_ARRAYS)!=1||
               G_Cod1GscFeature47(C1CP47_F_DELETE)!=1||
               G_Cod1GscFeature47(C1CP47_F_GRENADES)!=(!strcmp(path,"scripts/gsc_tests/r225_training_followup.gsc")||!strcmp(path,"scripts/gsc_tests/r228_training_opening.gsc")||(!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path))))||
               G_Cod1GscFeature47(C1CP47_F_OPENING)!=(!strcmp(path,"scripts/gsc_tests/r228_training_opening.gsc")||(!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path))))){
                G_Printf("R222 carbine requires explicit entry and real actor/AI/delete capabilities\n");GscTestLeave();return;
            }
            if(GscTestTurretEntryPath(path)){
                if(G_Cod1GscFeature47(C1CP47_F_TURRET)!=1){G_Printf("R232 requires explicit original turret profile selection\n");GscTestLeave();return;}
                gscTestTurret.selected=1;gscTestTurret.epoch=G_Cod1EntityMapEpoch();
            }
            gscTestCarbine.selected=1;gscTestCarbine.moody=strcmp(path,"scripts/gsc_tests/r222_training_carbine.gsc")!=0;gscTestCarbine.opening=!strcmp(path,"scripts/gsc_tests/r228_training_opening.gsc")||(!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path)));gscTestFinaleSelected=(!strcmp(path,"scripts/gsc_tests/r229_training_finale.gsc")||(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path)));gscSoundStreamedSelected=(GscTestOriginalTrainingPath(path)||!strcmp(path,"scripts/gsc_tests/r234_training_turret.gsc")||!strcmp(path,"scripts/gsc_tests/r243_original_shuffle_demo.gsc"))?2:(!strcmp(path,"scripts/gsc_tests/r230_training_voice.gsc")||GscTestTurretEntryPath(path));gscTestCarbine.followup=gscTestCarbine.opening||!strcmp(path,"scripts/gsc_tests/r225_training_followup.gsc");gscTestCarbine.sequence=gscTestCarbine.followup||!strcmp(path,"scripts/gsc_tests/r224_training_weapons.gsc");gscTestCarbine.epoch=G_Cod1EntityMapEpoch();
        }
        if(G_Cod1GscFeature47(C1CP47_F_GRENADES)==1) {
            if(strcmp(path,"scripts/gsc_tests/r216_training_grenades.gsc") &&
               strcmp(path,"scripts/gsc_tests/r217_training_consumables.gsc") && !gscTestCarbine.followup) {
                G_Printf("[R216] grenade profile requires its explicit original-function entry\n");
                GscTestLeave();return;
            }
            gscTestGrenades.selected=1;gscTestGrenades.epoch=G_Cod1EntityMapEpoch();
            gscTestGrenades.startTime=levelTime;
        }
#endif
    }
    if((gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))gscTestMapEpoch=G_Cod1EntityMapEpoch();
    if(early&&(!GscTestEarlyCurrent()||strcmp(path,gscTestEarlyProgram.values[16]))){GscTestDrop();GscTestLeave();return;}
    if(gscTestEntityMode==47) {
        gscTestMapEpoch=G_Cod1EntityMapEpoch();gscTestNamespaceStartOriginQueries=0;
        gscTestSession=GscTestCreateMode40(&host,error,sizeof(error));
        if(gscTestSession &&
           (!gsc_session_select_r198_execution(gscTestSession,G_Cod1PlayerNativeHost(),error,sizeof(error)) || !gsc_session_select_player_stance_permissions(gscTestSession,error,sizeof(error)) ||
            !gsc_session_select_r199_node_queries(gscTestSession,G_Cod1NodeQueryHost(),error,sizeof(error)) ||
            !GscTestAnimationNamespaceSelect(error,sizeof(error)) ||
            !GscTestNamespaceStartOriginSelect(error,sizeof(error)))) {
            G_Printf("[GSC R215] namespace start-origin selection failed: %s\n",error);
            GscTestDrop();GscTestLeave();return;
        }
        if(gscTestSession)G_Printf("[GSC R215] mode47 genuine Guy and gear namespace start-origin selected\n");
    } else
    if(gscTestEntityMode==46) {
        gscTestMapEpoch=G_Cod1EntityMapEpoch();
        gscTestSession=GscTestCreateMode40(&host,error,sizeof(error));
        if(gscTestSession && !GscTestAnimationNamespaceSelect(error,sizeof(error))) {
            G_Printf("[GSC R211] animation namespace selection failed: %s\n",error);
            GscTestDrop();GscTestLeave();return;
        }
        if(gscTestSession)G_Printf("[GSC R211] mode46 original animation metadata namespace selected\n");
    } else
    if(gscTestEntityMode==45) {
        gscTestMapEpoch=G_Cod1EntityMapEpoch();gscTestSpawnerPoseWrites=0;
        gscTestSession=GscTestCreateMode40(&host,error,sizeof(error));
        if(gscTestSession &&
           (!gsc_session_select_r198_execution(gscTestSession,G_Cod1PlayerNativeHost(),error,sizeof(error)) || !gsc_session_select_player_stance_permissions(gscTestSession,error,sizeof(error)) ||
            !gsc_session_select_r199_node_queries(gscTestSession,G_Cod1NodeQueryHost(),error,sizeof(error)) ||
            !GscTestStartOriginSelect(error,sizeof(error)))) {
            G_Printf("[GSC R210] spawner-origin selection failed: %s\n",error);
            GscTestDrop();GscTestLeave();return;
        }
        if(gscTestSession)G_Printf("[GSC R210] mode45 real jumpnode query and typed-spawner origin selected\n");
    } else
    if(gscTestEntityMode==44) {
        gscTestMapEpoch=G_Cod1EntityMapEpoch();
        gscTestSession=GscTestCreateMode40(&host,error,sizeof(error));
        if(gscTestSession && !GscTestStartOriginSelect(error,sizeof(error))) {
            G_Printf("[GSC R209] start-origin selection failed: %s\n",error);
            GscTestDrop();GscTestLeave();return;
        }
        if(gscTestSession)G_Printf("[GSC R209] mode44 original Guy start-origin Session selected\n");
    } else
    if((gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))) {
        gscTestSession=GscTestCreateMode40(&host,error,sizeof(error));
        if((gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))&&gscTestSession&&
           (!gsc_session_select_r198_execution(gscTestSession,GSC_TEST_R216_PLAYER_HOST,error,sizeof(error)) || !gsc_session_select_player_stance_permissions(gscTestSession,error,sizeof(error))||
            !gsc_session_select_r198_entity_operations(gscTestSession,G_Cod1EntityDeleteHost(),error,sizeof(error))||
            !gsc_session_select_origin_method(gscTestSession,G_Cod1OriginMethodHost(),error,sizeof(error))||
            !gsc_session_select_r198_spatial_natives(gscTestSession,GSC_TEST_R232_SPATIAL_HOST,error,sizeof(error)))) {
            G_Printf("[GSC R198] execution selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
        }
        if((gscTestEntityMode==42||gscTestEntityMode==43)&&gscTestSession&&
           !gsc_session_select_r199_node_queries(gscTestSession,GSC_TEST_R233_NODE_HOST,error,sizeof(error))) {
            G_Printf("[GSC R199] node selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
        }
        if((gscTestEntityMode==42||gscTestEntityMode==43))G_Printf("[GSC R199] mode42 node queries selected\n");
        if((gscTestEntityMode==42||gscTestEntityMode==43)&&gscTestSession&&
           !gsc_session_select_vehicle_nodes(gscTestSession,G_Cod1VehicleNodeHost(),error,sizeof(error))){
            G_Printf("[GSC class3] vehicle node selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
        }
        if(gscTestSession&&(!strcmp(path,"maps/vehicle_factory_probe.gsc")||!strcmp(path,"maps/pathfinder.gsc"))&&(!gsc_session_select_vehicle_spawn(gscTestSession,&gscVehicleHost,error,sizeof(error))||!gsc_session_select_vehicle_methods(gscTestSession,&gscVehicleMethods,error,sizeof(error)))){GscTestDrop();GscTestLeave();return;}
        if(gscTestSession&&(!strcmp(path,"maps/vehicle_factory_probe.gsc")||!strcmp(path,"maps/pathfinder.gsc"))){
         GscMapExecutionCertificate map={G_Cod1EntityMapEpoch(),GscCampaignProgramCurrent};
         if(!gsc_session_select_root_map(gscTestSession,&map,error,sizeof(error))||!gsc_session_select_r237_frame_buckets(gscTestSession,error,sizeof(error))||
            !gsc_session_select_r243_thread_start(gscTestSession,error,sizeof(error))||
            !gsc_session_select_r243_same_bucket_wait(gscTestSession,error,sizeof(error))||
            !gsc_session_select_r243_thread_entities(gscTestSession,G_Cod1ScriptThreadAuthorityHost(),error,sizeof(error))){G_Printf("[vehicle] frame selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_r216_yaw(gscTestSession,G_Cod1YawMotionHost(),error,sizeof(error))){G_Printf("[Campaign] script rotation selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_map_moveto(gscTestSession,error,sizeof(error))){G_Printf("[Campaign] moveto selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_map_source_float(gscTestSession,error,sizeof(error))){G_Printf("[Campaign] map arithmetic refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_bullet_trace(gscTestSession,&gscBulletTraceHost,error,sizeof(error))){G_Printf("[Campaign] bulletTrace selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_weapon_classname(gscTestSession,G_Cod1WeaponClassnameHost(),error,sizeof(error))){G_Printf("[Campaign] weapon classname selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         {char none[128];if(!G_Cod1QueryWeaponModel("none",none,error,sizeof(error))&&(!(gscTestPreparingEarly?G_Cod1SelectCWWeaponModelsForProgram(&map,error,sizeof(error)):G_Cod1SelectCWWeaponModels(error,sizeof(error)))||!G_Cod1QueryWeaponModel("none",none,error,sizeof(error)))){G_Printf("[Campaign] weapon model resources refused: %s\n",error);GscTestDrop();GscTestLeave();return;}if(none[0]){GscTestDrop();GscTestLeave();return;}error[0]=0;
          if(!gsc_session_select_map_weapon_model(gscTestSession,G_Cod1WeaponModelHost(),error,sizeof(error))){G_Printf("[Campaign] map weapon model selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}}
         if(!gsc_session_select_model_part(gscTestSession,&gscMapModelPartHost,error,sizeof(error))){G_Printf("[Campaign] model part selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_map_actor_presence(gscTestSession,G_Cod1ActorPresenceHost(),error,sizeof(error))){G_Printf("[Campaign] map actor presence selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_turret_precache(gscTestSession,GscTurretPrecacheProvider(),error,sizeof(error))){G_Printf("[Campaign] turret precache selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         if(!gsc_session_select_turret_spawn(gscTestSession,&gscTurretFactoryHost,error,sizeof(error))){G_Printf("[Campaign] turret factory selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}

         if(!GscMapAnimationNamespace(error,sizeof(error))){G_Printf("[Campaign] map declarations refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
         gscVehicleFramesSelected=1;
        }

        if(gscTestEntityMode==43&&gscTestSession&&
           (!gsc_session_select_r200_hud_elements(gscTestSession,&gscTestHudHost,error,sizeof(error))||
            (!gsc_session_select_r200_entity_collision(gscTestSession,G_Cod1R200CollisionHost(),error,sizeof(error)) || !gsc_session_select_connectpaths(gscTestSession,GscTestConnectPaths)||!gsc_session_select_disconnectpaths(gscTestSession,GscTestDisconnectPaths)))){
            G_Printf("[GSC R200] HUD/collision selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
        }
        if(gscTestEntityMode==43)G_Printf("[GSC R200] mode43 real HUD stopwatch and brush collision selected\n");
        /* R202 is a separate fresh-map capability; the normal bomb scene does
         * not enable AI queries or claim a complete training actor domain. */
        if(gscTestEntityMode==43 && gscTestSession &&
           (G_Cod1GscFeature47(C1CP47_F_AI_ARRAYS)==1 ||
            G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1)) {
            if(!gsc_session_select_r202_ai_arrays(gscTestSession,G_Cod1AiArrayHost(),error,sizeof(error))) {
                G_Printf("[GSC R202] AI query selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
            }
            G_Printf("[GSC R202] typed AI queries selected; world completeness checked on each query\n");
        }
        if(gscTestEntityMode==43 && gscTestSession &&
           G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1) {
            const GscSessionActorProgramPlan *decl=G_Cod1ActorProgramPlan();
            GscSessionActorProgramPlan plan;char module[128];uint32_t i;
            gscTestActorPrefixEpoch=gscTestMapEpoch;
            if(!decl){G_Printf("[GSC R71] Actor declarations unavailable\n");GscTestDrop();GscTestLeave();return;}
            plan=*decl;
            /* The turret root is owned by the pre-allocation declaration pass.
             * The late real turret host must still match at admission. */
            if(gscTestTurret.selected){
                if(!G_Cod1EarlyTurretModule("mg42_bipod_stand",module,sizeof(module))){G_Printf("[GSC R71] turret program resource unavailable\n");GscTestDrop();GscTestLeave();return;}
                for(i=0;i<plan.module_count;i++)if(!strcmp(plan.module_roots[i],module))break;
                if(i==plan.module_count){
                    if(i==GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY){G_Printf("[GSC R71] Actor program roots full\n");GscTestDrop();GscTestLeave();return;}
                    strcpy(plan.module_roots[plan.module_count++],module);
                }
            }
            if(!((!strcmp(path,"maps/pathfinder.gsc"))?gsc_session_select_map_actor_program(gscTestSession,&plan,error,sizeof(error)):gsc_session_select_actor_program(gscTestSession,&plan,error,sizeof(error)))){
                G_Printf("[GSC R71] Actor program selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
            }
            if(G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1&&
               !gsc_session_select_owned_wait_delivery(gscTestSession,error,sizeof(error))){
                G_Printf("[GSC R113] native wait delivery selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;
            }
            if(!strcmp(path,"maps/pathfinder.gsc")&&!gsc_session_select_map_actor_animations(gscTestSession,G_Cod1ActorComposedAnimationHost(),error,sizeof(error))){G_Printf("[Campaign] Actor animation selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            if(!strcmp(path,"maps/pathfinder.gsc")&&(!gsc_session_select_r233_actor_animation_end(gscTestSession,error,sizeof(error))||!gsc_session_select_map_flagged_all(gscTestSession,error,sizeof(error)))){G_Printf("[Campaign] named animation selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            if(!strcmp(path,"maps/pathfinder.gsc")&&!gsc_session_select_map_actor_look(gscTestSession,G_Cod1ActorLookHost(),error,sizeof(error))){G_Printf("[Campaign] Actor look selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            if(!strcmp(path,"maps/pathfinder.gsc")&&!gsc_session_select_map_script_state(gscTestSession,G_Cod1ActorScriptStateHost(),error,sizeof(error))){G_Printf("[Campaign] Actor tracking selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            if(!strcmp(path,"maps/pathfinder.gsc")&&(!gsc_session_select_r222_actor_goal(gscTestSession,G_Cod1ActorGoalHost(),error,sizeof(error))||!gsc_session_select_entity_goal(gscTestSession))){G_Printf("[Campaign] Actor goal selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            if(!strcmp(path,"maps/pathfinder.gsc")&&!gsc_session_select_map_actor_goal(gscTestSession,error,sizeof(error))){G_Printf("[Campaign] map vector goal selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
            G_Printf("[GSC R203] original actor constructor prefix selected; full AI controller remains unavailable\n");
        }
        if(gscTestEntityMode==43 && gscTestSession &&
           G_Cod1GscFeature47(C1CP47_F_DELETE)==1) {
            if(!gsc_session_select_r204_delete_transactions(gscTestSession,
                GscTestLoopFxDeleteHost(),error,sizeof(error))) {
                G_Printf("[GSC R204] delete transaction selection failed: %s\n",error);
                GscTestDrop();GscTestLeave();return;
            }
            gscTestDeleteTransactionsSelected=1;
            if(!GscTestLoopFxSelect(gscTestSession,error,sizeof(error))) {
                G_Printf("[GSC R243] loop FX owner selection failed: %s\n",error);
                GscTestDrop();GscTestLeave();return;
            }
            if(!gsc_session_select_loop_sound(gscTestSession,GscLoopSoundHostAPI(),error,sizeof(error))){
                G_Printf("[GSC R243] singleton loop sound selection failed: %s\n",error);
                GscTestDrop();GscTestLeave();return;
            }
            /* Original _fx::loopSoundthread declares Pos and reads pos.
             * Select the proven native local-name contract in this profile;
             * string values and array keys retain their existing case. */
            if(!gsc_session_select_local_identifier_case(gscTestSession,error,sizeof(error))){
                G_Printf("[GSC R243] loop sound local-name selection failed: %s\n",error);
                GscTestDrop();GscTestLeave();return;
            }
            G_Printf("[GSC R204] typed death-before-free delete transactions selected\n");
        }
        if((gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))G_Printf("[GSC R198] mode41 vectors/endon/player methods/large sources selected\n");
        else G_Printf("[GSC R152] mode40 conditional exact-domain dynamic randomfloat selected\n");
    } else
    if(gscTestEntityMode==39) {
        gscTestSession=GscTestCreateMode39(&host,error,sizeof(error));
        G_Printf("[GSC R151] mode39 bounded FLOAT-to-int randomint conversion selected\n");
    } else
    if(gscTestEntityMode==38) {
        gscTestSession=GscTestCreateMode38(&host,error,sizeof(error));
        G_Printf("[GSC R150] mode38 runtime-checked integer randomint bounds selected\n");
    } else
    if(gscTestEntityMode==37) {
        gscTestSession=GscTestCreateMode37(&host,error,sizeof(error));
        G_Printf("[GSC R149] mode37 entity-number and named-team spawner queries selected\n");
    } else
    if(gscTestEntityMode==36) {
        gscTestSession=GscTestCreateMode36(&host,error,sizeof(error));
        G_Printf("[GSC R148] mode36 structure objects/cached GAME difficulty selected\n");
    } else
    if(gscTestEntityMode==35) {
        gscTestSession=GscTestCreateMode35(&host,error,sizeof(error));
        G_Printf("[GSC R136] mode35 proved computed-int randomint/shared fresh GAME RNG selected\n");
    } else
    if(gscTestEntityMode==34) {
        gscTestSession=GscTestCreateMode34(&host,error,sizeof(error));
        G_Printf("[GSC R135] mode34 copied literal getkeybinding/current CGAME cache selected\n");
    } else
    if(gscTestEntityMode==33) {
        gscTestSession=GscTestCreateMode33(&host,error,sizeof(error));
        G_Printf("[GSC R134] mode33 exact-dyadic randomfloat/shared fresh GAME RNG selected\n");
    } else
    if(gscTestEntityMode==32) {
        gscTestSession=GscTestCreateMode32(&host,error,sizeof(error));
        G_Printf("[GSC R133] mode32 literal-int32 randomint/shared fresh GAME RNG selected\n");
    } else
    if(gscTestEntityMode==31) {
        gscTestSession=GscTestCreateMode31(&host,error,sizeof(error));
        G_Printf("[GSC R131] mode31 genuine copied spawner arrays/cumulative mode30 hosts selected\n");
    } else
    if(gscTestEntityMode==30) {
        gscTestSession=GscTestCreateMode30(&host,error,sizeof(error));
        G_Printf("[GSC R130] mode30 startup strings/native queries/genuine tag-FX provider selected\n");
    } else
    if (gscTestEntityMode == 16 || gscTestEntityMode == 17 || gscTestEntityMode == 18 || gscTestEntityMode == 19 || gscTestEntityMode == 20 || gscTestEntityMode == 21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
        GscSessionMusicHost music;GscSessionResourceHost resources;
        GscSessionObjectiveHost objectives;GscSessionConsoleHost console;GscSessionHudHost hud;
        GscSessionMissionHost mission;GscSessionBlastHost blast;GscSessionQuakeHost quake;GscSessionFxHost fx;GscSessionCvarHost cvars;GscSessionShellshockHost shellshock;GscSessionDebugTextHost debug_text;GscSessionDebugLineHost debug_line;GscSessionAmbientHost ambient;GscSessionPrecacheItemHost precache_item;GscSessionSaveGameHost save_game;
        if (!GscTestObjectiveBegin()) { GscTestDrop();GscTestLeave();return; }
        memset(&music,0,sizeof(music));memset(&resources,0,sizeof(resources));
        memset(&objectives,0,sizeof(objectives));memset(&console,0,sizeof(console));memset(&hud,0,sizeof(hud));memset(&mission,0,sizeof(mission));memset(&blast,0,sizeof(blast));memset(&quake,0,sizeof(quake));memset(&fx,0,sizeof(fx));
        memset(&cvars,0,sizeof(cvars));memset(&shellshock,0,sizeof(shellshock));memset(&debug_text,0,sizeof(debug_text));memset(&debug_line,0,sizeof(debug_line));memset(&ambient,0,sizeof(ambient));memset(&precache_item,0,sizeof(precache_item));memset(&save_game,0,sizeof(save_game));
        music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
        resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
        objectives.add=GscTestObjectiveAdd;objectives.delete_record=GscTestObjectiveDelete;
        objectives.state=GscTestObjectiveState;objectives.string=GscTestObjectiveString;
        objectives.string_nomessage=GscTestObjectiveStringNoMessage;
        objectives.position=GscTestObjectivePosition;objectives.current=GscTestObjectiveSelect;
        objectives.ring=GscTestObjectiveRing;
        if (gscTestEntityMode==17 || gscTestEntityMode==18 || gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
            console.query_gate=GscTestConsoleGate;
            console.print=GscTestConsolePrint;console.println=GscTestConsolePrintln;
            if (gscTestEntityMode==18 || gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
                hud.print=GscTestHudPrint;hud.bold=GscTestHudBold;
                if (gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
                    if (!GscTestMissionBegin()) { GscTestDrop();GscTestLeave();return; }
                    mission.query_gate=GscTestMissionGate;
                    mission.success=GscTestMissionSuccess;mission.failed=GscTestMissionFailed;
                    if (gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
                        blast.radius=GscTestBlastRadius;
                        if (gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
                            quake.emit=GscTestQuakeEmit;
                            if((gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) {
                                fx.load=GscTestFxLoad;fx.play=GscTestFxPlay;
                                if(gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) {
                                    cvars.publish=GscTestCvarPublish;
                                    if(gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) {
                                        shellshock.publish=GscTestShellshockPublish;
                                        if(gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) {
                                            debug_text.publish=GscTestDebugTextPublish;
                                            if((gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) {
                                                debug_line.publish=GscTestDebugLinePublish;
                                                if((gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))) {
                                                    ambient.publish=GscTestAmbientPublish;
                                                    if((gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))) {
                                                        precache_item.publish=GscTestItemPublish;
                                                        if((gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))) {
                                                            save_game.query=GscTestSaveQuery;save_game.publish=GscTestSavePublish;
                                                        gscTestSession=gsc_session_create_with_save_game(&host,
                                                            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                                            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                                            G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,&debug_text,&debug_line,&ambient,&precache_item,&save_game,error,sizeof(error));
                                                            G_Printf("[GSC R123] native saveGame request queue enabled; GSC VM persistence remains unavailable\n");
                                                        } else {
                                                        gscTestSession=gsc_session_create_with_precache_item(&host,
                                                            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                                            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                                            G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,&debug_text,&debug_line,&ambient,&precache_item,error,sizeof(error));
                                                        }
                                                        G_Printf("[GSC R121] native item descriptor/model acquisition/private client precache enabled\n");
                                                    } else {
                                                    gscTestSession=gsc_session_create_with_ambient(&host,
                                                        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                                        G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                                        G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,&debug_text,&debug_line,&ambient,error,sizeof(error));
                                                    }
                                                    G_Printf("[GSC R120] independent owned ambience transport enabled\n");
                                                } else {
                                                gscTestSession=gsc_session_create_with_debug_line(&host,
                                                    G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                                    G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                                    G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,&debug_text,&debug_line,error,sizeof(error));
                                                }
                                                G_Printf("[GSC R119] reliable SP world line enabled (100ms/width1 overlay; optional integer zero only)\n");
                                            } else {
                                            gscTestSession=gsc_session_create_with_debug_text(&host,
                                                G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                                G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                                G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,&debug_text,error,sizeof(error));
                                            }
                                            G_Printf("[GSC R118] reliable SP world glyph text enabled (100ms renderer policy)\n");
                                        } else {
                                        gscTestSession=gsc_session_create_with_shellshock(&host,
                                            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                            G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,&shellshock,error,sizeof(error));
                                        }
                                        G_Printf("[GSC R117] startup shellshock server names enabled (client load/application deferred)\n");
                                    } else {
                                    gscTestSession=gsc_session_create_with_cvars(&host,
                                        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                        G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                        G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,&cvars,error,sizeof(error));
                                    }
                                    G_Printf("[GSC R116] typed setcvar REGISTER/SET phases enabled (flags0, forced port import)\n");
                                } else {
                                gscTestSession=gsc_session_create_with_fx(&host,
                                    G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                    G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                    G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,&fx,error,sizeof(error));
                                G_Printf("[GSC R115] typed FX resource table and reliable SP scene channel enabled\n");
                                }
                            } else {
                            gscTestSession=gsc_session_create_with_quake(&host,
                                G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                                G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                                G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,&quake,error,sizeof(error));
                            }
                            G_Printf("[GSC R114] paired script earthquake channel enabled (reliable SP client policy)\n");
                        } else {
                        gscTestSession=gsc_session_create_with_blast(&host,
                            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                            G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,&blast,error,sizeof(error));
                        G_Printf("[GSC R113] checked nonactor script radius damage enabled (actor/model LOS unavailable)\n");
                        }
                    } else {
                        gscTestSession=gsc_session_create_with_mission(&host,
                            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                            G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,&mission,error,sizeof(error));
                        G_Printf("[GSC R112] bounded diagnostic mission intent enabled (no map transition/save/menu completion)\n");
                    }
                } else {
                    gscTestSession=gsc_session_create_with_hud(&host,
                        G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                        G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                        G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,&hud,error,sizeof(error));
                    G_Printf("[GSC R111] bounded normal/bold game HUD messages enabled (isolated test, not mission GSC)\n");
                }
            } else {
                gscTestSession=gsc_session_create_with_console(&host,
                    G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                    G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                    G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,&console,error,sizeof(error));
                G_Printf("[GSC R110] separately gated console output enabled (isolated test, not mission GSC)\n");
            }
        } else {
            gscTestSession=gsc_session_create_with_objectives(&host,
                G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
                G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
                G_Cod1EntityPredicateHost(),NULL,&music,&resources,&objectives,error,sizeof(error));
            G_Printf("[GSC R109] map-owned mission objective records enabled (isolated test, not mission GSC)\n");
        }
    } else if (gscTestEntityMode == 15) {
        GscSessionMusicHost music;GscSessionResourceHost resources;
        memset(&music,0,sizeof(music));memset(&resources,0,sizeof(resources));
        music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
        resources.shader=GscTestResourceShader;resources.model=GscTestResourceModel;
        gscTestSession=gsc_session_create_with_resources(&host,
            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
            G_Cod1EntityPredicateHost(),NULL,&music,&resources,error,sizeof(error));
        G_Printf("[GSC R108] initialization-only shader/model precache enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 14) {
        GscSessionMusicHost music;
        memset(&music,0,sizeof(music));
        music.play=GscTestMusicPlay;music.stop=GscTestMusicStop;
        gscTestSession=gsc_session_create_with_music(&host,
            G_Cod1EntityHost(),G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(),G_Cod1EntityCollectionHost(),
            G_Cod1EntityPredicateHost(),NULL,&music,error,sizeof(error));
        G_Printf("[GSC R107] canonical function names and mounted alias music enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 13) {
        gscTestSession = gsc_session_create_with_predicates(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), G_Cod1EntityCollectionHost(),
            G_Cod1EntityPredicateHost(), error, sizeof(error));
        G_Printf("[GSC R98] native entity predicates with combat payloads enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 11 || gscTestEntityMode == 12) {
        gscTestSession = gsc_session_create_with_event_payloads(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), G_Cod1EntityCollectionHost(),
            error, sizeof(error));
        if (gscTestEntityMode == 12)
            G_Printf("[GSC R97] guarded native damage/death events enabled (isolated test, not mission GSC)\n");
        G_Printf("[GSC R96] typed event payloads and local waittill outputs enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 10) {
        gscTestSession = gsc_session_create_with_modules(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), G_Cod1EntityCollectionHost(),
            error, sizeof(error));
        G_Printf("[GSC R95] bounded module dependencies and function results enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 9) {
        gscTestSession = gsc_session_create_with_results(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), G_Cod1EntityCollectionHost(),
            error, sizeof(error));
        G_Printf("[GSC R94] resumable script expressions and function results enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 8) {
        gscTestSession = gsc_session_create_with_arrays(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), G_Cod1EntityCollectionHost(),
            error, sizeof(error));
        G_Printf("[GSC R93] bounded arrays and entity collections enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 7) {
        gscTestSession = gsc_session_create_with_flow(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), error, sizeof(error));
        G_Printf("[GSC R92] bounded statement flow and entity controls enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 6) {
        gscTestSession = gsc_session_create_with_controls(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(),
            G_Cod1EntityControlHost(), error, sizeof(error));
        G_Printf("[GSC R91] entity controls and brush USE enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 5) {
        gscTestSession = gsc_session_create_with_state(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(), error, sizeof(error));
        G_Printf("[GSC R89] brush USE selection and hint enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 4) {
        gscTestSession = gsc_session_create_with_state(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(), error, sizeof(error));
        G_Printf("[GSC R88] script state host enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 3) {
        gscTestSession = gsc_session_create_with_events(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(), error, sizeof(error));
        G_Printf("[GSC R86] entity events host enabled (isolated event test, not mission GSC)\n");
    } else if (gscTestEntityMode == 2) {
        gscTestSession = gsc_session_create_with_access(&host,
            G_Cod1EntityHost(), G_Cod1EntityAccessHost(), error, sizeof(error));
        G_Printf("[GSC R85] entity access host enabled (isolated test, not mission GSC)\n");
    } else if (gscTestEntityMode == 1) {
        gscTestSession = gsc_session_create_with_entities(&host,
            G_Cod1EntityHost(), error, sizeof(error));
        G_Printf("[GSC R84] entity host enabled (isolated test, not mission GSC)\n");
    } else {
        gscTestSession = gsc_session_create(&host, error, sizeof(error));
    }
    if (!gscTestSession) {
        G_Printf("[GSC R77] create failed: %s\n", error);
        GscTestLeave(); return;
    }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestGrenades.selected && !gscTestCarbine.followup &&
       (!gsc_session_select_r216_items(gscTestSession,GscTestGrenadeFieldHost(),error,sizeof(error)) ||
        !gsc_session_select_r216_yaw(gscTestSession,GscTestGrenadeYawHost(),error,sizeof(error)))) {
        G_Printf("[R216] native item fields selection failed: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
#endif
    if(gscTestCarbine.selected&&
       (!gsc_session_select_r216_yaw(gscTestSession,G_Cod1YawMotionHost(),error,sizeof(error))||
        (!early&&!G_Cod1EntitySelectCarbineSpawn(G_Cod1SpawnTrainingWeapon))||
        !gsc_session_select_r222_carbine_items(gscTestSession,error,sizeof(error))||
        (!gsc_session_select_r222_actor_goal(gscTestSession,G_Cod1ActorGoalHost(),error,sizeof(error))||!gsc_session_select_entity_goal(gscTestSession)))){
        G_Printf("R222 carbine capabilities refused: %s\n",error);GscTestDrop();GscTestLeave();return;
    }
    if(gscTestCarbine.followup&&!gsc_session_select_r225_training_followup(gscTestSession,error,sizeof(error))){G_Printf("R225 hint selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
    if(gscTestCarbine.opening&&!gsc_session_select_r228_training_opening(gscTestSession,error,sizeof(error))){G_Printf("R228 opening selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
    if(gscTestTurret.selected&&!GscTestTurretSelect(G_Cod1ActorComposedAnimationHost(),G_Cod1ActorLookHost(),error,sizeof(error))){
        G_Printf("R232 original turret capabilities refused: %s\n",error);GscTestDrop();GscTestLeave();return;
    }
    {int selected=trap_Cvar_VariableIntegerValue("g_cod1GscMoveDelta");
     if(selected!=0&&selected!=1){G_Printf("[R129] invalid GetMoveDelta selector\n");GscTestDrop();GscTestLeave();return;}
     if(early&&gscTestTurret.nativeThreads&&gscTestCarbine.opening&&GscTestOriginalTrainingPath(path))selected=1;
     if(selected&&(!early||!R243GameMoveDeltaPrepare()||!gsc_session_select_r243_move_delta(gscTestSession,R243GameMoveDeltaHost(),error,sizeof(error)))){G_Printf("[R129] GetMoveDelta bank/Session selection refused\n");GscTestDrop();GscTestLeave();return;}}
    {int enemy=trap_Cvar_VariableIntegerValue("g_cod1GscActorFloatQuery"),path=trap_Cvar_VariableIntegerValue("g_cod1GscApproxPathDist");unsigned methods=((enemy||gscTestTurret.nativeThreads)?2u:0u)|(path?1u:0u)|(gscTestTurret.nativeThreads?4u:0u);
     if((enemy!=0&&enemy!=1)||(path!=0&&path!=1)||(methods&&(!gscTestTurret.selected||!gscTestTurret.programSelected||!gsc_session_select_r243_actor_float_methods(gscTestSession,R243GameActorFloatHost(),methods,error,sizeof(error))))){G_Printf("[R130] Actor float Session selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}}
    /* T04: explicit Training opening admits normal health and loadout hands.
     * No Actor health or original full-root admission is inferred. */
    if(gscTestCarbine.opening&&(!gsc_session_select_player_normal_health(gscTestSession,error,sizeof(error))||!gsc_session_select_player_viewmodel(gscTestSession,error,sizeof(error))||!gsc_session_select_map_game_array(gscTestSession,error,sizeof(error))||!gsc_session_select_player_max_ammo(gscTestSession,error,sizeof(error)))) {
        G_Printf("R243 player startup capabilities refused: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if(gscVehicleFramesSelected&&!GscMapItemSelect(error,sizeof(error))){G_Printf("[Campaign] item services selection refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
    if(gscTestEntityMode==43&&G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1){
        GscSessionNodeProgramPlan nodes;
        if(!G_Cod1EarlyNodeProgramPlan(&nodes)){G_Printf("[GSC R73] early node declarations unavailable\n");GscTestDrop();GscTestLeave();return;}
        if(nodes.scripts.count){
            if(!gsc_session_select_node_program(gscTestSession,&nodes,error,sizeof(error))){G_Printf("[GSC R73] node program selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
            nodeProgramSelected=1;
        }
    }
    if(early){
        GscSessionScriptRootsPlan roots;memset(&roots,0,sizeof(roots));
        roots.epoch=gscTestEarlyProgram.epoch;roots.receipt=gscTestEarlyProgram.receipt;
        roots.user=&gscTestEarlyProgram;roots.current=GscTestScriptRootsCurrent;
        if(strlen(gscTestEarlyProgram.values[15])>=sizeof(roots.mapname)){GscTestDrop();GscTestLeave();return;}
        strcpy(roots.mapname,gscTestEarlyProgram.values[15]);
        if(!gsc_session_select_script_roots(gscTestSession,&roots,error,sizeof(error))){G_Printf("[GSC R78] global/map program selection failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
    }
    if (!gsc_session_prepare_program(gscTestSession, path, 1, &gscTestId, error, sizeof(error))) {
        G_Printf("[GSC R77] load/preflight failed: %s\n", error);
        GscTestDrop(); GscTestLeave(); return;
    }
    if(early){
        GscSessionSnapshot snapshot;
        if(!GscTestEarlyCurrent()||!gsc_session_snapshot(gscTestSession,&snapshot)||snapshot.state!=GSC_SESSION_COMPILED||snapshot.session_id!=gscTestId){GscTestDrop();GscTestLeave();return;}
        gscTestEarlyProgram.id=gscTestId;gscTestEarlyProgram.nodeSelected=nodeProgramSelected;
        memcpy(gscTestEarlyProgram.path,path,sizeof(path));gscTestEarlyProgram.prepared=1;
        G_Printf("[GSC R76] program compiled before BSP allocation epoch=%llu id=%llu\n",(unsigned long long)gscTestEarlyProgram.epoch,(unsigned long long)gscTestId);
        GscTestLeave();return;
    }
admit_program:
    if(gscTestEarlyProgram.consumed&&gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch()&&gscTestCarbine.selected&&
       !G_Cod1EntitySelectCarbineSpawn(G_Cod1SpawnTrainingWeapon)){G_Printf("[GSC R76] late weapon spawn binding refused\n");GscTestDrop();GscTestLeave();return;}
    if(gscTestEntityMode==43&&G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1){
        const GscSessionActorBootstrapHost *actors=G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1?G_Cod1ActorBspBootstrapHost():G_Cod1ActorBootstrapHost();
        if(gscTestCarbine.selected)actors=gscTestTurret.selected?GscTestTurretActorHost(actors):GscTestCarbineActorHost(actors);
        if(!actors){G_Printf("[GSC R70] real actor cohort host unavailable (retained BSP cohort or capability wrapper refused)\n");GscTestDrop();GscTestLeave();return;}
        if(!gsc_session_admit_actor_bootstrap(gscTestSession,gscTestId,actors,error,sizeof(error))){G_Printf("[GSC R70] real actor cohort admission failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
        readonlyContext.map=actors->map_certificate;
    }
    if(nodeProgramSelected){GscSessionNodeScriptAdmission nodes;
        if(!G_Cod1NodeScriptAdmission(&nodes)||!gsc_session_admit_node_scripts(gscTestSession,gscTestId,&nodes,error,sizeof(error))){G_Printf("[GSC R73] real node registry admission failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
    }
    if(gscTestTurret.selected&&!GscTestTurretAdmit(error,sizeof(error),&readonlyContext)){G_Printf("[GSC R74] turret resource admission failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
    if(!gsc_session_prepare_runtime(gscTestSession,gscTestId,error,sizeof(error))){G_Printf("[GSC R70] runtime preparation failed: %s\n",error);GscTestDrop();GscTestLeave();return;}
    if (readonlyContext.map.current && readonlyContext.animation.current) {
        readonlyContext.frame_active=GscTestReadOnlyFrameActive;
        G_Cod1BindReadOnlyActor(&readonlyContext);
        gsc_session_bind_scan(gscTestSession,gscTestId,G_Cod1EntityHost(),&readonlyContext,
            gscTestEntityMode==43 && G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1);
    }
    if(gscTestEarlyProgram.consumed&&gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch()&&
       (!GscTestEarlyCurrent()||gscTestStopRequested)){G_Printf("[GSC R76] early configuration retired before constructors\n");GscTestDrop();GscTestLeave();return;}
    if(gscVehicleFramesSelected&&!GscMapLengthResources(error,sizeof(error))){G_Printf("[Campaign] animation length precache refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
    G_Printf("[GSC R77] prepared %s (no effects during load)\n", path);
    if(G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1)G_Printf("[GSC R109] retained BSP runtime promoted in place; private fields preserved, no constructor replay\n");
    /* Distinct synchronous constructor handoff. Original per-BSP scheduling
     * and export2 graph integration remain open; no late-phase readiness is
     * inferred from these original allocations being constructed. */
    if(gscTestEntityMode==43&&G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)==1&&
       !gsc_session_construct_actors(gscTestSession,gscTestId,error,sizeof(error))){
        G_Printf("[GSC R66] explicit actor construction failed: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if(G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)!=0){
        int graphMode=G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP);
        if((graphMode!=1&&graphMode!=2)||G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)!=1||G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)!=(graphMode==2?0:1)||(graphMode==2&&trap_Cvar_VariableIntegerValue("dedicated")!=1)){GscTestDrop();GscTestLeave();return;}
        gscTestGraph.probe=graphMode==2;
        if(gscTestEntityMode!=43||!nodeProgramSelected||!gscTestEarlyProgram.consumed){G_Printf("[GSC R110] graph boundary requires early mode43/node program\n");GscTestDrop();GscTestLeave();return;}
        gscTestGraph.id=gscTestId;gscTestGraph.epoch=G_Cod1EntityMapEpoch();gscTestGraph.selected=1;
        G_Printf("[GSC R110] native constructors complete; graph awaits pre-player export2 boundary\n");
    }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestGrenades.selected)gscTestGrenades.id=gscTestId;
    if(gscTestCarbine.selected)gscTestCarbine.id=gscTestId;
    if(gscTestTurret.selected)gscTestTurret.id=gscTestId;
#endif
    if ((gscTestEntityMode >= 10 && gscTestEntityMode<=47) && !GscTestModuleReadCurrent()) {
        G_Printf("[GSC R95] stopped: module load mode changed before start\n");
        GscTestDrop(); GscTestLeave(); return;
    }
    if (gscTestStopRequested) { GscTestLeave(); return; }
    if(G_Cod1GscFeature47(C1CP47_F_CLIENT_STARTUP)==1){
        if(!GscTestClientStartupArm(path,error,sizeof(error))){G_Printf("[GSC R104] pending startup refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
        if(gscTestGraph.selected&&!GscTestGraphPrepareItems(error,sizeof(error))){G_Printf("[GSC R110] prepared item construction refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
        G_Printf("[GSC R104] prepared startup awaits completed ClientBegin; no script executed\n");GscTestLeave();return;
    }
    if(gscTestGraph.selected){
        if(!GscTestPrecacheStringBegin(error,sizeof(error))){G_Printf("[GSC R110] probe loading owner refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
        gscTestGraph.string_incarnation=gscTestPrecacheString.incarnation;strcpy(gscTestGraph.owner,gscTestPrecacheString.marker);
        if(!GscTestGraphPrepareItems(error,sizeof(error))){G_Printf("[GSC R110] prepared item construction refused: %s\n",error);GscTestDrop();GscTestLeave();return;}
        GscTestLeave();return;
    }
    GscTestStartPrepared(levelTime);
}
static void GscTestStartPrepared(int levelTime){
    char error[GSC_SESSION_ERROR_CAP]={0};int started;
    if(gscTestGraph.selected){
        if(!gscTestGraph.published||!GscTestGraphCurrent(1)||(!gscTestGraph.probe&&gscTestClientStartup.phase!=2)){G_Printf("[GSC R110] later startup requires current graph and exact continuation owner\n");GscTestDrop();GscTestLeave();return;}
        gscTestGraph.started=1;
    }

    if ((gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && !GscTestQuakeBegin(error,sizeof(error))) {
        G_Printf("[GSC R114] scene marker refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    {char probePath[128];GscConfigString47(C1CP47_K_ROOT,probePath,sizeof(probePath));if((!strcmp(probePath,"maps/vehicle_factory_probe.gsc")||!strcmp(probePath,"maps/pathfinder.gsc"))&&!GscVehicleResourcesBegin()){G_Printf("[vehicle] resource bank refused\n");GscTestDrop();GscTestLeave();return;}}
    if((gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) && !GscTestFxBegin(error,sizeof(error))) {
        G_Printf("[GSC R115] FX scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) && !GscTestShellshockBegin(error,sizeof(error))) {
        G_Printf("[GSC R117] shellshock scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))) && !GscTestDebugTextBegin(error,sizeof(error))) {
        G_Printf("[GSC R118] debug text scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))) && !GscTestDebugLineBegin(error,sizeof(error))) {
        G_Printf("[GSC R119] debug line scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))) && !GscTestAmbientBegin(error,sizeof(error))) {
        G_Printf("[GSC R120] ambience scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))) && !GscTestItemBegin(error,sizeof(error))) {
        G_Printf("[GSC R121] item scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))) && !GscTestSaveBegin(error,sizeof(error))) {
        G_Printf("[GSC R123] saveGame request scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    if((gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))) && !(gscTestClientStartup.phase==2?GscTestClientStartupRetime(error,sizeof(error)):(gscTestGraph.selected?GscTestGraphProbeRetime(error,sizeof(error)):GscTestPrecacheStringBegin(error,sizeof(error))))) {
        G_Printf("[GSC R130] localized-string scene refused before start: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
    gscTestItemInitialization=(gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))));
    gscTestShellshockInitialization = gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))));
    gscTestResourceInitialization = gscTestEntityMode >= 15 && gscTestEntityMode<=43;
    /* Own metadata before scripts run; SET_CLOCK uses actual GAME time. */
    if(gscTestEntityMode==43&&!GscTestHudBegin(error,sizeof(error))){
        G_Printf("[GSC R200] HUD marker refused before start: %s\n",error);GscTestDrop();GscTestLeave();return;
    }
    {cod1LocalizedStartup_t scope;int scoped=gscTestClientStartup.phase==2,ended=1;uint64_t serial=gscTestClientStartup.serial;
    if(scoped&&!GscTestClientStartupBegin(&scope)){G_Printf("[GSC R104] connected startup scope refused\n");GscTestDrop();GscTestLeave();return;}
    started = GscMapItemBegin(error,sizeof(error)) && GscTestGraphExecuteItems(error,sizeof(error)) && gsc_session_start(gscTestSession, gscTestId, (int64_t)levelTime,
                           error, sizeof(error));
    if(started && gscTestCarbine.selected)
        started=GscTestRegisterPlacedWeapons(error,sizeof(error));
    if(scoped){scope.operation=COD1_LOCALIZED_STARTUP_END;ended=trap_Cod1LocalizedStartup(&scope);
      if(ended&&started&&gscTestClientStartup.serial==serial&&gscTestClientStartup.phase==2&&!gscTestClientStartup.cancelled)gscTestClientStartup.phase=3;else started=0;
      if(!ended)GscTestError(error,sizeof(error),"localized startup scope lost its owner");}
    }
    gscTestShellshockInitialization = 0;
    gscTestResourceInitialization = 0;
    gscTestItemInitialization = 0;
    if (!started) {
        G_Printf("[GSC R77] start failed: %s\n", error);
        GscTestDrop(); GscTestLeave(); return;
    }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestGrenades.selected && !GscTestGrenadeBootstrap(error,sizeof(error))) {
        G_Printf("[R216] original BSP grenade adoption failed: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
#endif
    if ((gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && !GscTestBlastEventsFlush(error,sizeof(error))) {
        G_Printf("[GSC R113] combat delivery failed after committed damage: %s\n",error);
        GscTestDrop();GscTestLeave();return;
    }
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(gscTestGrenadePrepared.transaction){cod1ItemPreparedReceipt_t prepared=gscTestGrenadePrepared;
        if(!GscTestGrenadeCurrent(NULL)||!G_Cod1ItemLifecyclePreparedCommit(&prepared,error,sizeof(error))){
            G_Printf("[GSC R111] prepared item runtime ownership commit refused: %s\n",error);GscTestDrop();GscTestLeave();return;
        }
        memset(&gscTestGrenadePrepared,0,sizeof(gscTestGrenadePrepared));
    }
#endif
    G_Printf("[GSC R77] started at game time %dms\n", levelTime);
    GscTestCheckCompleted(); GscTestLeave();
}

/* New-map configuration precedes actor census, DATA capture and all script
 * loading. No live Session is reconfigured. Scripts stay in original VFS. */
void G_Cod1GscCampaignMapProfile(void){
 char map[MAX_QPATH],path[MAX_QPATH];unsigned i;const c1cp47Route *route;
 static const char*off[]={"g_cod1GscActorPrefix","g_cod1GscAiArrays","g_cod1GscDeleteTransactions","g_cod1GscTrainingGrenades","g_cod1GscTrainingCarbine","g_cod1GscR232Turret","g_cod1GscTrainingFinale","g_cod1GscTrainingOpening","g_cod1GscNativeThreads","g_cod1GscPlacedInit","g_cod1GscPlacedCurrentState","g_cod1GscPlacedCohort","g_cod1GscGraphStartup","g_cod1GscBspConstruct","g_cod1GscMoodyCharacter","g_cod1GscTrainingRigs"};
 if(gscTestSession||gscTestBusy||level.restarted||g_gametype.integer!=GT_SINGLE_PLAYER||trap_Cvar_VariableIntegerValue("savegame_loading")||G_Cod1GscFeature47(C1CP47_F_ENABLE)!=1||GscTestEntityModeValue()!=43)return;
 GscConfigString47(C1CP47_K_MAP,map,sizeof(map));GscConfigString47(C1CP47_K_ROOT,path,sizeof(path));
 route=C1CP47_LegacyTransition(map,path);
 if(route&&route->id==C1CP47_US_INTRO){
  for(i=0;i<sizeof(off)/sizeof(off[0]);i++)trap_Cvar_Set(off[i],"0");
  trap_Cvar_Set("g_cod1GscTestScript",route->root);
  trap_Cvar_Set("g_cod1GscClientStartup","1");trap_Cvar_Set("g_cod1GscScriptSound","1");
  trap_Cvar_Set("g_cod1GscSoundNotify","1");trap_Cvar_Set("g_cod1GscStreamedVoice","1");
  G_Printf("[Campaign] map entry selected original maps/us_intro.gsc\n");
 }
 if(route&&route->id==C1CP47_PATHFINDER){
  trap_Cvar_Set("g_cod1GscAiArrays","1");
  trap_Cvar_Set("g_cod1GscDeleteTransactions","1");
  trap_Cvar_Set("g_cod1GscActorPrefix","1");trap_Cvar_Set("g_cod1GscGraphStartup","1");trap_Cvar_Set("g_cod1GscBspConstruct","1");
  G_Printf("[Campaign] Pathfinder early original program and real Actor/node construction selected\n");
 }
}

int G_Cod1GscTestPrepareBeforeBsp(int levelTime){
    const GscSessionActorProgramPlan *plan;
    if(!GscTestEarlySelected())return 1;
    if(gscTestBusy||(gscTestEarlyProgram.attempted&&gscTestEarlyProgram.epoch==G_Cod1EntityMapEpoch())){
        gscTestEarlyProgram.cancelled=1;gscTestStopRequested=1;return 0;
    }
    if(!level.spawning||level.restarted)return 0;
    plan=G_Cod1ActorProgramPlan();if(!plan||!plan->current)return 0;
    memset(&gscTestEarlyProgram,0,sizeof(gscTestEarlyProgram));
    gscTestEarlyProgram.epoch=plan->epoch;gscTestEarlyProgram.receipt=plan->receipt;gscTestEarlyProgram.current=plan->current;gscTestEarlyProgram.user=plan->user;
    gscTestEarlyProgram.attempted=1;
    GscTestInitPhase(levelTime,1);gscTestPreparingEarly=0;
    return gscTestEarlyProgram.prepared&&GscTestEarlyCurrent();
}
void G_Cod1GscTestInitAfterMapBootstrap(int levelTime){GscTestInitPhase(levelTime,0);}

/* Called by actual command2 before ClientConnect touches the player. The
 * dedicated console probe invokes this same transaction without claiming a
 * real connected-client or native full-bootstrap acceptance. */
int G_Cod1GscTestGraphBeforePlayer(int clientNum){
 GscSessionSnapshot view;uint64_t stage=0;cod1NodeGraphPublication_t info;int ok=0,itemRead=0;
 if(!gscTestGraph.selected)return G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==0;
 if(clientNum!=0||!GscTestEnter())return 0;
 if(!GscTestGraphCurrent(!gscTestGraph.started))goto done;
 if(gscTestGraph.published){ok=!gscTestStopRequested;goto done;}
 if(gscTestGraph.attempted||!gsc_session_snapshot(gscTestSession,&view)||view.state!=GSC_SESSION_PREPARED||view.active_threads)goto done;
 gscTestGraph.attempted=1;
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
 if(gscTestGrenades.selected){
  if(!gscTestGrenades.ready||gscTestGrenadePrepare.active)goto done;
  gscTestGrenadePrepare.session=gscTestSession;gscTestGrenadePrepare.id=gscTestId;
  gscTestGrenadePrepare.epoch=G_Cod1EntityMapEpoch();gscTestGrenadePrepare.active=1;itemRead=1;
 }
#endif
 if(!G_Cod1NodeGraphStageMap(gscTestSession,gscTestId,&stage)||!GscTestGraphCurrent(1)||
    !G_Cod1NodeGraphPublish(gscTestSession,gscTestId,stage)||!GscTestGraphCurrent(1))goto done;
 info=G_Cod1NodeGraphPublishedInfo(gscTestSession,gscTestId,stage);
 if(info.receipt!=stage||!info.nodes||!GscTestGraphCurrent(1))goto done;
 gscTestGraph.receipt=stage;gscTestGraph.published=1;ok=1;
 G_Printf("[GSC R110] pre-player graph published receipt=%llu nodes=%u links=%u BeginRefs=%u; acquisition world consumed\n",(unsigned long long)stage,info.nodes,info.links,info.beginRefs);
done:
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
 if(itemRead)memset(&gscTestGrenadePrepare,0,sizeof(gscTestGrenadePrepare));
#endif
 if(!ok){G_Cod1NodeGraphStageDiscard(stage);G_Printf("[GSC R110] pre-player graph refused; exact Session/native prefix cleanup requested\n");GscTestDrop();}
 if(gscTestStopRequested)ok=0;GscTestLeave();return ok;
}
int G_Cod1GscTestGraphCommand(const char*name){
 cod1NodeGraphPublication_t info;cod1NodeStageTarget_t target;
 if(!name||(strcmp(name,"cod1_graph_prepare")&&strcmp(name,"cod1_graph_continue")&&strcmp(name,"cod1_graph_check")))return 0;
 if(trap_Cvar_VariableIntegerValue("dedicated")!=1||G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)!=2){G_Printf("[GSC R110] bounded graph probe requires selected dedicated mode\n");return 1;}
 if(!strcmp(name,"cod1_graph_prepare")){G_Cod1GscTestGraphBeforePlayer(0);return 1;}
 if(!gscTestSession||!gscTestGraph.published)return 1;
 if(!strcmp(name,"cod1_graph_continue")){
  if(!gscTestGraph.probe||gscTestGraph.started||!GscTestEnter())return 1;gscTestGraph.started=1;GscTestStartPrepared(level.time);return 1; /* StartPrepared leaves its entry scope. */
 }
 info=G_Cod1NodeGraphPublishedInfo(gscTestSession,gscTestId,gscTestGraph.receipt);
 target=G_Cod1NodeGraphPublishedTarget(gscTestSession,gscTestId,gscTestGraph.receipt,"auto504");
 G_Printf("[GSC R110] graph after continuation nodes=%u links=%u BeginRefs=%u target_status=%d; no traversal grant\n",info.nodes,info.links,info.beginRefs,target.status);return 1;
}
static void GscTestGraphClientSpawned(struct gentity_s*entity){GscEntityRef ref;cod1PlayerLifetime_t completed;
 if(!gscTestGraph.selected||gscTestGraph.probe||!gscTestGraph.published||gscTestGraph.started||
    !gscTestSession||gscTestGraph.id!=gscTestId||gscTestGraph.epoch!=G_Cod1EntityMapEpoch()||
    !entity||entity!=&g_entities[0]||entity->client!=&g_clients[0]||entity->client->pers.connected!=CON_CONNECTED||
    level.spawning||level.restarted||!G_Cod1EntityCapture(entity,&ref)||
    !G_Cod1PlayerLifetimeRead(&ref,&completed))return;
 if(gscTestGraph.clientReady){
  if(gscTestGraph.completed_spawn.completion!=completed.completion||
     !G_Cod1PlayerLifetimeSameCompleted(&gscTestGraph.completed_spawn))gscTestGraph.cancelled=1;
  return;
 }
 gscTestGraph.player=ref;gscTestGraph.client=entity->client;
 gscTestGraph.completed_spawn=completed;gscTestGraph.clientReady=1;
}
/* Real map spawning boundary; no late fake spawning flag or resource stamp. */
int G_Cod1GscTestPreparePlacedResources(void){
 int selected=G_Cod1GscFeature47(C1CP47_F_PLACED_STATE);
 if(!selected)return 1;
 return selected==1&&level.spawning&&!level.restarted&&GscTestEarlyPending()&&GscTestEarlyCurrent()&&
  G_Cod1GscFeature47(C1CP47_F_NATIVE_THREADS)==1&&G_Cod1GscFeature47(C1CP47_F_PLACED_INIT)==1&&
  G_Cod1ActorPlacedCurrentResourcesLoad()&&GscTestEarlyCurrent()&&level.spawning&&!level.restarted;
}
#include "g_cod1_native_thread_frame.inc"
static int GscTestFrameSelectionRetired(void){
    if(!GscCampaignNativeLive47())return 1;
    return (gscSoundStreamedSelected&&G_Cod1GscFeature47(C1CP47_F_VOICE)!=1)||(gscTestFinaleSelected&&G_Cod1GscFeature47(C1CP47_F_FINALE)!=1)||(gscTestCarbine.opening&&G_Cod1GscFeature47(C1CP47_F_OPENING)!=1)||(gscSoundNotifySelected&&G_Cod1GscFeature47(C1CP47_F_SOUND_NOTIFY)!=1)||(gscTestCarbine.selected && G_Cod1GscFeature47(C1CP47_F_CARBINE)!=1) || (gscTestDeleteTransactionsSelected && G_Cod1GscFeature47(C1CP47_F_DELETE)!=1) ||
        (gscTestActorPrefixEpoch && G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX)!=1) ||
        G_Cod1GscFeature47(C1CP47_F_ENABLE) != 1 ||
        GscTestEntityModeValue() != gscTestEntityMode ||
        trap_Cvar_VariableIntegerValue("savegame_loading") != 0 ||
        level.restarted || ((gscTestEntityMode==44 || gscTestEntityMode==45 || gscTestEntityMode==46 || gscTestEntityMode==47) && (!gscTestMapEpoch || gscTestMapEpoch!=G_Cod1EntityMapEpoch())) || ((gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))) &&
            (!gscTestMapEpoch || gscTestMapEpoch!=G_Cod1EntityMapEpoch()));
}
static int GscTestBeginFrame(int levelTime,char *error){
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    G_Cod1LookAtRunFrame();
#endif
    if (!GscTestEnter()) return 0;
    if (!gscTestSession) { GscTestLeave(); return 0; }
    if(gscChangeLevel.phase==3){GscTestLeave();return 0;}
    if(GscTestGraphPendingFrame(levelTime))return 0;
    if(GscTestClientStartupFrame(levelTime))return 0;
#if defined(COD1_GSC_R216_GAMEPLAY_WIP) && COD1_GSC_R216_GAMEPLAY_WIP
    if(!gscTestCompleted && gscTestGrenades.selected && !GscTestGrenadeBootstrap(error,GSC_SESSION_ERROR_CAP)) {
        G_Printf("[R216] grenade bootstrap/currentness failed: %s\n",error);
        GscTestDrop();GscTestLeave();return 0;
    }
#endif
    if(GscTestFrameSelectionRetired()) {
        G_Printf("[GSC R77] stopped: disabled, save load or level restart\n");
        GscTestDrop(); GscTestLeave(); return 0;
    }
    if(!GscChangeFrame()){G_Printf("[Campaign] changelevel frame ownership refused\n");GscTestDrop();GscTestLeave();return 0;}
    if(gscTestCompleted||gscChangeLevel.phase==3){GscTestLeave();return 0;}
    if(!GscSoundNotifyFlush(levelTime,error,GSC_SESSION_ERROR_CAP)){G_Printf("[R227] sound completion failed: %s\n",error);GscTestDrop();GscTestLeave();return 0;}
    return 1;
}

static int GscTestFinishFrame(void){
    memset(&gscFrameCursor,0,sizeof(gscFrameCursor));
    GscTestLeave();
    return 1;
}

static int (*gscFrameDeadline)(void);
static int GscTestFrameExpired(void){
    /* A synchronous host callback can request retirement. Let the running
     * operation reach its original refusal/cleanup path first. */
    return !gscTestStopRequested && gscFrameDeadline();
}
static int GscTestPauseFrame(void){
    GscTestLeave();
    return 0;
}

static int GscTestResumeFrame(int now){
    int vmNext;
    if(level.time!=now || level.restarted || !gscTestSession || gscTestId!=gscFrameCursor.sessionId)return 0;
    if(gscFrameCursor.generation){
        /* This certificate includes the selected map, native threads, script
         * path, course settings and developer mode. It also admits a completed
         * Session whose committed damage/save queues are still due in POST. */
        if(!gscTestTurret.nativeThreads || !GscTestEarlyCurrent())return 0;
    }else if(GscTestFrameSelectionRetired())return 0;
    vmNext=gscFrameCursor.stage==GSC_FRAME_NATIVE &&
          (gscFrameCursor.nativeStage==NATIVE_CURRENT || gscFrameCursor.nativeStage==NATIVE_EXCLUSIVE);
    return vmNext || gsc_session_resume_game_frame(gscTestSession,gscTestId,now,
                gscFrameCursor.generation,gscFrameCursor.stage==GSC_FRAME_NATIVE);
}

int G_Cod1GscTestRunFrameSlice(int levelTime,int (*deadline)(void)){
    int (*expired)(void)=deadline?GscTestFrameExpired:NULL;
    char error[GSC_SESSION_ERROR_CAP]={0};
    int advanced=1,result;
    if(gscFrameCursor.stage==GSC_FRAME_BEGIN){
        if(!GscTestBeginFrame(levelTime,error))return 1;
        gscFrameCursor.sessionId=gscTestId;
        gscFrameCursor.profile=trap_Cvar_VariableIntegerValue("g_cod1GscProfile");
        gscFrameCursor.stage=GSC_FRAME_NATIVE;
    }else{
        if(!GscTestEnter())return 0;
        if(!GscTestResumeFrame(levelTime)){
            G_Printf("[GSC] paused frame lost its owner at %d\n",levelTime);
            GscTestDrop();
            return GscTestFinishFrame();
        }
    }
    gscFrameDeadline=deadline;
    if(gscFrameCursor.stage==GSC_FRAME_NATIVE){
        unsigned begin=gscFrameCursor.profile?(unsigned)trap_Milliseconds():0;
        c1fp35Token profilePart=C1FP35_Begin(levelTime);
        result=gscTestCompleted?1:GscTestNativeFrameAdvance(levelTime,&advanced,expired,error,sizeof(error));
        C1FP35_End(C1FP35_NATIVE,profilePart,result);
        if(gscFrameCursor.profile)gscFrameCursor.profileElapsed+=(unsigned)trap_Milliseconds()-begin;
        if(result==2)return GscTestPauseFrame();
        if(!result){
            G_Printf("[GSC R77] frame failed: %s\n",error);
            GscTestDrop();
            return GscTestFinishFrame();
        }
        if(gscFrameCursor.profile)G_Printf("[R235 profile] time=%d session ms=%d\n",levelTime,(int)gscFrameCursor.profileElapsed);
        if(!advanced)return GscTestFinishFrame();
        gscFrameCursor.stage=GSC_FRAME_POST;
        if(expired && expired())return GscTestPauseFrame();
    }
    if(gscFrameCursor.stage==GSC_FRAME_POST){
        if ((gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && !GscTestBlastEventsFlush(error,sizeof(error))) {
            G_Printf("[GSC R113] combat delivery failed after committed damage: %s\n",error);
            GscTestDrop();return GscTestFinishFrame();
        }
        if((gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))) && !GscTestSaveFlush(error,sizeof(error))) {
            G_Printf("[GSC R123] queued saveGame request dispatch refused: %s\n",error);
            GscTestDrop();return GscTestFinishFrame();
        }
        if(!GscChangeFrame()){G_Printf("[Campaign] changelevel commit refused\n");GscTestDrop();return GscTestFinishFrame();}
        /* Drain already committed damage/save work above even if advance ended the
         * last task. Then retire before services requiring a running Session. */
        GscTestCheckCompleted();
        if(gscTestCompleted||gscTestStopRequested){return GscTestFinishFrame();}
        if(gscTestCarbine.moody&&!GscTestMoodyFrame(levelTime,error,sizeof(error))){
            G_Printf("[R223] Moody motion failed: %s\n",error);GscTestDrop();return GscTestFinishFrame();
        }
        gscFrameCursor.stage=GSC_FRAME_ACTORS;
        if(expired && expired())return GscTestPauseFrame();
    }
    if(gscTestTurret.selected){
        c1fp35Token profilePart=C1FP35_Begin(levelTime);
        result=GscTestTurretFrame(levelTime,&gscFrameCursor.nextRunner,expired,error,sizeof(error));
        C1FP35_End(C1FP35_ACTORS,profilePart,result);
        if(result==2)return GscTestPauseFrame();
        if(!result){
            G_Printf("[R232] original turret course frame refused: %s\n",error);
            GscTestDrop();
            return GscTestFinishFrame();
        }
    }
    if(!GscTestFinaleFrame(error,sizeof(error))){
        G_Printf("[R229] finale frame refused: %s\n",error);GscTestDrop();return GscTestFinishFrame();
    }
    GscTestCheckCompleted();
    if(!gscTestCompleted&&!gscTestStopRequested&&!GscTestLoopFxFrame()){
        G_Printf("[GSC R243] pending loop FX delivery failed\n");GscTestDrop();
    }
    return GscTestFinishFrame();
}

void G_Cod1GscTestRunFrame(int levelTime){
    (void)G_Cod1GscTestRunFrameSlice(levelTime,NULL);
}

static int GscTestEventPlayerReady( const gentity_t *player, int use ) {
    if (!player || !player->client ||
        player->client->ps.stats[STAT_HEALTH] <= 0 || player->aiCharacter ||
        player->client->noclip || player->client->sess.sessionTeam == TEAM_SPECTATOR) return 0;
    /* Keep TOUCH's earlier eligibility. USE additionally excludes inactive
     * participants and the states handled by the existing activation tail. */
    return !use || (player->health > 0 && !player->client->ps.aiChar &&
        !(player->r.svFlags & SVF_CASTAI) &&
        !(player->client->ps.pm_flags & PMF_LIMBO) &&
        !player->active && !player->melee &&
        !player->client->ps.persistant[PERS_HWEAPON_USE]);
}

static int GscTestEntityEvent( gentity_t *owner, gentity_t *player,
                              const char *event, int use ) {
    GscEntityRef ref, playerRef;
    char error[GSC_SESSION_ERROR_CAP];
    int awakened = 0, accepted = 0;
    /* Disabled/older constructors cannot be cancelled by an event request. */
    if (!gscTestSession || gscTestCompleted || !GscTestEventMode(gscTestEntityMode)) return 0;
    /* A collision callback must not reenter an executing VM. Calling the
     * session guard also aborts an in-flight host callback; cleanup waits for
     * its return. If the session is idle, the missing owner rejects only this
     * request, and the outer adapter still observes StopRequested. */
    if (gscTestBusy) {
        gscTestStopRequested = 1;
        if (gscTestSession && GscTestEventMode(gscTestEntityMode))
            gsc_session_notify_entity(gscTestSession, gscTestId, NULL,
                event, &awakened, error, sizeof(error));
        return 0;
    }
    if (!GscTestEnter()) return 0;
    if (use >= 0 && !player) { GscTestLeave(); return 0; }
    /* Preserve this allocation before the selector's engine callbacks. A
     * replacement born in the same slot must never become this event owner. */
    if (!G_Cod1EntityCapture(owner, &ref)) {
        GscTestLeave(); return 0;
    }
    if (player && (!G_Cod1EntityCapture(player, &playerRef) ||
        !GscTestEventPlayerReady(player, use))) {
        GscTestLeave(); return 0;
    }
    if (!G_Cod1GscTestEntityEventsEnabled() || gscTestStopRequested ||
        ((gscTestEntityMode >= 5 && gscTestEntityMode<=43) &&
            GscTestEntityModeValue() != gscTestEntityMode) ||
        gscTestStopRequested ||
        level.restarted || !G_Cod1EntitySameAllocation(&ref) ||
        (player && (!G_Cod1EntitySameAllocation(&playerRef) ||
            !GscTestEventPlayerReady(player, use)))) {
        GscTestLeave(); return 0;
    }
    if ((gscTestEntityMode >= 11 && gscTestEntityMode<=43) && player && use >= 0) {
        GscEventValue activator;
        /* Original USE and supported TOUCH producers notify the trigger with
         * one actor entity. Preserve the earlier modes' event-only protocol. */
        memset(&activator, 0, sizeof(activator));
        activator.kind = GSC_EVENT_ENTITY; activator.entity = playerRef;
        accepted = gsc_session_notify_entity_with_payload(gscTestSession,
            gscTestId, &ref, event, &activator, 1, &awakened,
            error, sizeof(error));
    } else {
        accepted = gsc_session_notify_entity(gscTestSession, gscTestId, &ref,
            event, &awakened, error, sizeof(error));
    }
    if (accepted && !gscTestStopRequested && awakened > 0) {
        G_Printf("[GSC R86] entity event entity=%u generation=%u name=%s awakened=%d\n",
            (unsigned)ref.slot, (unsigned)ref.generation, event, awakened);
    }
    accepted = accepted && !gscTestStopRequested;
    GscTestLeave();
    return accepted;
}

int G_Cod1GscTestEntityEvent( struct gentity_s *owner, const char *event ) {
    return GscTestEntityEvent(owner, NULL, event, -1);
}

/* Explicit producer bridge: a spawned-entity notification is supplied by its
 * producer, never fabricated by USE/TOUCH. All borrowed inputs are captured
 * before any engine callback can replace the map, owner or caller buffers. */
int G_Cod1GscTestEntityEventWithPayload( struct gentity_s *owner,
    const char *event, const GscEventValue *values, size_t count,
    int *out_awakened ) {
    GscEntityRef ref;
    GscEventValue copy[GSC_EVENT_PAYLOAD_CAPACITY];
    char name[128], error[GSC_SESSION_ERROR_CAP];
    size_t length = 0;
    int accepted = 0, awakened = 0;
    if (out_awakened) *out_awakened = 0;
    if (!out_awakened || !gscTestSession || gscTestCompleted ||
        (gscTestEntityMode != 16 && gscTestEntityMode != 17 && gscTestEntityMode != 18 && gscTestEntityMode != 19 && gscTestEntityMode != 20 && gscTestEntityMode != 21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))) && (gscTestEntityMode < 11 || gscTestEntityMode > 13)) || !event ||
        count > GSC_EVENT_PAYLOAD_CAPACITY || (count && !values)) return 0;
    while (length < sizeof(name) && event[length]) ++length;
    if (!length || length == sizeof(name)) return 0;
    memcpy(name, event, length + 1);
    if (count) memcpy(copy, values, count * sizeof(copy[0]));
    if (gscTestBusy) {
        gscTestStopRequested = 1;
        (void)gsc_session_notify_entity_with_payload(gscTestSession, gscTestId,
            NULL, name, copy, count, &awakened, error, sizeof(error));
        return 0;
    }
    if (!GscTestEnter()) return 0;
    if (!G_Cod1EntityCapture(owner, &ref) ||
        !G_Cod1GscTestEntityEventsEnabled() || gscTestStopRequested ||
        GscTestEntityModeValue() != gscTestEntityMode || gscTestStopRequested ||
        level.restarted || !G_Cod1EntitySameAllocation(&ref)) {
        GscTestLeave(); return 0;
    }
    accepted = gsc_session_notify_entity_with_payload(gscTestSession, gscTestId,
        &ref, name, copy, count, &awakened, error, sizeof(error));
    accepted = accepted && !gscTestStopRequested;
    if (accepted) *out_awakened = awakened;
    GscTestLeave();
    return accepted;
}
int G_Cod1GscTestEntityTouchEvent( struct gentity_s *owner, struct gentity_s *player ) {
    return GscTestEntityEvent(owner, player, "trigger", 0);
}
int G_Cod1GscTestEntityUseEvent( struct gentity_s *owner, struct gentity_s *player ) {
    return GscTestEntityEvent(owner, player, "trigger", 1);
}

void G_Cod1GscTestCombatCapture( struct gentity_s *owner,
    struct gentity_s *attacker, GscCombatEventTicket *ticket ) {
    GscCombatEventTicket snapshot;
    GscEntityRef actor;
    if (!ticket) return;
    memset(ticket, 0, sizeof(*ticket));
    /* The older constructors, absent and completed sessions need no new
     * captures or trap calls. Native combat continues through its old path. */
    if (!gscTestSession || gscTestCompleted ||
        (gscTestEntityMode != 16 && gscTestEntityMode != 17 && gscTestEntityMode != 18 && gscTestEntityMode != 19 && gscTestEntityMode != 20 && gscTestEntityMode != 21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))) && (gscTestEntityMode < 12 || gscTestEntityMode > 13))) return;
    memset(&snapshot, 0, sizeof(snapshot));
    if (!G_Cod1EntityCapture(owner, &snapshot.owner)) return;
    snapshot.ownerAddress = owner; snapshot.attackerAddress = attacker;
    snapshot.tracked = 1;
    snapshot.attacker.kind = GSC_EVENT_UNDEFINED;
    if (G_Cod1EntityCapture(attacker, &actor)) {
        snapshot.attacker.kind = GSC_EVENT_ENTITY;
        snapshot.attacker.entity = actor;
    }
    /* The engine's reserved world is not an allocated host entity yet.
     * Keep that limitation explicit rather than manufacture an actor ID. */
    *ticket = snapshot;
}

int G_Cod1GscTestCombatCurrent( const GscCombatEventTicket *ticket ) {
    return !ticket || !ticket->tracked ||
        G_Cod1EntitySameAllocation(&ticket->owner);
}

static int GscTestCombatEvent( const GscCombatEventTicket *ticket,
    const char *event, int take, int damage ) {
    GscCombatEventTicket snapshot;
    GscEventValue values[2];
    char error[GSC_SESSION_ERROR_CAP];
    int accepted = 0, awakened = 0;
    size_t count = damage ? 2 : 1;
    if (!ticket || !ticket->tracked) return 1;
    /* Both the target and attacker belong to the original combat operation.
     * Never recapture a replacement after cvar or native callbacks. */
    snapshot = *ticket;
    if (!G_Cod1GscTestCombatCurrent(&snapshot)) return 0;
    if ((damage && !take) || !gscTestSession || gscTestCompleted ||
        (gscTestEntityMode != 16 && gscTestEntityMode != 17 && gscTestEntityMode != 18 && gscTestEntityMode != 19 && gscTestEntityMode != 20 && gscTestEntityMode != 21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))) && (gscTestEntityMode < 12 || gscTestEntityMode > 13)))
        return G_Cod1GscTestCombatCurrent(&snapshot);
    memset(values, 0, sizeof(values));
    if (damage) {
        values[0].kind = GSC_EVENT_NUMBER; values[0].number = (float)take;
        values[1] = snapshot.attacker;
    } else {
        values[0] = snapshot.attacker;
    }
    if (gscTestBusy) {
        if ((gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && GscTestBlastEventsProducerActive())
            return GscTestBlastEventsCapture(&snapshot,take,damage);
        gscTestStopRequested = 1;
        (void)gsc_session_notify_entity_with_payload(gscTestSession, gscTestId,
            NULL, event, values, count, &awakened, error, sizeof(error));
        return G_Cod1GscTestCombatCurrent(&snapshot);
    }
    if (!GscTestEnter()) return G_Cod1GscTestCombatCurrent(&snapshot);
    /* New combat producers use the bounded six-query lease check. It is not
     * an atomic snapshot of arbitrary engine callback mutations. */
    if (G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER &&
        GscTestModuleReadCurrent() && gscTestEntityMode >= 12 && gscTestEntityMode<=43 &&
        !gscTestStopRequested && !level.restarted &&
        G_Cod1GscTestCombatCurrent(&snapshot)) {
        accepted = gsc_session_notify_entity_with_payload(gscTestSession,
            gscTestId, &snapshot.owner, event, values, count, &awakened,
            error, sizeof(error));
    }
    if (accepted && !gscTestStopRequested && awakened > 0)
        G_Printf("[GSC R97] combat event entity=%u generation=%u name=%s awakened=%d\n",
            (unsigned)snapshot.owner.slot, (unsigned)snapshot.owner.generation,
            event, awakened);
    GscTestLeave();
    /* Even a refused/cancelled event must not allow the old combat tail to
     * write into another object allocated in this target's former slot. */
    return G_Cod1GscTestCombatCurrent(&snapshot);
}

int G_Cod1GscTestCombatDamageEvent( const GscCombatEventTicket *ticket, int take ) {
    return GscTestCombatEvent(ticket, "damage", take, 1);
}

int G_Cod1GscTestCombatDeathEvent( const GscCombatEventTicket *ticket ) {
    return GscTestCombatEvent(ticket, "death", 0, 0);
}

void G_Cod1GscTestShutdown( void ) {
    G_Cod1GscCampaignRetire47();
    {char error[GSC_SESSION_ERROR_CAP];if(!GscTestHudReset(error,sizeof(error)))G_Printf("[GSC R200] shutdown HUD cleanup: %s\n",error);}
    G_Cod1PlayerNativeReset();
    GscTestPrecacheStringReset();
    GscTestTagFxReset();
    GscTestSaveReset();
    GscTurretPrecacheReset();GscTestItemReset();
    GscTestAmbientReset();
    GscTestDebugLineReset();
    GscTestDebugTextReset();
    GscTestShellshockReset();
    GscTestCvarReset();
    GscTestFxReset();
    GscLoopDeliveryReset();
    GscTestQuakeReset();
    GscTestBlastEventsReset();
    GscTestMissionReset();
    GscTestObjectiveReset();
    G_Cod1EntityMapEnd();
    if (!GscTestEnter()) return;
    GscTestDrop(); GscTestLeave();
}
void G_Cod1GscTestBeforeLoad( void ) {
    G_Cod1GscCampaignRetire47();
    {char error[GSC_SESSION_ERROR_CAP];if(!GscTestHudReset(error,sizeof(error)))G_Printf("[GSC R200] load HUD cleanup: %s\n",error);}
    G_Cod1PlayerNativeReset();
    GscTestPrecacheStringReset();
    GscTestTagFxReset();
    GscTestSaveReset();
    GscTurretPrecacheReset();GscTestItemReset();
    GscTestAmbientReset();
    GscTestDebugLineReset();
    GscTestDebugTextReset();
    GscTestShellshockReset();
    GscTestCvarReset();
    GscTestFxReset();
    GscLoopDeliveryReset();
    GscTestQuakeReset();
    GscTestBlastEventsReset();
    GscTestMissionReset();
    GscTestObjectiveReset();
    G_Cod1EntityMapEnd();
    if (!GscTestEnter()) return;
    if (gscTestSession)
        G_Printf("[GSC R77] stopped before save load: VM is not serialized/restored\n");
    GscTestDrop(); GscTestLeave();
}
#else
/* Avoid an empty translation unit in base builds. */
typedef int cod1_gsc_test_disabled_translation_unit;
#endif

int G_Cod1MapModelStarting(uint64_t epoch) {
    return epoch && epoch==G_Cod1EntityMapEpoch() && gscTestResourceInitialization &&
        !gscTestStopRequested && !level.restarted &&
        gsc_session_model_starting(gscTestSession,gscTestId,level.time);
}
