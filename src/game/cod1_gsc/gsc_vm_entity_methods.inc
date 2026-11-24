/* R170 private selected native model-method namespace. No registry entries,
 * legacy selector defaults or native import ABI are changed. */
int gsc_program_select_entity_model_methods(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->entity_model_methods_selected||
       !g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->entity_model_methods_selected=1;return 1;
}
int gsc_runtime_select_entity_methods(GscRuntime *r,const GscEntityMethodHost *host) {
    if(!r||!host||r->started||r->advancing||r->error[0]||r->entity_methods_enabled||
       !r->program->entity_model_methods_selected||!r->entities_enabled||
       !r->state_enabled||!r->scalar_switch_enabled||!r->names_enabled||
       !host->invoke||runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->entity_methods=*host;r->entity_methods_enabled=1;return 1;
}
int gsc_program_select_spawner_team_method(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->spawner_team_method_selected||!g->names_syntax||!g->scalar_switch_syntax)return 0;
    g->spawner_team_method_selected=1;return 1;
}
int gsc_runtime_select_spawner_team(GscRuntime *r,const GscSpawnerTeamMethodHost *host) {
    if(!r||!host||r->started||r->advancing||r->error[0]||r->spawner_team_enabled||
       !r->program->spawner_team_method_selected||!r->entities_enabled||!r->state_enabled||
       !r->scalar_switch_enabled||!r->names_enabled||!host->validate||!host->set||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->spawner_team=*host;r->spawner_team_enabled=1;return 1;
}
int gsc_runtime_spawner_team_available(const GscRuntime *r) {
    return r&&r->program->spawner_team_method_selected&&r->spawner_team_enabled&&!r->error[0];
}
static int entity_method_finish(GscRuntime *r,const GscEntityRef *owner,int line) {
    int alive;
    if(r->error[0])return -1;
    if(r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return -1;
        if(r->executing_task->status==TASK_DEAD)return -2;
    }
    alive=state_entity_live(r,owner);
    if(!entity_task_current(r))return r->error[0]?-1:-2;
    if(alive!=1) {
        rfail(r,line,"native method callback retired its receiver");return -1;
    }
    return 1;
}
#include "gsc_vm_player_natives.inc"
#include "gsc_vm_yaw_motion.inc"
#include "gsc_vm_attachment_queries.inc"
#include "gsc_vm_origin_method.inc"
#include "gsc_vm_vehicle_methods.inc"
#include "gsc_vm_entity_operations.inc"
#include "gsc_vm_entity_links.inc"
#include "gsc_vm_spatial_natives.inc"
#include "gsc_vm_entity_coordinates.inc"
#include "gsc_vm_animation_methods.inc"
#include "gsc_vm_actor_critical.inc"
#include "gsc_vm_r243_script_state.inc"
static int spawner_team_invoke(GscRuntime *r,const Value *receiver,const Value *args,int argc,int line) {
    GscEntityRef owner,borrowed;char team[128],error[384];int accepted,finished;
    if(!r->spawner_team_enabled||!receiver||receiver->kind!=V_ENTITY||argc<0||argc>MAX_PARAMS||(argc&&!args))
        return rfail(r,line,"spawner team capability or entity receiver is unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"spawner team callback reentered");
    owner=receiver->entity;borrowed=owner;r->entity_method_busy=1;
    finished=entity_method_finish(r,&owner,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
    error[0]=0;accepted=r->spawner_team.validate(r->spawner_team.user,&borrowed,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(accepted!=1){r->entity_method_busy=0;return rfail(r,line,"spawner team receiver refused: %s",error);}
    finished=entity_method_finish(r,&owner,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
    if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str))){
        r->entity_method_busy=0;return rfail(r,line,"spawner team GetString0 requires an admitted plain string");}
    memcpy(team,args[0].str,sizeof(team));borrowed=owner;error[0]=0;
    accepted=r->spawner_team.set(r->spawner_team.user,&borrowed,team,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(accepted!=1){r->entity_method_busy=0;return rfail(r,line,"spawner team setter refused: %s",error);}
    finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;return finished==1||finished==-2;
}
int gsc_program_select_script_sound(GscProgram *g) {
 if(!g||g->invalid||g->linked||g->script_sound_selected)return 0;
 g->script_sound_selected=1;return 1;
}
int gsc_runtime_select_script_sound(GscRuntime *r,const GscScriptSoundHost *host) {
 if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
    !r->program->script_sound_selected||r->script_sound_enabled||
    !r->entities_enabled||!r->state_enabled)return 0;
 r->script_sound=*host;r->script_sound_enabled=1;return 1;
}
int gsc_program_select_sound_notify(GscProgram *g) {
 if(!g||g->invalid||g->linked||!g->script_sound_selected||g->sound_notify_selected)return 0;
 g->sound_notify_selected=1;return 1;
}
int gsc_runtime_select_sound_notify(GscRuntime *r,const GscSoundNotifyHost *host) {
 if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||!r->program->sound_notify_selected||
    !r->script_sound_enabled||r->sound_notify_enabled||runtime_private_overlap(r,host,sizeof(*host)))return 0;
 r->sound_notify=*host;r->sound_notify_enabled=1;return 1;
}
static int script_sound_invoke(GscRuntime *r,const Value *receiver,const Value *args,int argc,int line) {
 GscEntityRef owner,borrowed;char alias[128],error[384];int finished,ok;
 if(!r->script_sound_enabled||!receiver||receiver->kind!=V_ENTITY||argc<0||argc>MAX_PARAMS)
  return rfail(r,line,"playsound capability or receiver unavailable");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
  return rfail(r,line,"playsound callback reentered");
 owner=receiver->entity;r->entity_method_busy=1;
 finished=entity_method_finish(r,&owner,line);
 if(finished<0){r->entity_method_busy=0;return finished==-2;}
 if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str))){
  r->entity_method_busy=0;return rfail(r,line,"playsound GetString0 requires plain string");}
 memcpy(alias,args[0].str,sizeof(alias));borrowed=owner;error[0]=0;
 if(argc>=2&&r->sound_notify_enabled){
  GscSoundNotifyRequest request,expected;
  memset(&request,0,sizeof(request));request.owner=owner;memcpy(request.alias,alias,sizeof(alias));request.argc=argc;
  if(argc==3&&args[2].kind==V_NUMBER&&isfinite(args[2].number)&&(args[2].number==0||args[2].number==1)){
   request.mode_valid=1;request.mode=(int)args[2].number;
  }
  if(argc==2)request.mode_valid=1;
  if(args[1].kind==V_STRING&&memchr(args[1].str,0,sizeof(args[1].str))){
   request.notify_valid=1;memcpy(request.notify,args[1].str,sizeof(request.notify));
  }
  expected=request;r->sound_notify_owner=owner;r->sound_notify_call_active=1;ok=r->sound_notify.invoke(r->sound_notify.user,&request,error,sizeof(error));r->sound_notify_call_active=0;
  if(memcmp(&request,&expected,sizeof(request)))ok=0;
 }else ok=r->script_sound.invoke(r->script_sound.user,&borrowed,alias,argc,error,sizeof(error));error[sizeof(error)-1]=0;
 if(r->error[0]){r->entity_method_busy=0;return 0;}
 if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
 if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"playsound refused: %s",error);}
 finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;
 return finished==1||finished==-2;
}
#include "gsc_vm_loop_sound.inc"
static int entity_method_invoke(GscRuntime *r,const Value *receiver,int descriptor,
    const Value *args,int argc,int line) {
    GscEntityRef owner,copied;GscStringCastValue values[MAX_PARAMS];
    char error[384];int consumed,n,ok,finished;uint32_t method;
    if(descriptor==504){
        if(r->program->carbine_items_selected!=2||!r->entity_fields_enabled||!receiver||receiver->kind!=V_ENTITY||argc!=1||!args||
           (args[0].kind!=V_LOCALIZED_STRING&&(args[0].kind!=V_STRING||args[0].str[0])))return rfail(r,line,"setHintString requires selected training and one localized string or empty clear");
        return entity_field_write(r,&receiver->entity,"cod1_hint_string",args,line);
    }
    if(descriptor==ENTITY_METHOD_SCRIPT_STATE)return r243_script_state_invoke(r,receiver,args,argc,line);
    if(descriptor==ENTITY_METHOD_PLAYLOOPSOUND||descriptor==ENTITY_METHOD_STOPLOOPSOUND)return loop_sound_invoke(r,receiver,descriptor,args,argc,line);
    if(descriptor==ENTITY_METHOD_PLAYSOUND)return script_sound_invoke(r,receiver,args,argc,line);
    if(descriptor>=ENTITY_METHOD_SETANIMKNOB&&descriptor<=ENTITY_METHOD_SETFLAGGEDANIMKNOBALL)return animation_method_invoke(r,receiver,descriptor,args,argc,line);
    if(descriptor>=ENTITY_METHOD_ENTERCRITICAL&&descriptor<=ENTITY_METHOD_ISCRITICAL)
        return actor_critical_invoke(r,receiver,descriptor,args,argc,line,NULL);
    if(descriptor==ENTITY_METHOD_SETSPAWNERTEAM)return spawner_team_invoke(r,receiver,args,argc,line);
    if(!r->entity_methods_enabled||!receiver||receiver->kind!=V_ENTITY||
       argc<0||argc>MAX_PARAMS||(argc&&!args)||
       descriptor<ENTITY_METHOD_ATTACH||descriptor>ENTITY_METHOD_DETACH_ALL)
        return rfail(r,line,"native entity method capability or receiver is unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"native entity method callback reentered");
    owner=receiver->entity;copied=owner;memset(values,0,sizeof(values));
    consumed=descriptor==ENTITY_METHOD_DETACH_ALL?0:(argc<2?argc:2);
    for(n=0;n<consumed;n++)if(!animation_value_exportable(r,&args[n])||!entity_field_export(&args[n],&values[n]))
        return rfail(r,line,"native method operand is outside the admitted domain");
    method=descriptor==ENTITY_METHOD_ATTACH?GSC_ENTITY_METHOD_ATTACH:
        descriptor==ENTITY_METHOD_DETACH?GSC_ENTITY_METHOD_DETACH:GSC_ENTITY_METHOD_DETACH_ALL;
    r->entity_method_busy=1;
    finished=entity_method_finish(r,&owner,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    memset(error,0,sizeof(error));
    ok=r->entity_methods.invoke(r->entity_methods.user,&copied,method,values,argc,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"native entity method refused: %s",error);}
    finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;
    return finished==1||finished==-2;
}
