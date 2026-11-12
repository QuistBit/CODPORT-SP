#ifndef COD1_TRAINING_RUN_ENTRY_H
#define COD1_TRAINING_RUN_ENTRY_H
#include <math.h>

/* animscripts/run::UpdateRunWeights and walk::Shuffle on leaving a ladder.
 * The four directional leaves share a phase. The outgoing shuffle retains
 * its own phase until its 300 ms fade ends. All times below are seconds. */
enum { C1_RUN_RIGHT, C1_RUN_LEFT, C1_RUN_BACK, C1_RUN_FORWARD, C1_SHUFFLE_A, C1_SHUFFLE_B };
#define C1_ENTRY_CLIPS 6
#define C1_ENTRY_FROM_LANDING (-1)
#define C1_ENTRY_MAX_INTERVALS 25
typedef struct {
    float current[4], target[4], remaining[4];
    float phase, shufflePhase, runWeight, shuffleWeight, parentRate;
    int elapsedMs, nextReviewMs, shuffle;
} c1CourseEntry_t;
typedef struct {
    float position[C1_ENTRY_CLIPS][C1_ENTRY_MAX_INTERVALS+1][3];
} c1CourseEntryRoots_t;
static const int c1EntryIntervals[C1_ENTRY_CLIPS]={12,13,14,20,25,25};

static void C1EntryReview(c1CourseEntry_t *state,float relativeYaw){
    int degrees=(int)(remainderf(relativeYaw,360.0f)+180.0f)%360;
    float radians=(degrees>180?degrees-360:degrees)*(3.14159265358979323846f/180.0f);
    float s=sinf(radians),c=cosf(radians);
    float target[4]={fmaxf(0,s),fmaxf(0,-s),fmaxf(0,c),fmaxf(0,-c)};
    int i;
    for(i=0;i<4;i++){
        if(state->target[i]==target[i])continue;
        state->remaining[i]=.2f*fabsf(target[i]-state->current[i]);
        state->target[i]=target[i];
        if(state->current[i]==0 && target[i]>0)state->current[i]=.001f*target[i];
    }
}

static void C1EntryDelta(const c1CourseEntryRoots_t *roots,int clip,float before,float after,float out[3]){
    const int intervals=c1EntryIntervals[clip];
    const float (*p)[3]=roots->position[clip];
    float a=before*intervals,b=after*intervals;
    int i=(int)a,j=(int)b,k;
    for(k=0;k<3;k++){
        float start=p[i][k]+(a-i)*(p[i<intervals?i+1:i][k]-p[i][k]);
        float end=p[j][k]+(b-j)*(p[j<intervals?j+1:j][k]-p[j][k]);
        out[k]=end-start;
        if(after<before)out[k]+=p[intervals][k]-p[0][k];
    }
}

/* Advance a prospective step. The movement controller commits this
 * state only after the owner and both physical sweeps remain current. */
static int C1EntryStep(c1CourseEntry_t *state,const c1CourseEntryRoots_t *roots,
                       float relativeYaw,int dt,float *distance){
    float sum=0,freq=0,seconds=dt*.001f,age=state->elapsedMs*.001f;
    float mixed[3]={0},delta[3],prev=state->phase,phase,total;
    int i,k;
    if((state->shuffle!=C1_SHUFFLE_A && state->shuffle!=C1_SHUFFLE_B && state->shuffle!=C1_ENTRY_FROM_LANDING) ||
       dt<1 || dt>100 || !isfinite(relativeYaw))return 0;
    /* BlendIntoStandRun selects precombatrun1 with setanimknob before
     * UpdateRunWeights writes the four quadrants. Its forward parent
     * already has a live .001 weight even when the new goal is small. */
    if(state->shuffle==C1_ENTRY_FROM_LANDING&&!state->elapsedMs)
        state->current[C1_RUN_FORWARD]=.001f;
    if(state->elapsedMs>=state->nextReviewMs){
        C1EntryReview(state,relativeYaw);
        state->nextReviewMs=state->elapsedMs+200;
    }
    for(i=0;i<4;i++){
        float rem=state->remaining[i];
        if(state->elapsedMs){
            state->current[i]=rem<=seconds?state->target[i]:
                state->current[i]+(state->target[i]-state->current[i])*(seconds/rem);
            state->remaining[i]=fmaxf(0,rem-seconds);
        }
        sum+=state->current[i];
        freq+=state->current[i]*30.0f/c1EntryIntervals[i];
    }
    if(!(sum>0))return 0;
    /* run.gsc sets the combatrun parent to this actor's own rate after
     * BlendIntoStandRun's one-second transition. Children retain rate 1. */
    if(state->shuffle==C1_ENTRY_FROM_LANDING&&state->elapsedMs>=1000){
        if(!(state->parentRate>=.8f&&state->parentRate<=1.2f))return 0;
        freq*=state->parentRate;
    }
    phase=prev+(freq/sum)*seconds;phase-=floorf(phase);
    for(i=0;i<4;i++)if(state->current[i]>0){
        C1EntryDelta(roots,i,prev,phase,delta);
        for(k=0;k<3;k++)mixed[k]+=state->current[i]*delta[k]/sum;
    }
    state->phase=phase;
    state->runWeight=fminf(1,.001f+age);
    state->shuffleWeight=fmaxf(0,1-age/.3f);
    total=state->runWeight+state->shuffleWeight;
    if(state->shuffle==C1_ENTRY_FROM_LANDING){
        /* The outgoing jump leaf is clamped to its final frame. Its local
         * root displacement is zero during utility::initAnimTree's fade. */
        for(k=0;k<3;k++)mixed[k]*=state->runWeight/total;
    }else if(state->shuffleWeight>0){
        float before=(age+.05f)*30.0f/25;
        state->shufflePhase=before+seconds*30.0f/25;
        C1EntryDelta(roots,state->shuffle,before,state->shufflePhase,delta);
        for(k=0;k<3;k++)mixed[k]=(state->runWeight*mixed[k]+state->shuffleWeight*delta[k])/total;
    }else state->shufflePhase=0;
    state->elapsedMs+=dt;
    *distance=hypotf(mixed[0],mixed[1]);
    return isfinite(*distance);
}
#endif
