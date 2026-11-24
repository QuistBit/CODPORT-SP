/* Existing model-part contract with a map-owned Session clock. No turret or
 * Training owner is assumed, and the original request remains immutable. */
typedef struct {GscSession*session;uint64_t id,epoch;int actual;GscModelPartRequest copy;const GscModelPartRequest*input;int(*current)(void*);void*ctx;} GscMapModelPartCall;
static int GscMapModelPartBase(const GscMapModelPartCall*c){
 return c&&c->current&&c->input&&c->session&&c->session==gscTestSession&&c->id&&c->id==gscTestId&&
  c->epoch&&c->epoch==gscTestMapEpoch&&c->epoch==G_Cod1EntityMapEpoch()&&!gscTestStopRequested&&
  c->actual==level.time&&!level.spawning&&!level.restarted&&c->copy.now_ms>=0&&
  !memcmp(c->input,&c->copy,sizeof(c->copy))&&GscVehicleMapCurrent(c->epoch)&&GscTestModuleReadCurrent()&&
  gsc_session_running_at(c->session,c->id,c->copy.now_ms,c->actual);
}
static int GscMapModelPartCurrent(void*u){GscMapModelPartCall*c=u;return GscMapModelPartBase(c)&&c->current(c->ctx)==1&&GscMapModelPartBase(c);}
static int GscMapModelPart(void*u,const GscModelPartRequest*input,char out[128],char*error,size_t cap,int(*current)(void*),void*ctx){
 GscMapModelPartCall c;GscModelPartRequest q,expected;GscModelPartHost h=*G_Cod1ModelPartHost();char result[128];int ok;(void)u;
 if(!input||!out||!current||(cap&&!error)||!h.query||!GscTestTurretBuffers(input,sizeof(*input),out,128,error,cap)||
    GscVehiclePrivate(input,sizeof(*input))||GscVehiclePrivate(out,128)||GscVehiclePrivate(error,cap))return 0;
 memset(&c,0,sizeof(c));c.session=gscTestSession;c.id=gscTestId;c.epoch=G_Cod1EntityMapEpoch();c.actual=level.time;c.input=input;c.copy=*input;c.current=current;c.ctx=ctx;
 if(!GscMapModelPartCurrent(&c))return 0;q=c.copy;q.now_ms=c.actual;expected=q;memset(result,0xff,sizeof(result));
 ok=h.query(h.user,&q,result,error,cap,GscMapModelPartCurrent,&c);
 if(ok!=1||!GscMapModelPartCurrent(&c)||memcmp(&q,&expected,sizeof(q))||memcmp(&h,G_Cod1ModelPartHost(),sizeof(h))||!memchr(result,0,128)||!result[0])return 0;
 memcpy(out,result,128);return 1;
}
static const GscModelPartHost gscMapModelPartHost={NULL,GscMapModelPart};
