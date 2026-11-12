/* Native GAME 20036390: arity, GetInt0, optional GetVector4/normalize,
 * optional GetFloat3, GetVector2, GetFloat1, allocate/link, return entity.
 * No fake spawn/global import, implicit FX capability, or death notification. */
int gsc_program_select_loop_fx(GscProgram *g){
 if(!g||g->invalid||g->linked||g->loop_fx_selected||!g->fx_syntax||
    !g->lifetime_syntax)return 0;
 g->loop_fx_selected=1;return 1;
}
int gsc_runtime_select_loop_fx(GscRuntime *r,const GscLoopFxHost *h){
 if(!r||!h||!h->prepare||!h->finish||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    !r->program->loop_fx_selected||r->loop_fx_enabled||!r->fx_enabled||!r->entities_enabled||
    !r->results_enabled||!r->lifetime_enabled||!gsc_runtime_storage_separate(r,h,sizeof(*h)))return 0;
 r->loop_fx=*h;r->loop_fx_enabled=1;return 1;
}
typedef struct {
 GscRuntime *r;Task *task;uint64_t serial,runtime,operation;int now,line,active,checking;
 const GscLoopFxRequest *request;GscLoopFxRequest expected;
 const GscLoopFxServices *services;GscLoopFxServices api;
} LoopFxLease;
static int loop_fx_current(void *p){
 LoopFxLease *l=p;GscRuntime *r=l?l->r:NULL;int ok;
 if(!l||!l->active||!r||r->loop_fx_scope!=l||r->error[0]||r->executing_task!=l->task||
    !l->task||l->task->serial!=l->serial||r->now_ms!=l->now||r->loop_fx_operation!=l->operation||r->array_token!=l->runtime||
    memcmp(l->request,&l->expected,sizeof(l->expected))||memcmp(l->services,&l->api,sizeof(l->api)))return 0;
 if(l->checking)return rfail(r,l->line,"loop FX current callback reentered");
 l->checking=1;ok=entity_task_current(r);l->checking=0;
 return ok&&!r->error[0]&&r->loop_fx_scope==l&&r->executing_task==l->task&&l->task->serial==l->serial&&
    r->now_ms==l->now&&r->array_token==l->runtime&&r->loop_fx_operation==l->operation&&
    !memcmp(l->request,&l->expected,sizeof(l->expected))&&!memcmp(l->services,&l->api,sizeof(l->api));
}
static int loop_fx_float(GscRuntime *r,const Value *v,int n,int line,float *out){
 int32_t integer;double x;
 if(v->kind!=V_NUMBER)return rfail(r,line,"playLoopedFx GetFloat%d requires a number",n);
 if(v->integer){if(!scalar_get_integer(r,v,line,&integer))return 0;x=integer;}else x=v->number;
 if(!isfinite(x)||x>FLT_MAX||x< -FLT_MAX)return rfail(r,line,"playLoopedFx GetFloat%d outside finite float domain",n);
 *out=(float)x;return 1;
}
static int loop_fx_call(GscRuntime *r,const Value *args,int argc,int line,Value *out){
 GscLoopFxRequest q;GscLoopFxTicket produced,ticket;GscLoopFxServices api;LoopFxLease l;
 cod1LoopFxValue_t direction_check;float direction[3]={0,0,0},origin[3],distance=0,seconds;
 int32_t index;int ok=0,accepted,alive,finish_ok;char why[384],cleanup[256];
 if(!r->loop_fx_enabled||!args||!out||argc<3||argc>5||!r->executing_task)
  return rfail(r,line,"playLoopedFx requires selected creation capability and 3..5 arguments");
 if(r->loop_fx_scope)return rfail(r,line,"loop FX creation reentered");
 if(args[0].kind!=V_NUMBER||args[0].integer!=1)return rfail(r,line,"playLoopedFx GetInt0 requires int32");
 if(!scalar_get_integer(r,&args[0],line,&index))return 0;
 if(argc==5){
  if(args[4].kind!=V_VECTOR)return rfail(r,line,"playLoopedFx GetVector4 requires vector");
  memcpy(direction,args[4].vector,sizeof(direction));
  /* Validate/normalize before the later getters, without masking bad original
   * indices or periods. Reuse the proven shared normalization body unchanged. */
  {const float zero[3]={0,0,0};if(!CLF_Prepare(1,1,zero,1,direction,0,&direction_check))
   return rfail(r,line,"playLoopedFx direction is zero or outside finite domain");}
 }
 if(argc>=4&&!loop_fx_float(r,&args[3],3,line,&distance))return 0;
 if(args[2].kind!=V_VECTOR)return rfail(r,line,"playLoopedFx GetVector2 requires vector");
 memcpy(origin,args[2].vector,sizeof(origin));
 if(!loop_fx_float(r,&args[1],1,line,&seconds))return 0;
 memset(&q,0,sizeof(q));
 if(!CLF_Prepare(index,seconds,origin,argc==5,direction,distance,&q.value))
  return rfail(r,line,"playLoopedFx parameters outside positive-period finite FX domain");
 if(r->loop_fx_operation==UINT64_MAX)return rfail(r,line,"loop FX operation exhausted");
 memset(&l,0,sizeof(l));memset(&api,0,sizeof(api));memset(&produced,0,sizeof(produced));
 q.runtime=r->array_token;q.operation=++r->loop_fx_operation;q.now_ms=r->now_ms;
 l.r=r;l.task=r->executing_task;l.serial=l.task->serial;l.runtime=q.runtime;l.operation=q.operation;l.now=r->now_ms;l.line=line;l.active=1;
 l.request=&q;l.expected=q;l.services=&api;api.context=&l;api.current=loop_fx_current;l.api=api;r->loop_fx_scope=&l;
 if(!loop_fx_current(&l))goto done;
 why[0]=0;accepted=r->loop_fx.prepare(r->loop_fx.user,&q,&produced,&api,why,sizeof(why));
 ticket=produced;why[sizeof(why)-1]=0;
 if(!loop_fx_current(&l))goto cancel;
 if(accepted!=1){rfail(r,line,"loop FX prepare refused: %s",why[0]?why:"host failure");goto cancel;}
 if(ticket.operation!=l.operation||!ticket.entity.epoch||!ticket.entity.generation){rfail(r,line,"loop FX malformed creation receipt");goto cancel;}
 alive=state_entity_live(r,&ticket.entity);
 if(!loop_fx_current(&l))goto cancel;
 if(alive!=1){rfail(r,line,"loop FX returned stale entity");goto cancel;}
 why[0]=0;accepted=r->loop_fx.finish(r->loop_fx.user,l.runtime,l.operation,1,&api,why,sizeof(why));why[sizeof(why)-1]=0;
 if(!loop_fx_current(&l))goto cancel;
 if(accepted!=1){rfail(r,line,"loop FX publication refused: %s",why[0]?why:"host failure");goto cancel;}
 alive=state_entity_live(r,&ticket.entity);
 if(!loop_fx_current(&l))goto cancel;
 if(alive!=1){rfail(r,line,"loop FX publication retired its entity");goto cancel;}
 memset(out,0,sizeof(*out));out->kind=V_ENTITY;out->entity=ticket.entity;ok=1;goto done;
cancel:
 /* Never trust a mutated reply/request to select cleanup. The private host
  * receipt is indexed by the VM's original operation, even on abort/error. */
 cleanup[0]=0;finish_ok=r->loop_fx.finish(r->loop_fx.user,l.runtime,l.operation,0,&l.api,cleanup,sizeof(cleanup));cleanup[sizeof(cleanup)-1]=0;
 if(finish_ok!=1){
  if(!r->error[0])rfail(r,line,"loop FX cancellation refused: %s",cleanup);
  else {size_t used=strlen(r->error);snprintf(r->error+used,sizeof(r->error)-used,"; loop FX owned cleanup refused: %.96s",cleanup);}
 }
done:
 if(!ok&&!r->error[0]&&entity_task_current(r))rfail(r,line,"loop FX transaction lost its execution context or immutable request");
 l.active=0;r->loop_fx_scope=NULL;return ok;
}
