#ifndef R243_STANCE_PROFILE_H
#define R243_STANCE_PROFILE_H
#include <string.h>
/* WORK-only exact1string receiver predicate. accepted1 and allowed0 differ. */
#define R243_STANCE_METHOD 1501
#define COD1_ALLOWED_STANCES_METHOD 1505
typedef struct {
 GscEntityRef entity;uint64_t program,runtime,catalog,task,call;
 int now;uint32_t bit,writeMask;char stance[8];
} R243StanceRequest;
typedef struct {int allowed;} R243StanceReply;
typedef struct {
 void*user;int(*validate)(void*,const GscEntityRef*);
 int(*query)(void*,const R243StanceRequest*,R243StanceReply*,char*,size_t,int(*)(void*),void*);
 int allowWrite;
} R243StanceHost;
int R243ProgramSelectStance(GscProgram*);
int Cod1ProgramSelectStanceWrite(GscProgram*);
int R243RuntimeSelectStance(GscRuntime*,const R243StanceHost*);
int R243StanceExactCallCurrent(const R243StanceRequest*,int(*)(void*),void*);
int R243StanceValidationRefCurrent(const GscEntityRef*);
int R243StanceNativeBuffersCurrent(const R243StanceRequest*,const R243StanceReply*,const char*,size_t,int(*)(void*),void*);
int R243StanceSessionBuffersCurrent(const R243StanceRequest*,const R243StanceReply*,const char*,size_t,int(*)(void*),void*);
const R243StanceHost*R243GameStanceHost(void);
int R243GameStanceMapCertificate(GscMapExecutionCertificate*);
int R243GameStanceComponentTarget(char*,size_t,GscEntityRef*);
const GscEntityHost*G_Cod1EntityHost(void);
static uint32_t R243StanceBit(const char*s){
 if(!strcmp(s,"stand"))return 1;if(!strcmp(s,"crouch"))return 2;if(!strcmp(s,"prone"))return 4;return 0;
}
int R243StanceSessionHostTimeCurrent(const R243StanceRequest*,int);
struct GscSession;
int gsc_session_select_r243_stance(struct GscSession*,const R243StanceHost*,char*,size_t);
#endif
