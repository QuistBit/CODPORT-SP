/* Construction is separate from actor spawning and ordinary script_model.
 * No spawned notification: native GAME 31050 pushes the constructed entity. */
int gsc_program_select_vehicle_spawn(GscProgram*g){
 if(!g||g->invalid||g->linked||g->vehicle_spawn_selected)return 0;
 g->vehicle_spawn_selected=1;return 1;
}
int gsc_runtime_select_vehicle_spawn(GscRuntime*r,const GscVehicleSpawnHost*h){
 if(!r||!h||!h->prepare||!h->finish||r->started||r->advancing||r->error[0]||r->vehicle_spawn_enabled||
    !r->program->vehicle_spawn_selected||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->vehicle_spawn=*h;r->vehicle_spawn_enabled=1;return 1;
}
typedef struct {GscRuntime*r;GscVehicleSpawnHost host;int now;uint64_t program;} VehicleSpawnLease;
static int vehicle_spawn_current(void*ctx){
 VehicleSpawnLease*l=ctx;GscRuntime*r=l->r;
 return r->vehicle_spawn_busy&&r->now_ms==l->now&&gsc_runtime_program_identity(r)==l->program&&
  !memcmp(&r->vehicle_spawn,&l->host,sizeof(l->host))&&!r->error[0]&&console_context(r)==1;
}
static int vehicle_spawn_finish(VehicleSpawnLease*l,const GscVehicleSpawnTicket*input,int commit,int line){
 GscVehicleSpawnTicket ticket=*input,expected=*input;char error[384];int ok;
 error[0]=0;
 ok=l->host.finish(l->host.user,&ticket,commit,error,sizeof(error),vehicle_spawn_current,l);
 error[sizeof(error)-1]=0;
 if(memcmp(&ticket,&expected,sizeof(ticket)))return rfail(l->r,line,"vehicle factory changed its finalization ticket");
 if(ok!=1)return rfail(l->r,line,"vehicle factory finalization refused: %s",error[0]?error:"host failed");
 return 1;
}
static int vehicle_spawn_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 GscVehicleSpawnRequest q,expected;GscVehicleSpawnTicket produced,ticket;
 VehicleSpawnLease lease;char error[384];Value result;int n,k,ok,live;
 if(!r->vehicle_spawn_enabled||!out||!args||argc<5||argc>MAX_PARAMS)return rfail(r,line,"spawnVehicle capability/arguments unavailable");
 if(r->turret_precache_busy||r->vehicle_spawn_busy||r->turret_spawn_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"vehicle factory callback reentered");
 memset(&q,0,sizeof(q));
 for(n=0;n<3;n++)if(args[n].kind!=V_STRING||!memchr(args[n].str,0,128))return rfail(r,line,"spawnVehicle requires plain string getters 0..2");
 for(n=3;n<5;n++){
  if(args[n].kind!=V_VECTOR)return rfail(r,line,"spawnVehicle requires vector getters 3..4");
  for(k=0;k<3;k++)if(!isfinite(args[n].vector[k]))return rfail(r,line,"spawnVehicle vector is nonfinite");
 }
 memcpy(q.model,args[0].str,128);memcpy(q.targetname,args[1].str,128);memcpy(q.definition,args[2].str,128);
 for(k=0;k<3;k++){q.origin[k]=(float)args[3].vector[k];q.angles[k]=(float)args[4].vector[k];
  if(!isfinite(q.origin[k])||!isfinite(q.angles[k]))return rfail(r,line,"spawnVehicle vector exceeds binary32");}
 q.now_ms=r->now_ms;q.program=gsc_runtime_program_identity(r);expected=q;
 lease.r=r;lease.host=r->vehicle_spawn;lease.now=r->now_ms;lease.program=q.program;
 memset(&produced,0,sizeof(produced));error[0]=0;r->vehicle_spawn_busy=1;
 if(!vehicle_spawn_current(&lease)){r->vehicle_spawn_busy=0;return 0;}
 ok=lease.host.prepare(lease.host.user,&q,&produced,error,sizeof(error),vehicle_spawn_current,&lease);
 ticket=produced;error[sizeof(error)-1]=0;
 if(ok!=1||memcmp(&q,&expected,sizeof(q))||!vehicle_spawn_current(&lease)){
  rfail(r,line,"vehicle factory prepare refused or changed its request: %s",error[0]?error:"expired or invalid factory");goto cancel;
 }
 if(!ticket.operation||ticket.program!=q.program||!ticket.entity.epoch||!ticket.entity.generation){rfail(r,line,"vehicle factory returned invalid ticket");goto cancel;}
 live=state_entity_live(r,&ticket.entity);
 if(live!=1||!vehicle_spawn_current(&lease)){rfail(r,line,"vehicle factory returned stale entity");goto cancel;}
 if(!vehicle_spawn_finish(&lease,&ticket,1,line)||!vehicle_spawn_current(&lease))goto cancel;
 live=state_entity_live(r,&ticket.entity);
 if(live!=1||!vehicle_spawn_current(&lease)){rfail(r,line,"vehicle factory retired its result during commit");goto cancel;}
 memset(&result,0,sizeof(result));result.kind=V_ENTITY;result.entity=ticket.entity;
 r->vehicle_spawn_busy=0;*out=result;return 1;
cancel:
 if(ticket.operation)(void)vehicle_spawn_finish(&lease,&ticket,0,line);
 r->vehicle_spawn_busy=0;return 0;
}
