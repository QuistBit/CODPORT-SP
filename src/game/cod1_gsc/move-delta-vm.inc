/* A lexical global resource-query origin. Identity is compared by value with
 * an unexported seal as well as membership; a private address alone never
 * confers a query lease. Original INIT/MAIN scopes are not consulted. */
typedef struct {
 GscRuntime*r;const GscProgram*g;Task*t;Frame*f;
 uint64_t serial,instance,catalog,program,timeRoot,call;
 int now,depth,fn,pc;GscMapExecutionCertificate map;R243MoveDeltaHost host;
} MoveDeltaIdentity;
typedef struct {
 MoveDeltaIdentity id,seal;R243MoveDeltaRequest request,expected;
 R243MoveDeltaReply reply;char error[384];int checking,cancelled;
} MoveDeltaCall;
static MoveDeltaCall*move_delta_active;
static uint64_t move_delta_next_call;
static int move_delta_private(const void*p,size_t n){
 return Cod1ModelStringOverlap(p,n,&move_delta_active,sizeof(move_delta_active))||
  Cod1ModelStringOverlap(p,n,&move_delta_next_call,sizeof(move_delta_next_call))||
  (move_delta_active&&Cod1ModelStringOverlap(p,n,move_delta_active,sizeof(*move_delta_active)));
}
int R243ProgramSelectMoveDelta(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
    !g->anim_data_selected||!g->animation_namespace_selected||g->move_delta_selected)return 0;
 g->move_delta_selected=1;return 1;
}
int R243RuntimeSelectMoveDelta(GscRuntime*r,const R243MoveDeltaHost*input){
 R243MoveDeltaHost host;const GscProgram*g;uint64_t runtime,catalog;
 if(!input||move_delta_active||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    !r->program->move_delta_selected||r->move_delta_enabled||
    !gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
 host=*input;if(!host.query||!host.resource_owner)return 0;g=r->program;runtime=r->array_token;catalog=r->animation_namespace.identity;
 if(!anim_data_current(r,NULL)||!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||
    r->program!=g||r->array_token!=runtime||r->animation_namespace.identity!=catalog||r->move_delta_enabled)return 0;
 r->move_delta=host;r->move_delta_enabled=1;return 1;
}
static int move_delta_scope(MoveDeltaCall*c){
 const MoveDeltaIdentity*i;GscRuntime*r;Task*t;int n,member=0;
 if(!c||move_delta_active!=c||c->cancelled||memcmp(&c->id,&c->seal,sizeof(c->id))||
    memcmp(&c->request,&c->expected,sizeof(c->request)))return 0;
 i=&c->seal;r=i->r;t=i->t;
 if(!r||!r->started||!r->advancing||r->error[0]||!r->move_delta_enabled||!r->move_delta_busy||
    !r->entity_method_busy||r->program!=i->g||!r->program->move_delta_selected||
    r->program->function_program_token!=i->program||r->array_token!=i->instance||r->now_ms!=i->now||
    !animation_namespace_scope(r)||r->animation_namespace.identity!=i->catalog||
    r->executing_task!=t||!r237_owner_current(r,i->timeRoot)||memcmp(&r->move_delta,&i->host,sizeof(i->host))||
    memcmp(&r->anim_data,&i->map,sizeof(i->map)))return 0;
 for(n=0;n<r->task_count;n++)if(task_at(r,n)==t){member=1;break;}
 if(!member||!t||t->serial!=i->serial||t->status!=TASK_READY||t->context_retired||
    t->depth!=i->depth||t->depth<1||t->depth>MAX_FRAMES||&t->frames[t->depth-1]!=i->f||
    i->f->fn!=i->fn||i->f->pc!=i->pc)return 0;
 return 1;
}
static int move_delta_current(void*opaque){
 MoveDeltaCall*c=move_delta_active;GscRuntime*r;int ok;
 /* Do not dereference a copied, expired or unrelated context. */
 if(!c||(uintptr_t)opaque!=c->seal.call||!move_delta_scope(c))return 0;
 if(c->checking){c->cancelled=1;return 0;}c->checking=1;r=c->seal.r;
 ok=entity_task_current(r)&&move_delta_scope(c)&&anim_data_current(r,c->seal.t)&&move_delta_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
int R243MoveDeltaExactCallCurrent(const R243MoveDeltaRequest*q,int(*current)(void*),void*context){MoveDeltaCall*c=move_delta_active;if(!c||!q||current!=move_delta_current||(uintptr_t)context!=c->seal.call||!move_delta_scope(c)||memcmp(q,&c->expected,sizeof(*q)))return 0;return current(context)==1&&move_delta_scope(c);}
int R243MoveDeltaNativeBuffersCurrent(const R243MoveDeltaRequest*q,const R243MoveDeltaReply*out,const char*error,size_t cap,int(*current)(void*),void*context){MoveDeltaCall*c=move_delta_active;if(!c||!q||!R243MoveDeltaExactCallCurrent(q,current,context))return 0;if(q==&c->request&&out==&c->reply&&error==c->error&&cap==sizeof(c->error))return move_delta_scope(c);return R243MoveDeltaSessionBuffersCurrent(q,out,error,cap,current,context)&&move_delta_scope(c);}
static int move_delta_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 MoveDeltaCall c;const AnimationNamespaceEntry*entry;Value value;float start=0,end=1;
 uint32_t poison=UINT32_C(0x7fc00001);int n,accepted=0,ok=0;
 if(!r||!args||!out||argc<1||argc>3||!r->program->move_delta_selected||!r->move_delta_enabled)
  return rfail(r,line,"GetMoveDelta selected finite1..3argument profile unavailable");
 if(move_delta_active||r->move_delta_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)
  return rfail(r,line,"GetMoveDelta callback reentered");
 if(args[0].kind!=V_NAMESPACED_ANIMATION||!animation_namespace_value_valid(r,args))
  return rfail(r,line,"GetMoveDelta requires a current declared animation value");
 entry=&r->animation_namespace.entries[args[0].object-1];
 if(entry->catalog>=(unsigned)animation_namespace_count(r->program)||!entry->declaration.ordinal||entry->declaration.flags)
  return rfail(r,line,"GetMoveDelta selected resource profile requires a declared leaf");
 if(argc>1&&!scalar_float(r,&args[1],line,&start))return 0;
 if(argc>2&&!scalar_float(r,&args[2],line,&end))return 0;
 if(!isfinite(start)||!isfinite(end)||start<0||start>1||end<0||end>1)
  return rfail(r,line,"GetMoveDelta phases must be finite and normalized");
 if(move_delta_next_call>=UINTPTR_MAX||!r->executing_task||r->executing_task->depth<1||r->executing_task->depth>MAX_FRAMES)
  return rfail(r,line,"GetMoveDelta executing call origin unavailable");
 memset(&c,0,sizeof(c));c.id.r=r;c.id.g=r->program;c.id.t=r->executing_task;c.id.serial=c.id.t->serial;
 c.id.f=&c.id.t->frames[c.id.t->depth-1];c.id.depth=c.id.t->depth;c.id.fn=c.id.f->fn;c.id.pc=c.id.f->pc;
 c.id.instance=r->array_token;c.id.catalog=r->animation_namespace.identity;c.id.program=r->program->function_program_token;
 c.id.timeRoot=r237_owner_stamp(r);c.id.call=++move_delta_next_call;c.id.now=r->now_ms;c.id.map=r->anim_data;c.id.host=r->move_delta;
 c.request.runtime=c.id.instance;c.request.catalog=c.id.catalog;c.request.program=c.id.program;c.request.task=c.id.serial;
 c.request.call=c.id.call;c.request.resource_owner=c.id.host.resource_owner;c.request.now=c.id.now;c.request.argc=argc;c.request.start=start;c.request.end=end;
 c.request.ordinal=entry->declaration.ordinal;c.request.flags=entry->declaration.flags;
 strcpy(c.request.tree,r->animation_namespace.catalogs[entry->catalog].tree);strcpy(c.request.name,entry->declaration.name);
 /* Copy every borrowed argument/declaration before any host callback. */
 c.seal=c.id;c.expected=c.request;for(n=0;n<3;n++)memcpy(&c.reply.vector[n],&poison,4);
 move_delta_active=&c;r->move_delta_busy=1;r->entity_method_busy=1;
 if(!move_delta_current((void*)(uintptr_t)c.id.call))goto done;
 accepted=c.seal.host.query(c.seal.host.user,&c.request,&c.reply,c.error,sizeof(c.error),move_delta_current,(void*)(uintptr_t)c.id.call);
 c.error[sizeof(c.error)-1]=0;
 if(!move_delta_current((void*)(uintptr_t)c.id.call))goto done;
 if(accepted!=1){rfail(r,line,"GetMoveDelta resource provider refused: %s",c.error[0]?c.error:"missing owned clip");goto done;}
 for(n=0;n<3;n++)if(!isfinite(c.reply.vector[n])){rfail(r,line,"GetMoveDelta requires three finite reply components");goto done;}
 memset(&value,0,sizeof(value));value.kind=V_VECTOR;memcpy(value.vector,c.reply.vector,12);ok=1;
done:
 r->move_delta_busy=0;r->entity_method_busy=0;move_delta_active=NULL;
 if(!ok&&!r->error[0]&&c.seal.t->status!=TASK_DEAD)rfail(r,line,"GetMoveDelta executing origin retired or changed");
 if(!ok||r->error[0])return 0;*out=value;return 1;
}
