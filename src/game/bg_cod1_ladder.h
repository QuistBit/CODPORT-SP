/* PATCH51: CoD1 SP cgame 0x30007090 / 0x300072a0.
 * Included by bg_pmove.c after PM_AirMove. Keep the normal in playerState:
 * a global normal is neither prediction-safe nor shared-player-safe.
 * The original origin is at the feet; RTCW's origin is 24 units higher.
 */
static qboolean PM_Cod1LadderActive(void) {
    return (pm->cmd.wbuttons&WBUTTON_COD1_STANCE) && !pm->ps->aiChar;
}

static void PM_Cod1CheckLadder(void) {
    playerState_t *ps=pm->ps;
    vec3_t direction,end,mins,maxs;
    trace_t tr;
    int was=!!(ps->pm_flags&PMF_LADDER);
    float distance=pml.walking?8.0f:30.0f;
    /* Native timer early return retains the flag; pml is rebuilt each tick. */
    pml.ladder=was;
    if(ps->pm_time)return;
    if(was && ps->groundEntityNum==ENTITYNUM_NONE)
        VectorNegate(ps->cod1LadderNormal,direction);
    else {
        VectorSet(direction,pml.forward[0],pml.forward[1],0);
        VectorNormalize(direction);
    }
    pml.ladder=qfalse;ps->pm_flags&=~PMF_LADDER;
    if(ps->pm_type!=PM_NORMAL || ps->stats[STAT_HEALTH]<=0) {
        ps->groundEntityNum=ENTITYNUM_NONE;pml.walking=pml.groundPlane=qfalse;return;
    }
    /* Same effective prone classification used by native 0x30002c10. */
    if(BG_Cod1LeanLimit(ps)==.25f || pm->cmd.serverTime-ps->jumpTime<300)return;
    VectorCopy(pm->mins,mins);VectorCopy(pm->maxs,maxs);
    mins[0]+=6;mins[1]+=6;maxs[0]-=6;maxs[1]-=6;
    mins[2]=pm->mins[2]+8;
    if(maxs[2]<mins[2])maxs[2]=mins[2];
    VectorMA(ps->origin,distance,direction,end);
    pm->trace(&tr,ps->origin,mins,maxs,end,ps->clientNum,pm->tracemask);
    if(tr.fraction>=1 || !(tr.surfaceFlags&SURF_LADDER))return;
    if(pml.walking && pm->cmd.forwardmove<=0)return;
    VectorCopy(tr.plane.normal,ps->cod1LadderNormal);
    ps->pm_flags|=PMF_LADDER;pml.ladder=qtrue;
}

/* Native 0x30002e50: unweighted three-axis scale. Stock run multiplier is 1;
 * ADS request is disabled on ladders. RTCW sprint/run factors do not apply. */
static float PM_Cod1LadderScale(void) {
    int f=pm->cmd.forwardmove,r=pm->cmd.rightmove,u=pm->cmd.upmove;
    int largest=abs(f);
    if(abs(r)>largest)largest=abs(r);if(abs(u)>largest)largest=abs(u);
    if(!largest)return 0;
    return pm->ps->speed*(float)largest/(127.0f*(float)sqrt((float)(f*f+r*r+u*u)));
}

static void PM_Cod1LadderVelocity(void) {
    playerState_t *ps=pm->ps;
    vec3_t right,wish,dir;
    float upscale=(pml.forward[2]+.25f)*2.5f,scale,speed,add,accel,d,drop,n2;
    int i;
    if(upscale>1)upscale=1;else if(upscale< -1)upscale=-1;
    pml.forward[2]=0;VectorNormalize(pml.forward);
    pml.right[2]=0;VectorNormalize2(pml.right,right);
    /* Original projection divides by normal length squared twice. Normals
     * from collision are unit vectors; retain the operation for parity. */
    n2=DotProduct(ps->cod1LadderNormal,ps->cod1LadderNormal);
    d=n2?DotProduct(right,ps->cod1LadderNormal)/n2/n2:0;
    VectorMA(right,-d,ps->cod1LadderNormal,pml.right);
    scale=PM_Cod1LadderScale();VectorClear(wish);
    wish[2]=pm->cmd.forwardmove*.5f*scale*upscale;
    if(pm->cmd.rightmove)VectorMA(wish,pm->cmd.rightmove*scale*.2f,pml.right,wish);
    speed=VectorNormalize2(wish,dir);
    add=speed-DotProduct(ps->velocity,dir);
    if(add>0) {
        /* Native acceleration has a 100-unit lower speed bound. */
        accel=(speed<100?100:speed)*pml.frametime*9;
        if(accel>add)accel=add;
        if(ps->groundEntityNum!=ENTITYNUM_NONE && ps->friction>0)accel/=ps->friction;
        if(accel>add)accel=add;
        VectorMA(ps->velocity,accel,dir,ps->velocity);
    }
    if(!pm->cmd.forwardmove) {
        drop=ps->gravity*pml.frametime;
        if(ps->velocity[2]>0) {ps->velocity[2]-=drop;if(ps->velocity[2]<0)ps->velocity[2]=0;}
        else {ps->velocity[2]+=drop;if(ps->velocity[2]>0)ps->velocity[2]=0;}
    }
    if(!pm->cmd.rightmove) {
        VectorSet(right,pml.right[0],pml.right[1],0);VectorNormalize(right);
        d=ps->velocity[0]*right[0]+ps->velocity[1]*right[1];
        if(d) {
            for(i=0;i<2;i++)ps->velocity[i]-=d*right[i];
            drop=16*pml.frametime*d;
            if(fabs(d)>fabs(drop)) {
                if(fabs(drop)<1)drop=drop<0?-1:1;
                for(i=0;i<2;i++)ps->velocity[i]+=(d-drop)*right[i];
            }
        }
    }
    if(!pml.walking) {
        d=ps->velocity[0]*ps->cod1LadderNormal[0]+ps->velocity[1]*ps->cod1LadderNormal[1];
        for(i=0;i<2;i++)ps->velocity[i]+=( (wish[2]>0?-500.0f:-250.0f)-d)*ps->cod1LadderNormal[i];
    }
}

static void PM_Cod1LadderMove(void) {
    if(PM_Cod1CheckJump()) {
        PM_AirMove();pm->ps->jumpTime=pm->cmd.serverTime;return;
    }
    PM_Cod1LadderVelocity();
    PM_StepSlideMove(qfalse);
    /* RTCW movementDir is an animation index, not CoD's signed leg yaw. */
    pm->ps->movementDir=0;
}
