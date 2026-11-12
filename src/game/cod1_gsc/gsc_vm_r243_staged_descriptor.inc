/* Private admitted-Program associations. Original-byte runtime provenance
 * and a real Session/native staged loader are separate, currently held work.
 * Legacy/default lanes never request or mint these descriptor identities. */
static int r243_controller_lane_selected(const GscRuntime*r){
 return r&&(r->r243_controller_enabled||r->r243s_profile.enabled);
}
static const char r243s_scripted_registration[9]="scripted";

static int r243s_program_descriptor_current(const GscProgram*g){
 const R243SProgramDescriptor*d;
 if(!g||!g->r243s_requested||!g->linked||g->invalid||
    !g->function_admission_selected||!g->function_ref_mode)return 0;
 d=&g->r243s_descriptor;
 if(d->program!=g||!d->program_token||d->program_token!=g->function_program_token||
    !d->admission_token||d->admission_token!=g->function_admission_token||
    !d->init_key||!d->scripted_key||d->init_key==d->scripted_key||
    memcmp(d->registration,r243s_scripted_registration,sizeof(d->registration))||
    d->funcs!=g->funcs||d->code!=g->code||d->expr!=g->expr||d->modules!=g->modules||
    d->func_count!=g->func_count||d->code_count!=g->code_count||
    d->expr_count!=g->expr_count||d->module_count!=g->module_count||
    d->init_module<0||d->init_module>=g->module_count||
    d->scripted_module<0||d->scripted_module>=g->module_count||
    d->init_fn<0||d->init_fn>=g->func_count||
    d->scripted_fn<0||d->scripted_fn>=g->func_count)return 0;
 return !strcmp(g->modules[d->init_module],"animscripts/init")&&
  !strcmp(g->modules[d->scripted_module],"animscripts/scripted")&&
  !memcmp(g->funcs+d->init_fn,&d->init_record,sizeof(Func))&&
  !memcmp(g->funcs+d->scripted_fn,&d->scripted_record,sizeof(Func))&&
  d->init_record.module==d->init_module&&d->scripted_record.module==d->scripted_module&&
  !strcmp(d->init_record.name,"main")&&!strcmp(d->scripted_record.name,"main")&&
  !d->init_record.argc&&!d->scripted_record.argc;
}
/* Private selected-load producer; NOT publicly callable from a host callback.
 * Atomic descriptor commit, no external callback; token gaps on exhaustion
 * are okay, but no token rollback/reuse. Must never be a live upgrade. */
static int r243s_program_descriptor_install(GscProgram*g){
 R243SProgramDescriptor d;char why[384];
 if(!g||!g->r243s_requested||!g->linked||g->invalid||g->r243s_descriptor.program||
    !g->function_ref_mode||!g->function_program_token||!g->function_admission_selected||
    !g->function_admission_token)return 0;
 memset(&d,0,sizeof(d));d.init_module=find_module(g,"animscripts/init");
 d.scripted_module=find_module(g,"animscripts/scripted");
 if(d.init_module<0||d.scripted_module<0)return 0;
 d.init_fn=find_func(g,d.init_module,"main");d.scripted_fn=find_func(g,d.scripted_module,"main");
 if(d.init_fn<0||d.scripted_fn<0||g->funcs[d.init_fn].argc||g->funcs[d.scripted_fn].argc||
    !function_admission_current(g,d.init_fn,why,sizeof(why))||
    !function_admission_current(g,d.scripted_fn,why,sizeof(why)))return 0;
 d.program=g;d.program_token=g->function_program_token;d.admission_token=g->function_admission_token;
 d.init_record=g->funcs[d.init_fn];d.scripted_record=g->funcs[d.scripted_fn];
 d.funcs=g->funcs;d.code=g->code;d.expr=g->expr;d.modules=g->modules;
 d.func_count=g->func_count;d.code_count=g->code_count;d.expr_count=g->expr_count;d.module_count=g->module_count;
 memcpy(d.registration,r243s_scripted_registration,sizeof(d.registration));
 if(!function_token_new(&d.init_key)||!function_token_new(&d.scripted_key))return 0;
 g->r243s_descriptor=d;return r243s_program_descriptor_current(g);
}
/* Only callable within a real fresh configured-load path. Full host/selector
 * eligibility and post-callback atomicity remain root's Session integration.
 * Other staged roles are not implemented by this draft and MUST stay HELD. */
static int r243s_runtime_bind_init_component(GscRuntime*r,const GscStagedScriptedControllerHostV2*h){
 R243SProfile p;const R243SProgramDescriptor*d;char why[384];
 if(!r||!h||!h->init_return||!r->program||!r243s_program_descriptor_current(r->program)||
    r->r243_controller_enabled||r->r243s_profile.enabled||r->started||r->task_count||
    r->advancing||r->executing_task||r->error[0]||!r->array_token||
    !r->animation_namespace_enabled||!r->animation_namespace.identity)return 0;
 d=&r->program->r243s_descriptor;
 if(!function_admission_runtime_current(r,d->init_fn,why,sizeof(why))||
    !function_admission_runtime_current(r,d->scripted_fn,why,sizeof(why)))return 0;
 memset(&p,0,sizeof(p));p.enabled=1;p.profile_identity=r->array_token;
 p.next_dispatch=1;p.descriptor=d;p.host=*h;
 r->r243s_profile=p;return 1;
}
static int r243s_profile_current(const GscRuntime*r){
 const R243SProfile*p;const R243SProgramDescriptor*d;
 if(!r||!r->program||r->r243_controller_enabled)return 0;p=&r->r243s_profile;
 if(!p->enabled||p->failed||p->profile_identity!=r->array_token||
    p->descriptor!=&r->program->r243s_descriptor||!r243s_program_descriptor_current(r->program))return 0;
 d=p->descriptor;return r->function_admission_token==d->admission_token&&
  r->function_admission[d->init_fn]&&r->function_admission[d->scripted_fn];
}
static void r243s_binding(const GscRuntime*r,GscStagedControllerProgramBinding*out){
 const R243SProgramDescriptor*d=r->r243s_profile.descriptor;
 memset(out,0,sizeof(*out));out->profile_identity=r->r243s_profile.profile_identity;
 out->program_identity=d->program_token;out->init_source=d->init_key;
 out->scripted_main_source=d->scripted_key;
}
