/* The two script_models keep their separate original XModels and XAnim clocks. */
typedef struct {
 qhandle_t handle;
 c1aw14Skeleton_t *skeleton;
 c1ab14Clip_t *clip;
 unsigned char map[128];
 int tag,reported,lastTick;
} cgTrainingRig_t;
static cgTrainingRig_t cgTrainingRigs[2];
static const char *const cgRigModels[2]={"training_jumprig_gear","training_jumprig_wire"};
static const char *const cgRigClips[2]={"training_gear_practicejump","training_wire_practicejump"};
static void CG_TrainingRigsReset(void){
 int i;for(i=0;i<2;i++){C1AW14_Free(cgTrainingRigs[i].skeleton);C1AB14_Free(cgTrainingRigs[i].clip);}
 memset(cgTrainingRigs,0,sizeof(cgTrainingRigs));
}
static int CG_TrainingRigLoad(int kind){
 cgTrainingRig_t r={0};cod1TurretActorResourceState_t def={0};char path[MAX_QPATH];
 xm14Model *model=malloc(sizeof(*model));int n,ok=0;void *data=NULL;unsigned i;
 if(!model)return 0;
 snprintf(path,sizeof(path),"xmodel/%s",cgRigModels[kind]);
 data=CGTA_Read(path,&n);if(!data||!XM14_Model(data,n,model)||!C1TAW_Name(model->lod[0]))goto done;
 free(data);snprintf(path,sizeof(path),"xmodelparts/%s",model->lod[0]);
 data=CGTA_Read(path,&n);if(!data||!C1AW14_Decode(data,n,&r.skeleton))goto done;
 free(data);snprintf(path,sizeof(path),"xanim/%s",cgRigClips[kind]);
 data=CGTA_Read(path,&n);
 if(!data||!C1AB14_Decode(data,n,&r.clip)||r.clip->fps!=30||r.clip->intervals!=175||
    !C1AW14_Map(r.skeleton,r.clip,r.map))goto done;
 for(i=0;i<r.skeleton->source.boneCount;i++)
  if(!Q_stricmp(r.skeleton->source.boneName[i],kind?"midbone":"tag_gear"))break;
 if(i==r.skeleton->source.boneCount)goto done;
 r.tag=i;def.version=COD1_TURRET_ACTOR_RESOURCE_VERSION;def.size=sizeof(def);
 def.partCount=1;strcpy(def.model[0],cgRigModels[kind]);
 r.handle=trap_R_RegisterCod1TurretActor(&def);if(!r.handle)goto done;
 cgTrainingRigs[kind]=r;ok=1;
 done:free(model);free(data);
 if(!ok){C1AB14_Free(r.clip);C1AW14_Free(r.skeleton);}
 return ok;
}
static int CG_TrainingRig(centity_t *cent){
 const entityState_t *s=&cent->currentState;int kind=s->effect1Time-0x4a5200,accepted;
 cgTrainingRig_t *r;float phase;unsigned i,k;const char *path;
 c1ab14Accumulator_t local[128]={0},mapped[128]={0};c1aw14Pose_t pose;
 cod1TurretActorRenderState_t state={0};
 if(kind<0||kind>1||!s->effect2Time||!s->effect3Time||s->time<=0||!cg.snap||
    s->time>cg.snap->serverTime||!cgs.cod1MapEntities||s->density!=COD1_SCRIPT_MODEL_NET_MARKER)return 0;
 path=CG_ConfigString(CS_MODELS+s->modelindex);
 if(strncmp(path,"xmodel/",7)||strcmp(path+7,cgRigModels[kind]))return 0;
 r=cgTrainingRigs+kind;
 if(!r->handle&&!CG_TrainingRigLoad(kind)){
  if(!r->reported){CG_Printf("[Training tower client] resource unavailable model=%s\n",path);r->reported=1;}
  return 0;
 }
 phase=(float)((double)(cg.time-s->time)*r->clip->fps/(1000.0*r->clip->intervals));
 if(phase<0)phase=0;if(phase>1)phase=1;
 if(!C1AB14_Accumulate(r->clip,phase,1,local,r->clip->bones))return 0;
 for(i=0;i<r->clip->bones;i++)mapped[r->map[i]]=local[i];
 if(!C1AW14_Build(r->skeleton,mapped,r->skeleton->source.boneCount,&pose))return 0;
 state.version=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION;state.size=sizeof(state);state.hModel=r->handle;
 state.ownerNum=s->number;state.generation=s->effect2Time;state.epochLo=s->effect3Time;
 state.sampleTick=cg.snap->serverTime;state.boneCount=pose.bones;
 VectorCopy(cent->lerpOrigin,state.origin);VectorCopy(cent->lerpAngles,state.baseAngles);
 for(i=0;i<pose.bones;i++){
  double len=0;for(k=0;k<4;k++)len+=(double)pose.q[i][k]*pose.q[i][k];
  if(!(len>1e-12&&len<1e15))return 0;
  len=sqrt(len);for(k=0;k<4;k++)state.q[i][k]=(float)(pose.q[i][k]/len);
  memcpy(state.p[i],pose.matrix[i]+12,12);
 }
 accepted=trap_R_AddCod1TurretActorToScene(&state);
 if(cg_debugAnim.integer&&(r->lastTick!=state.sampleTick)&&(state.sampleTick-s->time)%500==0){
  vec3_t axis[3],pos;AnglesToAxis(state.baseAngles,axis);VectorCopy(state.origin,pos);
  for(i=0;i<3;i++)VectorMA(pos,state.p[r->tag][i],axis[i],pos);
  CG_Printf("[Training tower client] owner=%d model=%d server=%d phase=%.5f tag=%.3f %.3f %.3f accepted=%d\n",
    s->number,kind,state.sampleTick,phase,pos[0],pos[1],pos[2],accepted);r->lastTick=state.sampleTick;
 }
 return accepted;
}
