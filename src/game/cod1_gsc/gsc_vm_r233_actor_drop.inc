
/* Native200678B0 returns void. none bypasses hand/float getters; all supplied
 * operand expressions still evaluate normally before entering this method. */
int gsc_program_select_r233_actor_drop(GscProgram*g){if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||g->actor_drop_selected)return 0;g->actor_drop_selected=1;return 1;}
int gsc_runtime_select_r233_actor_drop(GscRuntime*r,const GscActorDropHost*input){GscActorDropHost h;if(!input||!animation_namespace_quiescent(r)||!r->program->actor_drop_selected||!r->turret_animations_enabled||!animation_namespace_scope(r)||r->actor_drop_enabled||runtime_private_overlap(r,input,sizeof(*input)))return 0;h=*input;if(!h.invoke||!anim_data_current(r,NULL)||memcmp(input,&h,sizeof(h)))return 0;r->actor_drop=h;r->actor_drop_enabled=1;return 1;}
typedef struct {GscRuntime*r;Task*task;uint64_t serial,instance;int now,checking,cancelled;GscActorDropHost host;GscActorDropRequest*input,copy;} ActorDropLease;
static int actor_drop_scope(const ActorDropLease*l){GscRuntime*r=l->r;return !l->cancelled&&r&&!r->error[0]&&r->entity_method_busy&&r->actor_drop_enabled&&r->array_token==l->instance&&r->now_ms==l->now&&r->executing_task==l->task&&l->task&&l->task->serial==l->serial&&l->task->status!=TASK_DEAD&&animation_namespace_scope(r)&&!memcmp(&r->actor_drop,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));}
static int actor_drop_current(void*opaque){ActorDropLease*l=opaque;int ok;if(!actor_drop_scope(l))return 0;if(l->checking){l->cancelled=1;return 0;}l->checking=1;ok=entity_task_current(l->r)&&actor_drop_scope(l)&&anim_data_current(l->r,l->task)&&actor_drop_scope(l)&&state_entity_live(l->r,&l->copy.entity)==1&&actor_drop_scope(l)&&entity_task_current(l->r)&&actor_drop_scope(l);l->checking=0;return ok;}
static int actor_drop_invoke(GscRuntime*r,const Value*receiver,const Value*args,int argc,int line){GscActorDropRequest q;ActorDropLease l;char error[384];const char*end;int ok;
 if(!r->actor_drop_enabled||!receiver||receiver->kind!=V_ENTITY||!args||argc<1||argc>MAX_PARAMS)return rfail(r,line,"DropWeapon receiver/capability/arity unavailable");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"DropWeapon callback reentered");
 memset(&q,0,sizeof(q));q.entity=receiver->entity;q.runtime_identity=r->array_token;q.now_ms=r->now_ms;
 if(args[0].kind!=V_STRING||!(end=memchr(args[0].str,0,sizeof(args[0].str)))||end==args[0].str||(size_t)(end-args[0].str)>=sizeof(q.name))return rfail(r,line,"DropWeapon requires a bounded nonempty weapon name");memcpy(q.name,args[0].str,(size_t)(end-args[0].str));
 if(strcmp(q.name,"none")&&strcmp(q.name,"NONE")&&!(strlen(q.name)==4&&(q.name[0]=='n'||q.name[0]=='N')&&(q.name[1]=='o'||q.name[1]=='O')&&(q.name[2]=='n'||q.name[2]=='N')&&(q.name[3]=='e'||q.name[3]=='E'))){
  if(argc<3||args[1].kind!=V_STRING||!(end=memchr(args[1].str,0,sizeof(args[1].str)))||(size_t)(end-args[1].str)>=sizeof(q.hand))return rfail(r,line,"DropWeapon requires a hand string and speed");memcpy(q.hand,args[1].str,(size_t)(end-args[1].str));if(!actor_animation_float(r,&args[2],line,1,&q.speed_bits))return 0;
 }
 memset(&l,0,sizeof(l));l.r=r;l.task=r->executing_task;l.serial=l.task?l.task->serial:0;l.instance=r->array_token;l.now=r->now_ms;l.host=r->actor_drop;l.input=&q;l.copy=q;r->entity_method_busy=1;
 if(!actor_drop_current(&l))goto retired;error[0]=0;ok=l.host.invoke(l.host.user,&q,error,sizeof(error),actor_drop_current,&l);error[sizeof(error)-1]=0;if(!actor_drop_current(&l))goto retired;r->entity_method_busy=0;return ok==1?1:rfail(r,line,"DropWeapon host refused: %s",error);
retired:r->entity_method_busy=0;return r->error[0]?0:rfail(r,line,"DropWeapon task/entity/resources retired");
}
