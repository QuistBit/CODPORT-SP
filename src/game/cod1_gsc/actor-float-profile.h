#ifndef R243_ACTOR_FLOAT_PROFILE_H
#define R243_ACTOR_FLOAT_PROFILE_H
#include <stdint.h>
#define R243_ACTOR_FLOAT_PATH 1502
#define R243_ACTOR_FLOAT_ENEMY 1503
typedef struct {GscEntityRef entity;uint64_t program,runtime,catalog,task,call;int now;uint32_t method;} R243ActorFloatRequest;
typedef struct {uint32_t bits;} R243ActorFloatReply;
typedef struct {void*user;int(*validate)(void*,const GscEntityRef*);int(*query)(void*,const R243ActorFloatRequest*,R243ActorFloatReply*,char*,size_t,int(*)(void*),void*);} R243ActorFloatHost;
static unsigned R243ActorFloatMask(unsigned method){return method==R243_ACTOR_FLOAT_PATH?1u:method==R243_ACTOR_FLOAT_ENEMY?2u:0u;}
int R243ProgramSelectActorFloat(GscProgram*,unsigned);
int R243RuntimeSelectActorFloat(GscRuntime*,const R243ActorFloatHost*);
int R243ActorFloatExactCallCurrent(const R243ActorFloatRequest*,int(*)(void*),void*);
int R243ActorFloatValidationRefCurrent(const GscEntityRef*);
int R243ActorFloatRuntimeMaskCurrent(const GscRuntime*,unsigned);
int R243ActorFloatNativeBuffersCurrent(const R243ActorFloatRequest*,const R243ActorFloatReply*,const char*,size_t,int(*)(void*),void*);
int R243ActorFloatSessionBuffersCurrent(const R243ActorFloatRequest*,const R243ActorFloatReply*,const char*,size_t,int(*)(void*),void*);
const R243ActorFloatHost*R243GameActorFloatHost(void);
int R243ActorFloatSessionHostTimeCurrent(const R243ActorFloatRequest*,int);
struct GscSession;
int gsc_session_select_r243_actor_float(struct GscSession*,const R243ActorFloatHost*,char*,size_t);
int gsc_session_select_r243_actor_float_methods(struct GscSession*,const R243ActorFloatHost*,unsigned,char*,size_t);
#endif
