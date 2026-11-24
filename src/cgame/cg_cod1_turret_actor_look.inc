#include "../game/bg_cod1_unmounted_actor_wire.h"
/* Wirev2 exact98 current ancestor weights and actual42 MG phases. All40Look
 * leaves are original one-frame resources; actual HEAD/eyes participate in the
 * same ordinary DObj tree. No missing-leaf or BODY-only fallback. */
#include "../qcommon/cod1_actor_look_tree14.h"
typedef struct {cod1TurretActorResourceState_t definition;int handle;c1ad14Resource_t*resource;c1ast14Bank_t bank;} cgta2Resource_t;
typedef struct {c1al14Bank_t look;c1ab14Clip_t*mg[2][21];} cgta2Clips_t;
typedef struct {uint64_t revision;snapshot_t*snapshot;entityState_t actor,gun;char config[1024];} cgta2Lease_t;
static cgta2Resource_t cgta2Resources[CGTA_CACHE];static cgta2Clips_t cgta2Clips;static int cgta2Ready;static uint64_t cgta2Revision=1,cgta2Busy;
static void CGTA2_FreeClips(cgta2Clips_t*b){unsigned g,i;C1AL14_Free(&b->look);for(g=0;g<2;g++)for(i=0;i<21;i++){C1AB14_Free(b->mg[g][i]);b->mg[g][i]=NULL;}}
static void CGTA2_Reset(void){unsigned i;if(!++cgta2Revision)++cgta2Revision;for(i=0;i<CGTA_CACHE;i++){C1AD14_Free(cgta2Resources[i].resource);memset(cgta2Resources+i,0,sizeof(cgta2Resources[i]));}CGTA2_FreeClips(&cgta2Clips);cgta2Ready=0;}
static int CGTA2_Detached(const entityState_t *actor){
 return actor->cod1TurretActorVersion==COD1_UNMOUNTED_ACTOR_WIRE_VERSION &&
        actor->cod1TurretActorFlags==COD1_UNMOUNTED_MG_FLAGS;
}
static int CGTA2_Current(const cgta2Lease_t*l){int i,actors=0,guns=0;if(!l||l->revision!=cgta2Revision||!cgs.cod1MapEntities||(!l->snapshot||(cg.snap!=l->snapshot&&cg.nextSnap!=l->snapshot))||l->snapshot->serverTime<l->actor.cod1TurretActorTick)return 0;
 for(i=0;i<l->snapshot->numEntities;i++){const entityState_t*s=l->snapshot->entities+i;if(s->number==l->actor.number){if(memcmp(s,&l->actor,sizeof(*s)))return 0;actors++;}if(!CGTA2_Detached(&l->actor) && s->number==l->gun.number){if(memcmp(s,&l->gun,sizeof(*s)))return 0;guns++;}}
 if(CGTA2_Detached(&l->actor))
  return actors==1 && C1UAW_Valid(&l->actor,l->snapshot->serverTime) &&
         !strcmp(CG_ConfigString(l->actor.cod1TurretActorConfig),l->config);
 return actors==1&&guns==1&&!strcmp(CG_ConfigString(l->actor.cod1TurretActorConfig),l->config)&&!strcmp(CG_ConfigString(CS_MODELS+l->gun.modelindex),COD1_TURRET_MODEL_ALIAS)&&cgs.gameModels[l->gun.modelindex]!=0&&l->revision==cgta2Revision&&(cg.snap==l->snapshot||cg.nextSnap==l->snapshot);
}
static void*CGTA2_Read(const cgta2Lease_t*l,const char*path,int*size){fileHandle_t f=0;void*data=NULL;int n=trap_FS_FOpenFile(path,&f,FS_READ);if(!CGTA2_Current(l)||n<1||n>16*1024*1024||!f)goto done;data=malloc(n);if(!data)goto done;memset(data,0,n);trap_FS_Read(data,n,f);if(!CGTA2_Current(l)){free(data);data=NULL;}else *size=n;done:if(f)trap_FS_FCloseFile(f);if(!CGTA2_Current(l)){free(data);data=NULL;}return data;}
static int CGTA2_LoadClips(const cgta2Lease_t*l){cgta2Clips_t staged;unsigned i,g,row,y;int n,ok=0;void*data=NULL;char path[128];if(cgta2Ready)return CGTA2_Current(l);memset(&staged,0,sizeof(staged));
 for(i=0;i<C1AL14_COUNT;i++){snprintf(path,sizeof(path),"xanim/%s",c1al14Names[i]);data=CGTA2_Read(l,path,&n);if(!data||!C1AL14_Add(&staged.look,i,data,n))goto done;free(data);data=NULL;}
 for(g=0;g<2;g++)for(row=0;row<3;row++)for(y=0;y<7;y++){c1ar14Root_t*root=NULL;c1ab14Clip_t*c=NULL;static const char*rows[]={"15up","level","15down"};static const char*yaws[]={"45right","30right","15right","forward","15left","30left","45left"};snprintf(path,sizeof(path),"xanim/standMG42gunner_%s_%s_%s",g?"fire":"aim",yaws[y],rows[row]);data=CGTA2_Read(l,path,&n);if(!data||!C1AR14_Decode(data,n,&root)||!C1AB14_Decode(data,n,&c)||root->noteCount||root->flags!=(g?3u:2u)||root->fps!=30||root->intervals!=(g?5u:0u)||c->flags!=root->flags||c->fps!=root->fps||c->intervals!=root->intervals){C1AR14_Free(root);C1AB14_Free(c);goto done;}C1AR14_Free(root);free(data);data=NULL;staged.mg[g][row*7+y]=c;}
 if(!CGTA2_Current(l)||!C1AL14_Ready(&staged.look))goto done;cgta2Clips=staged;memset(&staged,0,sizeof(staged));cgta2Ready=1;ok=1;
 done:free(data);CGTA2_FreeClips(&staged);return ok;
}
static cgta2Resource_t*CGTA2_Resource(const cgta2Lease_t*l){cgta2Resource_t staged,*slot=NULL;cod1TurretActorResourceState_t def;c1at14Input_t inputs[8];void*models[8]={0},*parts[8]={0};char names[8][128],path[128];unsigned i;int n,ok=0;
 if(!C1TAW_Composition(l->config,&def)||!CGTA2_LoadClips(l))return NULL;for(i=0;i<CGTA_CACHE;i++){if(cgta2Resources[i].handle&&!memcmp(&cgta2Resources[i].definition,&def,sizeof(def)))return cgta2Resources+i;if(!cgta2Resources[i].resource&&!slot)slot=cgta2Resources+i;}if(!slot)return NULL;memset(&staged,0,sizeof(staged));memset(inputs,0,sizeof(inputs));staged.definition=def;
 for(i=0;i<(unsigned)def.partCount;i++){c1mrModelHeader_t model;snprintf(path,sizeof(path),"xmodel/%s",def.model[i]);models[i]=CGTA2_Read(l,path,&n);if(!models[i]||!C1MR_DecodeModel(models[i],n,&model)||!C1TAW_Name(model.lodName[0]))goto done;inputs[i].model=models[i];inputs[i].modelBytes=n;inputs[i].modelName=def.model[i];strcpy(names[i],model.lodName[0]);inputs[i].partsName=names[i];inputs[i].tag=def.tag[i];snprintf(path,sizeof(path),"xmodelparts/%s",names[i]);parts[i]=CGTA2_Read(l,path,&n);if(!parts[i])goto done;inputs[i].parts=parts[i];inputs[i].partsBytes=n;}
 if(!C1AD14_Create(inputs,def.partCount,&staged.resource)||!C1ALT14_Bank(staged.resource,(const c1ab14Clip_t*const(*)[21])cgta2Clips.mg,&cgta2Clips.look,&staged.bank)||!CGTA2_Current(l))goto done;
 staged.handle=trap_R_RegisterCod1TurretActor(&def);if(!staged.handle||!CGTA2_Current(l)||slot->resource)goto done;*slot=staged;staged.resource=NULL;ok=1;
 done:for(i=0;i<8;i++){free(models[i]);free(parts[i]);}C1AD14_Free(staged.resource);return ok?slot:NULL;
}
static void CG_BlendActorPose(cod1TurretActorRenderState_t*,const cod1TurretActorRenderState_t*,float);
static int CGTA2_SamplePose(const cgta2Lease_t *lease,cgta2Resource_t *r,cod1TurretActorRenderState_t *state){
 unsigned i,k;c1ast14Node_t nodes[98];c1ab14Accumulator_t accumulated[128];c1aw14Pose_t pose;
 if(!r||!CGTA2_Current(lease)||!C1ALT14_Nodes(lease->actor.cod1TurretActorGenericCurrent,lease->actor.cod1TurretActorPhases,nodes)||!C1AST14_Sample(&r->bank,nodes,98,accumulated,128)||!C1AD14_Build(r->resource,accumulated,r->bank.bones,&pose))return 0;
 memset(state,0,sizeof(*state));state->version=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION;state->size=sizeof(*state);state->hModel=r->handle;state->ownerNum=lease->actor.number;state->epochLo=lease->actor.cod1TurretEpochLo;state->epochHi=lease->actor.cod1TurretEpochHi;state->generation=lease->actor.cod1TurretActorGeneration;state->sampleTick=lease->actor.cod1TurretActorTick;state->boneCount=pose.bones;memcpy(state->origin,lease->actor.pos.trBase,12);memcpy(state->baseAngles,lease->actor.apos.trBase,12);
 for(i=0;i<pose.bones;i++){double norm=0;for(k=0;k<4;k++)norm+=(double)pose.q[i][k]*pose.q[i][k];if(!(norm>1e-15&&norm<1e15))return 0;norm=sqrt(norm);for(k=0;k<4;k++)state->q[i][k]=(float)(pose.q[i][k]/norm);memcpy(state->p[i],pose.matrix[i]+12,12);}if(!CGTA2_Current(lease))return 0;return 1;
}
static int CGTA2_Entity(const entityState_t *candidate,qboolean prepareOnly){cgta2Lease_t lease;cgta2Resource_t*r;const entityState_t*s=NULL,*gun=NULL;int j,actors=0,guns=0,ok=0;cod1TurretActorRenderState_t state;
 if(!candidate||!cg.snap||!cgs.cod1MapEntities||cgta2Busy)return 1;for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==candidate->number){s=cg.snap->entities+j;actors++;}if(actors!=1||!s||(s->eFlags&EF_NODRAW)||(CGTA2_Detached(s)?!C1UAW_Valid(s,cg.snap->serverTime):s->cod1TurretActorVersion!=COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION||!C1TAW_Valid(s,cg.snap->serverTime)))return 1;
 if(!CGTA2_Detached(s)){
 for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==s->cod1TurretActorGun){gun=cg.snap->entities+j;guns++;}if(guns!=1||!gun||(gun->eFlags&EF_NODRAW)||!C1TW_Valid(gun,cg.snap->serverTime)||gun->cod1TurretGeneration!=s->cod1TurretActorGunGeneration||gun->cod1TurretTick!=s->cod1TurretActorTick||gun->cod1TurretEpochLo!=s->cod1TurretEpochLo||gun->cod1TurretEpochHi!=s->cod1TurretEpochHi)return 1;
 }
 memset(&lease,0,sizeof(lease));lease.revision=cgta2Revision;lease.snapshot=cg.snap;lease.actor=*s;if(gun)lease.gun=*gun;{const char*text=CG_ConfigString(s->cod1TurretActorConfig);if(!text||strlen(text)>=sizeof(lease.config))return 1;strcpy(lease.config,text);}if(!CGTA2_Current(&lease))return 1;cgta2Busy=lease.revision;
 r=CGTA2_Resource(&lease);if(prepareOnly)goto done;
 if(!CGTA2_SamplePose(&lease,r,&state))goto done;
 if(cg.nextSnap && cg_entities[s->number].interpolate && cg.frameInterpolation>0 && cg.frameInterpolation<=1){
    cgta2Lease_t next=lease;const entityState_t *a=&cg_entities[s->number].nextState,*g=gun?&cg_entities[gun->number].nextState:NULL;
    if(a->number==s->number && a->cod1TurretActorVersion==s->cod1TurretActorVersion &&
       a->cod1TurretActorGeneration==s->cod1TurretActorGeneration && a->cod1TurretActorConfig==s->cod1TurretActorConfig &&
       a->cod1TurretEpochLo==s->cod1TurretEpochLo && a->cod1TurretEpochHi==s->cod1TurretEpochHi &&
       !(a->eFlags&EF_NODRAW) && !((a->eFlags^s->eFlags)&EF_TELEPORT_BIT) &&
       a->cod1TurretActorTick>s->cod1TurretActorTick &&
       (gun ? a->cod1TurretActorGun==gun->number && a->cod1TurretActorGunGeneration==s->cod1TurretActorGunGeneration &&
              g->number==gun->number && g->cod1TurretGeneration==gun->cod1TurretGeneration &&
              !(g->eFlags&EF_NODRAW) && a->cod1TurretActorTick==g->cod1TurretTick &&
              C1TAW_Valid(a,cg.nextSnap->serverTime) && C1TW_Valid(g,cg.nextSnap->serverTime)
            : CGTA2_Detached(a) && C1UAW_Valid(a,cg.nextSnap->serverTime))){
        cod1TurretActorRenderState_t pose;
        next.snapshot=cg.nextSnap;next.actor=*a;if(g)next.gun=*g;
        if(CGTA2_Current(&next) && CGTA2_SamplePose(&next,r,&pose) && pose.boneCount==state.boneCount){
            CG_BlendActorPose(&state,&pose,cg.frameInterpolation);
            if(cg_debugAnim.integer>2)CG_Printf("[Training mounted render] owner=%d/%u t=%d first=%d next=%d f=%.4f\n",
                state.ownerNum,state.generation,cg.time,s->cod1TurretActorTick,a->cod1TurretActorTick,cg.frameInterpolation);
        }
    }
 }
 if(!CGTA2_Current(&lease))goto done;
 ok=trap_R_AddCod1TurretActorToScene(&state)==1;
 if(ok && !gun && cg_debugAnim.integer>1){
  static int previousTick=-1;
  if(previousTick!=state.sampleTick){
   previousTick=state.sampleTick;
   CG_Printf("[dismount render] owner=%d generation=%u tick=%d bones=%d\n",
             state.ownerNum,state.generation,state.sampleTick,state.boneCount);
  }
 }
 done:if(cgta2Busy==lease.revision)cgta2Busy=0;(void)ok;return 1;
}

/* Prepare only resources of mounted actors that this initial snapshot already
 * owns. The normal scene pass must acquire its own lease to pose and draw. */
void CG_Cod1PrepareInitialMountedActors(const snapshot_t *snap) {
 int i;
 if(!snap || !cgs.cod1MapEntities || cgs.gametype!=GT_SINGLE_PLAYER ||
    (snap->snapFlags & SNAPFLAG_NOT_ACTIVE))return;
 for(i=0;cg.snap==snap && i<snap->numEntities;i++){
  const entityState_t *s=snap->entities+i;
  if(s->cod1TurretActorVersion==COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION &&
     s->number>=0 && s->number<MAX_GENTITIES)
   CGTA2_Entity(s,qtrue);
 }
}
