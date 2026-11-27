#ifndef COD1_COURSE_PROFILE46_H
#define COD1_COURSE_PROFILE46_H
/* Internal GAME observer. The existing call boundaries stay independent;
 * no current callback, resource admission, receipt or revision is elided.
 * Begin/Step/Drain and note/bucket timings are inclusive. */
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
#include "g_cod1_phase_profile46.h"
static int CourseClockBegin46(const GscEntityRef*ref,int now,uint64_t generation,int(*current)(void*),void*ctx){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(ref,now);
 int ok=G_Cod1ActorFaceFrameBegin(ref,now,generation,current,ctx);
 ActorPhaseProfileEnd46(C1AP46_COURSE_BEGIN,t,ok);return ok;
}
static int CourseClockStep46(const GscEntityRef*ref,int now,uint64_t generation,int*resume,int(*current)(void*),void*ctx){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(ref,now);
 int ok=G_Cod1ActorFaceFrameStep(ref,now,generation,resume,current,ctx);
 ActorPhaseProfileEnd46(C1AP46_COURSE_STEP,t,ok);return ok;
}
static int CourseClockDrain46(const GscEntityRef*ref,int now,uint64_t generation,int(*current)(void*),cod1ActorFaceNoteCallback deliver,void*ctx){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(ref,now);
 int ok=G_Cod1ActorFaceFrameDrain(ref,now,generation,current,deliver,ctx);
 ActorPhaseProfileEnd46(C1AP46_COURSE_DRAIN,t,ok);return ok;
}
static int CourseClockSample46(const GscEntityRef*ref,int now,float output[98],float phases[2][21],float facial[2][25],int(*current)(void*),void*ctx){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(ref,now);
 int ok=G_Cod1ActorScriptedSampleFull(ref,now,output,phases,facial,current,ctx);
 ActorPhaseProfileEnd46(C1AP46_COURSE_SAMPLE,t,ok);return ok;
}
static int CourseClockBucket46(const GscEntityRef*ref,int now,uint64_t generation,const cod1ActorLease_t*lease,int*retired){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(ref,now);
 int ok=FoleyRunNoteBucket(ref,now,generation,lease,retired);
 ActorPhaseProfileEnd46(C1AP46_COURSE_BUCKET,t,ok);return ok;
}
static int CourseClockQueue46(const cod1ActorFaceNote_t*note){
 C1PhaseProfile46 t=ActorPhaseProfileStart46(note?&note->entity:NULL,note?note->produced_ms:level.time);
 int ok=FoleySessionQueueFrameNote(note);
 ActorPhaseProfileEnd46(C1AP46_COURSE_NOTE,t,ok);return ok;
}
#else
#define CourseClockBegin46 G_Cod1ActorFaceFrameBegin
#define CourseClockStep46 G_Cod1ActorFaceFrameStep
#define CourseClockDrain46 G_Cod1ActorFaceFrameDrain
#define CourseClockSample46 G_Cod1ActorScriptedSampleFull
#define CourseClockBucket46 FoleyRunNoteBucket
#define CourseClockQueue46 FoleySessionQueueFrameNote
#endif
#endif
