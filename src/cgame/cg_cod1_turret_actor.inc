#include "../game/source_pose_wire.h"
#include "../game/scripted_wire.h"
/* Current snapshot -> owned original resources -> ordinary nested current
 * tree -> native modelspace body -> typed renderer capture. No interpolation. */
#include "../game/bg_cod1_turret_actor_wire.h"
#include "../game/bg_cod1_turret_wire.h"
#include "../game/bg_cod1_face_unmounted_actor_wire.h"
#include "cod1_turret_actor_pose14.h"
#include "../renderer/cod1_xmodel14.h"
#define CGTA_CACHE 16
typedef struct {cod1TurretActorResourceState_t definition;int handle;c1aw14Skeleton_t*body;cgta14Bank_t bank;} cgtaCache_t;
static cgtaCache_t cgtaCache[CGTA_CACHE];static c1ab14Clip_t*cgtaClips[2][21];static int cgtaBankState;
static void CGFA4_Reset(void);static int CGFA4_Entity(centity_t*);
static void CGUA3_Reset(void);static int CGUA3_Entity(centity_t*);
static void CGTA2_Reset(void);static int CGTA2_Entity(const entityState_t*,qboolean);
static void CG_TrainingRigsReset(void);
void CG_Cod1TurretActorReset(void){unsigned i,g,j;CG_TrainingRigsReset();CGFA4_Reset();CGUA3_Reset();CGTA2_Reset();for(i=0;i<CGTA_CACHE;i++){C1AW14_Free(cgtaCache[i].body);memset(cgtaCache+i,0,sizeof(cgtaCache[i]));}for(g=0;g<2;g++)for(j=0;j<21;j++){C1AB14_Free(cgtaClips[g][j]);cgtaClips[g][j]=NULL;}cgtaBankState=0;}
static void*CGTA_Read(const char*path,int*size){fileHandle_t f=0;void*p;int n=trap_FS_FOpenFile(path,&f,FS_READ);if(n<1||n>16*1024*1024||!f){if(f)trap_FS_FCloseFile(f);return NULL;}p=malloc(n);if(!p){trap_FS_FCloseFile(f);return NULL;}trap_FS_Read(p,n,f);trap_FS_FCloseFile(f);*size=n;return p;}
static int CGTA_LoadClips(void){static const char*rows[]={"15up","level","15down"};static const char*yaws[]={"45right","30right","15right","forward","15left","30left","45left"};unsigned g,r,y;if(cgtaBankState)return cgtaBankState>0;cgtaBankState=-1;
 for(g=0;g<2;g++)for(r=0;r<3;r++)for(y=0;y<7;y++){char path[128];int n;void*data;Com_sprintf(path,sizeof(path),"xanim/standMG42gunner_%s_%s_%s",g?"fire":"aim",yaws[y],rows[r]);data=CGTA_Read(path,&n);if(!data||!C1AB14_Decode(data,n,&cgtaClips[g][r*7+y])){free(data);goto bad;}free(data);if(cgtaClips[g][r*7+y]->flags!=(unsigned)(g?3:2)||cgtaClips[g][r*7+y]->fps!=30||cgtaClips[g][r*7+y]->intervals!=(unsigned)(g?5:0))goto bad;}
 cgtaBankState=1;return 1;
 bad:for(g=0;g<2;g++)for(r=0;r<21;r++){C1AB14_Free(cgtaClips[g][r]);cgtaClips[g][r]=NULL;}return 0;
}
static cgtaCache_t*CGTA_Resource(const char*config){cod1TurretActorResourceState_t def;cgtaCache_t staged,*slot=NULL;unsigned i,g,j;char path[128];void*data=NULL;int n;xm14Model*model=NULL;
 if(!C1TAW_Composition(config,&def)||!CGTA_LoadClips())return NULL;
 for(i=0;i<CGTA_CACHE;i++){if(cgtaCache[i].handle&&!memcmp(&cgtaCache[i].definition,&def,sizeof(def)))return cgtaCache+i;if(!cgtaCache[i].handle&&!slot)slot=cgtaCache+i;}if(!slot)return NULL;memset(&staged,0,sizeof(staged));staged.definition=def;model=malloc(sizeof(*model));if(!model)return NULL;
 Com_sprintf(path,sizeof(path),"xmodel/%s",def.model[0]);data=CGTA_Read(path,&n);if(!data||!XM14_Model(data,n,model))goto bad;free(data);data=NULL;
 if(!C1TAW_Name(model->lod[0]))goto bad;Com_sprintf(path,sizeof(path),"xmodelparts/%s",model->lod[0]);data=CGTA_Read(path,&n);if(!data||!C1AW14_Decode(data,n,&staged.body))goto bad;free(data);data=NULL;staged.bank.bones=staged.body->source.boneCount;
 for(g=0;g<2;g++)for(j=0;j<21;j++){staged.bank.clip[g][j]=cgtaClips[g][j];if(!C1AW14_Map(staged.body,cgtaClips[g][j],staged.bank.map[g][j]))goto bad;}
 staged.handle=trap_R_RegisterCod1TurretActor(&def);if(!staged.handle)goto bad;free(model);*slot=staged;return slot;
 bad:free(data);free(model);C1AW14_Free(staged.body);return NULL;
}
static int CG_Cod1TurretActorEntity(centity_t*cent){const entityState_t*s=NULL,*gun=NULL;cgtaCache_t*r;c1ab14Accumulator_t accumulated[128];c1aw14Pose_t pose;cod1TurretActorRenderState_t state;unsigned i,k;int j;
 if(!cent||!cent->currentState.cod1TurretActorVersion)return 0;
 if(CGTA2_Detached(&cent->currentState) || cent->currentState.cod1TurretActorVersion==COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION)return CGTA2_Entity(&cent->currentState,qfalse);
 if(cent->currentState.cod1TurretActorVersion==3)return CGUA3_Entity(cent);
 if(cent->currentState.cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION || cent->currentState.cod1TurretActorVersion==COD1_FACE_UNMOUNTED_ACTOR_WIRE_VERSION || cent->currentState.cod1TurretActorVersion==COD1_SCRIPTED_ACTOR_WIRE_VERSION)return CGFA4_Entity(cent);
 if(!cgs.cod1MapEntities||!cg.snap)return 1;
 for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==cent->currentState.number){s=cg.snap->entities+j;break;}
 if(!s||(s->eFlags&EF_NODRAW)||!C1TAW_Valid(s,cg.snap->serverTime))return 1;
 for(j=0;j<cg.snap->numEntities;j++)if(cg.snap->entities[j].number==s->cod1TurretActorGun){gun=cg.snap->entities+j;break;}
 if(!gun||(gun->eFlags&EF_NODRAW)||!C1TW_Valid(gun,cg.snap->serverTime)||strcmp(CG_ConfigString(CS_MODELS+gun->modelindex),COD1_TURRET_MODEL_ALIAS)||!cgs.gameModels[gun->modelindex]||gun->cod1TurretGeneration!=s->cod1TurretActorGunGeneration||gun->cod1TurretTick!=s->cod1TurretActorTick||gun->cod1TurretEpochLo!=s->cod1TurretEpochLo||gun->cod1TurretEpochHi!=s->cod1TurretEpochHi)return 1;
 r=CGTA_Resource(CG_ConfigString(s->cod1TurretActorConfig));if(!r||!CGTA14_Sample(&r->bank,s->cod1TurretActorCurrent,s->cod1TurretActorPhases,accumulated,128)||!C1AW14_Build(r->body,accumulated,r->bank.bones,&pose))return 1;
 memset(&state,0,sizeof(state));state.version=COD1_TURRET_ACTOR_RENDER_VERSION;state.size=sizeof(state);state.hModel=r->handle;state.ownerNum=s->number;state.epochLo=s->cod1TurretEpochLo;state.epochHi=s->cod1TurretEpochHi;state.generation=s->cod1TurretActorGeneration;state.sampleTick=s->cod1TurretActorTick;state.boneCount=pose.bones;VectorCopy(s->pos.trBase,state.origin);VectorCopy(s->apos.trBase,state.baseAngles);
 for(i=0;i<pose.bones;i++){double norm=0;for(k=0;k<4;k++)norm+=(double)pose.q[i][k]*pose.q[i][k];if(!(norm>1e-15&&norm<1e15))return 1;norm=sqrt(norm);for(k=0;k<4;k++)state.q[i][k]=(float)(pose.q[i][k]/norm);memcpy(state.p[i],pose.matrix[i]+12,12);}
 trap_R_AddCod1TurretActorToScene(&state);return 1;
}
#include "cg_cod1_turret_actor_look.inc"

#include "cg_cod1_unmounted_actor.inc"

#include "cg_cod1_face_unmounted_actor.inc"

#include "cg_cod1_training_rig.inc"
