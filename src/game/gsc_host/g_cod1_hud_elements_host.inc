/* R200 real bounded HUD state publication. Ref/slot values are copies; no
 * engine pointer enters the VM or wire. Cleanup is independent of a running
 * session/player and refuses without lying if its atomic marker clear cannot
 * be queued. The owning Session remains available for a cleanup retry. */
#include "../bg_cod1_script_hud.h"
#include "g_cod1_hud_intro.inc"
typedef struct {
    GscSession *session;
    uint64_t id,map,epoch,owner,operation,sequence;
    int owned,busy,cancelled;
    uint32_t generation[COD1_SCRIPT_HUD_CAPACITY];
    cod1ScriptHud_t slot[COD1_SCRIPT_HUD_CAPACITY];
} GscTestHudScene;
static GscTestHudScene gscTestHud;
static uint64_t gscTestHudEpoch;
static int GscTestHudOverlap(const void *a,size_t an,const void *b,size_t bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;
}
static int GscTestHudSame(const GscTestHudScene *lease){
    return lease&&gscTestHud.owned&&lease->owned&&lease->epoch==gscTestHud.epoch&&lease->map==gscTestHud.map&&
        lease->map&&lease->map==G_Cod1EntityMapEpoch()&&lease->session==gscTestHud.session&&lease->id==gscTestHud.id;
}
static int GscTestHudLeaseCurrent(const GscTestHudScene *lease,int now,int (*current)(void *),void *context){
    GscSessionSnapshot snap;
    return current&&current(context)==1&&GscTestHudSame(lease)&&!gscTestHud.cancelled&&gscTestSession==lease->session&&
        gscTestId==lease->id&&gscTestEntityMode==43&&!gscTestCompleted&&!gscTestStopRequested&&!level.restarted&&
        G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&GscTestModuleReadCurrent()&&
        gsc_session_snapshot(lease->session,&snap)&&snap.session_id==lease->id&&snap.state==GSC_SESSION_RUNNING&&
        !snap.error[0]&&snap.logical_ms==now&&GscTestHudSame(lease);
}
static int GscTestHudReset(char *error,size_t size){
    GscTestHudScene old=gscTestHud;char expected[COD1_SCRIPT_HUD_MARKER_BYTES],actual[MAX_STRING_CHARS];
    if(gscTestHud.busy){gscTestHud.cancelled=1;GscTestError(error,size,"HUD cleanup during publication refused");return 0;}
    if(!old.owned)return 1;
    if(!old.map||old.map!=G_Cod1EntityMapEpoch()){memset(&gscTestHud,0,sizeof(gscTestHud));return 1;}
    if(!CH_Marker(expected,sizeof(expected),old.epoch)){GscTestError(error,size,"HUD marker identity unavailable");return 0;}
    gscTestHud.busy=1;
    trap_GetConfigstring(CS_COD1_SCRIPT_HUD,actual,sizeof(actual));
    if(!GscTestHudSame(&old)){gscTestHud.busy=0;GscTestError(error,size,"HUD cleanup owner changed");return 0;}
    if(!actual[0]){memset(&gscTestHud,0,sizeof(gscTestHud));return 1;}
    if(strcmp(actual,expected)||!trap_Cod1TrySetHudMarker("",expected)){
        gscTestHud.busy=0;GscTestError(error,size,"HUD marker cleanup refused; owner retained for retry");return 0;
    }
    if(!GscTestHudSame(&old)){gscTestHud.busy=0;GscTestError(error,size,"HUD cleanup map changed after clear");return 0;}
    memset(&gscTestHud,0,sizeof(gscTestHud));return 1;
}
static int GscTestHudBegin(char *error,size_t size){
    GscTestHudScene scene;char marker[COD1_SCRIPT_HUD_MARKER_BYTES];int cancelled;
    if(!GscTestHudReset(error,size))return 0;
    if(gscTestHudEpoch==UINT64_MAX||gscTestEntityMode!=43||!gscTestSession||!gscTestId||!G_Cod1EntityMapEpoch())return 0;
    memset(&scene,0,sizeof(scene));scene.session=gscTestSession;scene.id=gscTestId;scene.map=G_Cod1EntityMapEpoch();scene.epoch=++gscTestHudEpoch;scene.owned=1;
    if(!CH_Marker(marker,sizeof(marker),scene.epoch))return 0;
    gscTestHud.busy=1;gscTestHud.cancelled=0;
    if(!trap_Cod1TrySetHudMarker(marker,"")){
        gscTestHud.busy=0;
        GscTestError(error,size,"HUD scene marker unavailable or queue full");return 0;
    }
    cancelled=gscTestHud.cancelled;
    if(cancelled||scene.map!=G_Cod1EntityMapEpoch()||gscTestSession!=scene.session||gscTestId!=scene.id){
        /* Keep exact ownership so teardown can retry genuine marker cleanup. */
        gscTestHud=scene;GscTestError(error,size,"HUD scene changed during marker publication");return 0;
    }
    gscTestHud=scene;return 1;
}
static int GscTestHudRef(const GscHudElemRef *ref){
    return ref&&gscTestHud.owned&&ref->epoch==gscTestHud.epoch&&ref->owner==gscTestHud.owner&&ref->owner&&
        ref->index<COD1_SCRIPT_HUD_CAPACITY&&ref->generation&&ref->generation==gscTestHud.generation[ref->index]&&gscTestHud.slot[ref->index].alive;
}
static int GscTestHudRetire(void *user,uint64_t owner,char *error,size_t size){
    (void)user;if(!owner){GscTestError(error,size,"HUD retirement owner missing");return 0;}
    if(!gscTestHud.owned||!gscTestHud.owner||gscTestHud.owner!=owner)return 1;
    return GscTestHudReset(error,size);
}
static int GscTestHudValidate(void *user,const GscHudElemRef *input,int now,int *alive,char *error,size_t size,int (*current)(void *),void *context){
    GscTestHudScene lease=gscTestHud;GscHudElemRef ref;int result;(void)user;
    if(!input||!alive||!current||gscTestHud.busy){if(gscTestHud.busy)gscTestHud.cancelled=1;GscTestError(error,size,"HUD validation unavailable or reentered");return 0;}
    if(GscTestHudOverlap(input,sizeof(*input),alive,sizeof(*alive)))return 0;
    ref=*input;gscTestHud.busy=1;
    if(!GscTestHudLeaseCurrent(&lease,now,current,context)){gscTestHud.busy=0;return 0;}result=GscTestHudRef(&ref);
    if(!GscTestHudLeaseCurrent(&lease,now,current,context)||memcmp(input,&ref,sizeof(ref))){gscTestHud.busy=0;return 0;}
    *alive=result;gscTestHud.busy=0;return 1;
}
static int GscTestHudInvoke(void *user,const GscHudElemRequest *input,GscHudElemResult *out,char *error,size_t size,int (*current)(void *),void *context){
    GscHudElemRequest request;GscHudElemResult result;GscTestHudScene lease=gscTestHud;cod1ScriptHud_t state;
    GscEntityRef player;char command[COD1_SCRIPT_HUD_PACKET_BYTES],marker[COD1_SCRIPT_HUD_MARKER_BYTES],actual[MAX_STRING_CHARS];
    int index=-1,k,server_time=level.time,mutating,ok=0;uint32_t generation=0;(void)user;
    if(gscTestHud.busy){gscTestHud.cancelled=1;GscTestError(error,size,"HUD callback reentered");return 0;}
    if(!input||!out||!current||!memchr(input->text,0,sizeof(input->text))||GscTestHudOverlap(input,sizeof(*input),out,sizeof(*out))){GscTestError(error,size,"invalid HUD request");return 0;}
    request=*input;memset(&result,0,sizeof(result));memset(&state,0,sizeof(state));mutating=request.kind!=GSC_HUDELEM_READ;
    gscTestHud.busy=1;
    if(!request.owner||!request.operation_id||request.now_ms<0||request.kind<GSC_HUDELEM_CREATE||request.kind>GSC_HUDELEM_FADE||
       !GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||(lease.owner&&lease.owner!=request.owner))goto refuse;
    if(request.kind==GSC_HUDELEM_CREATE){
        for(k=0;k<COD1_SCRIPT_HUD_CAPACITY;k++)if(!gscTestHud.slot[k].alive&&gscTestHud.generation[k]!=UINT32_MAX){index=k;break;}
        if(index<0||request.element.epoch||request.element.owner||request.element.index||request.element.generation)goto refuse;
        generation=gscTestHud.generation[index]+1;state.alive=1;state.alpha=255;state.font_scale=1;
    }else{
        if(request.element.owner!=request.owner||!GscTestHudRef(&request.element))goto refuse;
        index=(int)request.element.index;generation=request.element.generation;state=gscTestHud.slot[index];result.element=request.element;result.alive=1;
    }
    if(((request.kind==GSC_HUDELEM_WRITE||request.kind==GSC_HUDELEM_READ)&&request.field>=GSC_HUDELEM_FIELD_ALPHA)||request.kind>=GSC_HUDELEM_SET_SHADER){
        /* The builtin black material predates map shader precaching and is
         * used directly by unchanged maps/_introscreen. Other materials need
         * their current map precache entry. */
        if(request.kind==GSC_HUDELEM_SET_SHADER&&Q_stricmp(request.text,"black")){
            char name[MAX_STRING_CHARS];int shader;
            for(shader=1;shader<MAX_COD1_SHADERS;shader++){
                trap_GetConfigstring(CS_COD1_SHADERS+shader,name,sizeof(name));
                if(!GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||memcmp(input,&request,sizeof(request)))goto refuse;
                if(!name[0]||!Q_stricmp(name,request.text))break;
            }
            if(shader==MAX_COD1_SHADERS||!name[0])goto refuse;
        }
        if(!HudIntroApply(&state,&request,server_time,&result))goto refuse;
    }else if(request.kind==GSC_HUDELEM_WRITE){
        if(request.field==GSC_HUDELEM_FIELD_X||request.field==GSC_HUDELEM_FIELD_Y){
            if(request.value_kind!=GSC_HUDELEM_VALUE_NUMBER||!CQ_Finite(request.number)||request.number<-8192||request.number>8192)goto refuse;
            if(request.field==GSC_HUDELEM_FIELD_X)state.x=request.number;else state.y=request.number;
        }else{
            static const char *const x[]={"left","center","right"},*const y[]={"top","middle","bottom"};const char *const *names;
            if(request.value_kind!=GSC_HUDELEM_VALUE_STRING||(request.field!=GSC_HUDELEM_FIELD_ALIGN_X&&request.field!=GSC_HUDELEM_FIELD_ALIGN_Y))goto refuse;
            names=request.field==GSC_HUDELEM_FIELD_ALIGN_X?x:y;for(k=0;k<3;k++)if(!strcmp(request.text,names[k]))break;if(k==3)goto refuse;
            if(request.field==GSC_HUDELEM_FIELD_ALIGN_X)state.align_x=(uint32_t)k;else state.align_y=(uint32_t)k;
        }
    }else if(request.kind==GSC_HUDELEM_SET_CLOCK){
        if(strcmp(request.text,"hudStopwatch")||!CQ_Finite(request.remaining_seconds)||!CQ_Finite(request.total_seconds)||
           request.remaining_seconds<0||request.remaining_seconds>request.total_seconds||request.total_seconds<=0||request.total_seconds>86400||
           request.width<1||request.width>1024||request.height<1||request.height>1024)goto refuse;
        state.content=0;state.clock=1;state.remaining_ms=(int)((double)request.remaining_seconds*1000.0);state.total_ms=(int)((double)request.total_seconds*1000.0);
        if(state.total_ms<1)goto refuse;state.server_time=server_time;state.width=request.width;state.height=request.height;
    }else if(request.kind==GSC_HUDELEM_DESTROY){state.alive=0;state.clock=0;result.alive=0;
    }else if(request.kind==GSC_HUDELEM_READ){
        static const char *const x[]={"left","center","right"},*const y[]={"top","middle","bottom"};
        if(request.field==GSC_HUDELEM_FIELD_X||request.field==GSC_HUDELEM_FIELD_Y){result.kind=GSC_HUDELEM_RESULT_NUMBER;result.number=request.field==GSC_HUDELEM_FIELD_X?state.x:state.y;}
        else if(request.field==GSC_HUDELEM_FIELD_ALIGN_X||request.field==GSC_HUDELEM_FIELD_ALIGN_Y){result.kind=GSC_HUDELEM_RESULT_STRING;strcpy(result.text,request.field==GSC_HUDELEM_FIELD_ALIGN_X?x[state.align_x]:y[state.align_y]);}else goto refuse;
    }
    if(!GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||memcmp(input,&request,sizeof(request)))goto refuse;
    if(!mutating){*out=result;ok=1;goto done;}
    if(request.operation_id<=lease.operation||lease.sequence==UINT64_MAX||!G_Cod1EntityCapture(&g_entities[0],&player)||!GscTestQuakePlayerCurrent(&player))goto refuse;
    state.epoch=lease.epoch;state.owner=request.owner;state.index=(uint32_t)index;state.generation=generation;state.sequence=lease.sequence+1;
    if(!state.clock)state.server_time=server_time;
    if(!CH_Encode(command,sizeof(command),&state)||!CH_Marker(marker,sizeof(marker),lease.epoch))goto refuse;
    for(k=0;k<2;k++){
        if(!GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||!GscTestQuakePlayerCurrent(&player)||level.time!=server_time||gscTestHud.operation!=lease.operation||gscTestHud.sequence!=lease.sequence)goto done;
        trap_GetConfigstring(CS_COD1_SCRIPT_HUD,actual,sizeof(actual));
        if(!GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||strcmp(actual,marker)||!GscTestQuakePlayerCurrent(&player)||level.time!=server_time||memcmp(input,&request,sizeof(request)))goto done;
    }
    gscTestHud.operation=request.operation_id; /* Consumed even on definite queue refusal. */
    if(!trap_Cod1TrySendServerCommand(0,command))goto done;
    /* Actual enqueue survives post-callback cancellation; teardown retains
     * the complete owner so a visible prefix is always eligible for cleanup. */
    if(GscTestHudSame(&lease)){
        gscTestHud.owner=request.owner;gscTestHud.sequence=state.sequence;gscTestHud.generation[index]=generation;gscTestHud.slot[index]=state;
    }
    if(!GscTestHudLeaseCurrent(&lease,request.now_ms,current,context)||!GscTestQuakePlayerCurrent(&player)||level.time!=server_time||memcmp(input,&request,sizeof(request)))goto done;
    result.element.epoch=lease.epoch;result.element.owner=request.owner;result.element.index=(uint32_t)index;result.element.generation=generation;
    result.alive=(int)state.alive;result.kind=request.kind==GSC_HUDELEM_CREATE?GSC_HUDELEM_RESULT_REF:GSC_HUDELEM_RESULT_VOID;*out=result;ok=1;
done:gscTestHud.busy=0;if(ok)return 1;
refuse:gscTestHud.busy=0;GscTestError(error,size,"HUD operation refused: bounds, stale owner, missing player or queue capacity");return 0;
}
static const GscHudElemHost gscTestHudHost={NULL,GscTestHudInvoke,GscTestHudValidate,GscTestHudRetire};
