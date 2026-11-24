/* Included only by bg_pmove.c and headless tests. */
static qboolean BG_Cod1Fits(pmove_t*m,float top){trace_t tr;vec3_t hi;VectorCopy(m->maxs,hi);hi[2]=top;m->trace(&tr,m->ps->origin,m->mins,hi,m->ps->origin,m->ps->clientNum,m->tracemask);return !tr.startsolid && !tr.allsolid;}
static qboolean BG_Cod1CheckStance(pmove_t*m){
 playerState_t*s=m->ps;int old,want;float feet,top,oldtop;static const float height[3]={70,50,24};static const int eyes[3]={60,40,11};
 if(!(m->cmd.wbuttons&WBUTTON_COD1_STANCE) && !(s->pm_flags&PMF_COD1_PRONE))return qfalse;
 if(s->pm_type!=PM_NORMAL || (s->eFlags&EF_MG42_ACTIVE)){s->pm_flags&=~PMF_COD1_PRONE;return qfalse;}
 VectorCopy(s->mins,m->mins);VectorCopy(s->maxs,m->maxs);feet=m->mins[2];
 /* Original CoD1 default horizontal bounds: -15 .. +15.
  * Keep the RTCW vertical origin convention (feet offset) unchanged. */
 m->mins[0]=m->mins[1]=-15.0f;
 m->maxs[0]=m->maxs[1]=15.0f;
 old=(s->pm_flags&PMF_COD1_PRONE)?2:((s->pm_flags&PMF_DUCKED)?1:0);
 want=(m->cmd.wbuttons&WBUTTON_COD1_PRONE)?2:(m->cmd.upmove<0?1:0);
 if((m->cmd.buttons&BUTTON_TALK) || (s->pm_flags&PMF_IGNORE_INPUT) || !(m->cmd.wbuttons&WBUTTON_COD1_STANCE))want=old;
 /* Apply the same ordinary permission arbitration in GAME and prediction.
  * The original reports an impossible three-way ban when movement consumes it,
  * not while sequential script setters are still running. */
 want=BG_Cod1PermittedStance((unsigned)s->stats[STAT_COD1_MODE]>>COD1_STANCE_PERMISSION_SHIFT,old,want);
 if(want<0){Com_Error(ERR_DROP,"All stances disallowed for player");return qfalse;}
 /* Swimming/ladder prone requires its own original transition behavior. */
 if(want==2 && (m->waterlevel>1 || (s->pm_flags&PMF_LADDER)))want=1;
 oldtop=feet+height[old];top=feet+height[want];
 if(top>oldtop && !BG_Cod1Fits(m,top)){
  if(old==2 && want==0 && BG_Cod1Fits(m,feet+height[1]))want=1;
  else want=old;
 }
 s->pm_flags&=~(PMF_COD1_PRONE|PMF_DUCKED);
 if(want)s->pm_flags|=PMF_DUCKED;
 if(want==2)s->pm_flags|=PMF_COD1_PRONE;
 m->maxs[2]=feet+height[want];s->viewheight=(int)feet+eyes[want];
 /* Cannot jump through a ceiling when a requested rise was rejected. */
 if(want && m->cmd.upmove>0)m->cmd.upmove=0;
 return qtrue;
}
