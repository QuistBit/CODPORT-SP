#ifndef R243_ANIMTIME_PROFILE_H
#define R243_ANIMTIME_PROFILE_H
#include <string.h>
/* Explicit typed animation query. Reply is the original stored float32 phase. */
#define R243_ANIMTIME_METHOD 1504
typedef struct {
 GscEntityRef entity;uint64_t program,runtime,catalog,task,call;
 int now;GscAnimationLengthRequest animation;
} R243AnimTimeRequest;
typedef struct {uint32_t bits;} R243AnimTimeReply;
typedef struct {
 void*user;int(*validate)(void*,const GscEntityRef*);
 int(*query)(void*,const R243AnimTimeRequest*,R243AnimTimeReply*,char*,size_t,int(*)(void*),void*);
} R243AnimTimeHost;
int R243ProgramSelectAnimTime(GscProgram*);
int R243RuntimeSelectAnimTime(GscRuntime*,const R243AnimTimeHost*);
int R243AnimTimeExactCallCurrent(const R243AnimTimeRequest*,int(*)(void*),void*);
int R243AnimTimeValidationRefCurrent(const GscEntityRef*);
int R243AnimTimeNativeBuffersCurrent(const R243AnimTimeRequest*,const R243AnimTimeReply*,const char*,size_t,int(*)(void*),void*);
int R243AnimTimeSessionBuffersCurrent(const R243AnimTimeRequest*,const R243AnimTimeReply*,const char*,size_t,int(*)(void*),void*);
const R243AnimTimeHost*R243GameAnimTimeHost(void);
int R243GameAnimTimeMapCertificate(GscMapExecutionCertificate*);
int R243GameAnimTimeComponentTarget(char*,size_t,GscEntityRef*);
const GscEntityHost*G_Cod1EntityHost(void);
int R243AnimTimeSessionHostTimeCurrent(const R243AnimTimeRequest*,int);
struct GscSession;
int gsc_session_select_r243_animtime(struct GscSession*,const R243AnimTimeHost*,char*,size_t);
#endif
