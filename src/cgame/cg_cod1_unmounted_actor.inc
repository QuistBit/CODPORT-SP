/* Separate unmounted profile3: current original shuffle/Look DObj, no gun. */
/* Wirev2 exact98 current ancestor weights and actual42 MG phases. All40Look
 * leaves are original one-frame resources; actual HEAD/eyes participate in the
 * same ordinary DObj tree. No missing-leaf or BODY-only fallback. */
#include "../qcommon/cod1_actor_unmounted_tree14.h"
#include "../game/bg_cod1_unmounted_actor_wire.h"
#define COD1_TOWER_GREETING
#include "../qcommon/cod1_actor_scripted_bank14.h"
extern int CG_Cod1BriefingElapsed(const entityState_t *);
typedef struct {cod1TurretActorResourceState_t definition;int handle;c1ad14Resource_t*resource;c1ast14Bank_t bank; c1asb14Bank_t briefing; int briefingTried;} cgua3Resource_t;
typedef c1aut14Bank_t cgua3Clips_t;
typedef struct {uint64_t revision;snapshot_t*snapshot;entityState_t actor;char config[1024];} cgua3Lease_t;
static cgua3Resource_t cgua3Resources[CGTA_CACHE];static cgua3Clips_t cgua3Clips;static int cgua3Ready;static uint64_t cgua3Revision=1,cgua3Busy;
static void CGUA3_FreeClips(cgua3Clips_t*b){C1AUT14_Free(b);}
static void CGUA3_Reset(void){unsigned i;if(!++cgua3Revision)++cgua3Revision;for(i=0;i<CGTA_CACHE;i++){C1ASB14_Free(&cgua3Resources[i].briefing);C1AD14_Free(cgua3Resources[i].resource);memset(cgua3Resources+i,0,sizeof(cgua3Resources[i]));}CGUA3_FreeClips(&cgua3Clips);cgua3Ready=0;}
static int CGUA3_Current(const cgua3Lease_t*l){int i,actors=0;if(!l||l->revision!=cgua3Revision||!cgs.cod1MapEntities||cg.snap!=l->snapshot||!cg.snap||!C1UAW_Valid(&l->actor,cg.snap->serverTime))return 0;for(i=0;i<cg.snap->numEntities;i++)if(cg.snap->entities[i].number==l->actor.number){if(memcmp(cg.snap->entities+i,&l->actor,sizeof(l->actor)))return 0;actors++;}return actors==1&&!strcmp(CG_ConfigString(l->actor.cod1TurretActorConfig),l->config)&&l->revision==cgua3Revision&&cg.snap==l->snapshot;}
static void*CGUA3_Read(const cgua3Lease_t*l,const char*path,int*size){fileHandle_t f=0;void*data=NULL;int n=trap_FS_FOpenFile(path,&f,FS_READ);if(!CGUA3_Current(l)||n<1||n>16*1024*1024||!f)goto done;data=malloc(n);if(!data)goto done;memset(data,0,n);trap_FS_Read(data,n,f);if(!CGUA3_Current(l)){free(data);data=NULL;}else *size=n;done:if(f)trap_FS_FCloseFile(f);if(!CGUA3_Current(l)){free(data);data=NULL;}return data;}
static int CGUA3_LoadClips(const cgua3Lease_t*l){cgua3Clips_t staged;unsigned i;int n,ok=0;void*data=NULL;char path[128];if(cgua3Ready)return CGUA3_Current(l);memset(&staged,0,sizeof(staged));for(i=0;i<42;i++){snprintf(path,sizeof(path),"xanim/%s",c1aut14Names[i]);data=CGUA3_Read(l,path,&n);if(!data||!C1AUT14_Add(&staged,i,data,n))goto done;free(data);data=NULL;}if(!CGUA3_Current(l)||!C1AUT14_Ready(&staged))goto done;cgua3Clips=staged;memset(&staged,0,sizeof(staged));cgua3Ready=1;ok=1;done:free(data);CGUA3_FreeClips(&staged);return ok;}
static cgua3Resource_t*CGUA3_Resource(const cgua3Lease_t*l){cgua3Resource_t staged,*slot=NULL;cod1TurretActorResourceState_t def;c1at14Input_t inputs[8];void*models[8]={0},*parts[8]={0};char names[8][128],path[128];unsigned i;int n,ok=0;
 if(!C1TAW_Composition(l->config,&def)||!CGUA3_LoadClips(l))return NULL;for(i=0;i<CGTA_CACHE;i++){if(cgua3Resources[i].handle&&!memcmp(&cgua3Resources[i].definition,&def,sizeof(def)))return cgua3Resources+i;if(!cgua3Resources[i].resource&&!slot)slot=cgua3Resources+i;}if(!slot)return NULL;memset(&staged,0,sizeof(staged));memset(inputs,0,sizeof(inputs));staged.definition=def;
 for(i=0;i<(unsigned)def.partCount;i++){c1mrModelHeader_t model;snprintf(path,sizeof(path),"xmodel/%s",def.model[i]);models[i]=CGUA3_Read(l,path,&n);if(!models[i]||!C1MR_DecodeModel(models[i],n,&model)||!C1TAW_Name(model.lodName[0]))goto done;inputs[i].model=models[i];inputs[i].modelBytes=n;inputs[i].modelName=def.model[i];strcpy(names[i],model.lodName[0]);inputs[i].partsName=names[i];inputs[i].tag=def.tag[i];snprintf(path,sizeof(path),"xmodelparts/%s",names[i]);parts[i]=CGUA3_Read(l,path,&n);if(!parts[i])goto done;inputs[i].parts=parts[i];inputs[i].partsBytes=n;}
 if(!C1AD14_Create(inputs,def.partCount,&staged.resource)||!C1AUT14_Bank(staged.resource,&cgua3Clips,&staged.bank)||!CGUA3_Current(l))goto done;
 staged.handle=trap_R_RegisterCod1TurretActor(&def);if(!staged.handle||!CGUA3_Current(l)||slot->resource)goto done;*slot=staged;staged.resource=NULL;ok=1;
 done:for(i=0;i<8;i++){free(models[i]);free(parts[i]);}C1AD14_Free(staged.resource);return ok?slot:NULL;
}
/* Extend the actual DObj sparse tree with the authored body5/scripted/
 * level_training leaf. Its root has no translation/rotation keys. Look nodes,
 * attachments, world origin and the current snapshot still belong to Foley. */
static void CGUA3_LoadBriefing(cgua3Resource_t *r,const cgua3Lease_t *l){
    unsigned i;
    r->briefingTried=1;
    for(i=0;i<2;i++){
        char path[128];int size=0;
        void *raw;
        snprintf(path,sizeof(path),"xanim/%s",c1asb14Names[i]);
        raw=CGUA3_Read(l,path,&size);
        if(!raw)return;
        if(!C1ASB14_Add(&r->briefing,i,raw,size)){free(raw);return;}
        free(raw);
        if(!C1AD14_Map(r->resource,r->briefing.body[i],r->bank.map[42+i]))return;
        r->bank.clip[42+i]=r->briefing.body[i];
    }
}
static int CGUA3_Briefing(cgua3Resource_t *r,const cgua3Lease_t *l,
                          c1ast14Node_t nodes[55],int *elapsed,int *bodyActive) {
    int ms,body,face;
    float weight,phase;
    if(Q_stricmp(r->definition.model[0],"character_Foley"))return 0;
    if(!r->briefingTried)CGUA3_LoadBriefing(r,l);
    ms=CG_Cod1BriefingElapsed(&l->actor);
    if(!r->bank.clip[42] || ms<0)return 0;
    body=ms<5200;
    /* First-line facial clip: 166 intervals at 30 fps, independently of
     * the 156-interval body. Reuse the qualified C026 owner/cue. */
    face=r->bank.clip[43]!=NULL && ms<5534;
    if(!body&&!face)return 0;
    *elapsed=ms;*bodyActive=body;
    weight=body?(ms<200?ms/200.0f:ms>5000?(5200-ms)/200.0f:1):0;
    phase=body?ms*(1.0f/5200.0f):1;
    nodes[1].currentWeight=1;
    if(body)nodes[2].currentWeight=1;
    nodes[50]=(c1ast14Node_t){801,2,-1,weight,0};
    nodes[51]=(c1ast14Node_t){879,50,-1,1,0};
    nodes[52]=(c1ast14Node_t){880,51,42,1,phase};
    if(!face)return 53;
    nodes[7].currentWeight=1;
    weight=ms<100?ms/100.0f:1;
    nodes[53]=(c1ast14Node_t){1614,7,-1,weight,0};
    nodes[54]=(c1ast14Node_t){1647,53,43,1,ms*(30.0f/166000.0f)};
    return 55;
}
static int CGUA3_Entity(centity_t*cent){cgua3Lease_t lease;cgua3Resource_t*r;const entityState_t*s=NULL;unsigned i,k;int j,actors=0,ok=0;const char*stage="resource";int gesture=0,bodyActive=0,elapsed=-1;c1ast14Node_t nodes[55];c1ab14Accumulator_t accumulated[128];c1aw14Pose_t pose;cod1TurretActorRenderState_t state;
 if(!cent||!cg.snap||!cgs.cod1MapEntities||cgua3Busy)return 1;for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==cent->currentState.number){s=cg.snap->entities+j;actors++;}if(actors!=1||!s||(s->eFlags&EF_NODRAW)||s->cod1TurretActorVersion!=COD1_UNMOUNTED_ACTOR_WIRE_VERSION||!C1UAW_Valid(s,cg.snap->serverTime))return 1;
 memset(&lease,0,sizeof(lease));lease.revision=cgua3Revision;lease.snapshot=cg.snap;lease.actor=*s;{const char*text=CG_ConfigString(s->cod1TurretActorConfig);if(!text||strlen(text)>=sizeof(lease.config))return 1;strcpy(lease.config,text);}if(!CGUA3_Current(&lease))return 1;cgua3Busy=lease.revision;
 r=CGUA3_Resource(&lease);if(!r||!CGUA3_Current(&lease))goto done;
 stage="wire-to-nodes";if(!C1AUT14_Nodes(lease.actor.cod1TurretActorGenericCurrent,lease.actor.cod1TurretActorPhases,nodes))goto done;
 stage="sample";
 gesture=CGUA3_Briefing(r,&lease,nodes,&elapsed,&bodyActive);
 if(!CGUA3_Current(&lease))goto done;
 if(gesture){
    c1ast14Bank_t bank=r->bank;bank.count=gesture==55?44:43;
    if(!C1AST14_Sample(&bank,nodes,gesture,accumulated,128))goto done;
 }else if(!C1AST14_Sample(&r->bank,nodes,50,accumulated,128))goto done;
 stage="build-pose";if(!C1AD14_Build(r->resource,accumulated,r->bank.bones,&pose))goto done;
 stage="normalize";
 memset(&state,0,sizeof(state));state.version=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION;state.size=sizeof(state);state.hModel=r->handle;state.ownerNum=lease.actor.number;state.epochLo=lease.actor.cod1TurretEpochLo;state.epochHi=lease.actor.cod1TurretEpochHi;state.generation=lease.actor.cod1TurretActorGeneration;state.sampleTick=lease.actor.cod1TurretActorTick;state.boneCount=pose.bones;memcpy(state.origin,lease.actor.pos.trBase,12);memcpy(state.baseAngles,lease.actor.apos.trBase,12);
 for(i=0;i<pose.bones;i++){double norm=0;for(k=0;k<4;k++)norm+=(double)pose.q[i][k]*pose.q[i][k];if(!(norm>1e-15&&norm<1e15))goto done;norm=sqrt(norm);for(k=0;k<4;k++)state.q[i][k]=(float)(pose.q[i][k]/norm);memcpy(state.p[i],pose.matrix[i]+12,12);}if(!CGUA3_Current(&lease))goto done;stage="renderer-submit";ok=trap_R_AddCod1TurretActorToScene(&state)==1;
 done:
 /* Opt-in diagnostic at source-snapshot cadence, not every render frame.
  * Rendering refusal remains refusal: never replace it with a bind pose. */
 if(cg_debugAnim.integer){static int lastTick=-1,lastOwner=-1;static uint64_t lastRevision;
  if(lastTick!=lease.actor.cod1TurretActorTick||lastOwner!=lease.actor.number||lastRevision!=lease.revision){
   lastTick=lease.actor.cod1TurretActorTick;lastOwner=lease.actor.number;lastRevision=lease.revision;
   if(gesture==55 && ok)CG_Printf("C027 listenup owner=%d generation=%d elapsed=%d phase=%.6f tracks=%u body=%d\n",lease.actor.number,lease.actor.cod1TurretActorGeneration,elapsed,nodes[54].phase,r->bank.clip[43]->bones,bodyActive);
   if(bodyActive && ok)CG_Printf("C026 todayobstacle owner=%d generation=%d elapsed=%d parts=%d\n",lease.actor.number,lease.actor.cod1TurretActorGeneration,elapsed,r->definition.partCount);
   CG_Printf("R243_UNMOUNTED_CLIENT owner=%d generation=%d tick=%d phases=(%.6f %.6f) stage=%s accepted=%d parts=%d bones=%d\n",lease.actor.number,lease.actor.cod1TurretActorGeneration,lease.actor.cod1TurretActorTick,lease.actor.cod1TurretActorPhases[0][0],lease.actor.cod1TurretActorPhases[0][1],stage,ok,r?r->definition.partCount:0,r?r->bank.bones:0);
  }
 }
 if(cgua3Busy==lease.revision)cgua3Busy=0;return 1;
}
