/* R195 shared Pmove integration. Included by bg_pmove.c and its focused test. */
static qboolean PM_Cod1Footsteps(void) {
 cod1GaitInput_t in;cod1GaitOutput_t out;int old,bits,parameter;
 /* PATCH54: ladder gait is native too. The ordinary movement-scale gate
  * excludes ladders because they have their own command scale. */
 if(!BG_Cod1PlayerMovementActive(pm) || !pm->ps->cod1EyeTarget ||
    (pm->ps->eFlags&EF_MG42_ACTIVE) || pm->ps->persistant[PERS_HWEAPON_USE])return qfalse;
 memset(&in,0,sizeof(in));old=pm->ps->bobCycle;in.cycle=old;
 in.msec=pml.msec;in.now=pm->cmd.serverTime;in.base_speed=190;
 in.flags=(pm->ps->pm_flags&PMF_COD1_PRONE)?1:(pm->ps->pm_flags&PMF_DUCKED)?2:0;
 if(pm->ps->pm_flags&PMF_LADDER)in.flags|=0x10;
 in.elapsed=pm->cmd.serverTime-pm->ps->jumpTime;
 if(pm->ps->pm_flags&PMF_BACKWARDS_RUN)in.flags|=0x40;
 /* Use the mode which actually scaled this step, not the ADS direction
  * changed by PM_Weapon between movement and this call. */
 if(pml.cod1AdsMove)in.flags|=0x80;
 in.ground=pm->ps->groundEntityNum==ENTITYNUM_NONE?2047:2046;
 in.forward=pm->cmd.forwardmove;in.right=pm->cmd.rightmove;
 in.vx=pm->ps->velocity[0];in.vy=pm->ps->velocity[1];in.vz=pm->ps->velocity[2];
 in.target=pm->ps->cod1EyeTarget;in.transition_start=pm->ps->cod1EyeStart;
 in.transition_down=pm->ps->cod1EyeDown;
 COD1_Gait(&in,&out);pm->ps->bobCycle=out.cycle;pm->xyspeed=out.xyspeed;
 /* Retain the existing third-person animation bridge. Original rung sound
  * dispatch still needs its material bridge; do not emit floor sounds here. */
 if((pm->ps->pm_flags&PMF_LADDER) && pm->ps->groundEntityNum==ENTITYNUM_NONE)
  BG_AnimScriptAnimation(pm->ps,pm->ps->aiState,
   pm->ps->velocity[2]>=0?ANIM_MT_CLIMBUP:ANIM_MT_CLIMBDOWN,qtrue);
 bits=CI_TraceBits(&pml.groundTrace);
 parameter=C1F_Parameter(old,out.cycle,out.event_call,
  pm->ps->groundEntityNum!=ENTITYNUM_NONE,(pm->ps->pm_flags&PMF_LADDER)!=0,
  pm->noFootsteps,(pml.groundTrace.surfaceFlags&SURF_NOSTEPS)!=0,
  (bits&CI_KNOWN)!=0,bits&31,pm->waterlevel,CGait_Stance(&in)==1,
  CGait_Stance(&in)==2||(pm->cmd.buttons&BUTTON_WALKING)||pml.cod1AdsMove);
 if(parameter)BG_AddPredictableEventToPlayerstate(EV_COD1_FOOTSTEP,parameter,pm->ps);
 return qtrue;
}
