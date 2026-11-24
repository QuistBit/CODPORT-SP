/* Native GetPartName resolves the named model, then reads its indexed part.
 * The resource cache is independent of GAME model/configstring registration.
 * No actor/tag fallback or pose query participates in this global getter. */
static uint64_t modelPartSerial=1;
static int modelPartBusy,modelPartCancelled;
static int ModelPartPrivate(const void*p,size_t n){return ActorOverlap(p,n,&modelPartSerial,sizeof(modelPartSerial))||ActorOverlap(p,n,&modelPartBusy,sizeof(modelPartBusy))||ActorOverlap(p,n,&modelPartCancelled,sizeof(modelPartCancelled));}
static void ModelPartReset(void){if(modelPartSerial!=UINT64_MAX)++modelPartSerial;if(modelPartBusy)modelPartCancelled=1;}
typedef struct {GscModelPartRequest q;const GscModelPartRequest*input;uint64_t epoch,serial;int now,count;int(*current)(void*);void*context;} ModelPartCall;
static int ModelPartBase(const ModelPartCall*c){
 return modelPartBusy&&!modelPartCancelled&&modelPartSerial!=UINT64_MAX&&modelPartSerial==c->serial&&
  ActorMapAvailable()&&cod1EntityReady&&!level.spawning&&!level.restarted&&cod1EntityEpoch==c->epoch&&
  level.time==c->now&&level.num_entities==c->count&&!memcmp(c->input,&c->q,sizeof(c->q));
}
static int ModelPartCurrent(const ModelPartCall*c){return ModelPartBase(c)&&c->current(c->context)==1&&ModelPartBase(c);}
static int ModelPartQuery(void*user,const GscModelPartRequest*input,char out[128],char*error,size_t cap,int(*current)(void*),void*context){
 ModelPartCall c;cod1ModelPrecache_t model;cod1ModelResourceReference_t ref;cod1ModelPartNameResult_t part;int slot,status,ok=0;size_t n;(void)user;
 if(!input||!out||!current||(cap&&!error)||!ActorGoalBuffers(input,sizeof(*input),out,128,error,cap)||ModelPartPrivate(input,sizeof(*input))||ModelPartPrivate(out,128)||ModelPartPrivate(error,cap))return 0;
 if(modelPartBusy){modelPartCancelled=1;EntityError(error,cap,"model part query reentered");return 0;}
 memset(&c,0,sizeof(c));c.input=input;c.q=*input;c.epoch=cod1EntityEpoch;c.serial=modelPartSerial;c.now=level.time;c.count=level.num_entities;c.current=current;c.context=context;
 if(c.q.now_ms!=c.now||c.now<0||!memchr(c.q.model,0,128)||(c.q.index_valid!=0&&c.q.index_valid!=1))return 0;
 n=strlen(c.q.model);if(n>=MAX_QPATH||!Cod1PrecacheName(c.q.model,1)){EntityError(error,cap,"getPartName model outside supported xmodel path domain");return 0;}
 memset(&model,0,sizeof(model));model.version=COD1_PRECACHE_VERSION;model.bytes=sizeof(model);memcpy(model.name,c.q.model,n+1);
 modelPartBusy=1;modelPartCancelled=0;
 if(!ModelPartCurrent(&c))goto done;
 slot=trap_Cod1ModelPrecache(&model);
 if(!ModelPartCurrent(&c)||slot<1||slot>COD1_PRECACHE_MODELS){EntityError(error,cap,"getPartName model resource unavailable or retired");goto done;}
 if(!ActorModelQuery((uint32_t)slot,model.name,&ref)||!ModelPartCurrent(&c))goto done;
 /* Native resolves model before invoking GetInt1. */
 if(!c.q.index_valid){EntityError(error,cap,"getPartName GetInt1 requires admitted int32");goto done;}
 memset(&part,0,sizeof(part));status=trap_Cod1ModelPartName(&ref,c.q.index,&part);
 if(!ModelPartCurrent(&c)||!ActorModelValidateResource(&ref)||!ModelPartCurrent(&c)||!ActorModelValidateResource(&ref)||!ModelPartBase(&c))goto done;
 if(status==COD1_MODEL_PART_RANGE){if(error&&cap)snprintf(error,cap,"index out of range (0 - %d)",part.count-1);goto done;}
 if(status==COD1_MODEL_PART_BAD_MODEL){EntityError(error,cap,"bad model");goto done;}
 if(status!=1||!memchr(part.name,0,128)||!part.name[0]){EntityError(error,cap,"getPartName part resource unavailable");goto done;}
 memcpy(out,part.name,128);ok=1;
 done:modelPartBusy=0;return ok;
}
const GscModelPartHost*G_Cod1ModelPartHost(void){static const GscModelPartHost h={NULL,ModelPartQuery};return &h;}
