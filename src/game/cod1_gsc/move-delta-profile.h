#ifndef R243_MOVE_DELTA_PROFILE_H
#define R243_MOVE_DELTA_PROFILE_H
/* Internal typed ABI129. Vector resource query, separate from old generic scalar and
 * fixed training start-origin hosts. finite normalized1..3 arguments only. */
enum {R243_EXPR_TARGET_MOVE_DELTA=GSC_EXPR_TARGET_R232_RANDOM_RANGE+1};
#define R243_MOVE_DELTA_STATEMENT_TARGET 13113
typedef struct {
 uint64_t runtime,catalog,program,task,call,resource_owner;
 int now,argc;uint32_t ordinal,flags;float start,end;char tree[64],name[64];
} R243MoveDeltaRequest;
typedef struct {float vector[3];} R243MoveDeltaReply;
typedef struct {void*user;int(*query)(void*,const R243MoveDeltaRequest*,R243MoveDeltaReply*,char*,size_t,int(*)(void*),void*);uint64_t resource_owner;} R243MoveDeltaHost;
int R243ProgramSelectMoveDelta(GscProgram*);
int R243RuntimeSelectMoveDelta(GscRuntime*,const R243MoveDeltaHost*);
int R243MoveDeltaExactCallCurrent(const R243MoveDeltaRequest*,int(*)(void*),void*);
int R243MoveDeltaNativeBuffersCurrent(const R243MoveDeltaRequest*,const R243MoveDeltaReply*,const char*,size_t,int(*)(void*),void*);
int R243MoveDeltaSessionBuffersCurrent(const R243MoveDeltaRequest*,const R243MoveDeltaReply*,const char*,size_t,int(*)(void*),void*);
int R243MoveDeltaSessionHostTimeCurrent(const R243MoveDeltaRequest*,int);
int R243MoveDeltaSessionCatalogCurrent(const R243MoveDeltaRequest*,const GscAnimationCatalog*);
struct GscSession;
int gsc_session_select_r243_move_delta(struct GscSession*,const R243MoveDeltaHost*,char*,size_t);
int R243GameMoveDeltaPrepare(void);
int R243GameMoveDeltaPrepareMap(const GscMapExecutionCertificate*);
const R243MoveDeltaHost*R243GameMoveDeltaHost(void);
#endif
