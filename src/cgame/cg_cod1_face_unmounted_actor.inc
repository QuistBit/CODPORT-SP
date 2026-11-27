#include "../game/source_pose_wire.h"
#include "../game/source_walk_wire.h"
#ifndef CG_COD1_FACE_UNMOUNTED_ACTOR_INC
#define CG_COD1_FACE_UNMOUNTED_ACTOR_INC
/* Independent unmounted v4 profile: same ordinary tree with original Elder
 * face curves. Snapshot current weights/phases only; no CG clock/events. */
#include "../qcommon/cod1_actor_face_unmounted14.h"
#include "../game/bg_cod1_face_unmounted_actor_wire.h"
typedef struct {cod1TurretActorResourceState_t definition;int handle,version;c1ad14Resource_t*resource;c1ast14Bank_t bank;unsigned char jumperMap[128];int jumperMapped;unsigned char gaitMap[128];int gaitMapped;unsigned char crouchMap[128];int crouchMapped;unsigned char crouch1Map[128];int crouch1Mapped;unsigned char crouch3Map[128];int crouch3Mapped;unsigned char crouch4Map[128];int crouch4Mapped;unsigned char diveMap[128];int diveMapped;unsigned char crawlMap[128];int crawlMapped;unsigned char runMap[128];int runMapped;unsigned char entryMap[5][128];int entryMapped[5];unsigned char ladderMap[4][128];int ladderMapped[4];unsigned char restMap[4][128];int restMapped[4];unsigned char walkMap[3][128];int walkMapped[3];unsigned char sourceMap[C1ASP14_RESOURCES][128],sourceMapped[C1ASP14_RESOURCES];} cgfa4Resource_t;
typedef c1afu14Bank_t cgfa4Clips_t;
typedef struct {uint64_t revision;snapshot_t*snapshot;entityState_t actor;char config[1024];} cgfa4Lease_t;
static cgfa4Resource_t cgfa4Resources[CGTA_CACHE];static cgfa4Clips_t cgfa4Clips;static int cgfa4Ready;static uint64_t cgfa4Revision=1,cgfa4Busy;
static c1asb14Bank_t scriptedClips;
static c1ab14Clip_t *cgSourcePoseClips[C1ASP14_RESOURCES];
static c1ab14Clip_t *foleyWalk,*rifleCrouchRun,*rifleCrouchRun1,*rifleCrouchRun3,*rifleCrouchRun4,*wallDive,*proneCrawl,*precombatRun,*entryClips[5],*ladderClips[4],*restClips[4],*courseWalk[3];
static int CGFA4_ValidWire(const entityState_t *actor,int now) {
    if(actor->cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION)return C1SourcePoseWireValid(actor,now);
    if(actor->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION)return C1ScriptedWireValid(actor,now);
    return C1FUAW_Valid(actor,now);
}
static void CGFA4_FreeClips(cgfa4Clips_t*b){C1AFU14_Free(b);}
static c1ab14Clip_t *cgJumperClip;
#include "cg_cod1_recyclable_resource40.inc"
static void CGFA4_Reset(void){unsigned i;for(i=0;i<C1ASP14_RESOURCES;i++){C1AB14_Free(cgSourcePoseClips[i]);cgSourcePoseClips[i]=NULL;}CGFA4_RecyclableReset();C1AB14_Free(cgJumperClip);cgJumperClip=NULL;if(!++cgfa4Revision)++cgfa4Revision;for(i=0;i<CGTA_CACHE;i++){C1AD14_Free(cgfa4Resources[i].resource);memset(cgfa4Resources+i,0,sizeof(cgfa4Resources[i]));}for(i=0;i<5;i++){C1AB14_Free(entryClips[i]);entryClips[i]=NULL;}C1AB14_Free(foleyWalk);foleyWalk=NULL;C1AB14_Free(rifleCrouchRun);rifleCrouchRun=NULL;C1AB14_Free(rifleCrouchRun1);rifleCrouchRun1=NULL;C1AB14_Free(rifleCrouchRun3);rifleCrouchRun3=NULL;C1AB14_Free(rifleCrouchRun4);rifleCrouchRun4=NULL;C1AB14_Free(wallDive);wallDive=NULL;C1AB14_Free(proneCrawl);proneCrawl=NULL;C1AB14_Free(precombatRun);precombatRun=NULL;for(i=0;i<4;i++){C1AB14_Free(ladderClips[i]);ladderClips[i]=NULL;C1AB14_Free(restClips[i]);restClips[i]=NULL;}for(i=0;i<3;i++){C1AB14_Free(courseWalk[i]);courseWalk[i]=NULL;}C1ASB14_Free(&scriptedClips);CGFA4_FreeClips(&cgfa4Clips);cgfa4Ready=0;}
static int CGFA4_Current(const cgfa4Lease_t*l){int i,actors=0;if(!l||l->revision!=cgfa4Revision||!cgs.cod1MapEntities||(!l->snapshot||(cg.snap!=l->snapshot&&cg.nextSnap!=l->snapshot))||!CGFA4_ValidWire(&l->actor,l->snapshot->serverTime))return 0;for(i=0;i<l->snapshot->numEntities;i++)if(l->snapshot->entities[i].number==l->actor.number){if(memcmp(l->snapshot->entities+i,&l->actor,sizeof(l->actor)))return 0;actors++;}return actors==1&&!strcmp(CG_ConfigString(l->actor.cod1TurretActorConfig),l->config)&&l->revision==cgfa4Revision&&(cg.snap==l->snapshot||cg.nextSnap==l->snapshot);}
static void*CGFA4_Read(const cgfa4Lease_t*l,const char*path,int*size){fileHandle_t f=0;void*data=NULL;int n=trap_FS_FOpenFile(path,&f,FS_READ);if(!CGFA4_Current(l)||n<1||n>16*1024*1024||!f)goto done;data=malloc(n);if(!data)goto done;memset(data,0,n);trap_FS_Read(data,n,f);if(!CGFA4_Current(l)){free(data);data=NULL;}else *size=n;done:if(f)trap_FS_FCloseFile(f);if(!CGFA4_Current(l)){free(data);data=NULL;}return data;}
static int CGFA4_LoadClips(const cgfa4Lease_t*l){cgfa4Clips_t staged;unsigned i;int n,ok=0;void*data=NULL;char path[128];if(cgfa4Ready)return CGFA4_Current(l);memset(&staged,0,sizeof(staged));for(i=0;i<47;i++){snprintf(path,sizeof(path),"xanim/%s",C1AFU14_Name(i));data=CGFA4_Read(l,path,&n);if(!data||!C1AFU14_Add(&staged,i,data,n))goto done;free(data);data=NULL;}if(!CGFA4_Current(l)||!C1AFU14_Ready(&staged))goto done;cgfa4Clips=staged;memset(&staged,0,sizeof(staged));cgfa4Ready=1;ok=1;done:free(data);CGFA4_FreeClips(&staged);return ok;}
static int CGFA4_BuildBank(const cgfa4Lease_t *lease,cgfa4Resource_t *resource) {
    c1asb14Bank_t staged={0};
    resource->version=lease->actor.cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION?COD1_SCRIPTED_ACTOR_WIRE_VERSION:lease->actor.cod1TurretActorVersion;
    if(resource->version==COD1_FACE_UNMOUNTED_ACTOR_WIRE_VERSION)
        return C1AFU14_Bank(resource->resource,&cgfa4Clips,&resource->bank);
    if(!C1ASB14_Ready(&scriptedClips)){
        unsigned i;
        for(i=0;i<C1ASB14_COUNT;i++){
            char path[128];int size=0;void *data;int decoded;
            snprintf(path,sizeof(path),"xanim/%s",c1asb14Names[i]);
            data=CGFA4_Read(lease,path,&size);
            decoded=data && C1ASB14_Add(&staged,i,data,size);
            free(data);
            if(!decoded || !CGFA4_Current(lease)){C1ASB14_Free(&staged);return 0;}
        }
        scriptedClips=staged;
    }
    return C1ASB14_Bank(resource->resource,&cgfa4Clips,&scriptedClips,&resource->bank);
}
#include "cg_cod1_face_resource40.inc"
#include "cg_cod1_anim_profile32.inc"
#include "cg_cod1_source_pose.inc"
/* Layer the authored pelvis/leg tracks before hierarchy composition. Hands,
 * head and eyes retain their selected local channels; root delta is not reapplied. */
static int CGFA4_Walk(const cgfa4Lease_t *l,cgfa4Resource_t *r,float phase,c1ab14Accumulator_t *pose){
 static const char *const lower[]={"bip01 pelvis","bip01 l thigh","bip01 r thigh","bip01 l calf","bip01 r calf","bip01 l foot","bip01 r foot","bip01 l toe0","bip01 r toe0"};
 c1ab14Accumulator_t sampled[128]={0};unsigned i,k,found=0;
 if(!foleyWalk){
  int size=0;void *raw=CGFA4_Read(l,"xanim/leaderwalk_cocky_idle",&size);c1ab14Clip_t *clip=NULL;
  int ok=raw&&C1AB14_Decode(raw,size,&clip)&&clip->frames==31&&clip->intervals==31&&clip->fps==30&&clip->bones==54;
  free(raw);if(!ok||!CGFA4_Current(l)){C1AB14_Free(clip);return 0;}foleyWalk=clip;
 }
 if(!r->gaitMapped){if(!C1AD14_Map(r->resource,foleyWalk,r->gaitMap))return 0;r->gaitMapped=1;}
 if(!C1AB14_Accumulate(foleyWalk,phase,1,sampled,foleyWalk->bones))return 0;
 for(i=0;i<foleyWalk->bones;i++)for(k=0;k<sizeof(lower)/sizeof(*lower);k++)if(!strcmp(foleyWalk->bone[i].name,lower[k])){
  if(found&(1u<<k)||r->gaitMap[i]>=r->bank.bones)return 0;
  found|=1u<<k;pose[r->gaitMap[i]]=sampled[i];break;
 }
 return found==511;
}
static int CGFA4_CourseAsset(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                             int frame,c1ab14Clip_t **clipOut,unsigned char **mapOut){
    int diving=frame==COD1_FRAME_WALL_DIVE;
    c1ab14Clip_t **cached=diving?&wallDive:&rifleCrouchRun;
    unsigned char *map=diving?resource->diveMap:resource->crouchMap;
    int *mapped=diving?&resource->diveMapped:&resource->crouchMapped;
    const char *path=diving?"xanim/jump_over_low_wall":"xanim/crouchrun_loop_forward_2";
    int fps=30,frames=diving?39:16,intervals=diving?38:16,bones=diving?54:55;
    if(frame==COD1_FRAME_RIFLE_CROUCHRUN_1){
        cached=&rifleCrouchRun1;map=resource->crouch1Map;mapped=&resource->crouch1Mapped;
        path="xanim/crouchrun_loop_forward_1";frames=intervals=20;bones=55;
    }
    if(frame==COD1_FRAME_RIFLE_CROUCHRUN_3){
        cached=&rifleCrouchRun3;map=resource->crouch3Map;mapped=&resource->crouch3Mapped;
        path="xanim/crouchrun_loop_forward_3";frames=intervals=14;bones=55;
    }
    if(frame==COD1_FRAME_RIFLE_CROUCHRUN_4){
        cached=&rifleCrouchRun4;map=resource->crouch4Map;mapped=&resource->crouch4Mapped;
        path="xanim/crouchrun_loop_forward_4";frames=intervals=20;bones=55;
    }
    if(frame>=COD1_CLIP_ENTRY_RIGHT && frame<=COD1_CLIP_ENTRY_SHUFFLE_B){
        static const char *const paths[]={"xanim/combatrun_right","xanim/combatrun_left","xanim/combatrun_back",
                                          "xanim/stand_alerta_shuffle_forward","xanim/stand_alertb_shuffle_forward"};
        static const int lengths[]={12,13,14,25,25};
        int i=frame-COD1_CLIP_ENTRY_RIGHT;
        cached=&entryClips[i];map=resource->entryMap[i];mapped=&resource->entryMapped[i];
        path=paths[i];frames=intervals=lengths[i];bones=55;
    }
    if(frame==COD1_FRAME_PRONE_CRAWL){
        cached=&proneCrawl;map=resource->crawlMap;mapped=&resource->crawlMapped;
        path="xanim/prone_crawl";frames=intervals=67;bones=54;
    }
    if(frame==COD1_FRAME_PRECOMBAT_RUN){
        cached=&precombatRun;map=resource->runMap;mapped=&resource->runMapped;
        path="xanim/precombatrun1";frames=intervals=20;bones=55;
    }
    if(COD1_IS_LADDER_FRAME(frame)){
        static const char *const paths[]={"xanim/ladder_climbup","xanim/ladder_climboff","xanim/ladder_climbon","xanim/ladder_climbdown"};
        int i=frame-COD1_FRAME_LADDER_UP;
        int transition=frame==COD1_FRAME_LADDER_ON || frame==COD1_FRAME_LADDER_OFF;
        cached=&ladderClips[i];map=resource->ladderMap[i];mapped=&resource->ladderMapped[i];
        path=paths[i];frames=transition?30:20;intervals=transition?29:20;bones=55;
    }
    if(COD1_IS_COURSE_WALK_FRAME(frame)){
        int loop=frame==COD1_FRAME_CROUCH_WALK_LOOP;
        cached=&courseWalk[loop];map=resource->walkMap[loop];mapped=&resource->walkMapped[loop];
        path=loop?"xanim/crouchwalk_loop":"xanim/crouch2crouchwalk_straight";
        frames=loop?18:14;intervals=loop?18:13;fps=15;bones=54;
    }
    if(frame==COD1_CLIP_CROUCH_STOP){
        cached=&courseWalk[2];map=resource->walkMap[2];mapped=&resource->walkMapped[2];
        path="xanim/crouchwalk_stop_rff";frames=10;intervals=9;fps=15;bones=54;
    }
    if(COD1_IS_REST_FRAME(frame)){
        static const char *const paths[]={"xanim/crouch_alert_A_idle","xanim/crouch_alert_A_twitch","xanim/crouch_alert_B_idle1","xanim/crouch_alert_B_twitch1"};
        static const int counts[]={137,70,100,111};
        int i=frame-COD1_FRAME_REST_A;
        cached=&restClips[i];map=resource->restMap[i];mapped=&resource->restMapped[i];
        path=paths[i];frames=counts[i];intervals=frames-(i&1);bones=53;
    }
    if(!*cached){
        uint64_t start=CGAP32_Start();int size=0;
        void *raw=CGFA4_Read(lease,path,&size);
        c1ab14Clip_t *clip=NULL;
        int ok=raw && C1AB14_Decode(raw,size,&clip) && clip->frames==frames &&
               clip->intervals==intervals && clip->fps==fps && clip->bones==bones;
        free(raw);
        ok=ok&&CGFA4_Current(lease);
        if(start)CGAP32_End(C1AP32_COURSE_LOAD,start,ok,(unsigned)lease->actor.number);
        if(!ok){C1AB14_Free(clip);return 0;}
        *cached=clip;
    }
    if(!*mapped){
        uint64_t start=CGAP32_Start();int ok=C1AD14_Map(resource->resource,*cached,map);
        if(start)CGAP32_End(C1AP32_COURSE_MAP,start,ok,(unsigned)lease->actor.number);
        if(!ok)return 0;
        *mapped=1;
    }
    *clipOut=*cached;
    *mapOut=map;
    return 1;
}
static int CGFA4_CourseLayer(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                            int frame,float phase,float weight,c1ab14Accumulator_t *pose){
    c1ab14Accumulator_t sampled[128]={0};
    c1ab14Clip_t *clip;
    unsigned char *map;
    unsigned i;
    if(!CGFA4_CourseAsset(lease,resource,frame,&clip,&map))return 0;
    if(weight<1){
        for(i=0;i<clip->bones;i++)sampled[i]=pose[map[i]];
        if(!C1AB14_NormalizeGroup(sampled,clip->bones,1-weight))return 0;
    }
    if(!C1AB14_Accumulate(clip,phase,weight,sampled,clip->bones))return 0;
    for(i=0;i<clip->bones;i++)pose[map[i]]=sampled[i];
    return 1;
}
/* The two active branches of original movement node 649 share the phase.
 * Their sole-child chains collapse in the ordinary XAnim tree sampler. */
static int CGFA4_CourseApproach(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                                float phase,c1ab14Accumulator_t *pose){
    c1ast14Node_t nodes[]={{0,-1,-1,1,0},{726,0,0},{727,0,1}};
    c1ast14Bank_t bank={0};
    c1ab14Accumulator_t mixed[128];
    const float blend=lease->actor.cod1TurretActorPhases[1][13];
    int frames[]={COD1_FRAME_PRECOMBAT_RUN,lease->actor.frame};
    unsigned i,j;
    bank.bones=resource->bank.bones;
    bank.count=2;
    nodes[1].currentWeight=1-blend;
    nodes[2].currentWeight=fminf(1,.001f+blend);
    for(i=0;i<2;i++){
        c1ab14Clip_t *clip;
        unsigned char *map;
        if(!CGFA4_CourseAsset(lease,resource,frames[i],&clip,&map))return 0;
        bank.clip[i]=clip;
        memcpy(bank.map[i],map,clip->bones);
        nodes[i+1].phase=phase;
    }
    if(!CGFA4_Current(lease) || !CGAP32_Tree(&bank,nodes,3,mixed,128))return 0;
    for(i=0;i<2;i++)for(j=0;j<bank.clip[i]->bones;j++){
        unsigned bone=bank.map[i][j];
        pose[bone]=mixed[bone];
    }
    return 1;
}
/* The retail body at the lower ladder mixes sibling groups 649 (movement)
 * and 668 (shuffle). Directional leaves share the group 726 phase. */
static int CGFA4_CourseEntry(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                             float phase,c1ab14Accumulator_t *pose){
    const float *p=lease->actor.cod1TurretActorPhases[1];
    int shuffleB=(lease->actor.cod1TurretActorFlags&COD1_ENTRY_SHUFFLE_B)!=0;
    int frames[5]={COD1_CLIP_ENTRY_RIGHT,COD1_CLIP_ENTRY_LEFT,COD1_CLIP_ENTRY_BACK,
                   COD1_FRAME_PRECOMBAT_RUN,shuffleB?COD1_CLIP_ENTRY_SHUFFLE_B:COD1_CLIP_ENTRY_SHUFFLE_A};
    c1ast14Node_t nodes[]={
        {0,-1,-1,1,0}, {649,0,-1,p[18],0}, {668,0,-1,p[19],0},
        {726,1,-1,1,0}, {739,3,0,p[14],phase}, {740,3,1,p[15],phase},
        {741,3,2,p[16],phase}, {742,3,-1,p[17],0}, {747,7,3,1,phase},
        {shuffleB?767u:768u,2,4,1,p[20]}
    };
    c1ast14Bank_t bank={0};
    c1ab14Accumulator_t mixed[128];
    unsigned i,j;
    bank.bones=resource->bank.bones;bank.count=5;
    for(i=0;i<bank.count;i++){
        c1ab14Clip_t *clip;
        unsigned char *map;
        if(!CGFA4_CourseAsset(lease,resource,frames[i],&clip,&map))return 0;
        bank.clip[i]=clip;memcpy(bank.map[i],map,clip->bones);
    }
    if(!CGFA4_Current(lease)||!CGAP32_Tree(&bank,nodes,sizeof(nodes)/sizeof(nodes[0]),mixed,128))return 0;
    for(i=0;i<bank.count;i++)for(j=0;j<bank.clip[i]->bones;j++){
        unsigned bone=bank.map[i][j];
        pose[bone]=mixed[bone];
    }
    return 1;
}

static int CGFA4_CourseClipImpl(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                            float phase,c1ab14Accumulator_t *pose){
    int flags=lease->actor.cod1TurretActorFlags;
    if(flags & COD1_COURSE_ENTRY_BIT)return CGFA4_CourseEntry(lease,resource,phase,pose);
    if(flags & COD1_COURSE_APPROACH_BIT){
        if(!CGFA4_CourseApproach(lease,resource,phase,pose))return 0;
    }else if(!CGFA4_CourseLayer(lease,resource,lease->actor.frame,phase,1,pose))return 0;
    if((flags & ~COD1_COURSE_APPROACH_BIT)!=COD1_COURSE_STOP_FLAGS)return 1;
    return CGFA4_CourseLayer(lease,resource,COD1_CLIP_CROUCH_STOP,
                             lease->actor.cod1TurretActorPhases[1][11],lease->actor.cod1TurretActorPhases[1][12],pose);
}
static int CGFA4_CourseClip(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,
                            float phase,c1ab14Accumulator_t *pose){
    uint64_t start=CGAP32_Start();unsigned owner=(unsigned)lease->actor.number;
    int ladder=COD1_IS_LADDER_FRAME(lease->actor.frame);
    int ok=CGFA4_CourseClipImpl(lease,resource,phase,pose);
    if(start)CGAP32_End(ladder?C1AP32_COURSE_LADDER:C1AP32_COURSE,start,ok,owner);
    return ok;
}
static int CGFA4_Jumper(const cgfa4Lease_t *lease,cgfa4Resource_t *resource,c1ab14Accumulator_t *pose){
 c1ab14Accumulator_t part[128]={0};unsigned i;float weight,phase;
 if(lease->actor.effect1Time!=0x4a5202)return 1;
 memcpy(&weight,&lease->actor.effect2Time,4);memcpy(&phase,&lease->actor.effect3Time,4);
 if(!(weight>=0&&weight<=1&&phase>=0&&phase<=1))return 0;
 if(!weight)return 1;
 if(!cgJumperClip){
  int n;void *data=CGFA4_Read(lease,"xanim/training_guy_practicejump",&n);
  c1ab14Clip_t *clip=NULL;int ok=data&&C1AB14_Decode(data,n,&clip)&&
    clip->frames==176&&clip->fps==30&&clip->bones==52&&CGFA4_Current(lease);
  free(data);if(!ok){C1AB14_Free(clip);return 0;}cgJumperClip=clip;
 }
 if(!resource->jumperMapped){
  if(!C1AD14_Map(resource->resource,cgJumperClip,resource->jumperMap))return 0;
  resource->jumperMapped=1;
 }
 if(weight<1){
  for(i=0;i<cgJumperClip->bones;i++)part[i]=pose[resource->jumperMap[i]];
  if(!C1AB14_NormalizeGroup(part,cgJumperClip->bones,1-weight))return 0;
 }
 if(!C1AB14_Accumulate(cgJumperClip,phase,weight,part,cgJumperClip->bones))return 0;
 for(i=0;i<cgJumperClip->bones;i++)pose[resource->jumperMap[i]]=part[i];
 return 1;
}

/* Cache endpoint poses, including complete wire state. Reuse between adjacent
 * snapshot pairs; collisions in the small cache only trigger resampling. */
typedef struct {
    uint64_t revision;
    entityState_t wire;
    cod1TurretActorRenderState_t pose;
} cgfa4Endpoint_t;
static cgfa4Endpoint_t cgfa4Endpoints[32][2];
static cgfa4Endpoint_t *CGFA4_PoseImpl(const cgfa4Lease_t *lease,cgfa4Resource_t *r,int *profileHit){
    const entityState_t *s=&lease->actor;
    cgfa4Endpoint_t *pair=cgfa4Endpoints[s->number%32],*slot;
    cod1TurretActorRenderState_t *state;
    c1ast14Node_t nodes[C1ASB14_NODES];c1ab14Accumulator_t accumulated[128];
    c1aw14Pose_t pose;unsigned i,k;
    for(i=0;i<2;i++)
        if(pair[i].revision==lease->revision && pair[i].pose.hModel==r->handle &&
           !memcmp(&pair[i].wire,s,sizeof(*s))){if(profileHit)*profileHit=1;return pair+i;}
    slot=pair[0].pose.sampleTick<=pair[1].pose.sampleTick?pair:pair+1;
    slot->revision=0;state=&slot->pose;
 float localPhases[2][21];int baseFlags=s->cod1TurretActorFlags & ~COD1_COURSE_EXTRA_FLAGS;int walking=s->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION && (baseFlags==3 || baseFlags==COD1_COURSE_STOP_FLAGS);
 memcpy(localPhases,s->cod1TurretActorPhases,sizeof(localPhases));if(walking){
  if(s->frame==0 && baseFlags==3)localPhases[1][10]=0;
  else memset(&localPhases[1][10],0,11*sizeof(float));
 }
 if(r && s->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION && s->cod1TurretActorGenericCurrent[97])
  memcpy(&localPhases[1][16],&s->effect2Time,sizeof(float));
 if(r && s->cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION){
  float phase=0;int gait=C1SourceWalkRead(s,&phase);
  if(gait<0||!CGFA4_SourceSample(lease,r,accumulated))return NULL;
  if(gait&&!CGFA4_Walk(lease,r,phase,accumulated))return NULL;
  if(!CGAP32_Skeleton(r->resource,accumulated,r->bank.bones,&pose))return NULL;
 }else if(!r||!CGFA4_Current(lease)||!(r->version==COD1_SCRIPTED_ACTOR_WIRE_VERSION?
 C1ASB14_NodesFull(lease->actor.cod1TurretActorGenericCurrent,localPhases,lease->actor.cod1TurretActorCurrent,nodes):
 C1AFU14_Nodes(lease->actor.cod1TurretActorGenericCurrent,localPhases,nodes))||!CGAP32_Tree(&r->bank,nodes,r->version==COD1_SCRIPTED_ACTOR_WIRE_VERSION?C1ASB14_NODES:56,accumulated,128)||(walking&&!((COD1_IS_RIFLE_CROUCHRUN(s->frame)||s->frame==COD1_FRAME_WALL_DIVE||s->frame==COD1_FRAME_PRONE_CRAWL||s->frame==COD1_FRAME_PRECOMBAT_RUN||COD1_IS_LADDER_FRAME(s->frame)||COD1_IS_REST_FRAME(s->frame)||COD1_IS_COURSE_WALK_FRAME(s->frame))?CGFA4_CourseClip:CGFA4_Walk)(lease,r,s->cod1TurretActorPhases[1][10],accumulated))||!CGFA4_Jumper(lease,r,accumulated)||!CGAP32_Skeleton(r->resource,accumulated,r->bank.bones,&pose))return NULL;
 memset(state,0,sizeof(*state));state->version=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION;state->size=sizeof(*state);state->hModel=r->handle;state->ownerNum=lease->actor.number;state->epochLo=lease->actor.cod1TurretEpochLo;state->epochHi=lease->actor.cod1TurretEpochHi;state->generation=lease->actor.cod1TurretActorGeneration;state->sampleTick=lease->actor.cod1TurretActorTick;state->boneCount=pose.bones;memcpy(state->origin,lease->actor.pos.trBase,12);memcpy(state->baseAngles,lease->actor.apos.trBase,12);
 for(i=0;i<pose.bones;i++){double norm=0;for(k=0;k<4;k++)norm+=(double)pose.q[i][k]*pose.q[i][k];if(!(norm>1e-15&&norm<1e15))return NULL;norm=sqrt(norm);for(k=0;k<4;k++)state->q[i][k]=(float)(pose.q[i][k]/norm);memcpy(state->p[i],pose.matrix[i]+12,12);}if(!CGFA4_Current(lease))return NULL;
    slot->wire=*s;slot->revision=lease->revision;return slot;
}
static cgfa4Endpoint_t *CGFA4_Pose(const cgfa4Lease_t *lease,cgfa4Resource_t *r){
 uint64_t start=CGAP32_Start();int hit=0;
 cgfa4Endpoint_t *result=CGFA4_PoseImpl(lease,r,start?&hit:NULL);
 if(start)CGAP32_End(hit?C1AP32_HIT:C1AP32_MISS,start,result!=NULL,(unsigned)lease->actor.number);
 return result;
}
static void CG_BlendActorPose(cod1TurretActorRenderState_t *out,const cod1TurretActorRenderState_t *next,float f){
    unsigned bone,j;
    for(j=0;j<3;j++){
        out->origin[j]+=f*(next->origin[j]-out->origin[j]);
        out->baseAngles[j]=LerpAngle(out->baseAngles[j],next->baseAngles[j],f);
    }
    for(bone=0;bone<out->boneCount;bone++){
        float dot=0,length=0,sign;
        for(j=0;j<4;j++)dot+=out->q[bone][j]*next->q[bone][j];
        sign=dot<0?-1.f:1.f;
        for(j=0;j<4;j++){
            float value=out->q[bone][j]+f*(sign*next->q[bone][j]-out->q[bone][j]);
            out->q[bone][j]=value;length+=value*value;
        }
        length=sqrtf(length);
        for(j=0;j<4;j++)out->q[bone][j]/=length;
        for(j=0;j<3;j++)out->p[bone][j]+=f*(next->p[bone][j]-out->p[bone][j]);
    }
}
static int CGFA4_Entity(centity_t*cent){cgfa4Lease_t lease;cgfa4Resource_t*r;const entityState_t*s=NULL;int j,actors=0,ok=0;cod1TurretActorRenderState_t state;
 if(!cent||!cg.snap||!cgs.cod1MapEntities||cgfa4Busy)return 1;for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==cent->currentState.number){s=cg.snap->entities+j;actors++;}if(actors!=1||!s||(s->eFlags&EF_NODRAW)||!CGFA4_ValidWire(s,cg.snap->serverTime))return 1;
 memset(&lease,0,sizeof(lease));lease.revision=cgfa4Revision;lease.snapshot=cg.snap;lease.actor=*s;{const char*text=CG_ConfigString(s->cod1TurretActorConfig);if(!text||strlen(text)>=sizeof(lease.config))return 1;strcpy(lease.config,text);}if(!CGFA4_Current(&lease))return 1;cgfa4Busy=lease.revision;
 int baseFlags=s->cod1TurretActorFlags & ~COD1_COURSE_EXTRA_FLAGS;
 int walking=s->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION && (baseFlags==3 || baseFlags==COD1_COURSE_STOP_FLAGS);
 cgfa4Endpoint_t *first,*second;const entityState_t *n=&cent->nextState;
 uint64_t profileActor,profilePart;CGAP32_Frame();profileActor=CGAP32_Start();
 if(s->cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION){static unsigned seen[MAX_GENTITIES];if(!seen[s->number]++)CG_Printf("[378 client arrived] owner=%d tick=%d\n",s->number,s->cod1TurretActorTick);}
 profilePart=CGAP32_Start();r=CGFA4_Resource(&lease);
 if(profilePart)CGAP32_End(C1AP32_RESOURCE,profilePart,r!=NULL,(unsigned)s->number);
 if(!r||!CGFA4_Current(&lease)||(first=CGFA4_Pose(&lease,r))==NULL)goto done;
 state=first->pose;
 if(cg.nextSnap && cent->interpolate && cg.frameInterpolation>0 && cg.frameInterpolation<=1 &&
    n->number==s->number && n->eType==s->eType && !(n->eFlags&EF_NODRAW) &&
    !((s->eFlags^n->eFlags)&EF_TELEPORT_BIT) && n->cod1TurretActorTick>s->cod1TurretActorTick &&
    n->cod1TurretEpochLo==s->cod1TurretEpochLo && n->cod1TurretEpochHi==s->cod1TurretEpochHi &&
    n->cod1TurretActorGeneration==s->cod1TurretActorGeneration &&
    n->cod1TurretActorConfig==s->cod1TurretActorConfig && n->cod1TurretActorVersion==s->cod1TurretActorVersion){
    cgfa4Lease_t nextLease=lease;
    nextLease.snapshot=cg.nextSnap;nextLease.actor=*n;
    if(CGFA4_Current(&nextLease) && (second=CGFA4_Pose(&nextLease,r))!=NULL &&
       second->pose.boneCount==state.boneCount){
        profilePart=CGAP32_Start();CG_BlendActorPose(&state,&second->pose,cg.frameInterpolation);
        if(profilePart)CGAP32_End(C1AP32_BLEND,profilePart,1,(unsigned)s->number);
        if(cg_debugAnim.integer>2)CG_Printf("[Training render pose] owner=%d/%u t=%d first=%d next=%d f=%.4f x=%.3f joint=%.4f\n",
            state.ownerNum,state.generation,cg.time,s->cod1TurretActorTick,n->cod1TurretActorTick,
            cg.frameInterpolation,state.origin[0],state.p[state.boneCount-1][2]);
    }
 }
 if(!CGFA4_Current(&lease))goto done;
 profilePart=CGAP32_Start();ok=trap_R_AddCod1TurretActorToScene(&state)==1;
 if(profilePart)CGAP32_End(C1AP32_SUBMIT,profilePart,ok,(unsigned)s->number);
 if(ok && cg_debugAnim.integer && s->cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION){
  float phase=0;int gait=C1SourceWalkRead(s,&phase);static unsigned seen[MAX_GENTITIES];static int tick[MAX_GENTITIES];
  if(gait==1)seen[s->number]=s->cod1TurretActorGeneration;
  if(seen[s->number]==(unsigned)s->cod1TurretActorGeneration && s->cod1TurretActorTick%250==0 && tick[s->number]!=s->cod1TurretActorTick){
   int left=C1AT14_Find(r->resource->assembly,"bip01 l foot"),right=C1AT14_Find(r->resource->assembly,"bip01 r foot"),head=C1AT14_Find(r->resource->assembly,"bip01 head");tick[s->number]=s->cod1TurretActorTick;
   if(left>=0&&right>=0&&head>=0)CG_Printf("[572 Foley walk render] tick=%d owner=%d active=%d phase=%.6f left=%.5f,%.5f,%.5f right=%.5f,%.5f,%.5f head=%.5f,%.5f,%.5f,%.5f accepted=1\n",s->cod1TurretActorTick,s->number,gait,phase,state.p[left][0],state.p[left][1],state.p[left][2],state.p[right][0],state.p[right][1],state.p[right][2],state.q[head][0],state.q[head][1],state.q[head][2],state.q[head][3]);
  }
 }
 if(cg_debugAnim.integer && s->cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION){
  static int tick[MAX_GENTITIES];if(tick[s->number]!=s->cod1TurretActorTick){tick[s->number]=s->cod1TurretActorTick;
   CG_Printf("[378 source render] owner=%d tick=%d accepted=%d bones=%u source_walk=%d\n",s->number,s->cod1TurretActorTick,ok,state.boneCount,s->effect1Time==C1_SOURCE_WALK_TAG);}
 }
 if(cg_debugAnim.integer&&s->effect1Time==0x4a5202&&state.sampleTick%500==0){
  static int last=-1;float w,p;memcpy(&w,&s->effect2Time,4);memcpy(&p,&s->effect3Time,4);
  if(last!=state.sampleTick){CG_Printf("[Training jumper client] tick=%d owner=%d/%d phase=%.6f weight=%.6f bones=%d parts=%d accepted=%d\n",
   state.sampleTick,state.ownerNum,state.generation,p,w,state.boneCount,r->definition.partCount,ok);last=state.sampleTick;}
 }
 if(ok&&cg_debugAnim.integer>1&&s->effect1Time==0x4a5202){
  static int lastHead=-1;
  if(lastHead!=state.sampleTick){
   int bone=C1AT14_Find(r->resource->assembly,"bip01 head");
   if(bone>=0){vec3_t axis[3],pos;int j;
    AnglesToAxis(state.baseAngles,axis);VectorCopy(state.origin,pos);
    for(j=0;j<3;j++)VectorMA(pos,state.p[bone][j],axis[j],pos);
    CG_Printf("[tower head] tick=%d xyz=%.4f %.4f %.4f\n",state.sampleTick,pos[0],pos[1],pos[2]);
   }
   lastHead=state.sampleTick;
  }
 }
 if(ok && cg_debugAnim.integer && s->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION){
   unsigned j;
   for(j=0;j<C1ASB14_FACIALS;j++)if(s->cod1TurretActorCurrent[0][j]>0){
     int bone=C1AT14_Find(r->resource->assembly,"m_bottomlip");
     if(bone>=0)CG_Printf("[Training dialogue face] owner=%d/%u tick=%d clip=%u phase=%.6f weight=%.6f bone=%d xyz=%.5f,%.5f,%.5f\n",
       state.ownerNum,state.generation,state.sampleTick,C1ASB14_Definition(C1ASB14_BASE_NODES+j)->ordinal,
       s->cod1TurretActorCurrent[1][j],s->cod1TurretActorCurrent[0][j],bone,state.p[bone][0],state.p[bone][1],state.p[bone][2]);
   }
 }
 if(ok && cg_debugAnim.integer && !walking && s->cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION &&
    (s->cod1TurretActorGenericCurrent[82]>0 || s->cod1TurretActorGenericCurrent[83]>0 ||
     s->cod1TurretActorGenericCurrent[84]>0 || s->cod1TurretActorGenericCurrent[85]>0 || s->cod1TurretActorGenericCurrent[86]>0)){
    int node;
    int hand=C1AT14_Find(r->resource->assembly,"bip01 r hand");
    if(hand>=0)for(node=82;node<=86;node++)if(s->cod1TurretActorGenericCurrent[node]>0)
        CG_Printf("[Training Moody gesture] owner=%d/%u tick=%d node=%d phase=%.6f weight=%.6f hand=%.4f,%.4f,%.4f\n",
          state.ownerNum,state.generation,state.sampleTick,node,s->cod1TurretActorPhases[1][node-71],
          s->cod1TurretActorGenericCurrent[node],state.p[hand][0],state.p[hand][1],state.p[hand][2]);
 }
 if(ok && walking && (s->cod1TurretActorFlags & COD1_COURSE_ENTRY_BIT) && cg_debugAnim.integer){
    int foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    if(foot>=0)CG_Printf("[Runner entry pose] owner=%d/%u tick=%d phase=%.6f run=%.6f shuffle=%.6f foot=%.6f\n",
        state.ownerNum,state.generation,state.sampleTick,s->cod1TurretActorPhases[1][10],
        s->cod1TurretActorPhases[1][18],s->cod1TurretActorPhases[1][19],state.p[foot][2]);
 }
 if(ok && walking && (s->cod1TurretActorFlags & COD1_COURSE_APPROACH_BIT) && cg_debugAnim.integer){
    int pelvis=C1AT14_Find(r->resource->assembly,"bip01 pelvis"),foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    if(pelvis>=0 && foot>=0)CG_Printf("[Runner approach pose] owner=%d/%u tick=%d frame=%d blend=%.6f phase=%.6f pelvisZ=%.4f footZ=%.4f\n",
        state.ownerNum,state.generation,state.sampleTick,s->frame,s->cod1TurretActorPhases[1][13],s->cod1TurretActorPhases[1][10],state.p[pelvis][2],state.p[foot][2]);
 }
 if(ok && baseFlags==COD1_COURSE_STOP_FLAGS && cg_debugAnim.integer){
    int foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    if(foot>=0)CG_Printf("[Runner stop] owner=%d/%u tick=%d gait=%d phase=%.4f weight=%.4f foot=%.4f\n",
        state.ownerNum,state.generation,state.sampleTick,s->frame,s->cod1TurretActorPhases[1][11],s->cod1TurretActorPhases[1][12],state.p[foot][2]);
 }
 if(ok && walking && COD1_IS_COURSE_WALK_FRAME(s->frame) && cg_debugAnim.integer){
    int foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    if(foot>=0)CG_Printf("[Runner walk] owner=%d/%u clip=%d phase=%.3f foot=%.3f\n",state.ownerNum,state.generation,s->frame,s->cod1TurretActorPhases[1][10],state.p[foot][2]);
 }else if(ok && walking && COD1_IS_REST_FRAME(s->frame) && cg_debugAnim.integer){
    int pelvis=C1AT14_Find(r->resource->assembly,"bip01 pelvis"),foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    int hand=C1AT14_Find(r->resource->assembly,"bip01 r hand");
    if(pelvis>=0 && foot>=0 && hand>=0)CG_Printf("[%s rest] owner=%d/%u clip=%d phase=%.3f xy=%.1f,%.1f pelvis=%.3f foot=%.3f hand=%.3f,%.3f,%.3f\n",Q_stricmp(r->definition.model[0],"character_Elder")?"Runner":"Elder",state.ownerNum,state.generation,s->frame,s->cod1TurretActorPhases[1][10],state.origin[0],state.origin[1],state.p[pelvis][2],state.p[foot][2],state.p[hand][0],state.p[hand][1],state.p[hand][2]);
 }else if(ok && walking && COD1_IS_LADDER_FRAME(s->frame) && cg_debugAnim.integer){
    CG_Printf("[%s ladder pose] owner=%d/%u frame=%d phase=%.3f z=%.2f\n",Q_stricmp(r->definition.model[0],"character_Elder")?"Runner":"Elder",state.ownerNum,state.generation,s->frame,s->cod1TurretActorPhases[1][10],state.origin[2]);
 }else if(ok && walking && s->frame==COD1_FRAME_WALL_DIVE && cg_debugAnim.integer){
    CG_Printf("[%s jump pose] owner=%d/%u phase=%.3f z=%.2f\n",Q_stricmp(r->definition.model[0],"character_Elder")?"Runner":"Elder",state.ownerNum,state.generation,s->cod1TurretActorPhases[1][10],state.origin[2]);
 }else if(ok && walking && (COD1_IS_RIFLE_CROUCHRUN(s->frame) || s->frame==COD1_FRAME_PRONE_CRAWL || s->frame==COD1_FRAME_PRECOMBAT_RUN) && cg_debugAnim.integer){
    int pelvis=C1AT14_Find(r->resource->assembly,"bip01 pelvis");
    int foot=C1AT14_Find(r->resource->assembly,"bip01 l foot");
    if(pelvis>=0 && foot>=0)CG_Printf("[%s %s] owner=%d/%u phase=%.3f xy=%.1f,%.1f pelvis=%.2f foot=%.2f frame=%d\n",
       Q_stricmp(r->definition.model[0],"character_Elder")?"Runner":"Elder",s->frame==COD1_FRAME_PRONE_CRAWL?"crawl":s->frame==COD1_FRAME_PRECOMBAT_RUN?"run":"crouch",state.ownerNum,state.generation,s->cod1TurretActorPhases[1][10],state.origin[0],state.origin[1],state.p[pelvis][2],state.p[foot][2],s->frame);
 }else if(ok && walking && cg_debugAnim.integer){
  int a=C1AT14_Find(r->resource->assembly,"bip01 l foot"),b=C1AT14_Find(r->resource->assembly,"bip01 r foot");
  if(a>=0&&b>=0)CG_Printf("[Foley gait] owner=%d/%u tick=%d phase=%.5f xy=%.3f,%.3f feetZ=%.3f,%.3f\\n",state.ownerNum,state.generation,state.sampleTick,s->cod1TurretActorPhases[1][10],state.origin[0],state.origin[1],state.p[a][2],state.p[b][2]);
 }
 done:if(cgfa4Busy==lease.revision)cgfa4Busy=0;
 if(profileActor)CGAP32_End(C1AP32_ACTOR,profileActor,ok,(unsigned)lease.actor.number);
 (void)ok;return 1;
}

#endif
