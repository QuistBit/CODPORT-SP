/* Authentic first entry root only. Inline children/recursive INIT frames do
 * not consume this separate local lease or obtain a controller grant. */
static int placed_return_bind(GscRuntime*r,PlacedInitReturnScope*l,Task*t,int fn,const GscEntityScriptEntry*e){
 if(!l||r->placed_return!=l||l->r!=r||l->bound||r->entity_entry_depth!=1||
    !t||t->depth!=1||t->frames[0].fn!=fn||t->frames[0].self.kind!=V_ENTITY||
    !state_ref_equal(&t->frames[0].self.entity,&l->certificate.owner)||
    !state_ref_equal(&e->entity,&l->certificate.owner)||
    strcmp(e->module,"animscripts/init")||strcmp(e->function,"main")||
    memcmp(&t->certificate,&l->certificate,sizeof(l->certificate))||
    memcmp(&t->map_certificate,&l->map,sizeof(l->map)))return rfail(r,0,"placed INIT entry root binding refused");
 l->root=t;l->serial=t->serial;l->fn=fn;l->bound=1;return 1;
}
static int placed_return_matches(GscRuntime*r,Task*t,int fn){PlacedInitReturnScope*l=r->placed_return;
 return l&&l->bound&&t==l->root&&t->serial==l->serial&&t->depth==1&&fn==l->fn;
}
static int placed_return_scope(PlacedInitReturnScope*l){GscRuntime*r=l?l->r:NULL;
 return r&&r->placed_return==l&&r->placed_init_initial_scope&&l->bound&&l->popped&&l->busy&&!l->completed&&
  !r->error[0]&&r->advancing&&r->executing_task==l->root&&l->root->status==TASK_READY&&
  l->root->serial==l->serial&&l->root->depth==0&&r->program==l->program&&
  r->array_token==l->instance&&r->animation_namespace.identity==l->catalog&&r->now_ms==l->now&&r237_bucket(r)==l->bucket&&
  r->execution_budget==l->budget&&l->budget&&*l->budget==l->budgetValue&&
  !memcmp(&l->request,&l->seal,sizeof(l->seal))&&
  !memcmp(&l->root->certificate,&l->certificate,sizeof(l->certificate))&&
  !memcmp(&l->root->map_certificate,&l->map,sizeof(l->map))&&!memcmp(&r->anim_data,&l->animMap,sizeof(l->animMap));
}
static int placed_return_current(void*context){PlacedInitReturnScope*l=context;GscRuntime*r=l?l->r:NULL;int ok;
 if(!placed_return_scope(l))return 0;if(l->checking)return rfail(r,0,"placed INIT pop current reentered");l->checking=1;
 ok=entity_task_current(r)&&placed_return_scope(l)&&anim_data_current(r,l->root)&&placed_return_scope(l)&&animation_namespace_scope(r)&&placed_return_scope(l);
 l->checking=0;return ok&&placed_return_scope(l);
}
static int placed_return_pop(GscRuntime*r,Task*t,int line){PlacedInitReturnScope*l=r->placed_return;Frame*f;GscPlacedInitReturnHost host;char error[384]={0};int ok;
 if(!l||l->popped||l->busy||l->completed||!l->host.invoke||!placed_return_matches(r,t,t->frames[t->depth-1].fn)||t->host_entry_pins==UINT_MAX||
    r->entity_method_busy||r->entity_field_busy||r->entity_prepare_busy||r->entity_certificate_busy||!r->execution_budget)
  return rfail(r,line,"placed INIT normal pop unavailable");
 f=&t->frames[0];if(strcmp(r->program->modules[r->program->funcs[f->fn].module],"animscripts/init")||strcmp(r->program->funcs[f->fn].name,"main")||f->self.kind!=V_ENTITY||!state_ref_equal(&f->self.entity,&l->certificate.owner))return rfail(r,line,"placed INIT canonical frame changed");
 l->program=r->program;l->instance=r->array_token;l->catalog=r->animation_namespace.identity;l->now=r->now_ms;l->bucket=r237_bucket(r);l->animMap=r->anim_data;
 l->budget=r->execution_budget;l->budgetValue=*l->budget;
 memset(&l->request,0,sizeof(l->request));l->request.entity=l->certificate.owner;l->request.runtime_identity=l->instance;l->request.catalog_identity=l->catalog;l->request.task_serial=l->serial;l->request.now_ms=l->now;l->seal=l->request;host=l->host;
 ++t->host_entry_pins;f->loop_count=0;--t->depth;l->popped=l->busy=1;
 ok=placed_return_current(l);
 if(ok)ok=host.invoke(host.user,&l->request,error,sizeof(error),placed_return_current,l);
 error[sizeof(error)-1]=0;
 if(!placed_return_current(l)||memcmp(&host,&l->host,sizeof(host)))ok=0;
 l->busy=0;--t->host_entry_pins;
 if(ok!=1)return rfail(r,line,"placed INIT actual pop callback refused: %s",error[0]?error:"owner retired");
 l->completed=1;return 1;
}
