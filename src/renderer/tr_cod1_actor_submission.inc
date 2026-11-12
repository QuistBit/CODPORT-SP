#include "cod1_foley_transition.h"
static ca14ActorClip cod1FoleyGestureClip,cod1MoodyWalkClip,cod1MoodyIdleClip;
static void R_Cod1FoleyGestureReset(void){CA14_ActorClipFree(&cod1FoleyGestureClip);CA14_ActorClipFree(&cod1MoodyWalkClip);CA14_ActorClipFree(&cod1MoodyIdleClip);}

static int R_Cod1ActorRequireClip(ca14ActorClip *clip,const char *path){
    void *data=NULL;int bytes,got=-1;ca14ActorClip loaded;
    if(clip->boneData)return 1;
    memset(&loaded,0,sizeof(loaded));bytes=ri.FS_ReadFile(path,NULL);
    if(bytes>0&&(size_t)bytes<=CA14_ACTOR_CLIP_LIMIT)got=ri.FS_ReadFile(path,&data);
    if(got!=bytes||!data||!CA14_ActorClipLoad(data,(size_t)got,&loaded)||
       ((loaded.rootRotationKeys||loaded.rootTranslationKeys)&&strcmp(path,"xanim/leaderwalk_cocky_idle"))){
        CA14_ActorClipFree(&loaded);if(data)ri.FS_FreeFile(data);return 0;
    }
    ri.FS_FreeFile(data);*clip=loaded;return 1;
}

/* R196 exact typed submission entry points; private renderer data stays here. */
int RE_AddCod1ActorPilot(const refEntity_t *ent, const cv14Pose *pose,
                         cod1ActorPilotTags_t *tags) {
	model_t *model;
	backEndData_t *frame;
	cod1ActorSubmission_t *snapshot;
	trRefEntity_t *submitted;
	size_t arenaBefore;
	if (tags) memset(tags, 0, sizeof(*tags));
	if (!tr.registered || !ent || !pose || !tags || ent->reType != RT_MODEL ||
	    ent->hModel < 1 || ent->hModel >= tr.numModels ||
	    r_numentities >= ENTITYNUM_WORLD) return 0;
	model = R_GetModelByHandle(ent->hModel);
	if (model->type != MOD_COD1_ACTOR_XMODEL || !model->cod1Actor) return 0;
	frame = backEndData[tr.smpFrame];
	arenaBefore = frame->cod1ActorArena.used;
	snapshot = R_Cod1ActorCapturePose(model->cod1Actor,&frame->cod1ActorArena,pose);
	if (!snapshot) return 0;
	if (model->cod1Actor->profile==0 &&
        (R_Cod1ActorFindTag(snapshot,COD1_ACTOR_BODY,
	                       "tag_weapon_right",&tags->weaponRight)<0 ||
	    R_Cod1ActorFindTag(snapshot,COD1_ACTOR_WEAPON,
	                       "tag_flash",&tags->flash)<0)) {
		/* The loader validated both anchors; a missing posed tag is a
		 * corrupted pose and must not advance the entity queue. */
		frame->cod1ActorArena.used = arenaBefore;
		memset(tags,0,sizeof(*tags));
		return 0;
	}
	submitted = &frame->entities[r_numentities];
	submitted->e = *ent;
	submitted->lightingCalculated = qfalse;
	submitted->cod1ModelLighting.valid = 0;
	submitted->cod1NativeSubmission = NULL;
	submitted->cod1ActorSubmission = snapshot;
    submitted->cod1TurretSubmission=NULL;
	r_numentities++;
	return 1;
}

/* The cgame owns actor placement and playback time. Decode the allowlisted
 * clip here so no renderer-private skeleton layout crosses the cgame syscall.
 * Each submission captures an independent immutable per-frame pose. */
int RE_AddCod1ActorAnimated(const refEntity_t *ent,
                            const cod1ActorAnimState_t *state) {
	cv14Pose pose;
	cod1ActorPilotTags_t tags;
	model_t *model;ca14ActorClip *clip;const char *clipPath;
	if (!tr.registered || !ent || !state ||
	    state->version != COD1_ACTOR_STATE_VERSION ||
	    state->size != (int)sizeof(*state) ||
	    (state->clip != COD1_ACTOR_CLIP_IDLE && state->clip != COD1_ACTOR_CLIP_FOLEY_BREATH && state->clip != COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE && state->clip != COD1_ACTOR_CLIP_MOODY_IDLE && state->clip != COD1_ACTOR_CLIP_MOODY_WALK) || state->elapsedMs < 0 ||
	    ent->reType != RT_MODEL || ent->hModel < 1 ||
	    ent->hModel >= tr.numModels || r_numentities >= ENTITYNUM_WORLD)
		return 0;
	model=R_GetModelByHandle(ent->hModel);
	if (model->type != MOD_COD1_ACTOR_XMODEL || !model->cod1Actor) return 0;
    if(model->cod1Actor->profile==COD1_ACTOR_MOODY_PROFILE){
        if(state->clip!=COD1_ACTOR_CLIP_MOODY_IDLE&&state->clip!=COD1_ACTOR_CLIP_MOODY_WALK)return 0;
        clip=state->clip==COD1_ACTOR_CLIP_MOODY_WALK?&cod1MoodyWalkClip:&cod1MoodyIdleClip;
        clipPath=state->clip==COD1_ACTOR_CLIP_MOODY_WALK?"xanim/leaderwalk_cocky_idle":"xanim/training_foley_breathing_loop";
    }else if(model->cod1Actor->profile==COD1_ACTOR_FOLEY_PROFILE){
        if(state->clip!=COD1_ACTOR_CLIP_FOLEY_BREATH&&state->clip!=COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE)return 0;
        clip=&cod1FoleyBridgeClip;clipPath="xanim/training_foley_breathing_loop";
    }else{
        if(state->clip!=COD1_ACTOR_CLIP_IDLE)return 0;
        clip=&cod1ActorBridgeClip;clipPath="xanim/stand_aim_straight_loop1";
    }
    if(!R_Cod1ActorRequireClip(clip,clipPath))return 0;
    if(state->clip==COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE){
        if(!R_Cod1ActorRequireClip(&cod1FoleyGestureClip,"xanim/dawn_foley_waving_followme")||
           !CF197_Sample(clip,&cod1FoleyGestureClip,state->elapsedMs,&pose))return 0;
    }else if(!CA14_ActorClipSample(clip,state->elapsedMs,&pose))return 0;
	return RE_AddCod1ActorPilot(ent,&pose,&tags);
}


int R_Cod1MoodyPrecache(void){
 return R_Cod1ActorRequireClip(&cod1MoodyIdleClip,"xanim/training_foley_breathing_loop")&&
        R_Cod1ActorRequireClip(&cod1MoodyWalkClip,"xanim/leaderwalk_cocky_idle");
}
