/* R128 genuine GAME publication adapter; R130 mode30 selects topology queries
 * and this pending request lane. Current client actor pose/admission remain open.
 * The R115 FX scene owns its lifetime.
 * Atomic Try admission bounds the reliable component, not future snapshot size.
 * Success means one reliable SP-slot0 request was submitted, never rendering,
 * retail AddEvent/PVS timing, or a transport acknowledgment from the client. */
#include "../bg_cod1_tag_fx.h"
#include "../g_cod1_actor.h"
#include "../cod1_gsc/gsc_tag_fx_payload.inc"

static struct {
    GscSession *session;
    uint64_t id,epoch,incarnation,map_epoch,operation,sequence,revision;
    int phase,logical_ms,server_time;
    GscTagFxQuery reply;
    char tokens[COD1_TAG_FX_SLOTS][COD1_TAG_FX_TOKEN_CAPACITY];
} gscTestTagFx;
static int gscTestTagFxBusy;

static void GscTestTagFxReset(void) { memset(&gscTestTagFx,0,sizeof(gscTestTagFx)); }
/* Mode43 already acquires its own FX scene through GscTestFxBegin. Keep
 * all scene/Session/resource/topology/current checks; no new receipt is made. */
static int GscTestTagFxMode(int mode){return (mode>=29&&mode<=34)||mode==43;}
static int GscTestTagFxOwner(const GscTestFxScene *lease,int now,int server_time) {
    return lease && GscTestTagFxMode(lease->mode) && GscTestFxTimeCurrent(lease,now) &&
        level.time==server_time && gscTestFx.revision==lease->revision &&
        gscTestFx.sequence==lease->sequence && gscTestTagFx.session==lease->session &&
        gscTestTagFx.id==lease->id && gscTestTagFx.epoch==lease->epoch &&
        gscTestTagFx.incarnation==lease->incarnation && gscTestTagFx.map_epoch==lease->map_epoch;
}
static int GscTestTagFxState(const GscTestFxScene *lease,int now,int server_time,
    uint64_t operation,int phase,uint64_t revision,uint64_t sequence) {
    return GscTestTagFxOwner(lease,now,server_time) && gscTestTagFx.operation==operation &&
        gscTestTagFx.phase==phase && gscTestTagFx.logical_ms==now &&
        gscTestTagFx.server_time==server_time && gscTestTagFx.revision==revision &&
        gscTestTagFx.sequence==sequence;
}
static int GscTestTagFxOverlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a && b && an && bn &&
        (an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an));
}
static int GscTestTagFxArguments(const void *input,size_t bytes,char *error,size_t capacity) {
    return !GscTestTagFxOverlap(input,bytes,error,capacity) &&
        !GscTestTagFxOverlap(&gscTestTagFx,sizeof(gscTestTagFx),error,capacity) &&
        !GscTestTagFxOverlap(&gscTestFx,sizeof(gscTestFx),error,capacity);
}
static int GscTestTagFxTopology(const GscTagFxQuery *reply) {
    cod1ActorDObjLease_t captured;
    return G_Cod1ActorCaptureDObj(&reply->entity,&captured,NULL,0) &&
        captured.topologySerial==reply->model_identity;
}
static int GscTestTagFxQuery(void *user,uint64_t id,int now,
    GscTagFxQuery *query,char *error,size_t capacity) {
    GscTestFxScene lease;GscTagFxQuery before,copy;
    uint64_t revision,sequence;int accepted=0,server_time,phase;
    (void)user;
    if(!GscTestTagFxArguments(query,sizeof(*query),error,capacity))return 0;
    if(gscTestTagFxBusy){gscTestStopRequested=1;goto refuse;}
    if(!query)goto refuse;
    before=copy=*query;lease=gscTestFx;server_time=level.time;
    if(!tag_fx_query_valid(&copy) || id!=lease.id || !GscTestTagFxMode(lease.mode) ||
        !GscTestFxTimeCurrent(&lease,now))goto refuse;
    if(copy.phase==GSC_TAG_FX_MODEL) {
        if(gscTestTagFx.session!=lease.session || gscTestTagFx.id!=lease.id ||
           gscTestTagFx.epoch!=lease.epoch || gscTestTagFx.incarnation!=lease.incarnation ||
           gscTestTagFx.map_epoch!=lease.map_epoch) {
            GscTestTagFxReset();gscTestTagFx.session=lease.session;gscTestTagFx.id=lease.id;
            gscTestTagFx.epoch=lease.epoch;gscTestTagFx.incarnation=lease.incarnation;
            gscTestTagFx.map_epoch=lease.map_epoch;
        }
        if(copy.operation_id<=gscTestTagFx.operation)goto refuse;
        /* Consume an operation before any callback; failed observations cannot
         * be retried using the same operation id. A new operation supersedes it. */
        gscTestTagFx.operation=copy.operation_id;gscTestTagFx.phase=1;
        gscTestTagFx.logical_ms=now;gscTestTagFx.server_time=server_time;
        memset(&gscTestTagFx.reply,0,sizeof(gscTestTagFx.reply));
    } else {
        if(!GscTestTagFxOwner(&lease,now,server_time) || gscTestTagFx.phase!=1 ||
           gscTestTagFx.operation!=copy.operation_id || gscTestTagFx.logical_ms!=now ||
           gscTestTagFx.server_time!=server_time || !gscTestTagFx.reply.model_present ||
           !tag_fx_ref_same(&copy.entity,&gscTestTagFx.reply.entity) ||
           copy.index!=gscTestTagFx.reply.index || copy.model_identity!=gscTestTagFx.reply.model_identity ||
           memcmp(copy.model_name,gscTestTagFx.reply.model_name,sizeof(copy.model_name)))goto refuse;
        gscTestTagFx.phase=2;
    }
    revision=gscTestTagFx.revision;sequence=gscTestTagFx.sequence;
    phase=copy.phase==GSC_TAG_FX_MODEL?1:2;gscTestTagFxBusy=1;
    if(!GscTestFxAdmission(&lease) || !GscTestFxMarker(&lease,now) ||
       !GscTestTagFxState(&lease,now,server_time,copy.operation_id,phase,revision,sequence) ||
       !G_Cod1EntityTagFxQuery(NULL,now,&copy,NULL,0) ||
       memcmp(query,&before,sizeof(before)) || !tag_fx_reply_valid(&before,&copy) ||
       !GscTestFxAdmission(&lease) || !GscTestFxMarker(&lease,now) ||
       !GscTestTagFxState(&lease,now,server_time,copy.operation_id,phase,revision,sequence) ||
       memcmp(query,&before,sizeof(before)) ||
       (copy.model_present && !GscTestTagFxTopology(&copy)))goto leave;
    gscTestTagFx.reply=copy;
    if(!copy.model_present || (copy.phase==GSC_TAG_FX_BONE && copy.bone_index<0))gscTestTagFx.phase=0;
    accepted=1;
leave:
    if(!accepted && GscTestTagFxOwner(&lease,now,server_time) &&
       gscTestTagFx.operation==copy.operation_id)gscTestTagFx.phase=0;
    gscTestTagFxBusy=0;
    if(accepted){*query=copy;return 1;}
refuse:
    GscTestError(error,capacity,"tag FX query refused: stale scene/time, operation order or copied payload");return 0;
}
static int GscTestTagFxResource(const GscTestFxScene *lease,int now,int index) {
    char actual[MAX_STRING_CHARS],expected[MAX_STRING_CHARS];
    return index>0 && index<MAX_COD1_SCRIPT_FX && gscTestFx.names[index][0] &&
        GscTestFxResourceText(expected,sizeof(expected),lease->epoch,index,gscTestFx.names[index]) &&
        GscTestFxRead(lease,CS_COD1_SCRIPT_FX_TABLE+index,actual,sizeof(actual),now,1) &&
        !strcmp(actual,expected) && GscTestFxMarker(lease,now);
}
static int GscTestTagFxPublish(void *user,uint64_t id,int now,
    const GscTagFxEvent *event,char *error,size_t capacity) {
    GscTestFxScene lease;GscTagFxEvent before,copy;GscEntityRef player;
    cod1TagFx_t wire;char command[COD1_TAG_FX_COMMAND_CAPACITY];
    uint64_t revision,sequence;int accepted=0,server_time,slot,insert,n;
    (void)user;
    if(!GscTestTagFxArguments(event,sizeof(*event),error,capacity))return 0;
    if(gscTestTagFxBusy){gscTestStopRequested=1;goto refuse;}
    if(!event)goto refuse;
    before=copy=*event;lease=gscTestFx;server_time=level.time;
    revision=gscTestTagFx.revision;sequence=gscTestTagFx.sequence;
    if(!tag_fx_event_valid(&copy) || id!=lease.id || sequence==UINT64_MAX ||
       !GscTestTagFxState(&lease,now,server_time,copy.operation_id,2,revision,sequence) ||
       gscTestTagFx.reply.bone_index<0 || copy.index!=gscTestTagFx.reply.index ||
       copy.model_identity!=gscTestTagFx.reply.model_identity ||
       !tag_fx_ref_same(&copy.entity,&gscTestTagFx.reply.entity) ||
       memcmp(copy.tag,gscTestTagFx.reply.tag,sizeof(copy.tag)) ||
       !G_Cod1EntityCapture(&g_entities[0],&player) || !GscTestQuakePlayerCurrent(&player))goto refuse;
    slot=CT_FindSlot(gscTestTagFx.tokens,copy.token,&insert);
    if(!slot || (insert && revision==UINT64_MAX))goto refuse;
    memset(&wire,0,sizeof(wire));wire.epoch=lease.epoch;wire.sequence=sequence+1;
    wire.map_epoch=copy.entity.epoch;wire.entity_slot=copy.entity.slot;
    wire.generation=copy.entity.generation;wire.model_identity=copy.model_identity;
    wire.dictionary_slot=(unsigned)slot;strcpy(wire.token,copy.token);
    if(!CT_EncodeCommand(command,sizeof(command),&wire))goto refuse;
    gscTestTagFxBusy=1;
    /* Read resource/gates and bone, then repeat resource/gates. The final pure
     * topology capture detects a rebuild in those last callbacks. No external
     * call intervenes between that capture and the one reliable publication.
     * Repeated engine reads are bounded observations, not atomic cvar services. */
    for(n=0;n<2;++n) {
        if(!GscTestFxAdmission(&lease) || !GscTestTagFxResource(&lease,now,copy.index) ||
           !GscTestTagFxState(&lease,now,server_time,copy.operation_id,2,revision,sequence) ||
           !GscTestQuakePlayerCurrent(&player) || memcmp(event,&before,sizeof(before)))goto leave;
        if(!n && !G_Cod1EntityTagFxValidate(&copy,NULL,0))goto leave;
    }
    if(!GscTestTagFxTopology(&gscTestTagFx.reply) ||
       slot!=CT_FindSlot(gscTestTagFx.tokens,copy.token,&n) || n!=insert)goto leave;
    /* Tentative commit before the atomic engine submission. A successful
     * enqueue cannot be rewound after a callback cancellation. On a definite
     * queue refusal, restore only the exact still-owned tentative state.
     * The actual engine helper has no GAME callbacks; the guards also cover
     * hostile typed test boundaries without writing a replacement scene. */
    if(insert){strcpy(gscTestTagFx.tokens[slot],copy.token);gscTestTagFx.revision=++revision;}
    gscTestTagFx.sequence=++sequence;gscTestTagFx.phase=3;
    if(!trap_Cod1TrySendServerCommand(0,command)) {
        if(GscTestTagFxState(&lease,now,server_time,copy.operation_id,3,revision,sequence) &&
           !strcmp(gscTestTagFx.tokens[slot],copy.token)) {
            if(insert){gscTestTagFx.tokens[slot][0]=0;gscTestTagFx.revision=revision-1;}
            gscTestTagFx.sequence=sequence-1;gscTestTagFx.phase=0;
        }
        goto leave;
    }
    accepted=GscTestTagFxState(&lease,now,server_time,copy.operation_id,3,revision,sequence) &&
        GscTestFxAdmission(&lease) && GscTestTagFxResource(&lease,now,copy.index) &&
        GscTestTagFxState(&lease,now,server_time,copy.operation_id,3,revision,sequence) &&
        !memcmp(event,&before,sizeof(before)) && !strcmp(gscTestTagFx.tokens[slot],copy.token) &&
        GscTestQuakePlayerCurrent(&player) && GscTestTagFxTopology(&gscTestTagFx.reply);
leave:
    gscTestTagFxBusy=0;
    if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"tag FX publication refused: queue refusal, unregistered resource, full dictionary, stale model/owner or copied request");return 0;
}
