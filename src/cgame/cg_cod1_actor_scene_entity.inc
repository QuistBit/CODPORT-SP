/* R196 exact CG_General bridge for allowlisted server-owned visual actors. */
static qboolean CG_Cod1ActorSceneEntity(centity_t *cent,refEntity_t *ent) {
 const entityState_t *s1=&cent->currentState;
 if(s1->modelindex<=0||s1->modelindex>=MAX_MODELS)return qfalse;
	if ( s1->eType == ET_GENERAL && s1->density == COD1_ACTOR_NET_MARKER &&
	     (!Q_stricmp( CG_ConfigString( CS_MODELS + s1->modelindex ), COD1_ACTOR_MODEL_ALIAS ) ||
          !Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_FOLEY_ALIAS) ||
          !Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_MOODY_ALIAS)) ) {
		cod1ActorAnimState_t state;
		vec3_t mins, maxs;
		unsigned int elapsed;
		if ( !ent->hModel ) return qtrue;
		trap_R_ModelBounds( ent->hModel, mins, maxs );
		if ( !( mins[2] >= -512.f && mins[2] < maxs[2] && maxs[2] <= 512.f ) ) return qtrue;
		ent->reType = RT_MODEL;
        /* CG_General uses frame/time as generic model fade metadata. Here
         * those fields select the walk clip and its start time. */
        if(!Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_MOODY_ALIAS)){
            ent->fadeStartTime=ent->fadeEndTime=0;
        }
		/* Moody controller publishes foot origin; bind-box correction belongs only to the retained debug scenes. */
        if(Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_MOODY_ALIAS))ent->origin[2] -= mins[2] - 1.f;
		VectorCopy( ent->origin, ent->oldorigin );
		AnglesToAxis( cent->lerpAngles, ent->axis );
		state.version = COD1_ACTOR_STATE_VERSION;
		state.size = sizeof( state );
		state.clip = !Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_MOODY_ALIAS)?(s1->frame==COD1_ACTOR_MOODY_WALK_FRAME?COD1_ACTOR_CLIP_MOODY_WALK:COD1_ACTOR_CLIP_MOODY_IDLE):!Q_stricmp(CG_ConfigString(CS_MODELS+s1->modelindex),COD1_ACTOR_FOLEY_ALIAS)?(s1->frame==COD1_ACTOR_FOLEY_GESTURE_FRAME?COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE:COD1_ACTOR_CLIP_FOLEY_BREATH):COD1_ACTOR_CLIP_IDLE;
		elapsed = cg.time < s1->time ? 0U : (unsigned int)cg.time - (unsigned int)s1->time;
		state.elapsedMs = elapsed > 2147483647U ? 0 : (int)elapsed;
		if(!trap_R_AddCod1ActorAnimated( ent, &state )){
            static int warnedFoley; if((state.clip==COD1_ACTOR_CLIP_FOLEY_BREATH||state.clip==COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE)&&!warnedFoley){warnedFoley=1;CG_Printf("[R197] FOLEY_SUBMISSION_REFUSED\n");}
        }else if((state.clip==COD1_ACTOR_CLIP_FOLEY_BREATH||state.clip==COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE)){
            static int reported; if(!reported){reported=1;CG_Printf("[R197] FOLEY_FRAME_ACCEPTED model=foley_training clip=%d (not screen acceptance)\n",state.clip);}
        }
		cent->refEnt = *ent;
		return qtrue;
	}

 return qfalse;
}
