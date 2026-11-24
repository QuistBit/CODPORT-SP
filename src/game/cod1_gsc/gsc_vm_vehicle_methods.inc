int gsc_program_select_vehicle_methods(GscProgram*g){if(!g||g->invalid||g->linked||g->vehicle_methods_selected)return 0;g->vehicle_methods_selected=1;return 1;}
int gsc_runtime_select_vehicle_methods(GscRuntime*r,const GscVehicleMethodHost*h){
 if(!r||!h||!h->invoke||r->started||r->advancing||r->error[0]||r->vehicle_methods_enabled||!r->program->vehicle_methods_selected||!r->entities_enabled||!r->state_enabled||!r->vehicle_nodes_enabled||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->vehicle_methods=*h;r->vehicle_methods_enabled=1;return 1;
}
typedef struct {GscRuntime*r;GscVehicleMethodHost host;uint64_t program;int now;} VehicleMethodLease;
static int vehicle_method_current(void*u){VehicleMethodLease*l=u;GscRuntime*r=l->r;return r->entity_method_busy&&!r->error[0]&&r->now_ms==l->now&&gsc_runtime_program_identity(r)==l->program&&!memcmp(&r->vehicle_methods,&l->host,sizeof(l->host))&&entity_task_current(r);}
static int vehicle_method_invoke(GscRuntime*r,const Value*receiver,int descriptor,const Value*args,int argc,int line){
 GscVehicleMethodRequest q,expected;VehicleMethodLease l;char error[384];int kind=vehicle_method_kind(descriptor),k,ok,finished;
 if(!r->vehicle_methods_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||!entity_method_argc_valid(descriptor,argc)||(argc&&!args))return rfail(r,line,"vehicle method receiver/capability/arity unavailable");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy||r->vehicle_spawn_busy)return rfail(r,line,"vehicle method callback reentered");
 memset(&q,0,sizeof(q));q.owner=receiver->entity;q.program=gsc_runtime_program_identity(r);q.now_ms=r->now_ms;q.kind=kind;
 r->entity_method_busy=1;finished=entity_method_finish(r,&q.owner,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
 if(kind==1||kind==3){
  if(args[0].kind!=V_VEHICLE_NODE){rfail(r,line,"vehicle path method requires class3 node");goto failed;}
  q.node=args[0].vehicle_node;
 }else if(kind!=2)for(k=0;k<(kind==5?2:1);k++){
  if(args[k].kind!=V_NUMBER||!isfinite(args[k].number)){rfail(r,line,"vehicle speed method requires finite number");goto failed;}
  q.values[k]=(float)args[k].number;if(!isfinite(q.values[k])){rfail(r,line,"vehicle speed getter exceeds binary32");goto failed;}
 }
 expected=q;l.r=r;l.host=r->vehicle_methods;l.program=q.program;l.now=q.now_ms;error[0]=0;
 if(!vehicle_method_current(&l))goto failed;
 ok=l.host.invoke(l.host.user,&q,error,sizeof(error),vehicle_method_current,&l);error[sizeof(error)-1]=0;
 if(ok!=1||memcmp(&q,&expected,sizeof(q))||!vehicle_method_current(&l)){rfail(r,line,"vehicle method refused or expired: %s",error[0]?error:"host boundary");goto failed;}
 finished=entity_method_finish(r,&expected.owner,line);r->entity_method_busy=0;return finished==1||finished==-2;
failed:r->entity_method_busy=0;return 0;
}
