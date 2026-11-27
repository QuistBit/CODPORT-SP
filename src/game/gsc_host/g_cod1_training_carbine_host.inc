/* Original whole carbine/target_counter/close_door/gate_open/give_ammo
 * functions and autosave dependencies; authored subsection initialization. */
#include "g_cod1_training_carbine_source.h"
#include "g_cod1_training_moody_source.h"
#include "g_cod1_training_sequence_source.h"
#include "g_cod1_training_followup_source.h"
#include "g_cod1_training_opening_gameplay_source.h"
#include "g_cod1_training_finale_source.h"
#include "../g_cod1_actor_bootstrap.h"
#include "../g_cod1_spawner.h"
static struct {int selected,moody,sequence,followup,opening;int moodyModel,moodyCharacter,moodyInitialStop;uint64_t epoch,id;GscEntityRef moodyRef;} gscTestCarbine;
static GscSessionActorBootstrapHost gscTestCarbineActors;
static int GscTestCarbineStorageProtected(const void *p,size_t n) {
 return CAR_Overlap(p,n,&gscTestCarbine,sizeof(gscTestCarbine))||CAR_Overlap(p,n,&gscTestCarbineActors,sizeof(gscTestCarbineActors));
}
static int GscTestCarbineCurrent(void *unused) {
    (void)unused;
    return gscTestCarbine.selected&&gscTestCarbine.epoch==G_Cod1EntityMapEpoch()&&gscTestCarbine.id==gscTestId&&
        gscTestSession&&!gscTestStopRequested&&!gscTestCompleted&&!level.restarted&&
        gscTestEntityMode==43&&gscTestActorPrefixEpoch==gscTestCarbine.epoch&&
        G_Cod1GscFeature47(C1CP47_F_CARBINE)==1&&GscTestModuleReadCurrent()&&
        gsc_session_is_running(gscTestSession,gscTestCarbine.id);
}
qboolean G_Cod1GscTestCarbineActive(void){return GscTestCarbineCurrent(NULL)?qtrue:qfalse;}
int G_Cod1MoodyIdleNeeded(const GscEntityRef *,int *);
static int FoleyReadReleasedBody(const GscEntityRef*,GscStringCastValue*,char*,size_t);
static int GscTestCarbineFieldClassify(void *unused,const GscEntityRef *ref,const char *name,char *error,size_t size){
 const GscEntityFieldHost *fields=G_Cod1ActorTrainingFieldHost();(void)unused;
 if(gscTestCarbine.opening&&name&&((!strcmp(name,"cod1_tower_idle_needed")||!strcmp(name,"cod1_tower_idle_current"))||!strcmp(name,"cod1_scripted_body_released")||!strcmp(name,"cod1_course_rest_rate")||!strcmp(name,"cod1_course_entry_idle")||!strcmp(name,"cod1_course_run_rate")))return 1;
 if(gscTestCarbine.followup&&name&&!strcmp(name,"cod1_hint_string"))return 1;
 if(gscTestCarbine.followup&&name&&!strcmp(name,"count")&&G_Cod1ItemLifecycleOwns(ref))return GscTestGrenadeFieldClassify(NULL,ref,name,error,size);
 return fields->classify(fields->user,ref,name,error,size);
}
static int GscTestCarbineFieldRead(void *unused,const GscEntityRef *ref,const char *name,GscStringCastValue *out,char *error,size_t size) {
    const GscEntityFieldHost *fields=G_Cod1ActorTrainingFieldHost();GscStringCastValue value;(void)unused;
    if(!ref||!name||!out||!GscTestCarbineCurrent(NULL))return 0;
    if(gscTestCarbine.opening&&(!strcmp(name,"cod1_tower_idle_needed")||!strcmp(name,"cod1_tower_idle_current"))){
        int needed;
        if(!gscTestCarbine.moodyCharacter || memcmp(ref,&gscTestCarbine.moodyRef,sizeof(*ref)) ||
           !G_Cod1MoodyIdleNeeded(ref,&needed))return 0;
        memset(out,0,sizeof(*out));out->type=5;out->integer=needed==(!strcmp(name,"cod1_tower_idle_needed")?1:2);return 1;
    }
    if(gscTestCarbine.opening&&!strcmp(name,"cod1_scripted_body_released"))return FoleyReadReleasedBody(ref,out,error,size);
    if(!strcmp(name,"count")&&G_Cod1SpawnerIntakeIdentity(ref))
        return G_Cod1SpawnerCountRead(ref,out,error,size);
    if(!strcmp(name,"enemy")&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1ActorReadAbsentEnemyField(ref,out,error,size);
    if((!strcmp(name,"weapon")||!strcmp(name,"name"))&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1ActorReadRuntimeStringField(ref,name,out,error,size);
    if(!Q_stricmp(name,"script_noteworthy"))return G_Cod1EntityReadNoteworthy(ref,out,error,size);
    if(!strcmp(name,"classname")&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1EntityReadClassnameField(ref,out,error,size);
    if(!strcmp(name,"model")&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1EntityReadModelField(ref,out,error,size);
    if(!strcmp(name,"anim_pose")&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1ActorPoseFieldHost()->read(NULL,ref,name,out,error,size);
    if(ref->slot>=MAX_CLIENTS&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1&&
       (!Q_stricmp(name,"grenade")||!Q_stricmp(name,"lookforward")||!Q_stricmp(name,"lookright")||!Q_stricmp(name,"lookup")||!Q_stricmp(name,"groundtype")||!Q_stricmp(name,"desiredangle")||!strcmp(name,"node")||!Q_stricmp(name,"chainnode")||!strcmp(name,"team")||!strcmp(name,"accuracy")||!strcmp(name,"accuracystationarymod")||
        !strcmp(name,"scariness")||!strcmp(name,"health")||!strcmp(name,"maxhealth")||!Q_stricmp(name,"grenadeawareness")||!Q_stricmp(name,"suppressionwait")||!Q_stricmp(name,"maxsightdistsqrd")||!Q_stricmp(name,"walkdist")||!Q_stricmp(name,"interval")||!Q_stricmp(name,"dontavoidplayer")||!Q_stricmp(name,"dropweapon")||!Q_stricmp(name,"pacifist")||!Q_stricmp(name,"chainfallback")||!Q_stricmp(name,"drawoncompass")||!Q_stricmp(name,"useable")))
        return G_Cod1ActorReadRuntimeField(ref,name,out,error,size);
    if(gscTestCarbine.followup&&!strcmp(name,"cod1_hint_string"))return G_Cod1HintRead(ref,out,error,size);
    if(gscTestCarbine.followup&&!strcmp(name,"count")&&G_Cod1ItemLifecycleOwns(ref))return GscTestGrenadeFieldRead(NULL,ref,name,out,error,size);
    if(gscTestCarbine.moody&&!strcmp(name,"goalradius")){
        float radius;if(!G_Cod1ActorGoalRadius(ref,&radius,0))return 0;
        memset(&value,0,sizeof(value));value.type=4;value.number=radius;*out=value;return 1;
    }
    /* A native assignment establishes the entity maximum. Older accepted
     * authored subsections did not execute _load; retain their legacy read
     * below until the whole startup root supplies native initialization. */
    if(ref->slot<MAX_CLIENTS&&(!strcmp(name,"health")||
       (!strcmp(name,"maxhealth")&&G_Cod1PlayerHealthFieldKnown(ref))))
        return G_Cod1PlayerHealthFieldRead(ref,name,out,error,size,GscTestCarbineCurrent,NULL);
    if((!strcmp(name,"health")||!strcmp(name,"maxhealth"))&&G_Cod1EntitySameAllocation(ref)&&ref->slot<MAX_CLIENTS&&
       level.clients&&ref->slot<(uint32_t)level.maxclients&&g_entities[ref->slot].client==&level.clients[ref->slot]&&
       g_entities[ref->slot].client->pers.connected==CON_CONNECTED) {
        memset(&value,0,sizeof(value));value.type=5;
        value.integer=!strcmp(name,"health")?g_entities[ref->slot].health:level.clients[ref->slot].ps.stats[STAT_MAX_HEALTH];
        if(!strcmp(name,"maxhealth")&&value.integer<=0)return 0;*out=value;return 1;
    }
    return fields->read(fields->user,ref,name,out,error,size);
}
static int GscTestCarbineFieldWrite(void *unused,const GscEntityRef *ref,const char *name,const GscStringCastValue *value,char *error,size_t size){
    const GscEntityFieldHost *fields=G_Cod1ActorTrainingFieldHost();float radius;
    if(gscTestCarbine.opening&&name&&!strcmp(name,"cod1_course_run_rate")){
        if(!value || (value->type!=4 && value->type!=5) || !GscTestCarbineCurrent(NULL))return 0;
        return G_Cod1ActorSetCourseRunRate(ref,value->type==4?value->number:(float)value->integer);
    }
    if(gscTestCarbine.opening&&name&&!strcmp(name,"cod1_course_entry_idle")){
        if(!value || value->type!=1 || !memchr(value->string,0,sizeof(value->string)) || !GscTestCarbineCurrent(NULL))return 0;
        return G_Cod1ActorSetCourseEntryIdle(ref,value->string);
    }
    if(gscTestCarbine.opening&&name&&!strcmp(name,"cod1_course_rest_rate")){
        if(!value || (value->type!=4 && value->type!=5) || !GscTestCarbineCurrent(NULL))return 0;
        return G_Cod1ActorStartRest(ref,value->type==4?value->number:(float)value->integer);
    }
    if(gscTestCarbine.opening&&name&&((!strcmp(name,"cod1_tower_idle_needed")||!strcmp(name,"cod1_tower_idle_current"))||!strcmp(name,"cod1_scripted_body_released")))return 0;
    if(ref&&name&&!strcmp(name,"count")&&G_Cod1SpawnerIntakeIdentity(ref)) {
        if(!GscTestCarbineCurrent(NULL))return 0;
        return G_Cod1SpawnerCountWrite(ref,value,error,size);
    }
    if(ref&&ref->slot<MAX_CLIENTS&&name&&(!strcmp(name,"health")||!strcmp(name,"maxhealth")))
        return G_Cod1PlayerHealthFieldWrite(ref,name,value,error,size,GscTestCarbineCurrent,NULL);
    if(name&&!strcmp(name,"anim_pose")&&GscTestCarbineCurrent(NULL)&&G_Cod1GscFeature47(C1CP47_F_TURRET)==1)
        return G_Cod1ActorPoseFieldHost()->write(NULL,ref,name,value,error,size);
    if(gscTestCarbine.followup&&name&&!strcmp(name,"cod1_hint_string"))return G_Cod1HintWrite(ref,value,error,size);
    if(gscTestCarbine.followup&&!strcmp(name,"count")&&G_Cod1ItemLifecycleOwns(ref))return GscTestGrenadeFieldWrite(NULL,ref,name,value,error,size);
    if(gscTestCarbine.moody&&!strcmp(name,"goalradius")){
        if(!GscTestCarbineCurrent(NULL)||!value||(value->type!=4&&value->type!=5))return 0;
        radius=value->type==5?(float)value->integer:value->number;return G_Cod1ActorGoalRadius(ref,&radius,1);
    }
    return fields->write(fields->user,ref,name,value,error,size);
}
static const GscSessionActorBootstrapHost *GscTestCarbineActorHost(const GscSessionActorBootstrapHost *actors) {
    if(!actors)return NULL;gscTestCarbineActors=*actors;if(gscTestCarbine.followup)gscTestCarbineActors.fields.classify=GscTestCarbineFieldClassify;gscTestCarbineActors.fields.read=GscTestCarbineFieldRead;gscTestCarbineActors.fields.write=GscTestCarbineFieldWrite;return &gscTestCarbineActors;
}
int G_Cod1GscTestCarbineSpawnAllowed(const char *classname) {
    cod1ItemDescriptor_t d;cod1WeaponRegistrationReceipt_t registration;cod1AmmoRegistryReceipt_t registry;
    const char *weapon;
    if(!classname||strncmp(classname,"weapon_",7))return 0;weapon=classname+7;
    if(strcmp(weapon,"m1carbine")&&(!gscTestCarbine.sequence||(strcmp(weapon,"springfield")&&strcmp(weapon,"thompson"))))return 0;
    if(!GscTestCarbineCurrent(NULL)||
       !CI_Resolve(weapon,&d)||!d.dynamic||!gscTestItem.registered[d.index]||!gscTestItem.complete[d.index]||
       !CAR_Build(&registry))return 0;
    return G_Cod1ReadWeaponRegistration(&registry,d.weapon_index+1,&registration,NULL,0,GscTestCarbineCurrent,NULL)&&
        GscTestCarbineCurrent(NULL);
}

#include "../bg_cod1_actor_runtime.h"
static int GscTestMoodyNotify(void *unused,const GscEntityRef *ref){
    int awakened=0;char error[GSC_SESSION_ERROR_CAP];(void)unused;
    return GscTestCarbineCurrent(NULL)&&gsc_session_notify_entity(gscTestSession,gscTestId,ref,"goal",&awakened,error,sizeof(error));
}
static int GscTestMoodyPoseFrame(int now);
static int GscTestMoodyFrame(int now,char *error,size_t size){
    int i;
    if(!GscTestCarbineCurrent(NULL))return 0;
    if(!gscTestCarbine.moodyRef.epoch){
        for(i=MAX_CLIENTS;i<level.num_entities;i++)if(g_entities[i].inuse&&g_entities[i].targetname&&!strcmp(g_entities[i].targetname,"moody"))break;
        if(i==level.num_entities||!G_Cod1EntityCapture(&g_entities[i],&gscTestCarbine.moodyRef))return 0;
        /* Select the topology owner before character::_utility::new. The
         * legacy scripts retain their explicit alias controller. */
        gscTestCarbine.moodyCharacter=G_Cod1GscFeature47(C1CP47_F_MOODY)==1;
        if(gscTestCarbine.moodyCharacter){
            G_Printf("[Training] Moody retained for character setup entity=%u\n",gscTestCarbine.moodyRef.slot);
        }else{
            gscTestCarbine.moodyModel=G_ModelIndex(COD1_ACTOR_MOODY_ALIAS);
            if(!GscTestCarbineCurrent(NULL)||!G_Cod1ActorMotionBegin(&gscTestCarbine.moodyRef,gscTestCarbine.moodyModel,now))return 0;
            G_Printf("[R223] Original Moody visible controller ready entity=%u\n",gscTestCarbine.moodyRef.slot);
        }
    }
    if(gscTestCarbine.moodyCharacter!=(G_Cod1GscFeature47(C1CP47_F_MOODY)==1)){
        GscTestError(error,size,"Moody topology selection changed during its lifetime");return 0;
    }
    if(gscTestCarbine.moodyCharacter){
        if(GscTestMoodyPoseFrame(now))return 1;
        GscTestError(error,size,"Moody character presentation refused");return 0;
    }
    if(!G_Cod1ActorMotionFrame(&gscTestCarbine.moodyRef,now,GscTestCarbineCurrent,GscTestMoodyNotify,NULL)){
        GscTestError(error,size,"original Moody lease, motion or notification refused");return 0;
    }
    return 1;
}
