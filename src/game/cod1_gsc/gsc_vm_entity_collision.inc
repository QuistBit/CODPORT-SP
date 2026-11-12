/* R200 narrow notsolid() boundary. The provider must commit actual collision
 * state; an ordinary visibility mutation cannot satisfy this operation. */
int gsc_program_select_entity_collision(GscProgram *g){
 if(!g||g->invalid||g->linked||g->entity_collision_selected)return 0;
 g->entity_collision_selected=1;return 1;
}
int gsc_runtime_select_entity_collision(GscRuntime *r,const GscEntityCollisionHost *host){
 if(!r||!host||!host->notsolid||r->started||r->advancing||r->executing_task||r->error[0]||
    r->entity_collision_enabled||!r->program->entity_collision_selected||!r->entities_enabled||!r->state_enabled||
    r->entity_method_busy||r->entity_field_busy||r->entity_prepare_busy||r->entity_certificate_busy||
    runtime_private_overlap(r,host,sizeof(*host)))return 0;
 r->entity_collision=*host;r->entity_collision_enabled=1;return 1;
}
static int entity_collision_invoke(GscRuntime *r,const Value *receiver,int argc,int line){
 GscEntityRef owner,copy;GscEntityCollisionHost host;char error[256];int ok,finished;
 if(!r->entity_collision_enabled||!receiver||receiver->kind!=V_ENTITY||argc)
  return rfail(r,line,"notsolid requires a current entity and selected collision service");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)
  return rfail(r,line,"collision callback reentered");
 owner=receiver->entity;host=r->entity_collision;r->entity_method_busy=1;
 finished=entity_method_finish(r,&owner,line);
 if(finished<0){r->entity_method_busy=0;return finished==-2;}
 copy=owner;error[0]=0;
 ok=host.notsolid(host.user,&copy,r->now_ms,error,sizeof(error),player_native_current,r);error[sizeof(error)-1]=0;
 if(r->error[0]){r->entity_method_busy=0;return 0;}
 if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
 if(ok!=1||memcmp(&host,&r->entity_collision,sizeof(host))||!entity_ref_equal(&owner,&copy)){
  r->entity_method_busy=0;return rfail(r,line,"notsolid refused or changed its service/identity: %s",error[0]?error:"invalid callback result");
 }
 finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;
 return finished>=0||finished==-2;
}
