/* Construction is separate from actor spawning and ordinary script_model.
 * No spawned notification: native GAME 31150 pushes the constructed entity. */
int gsc_program_select_turret_spawn(GscProgram*g){
 if(!g||g->invalid||g->linked||g->turret_spawn_selected)return 0;
 g->turret_spawn_selected=1;return 1;
}
int gsc_runtime_select_turret_spawn(GscRuntime*r,const GscTurretSpawnHost*h){
 if(!r||!h||!h->prepare||!h->finish||r->started||r->advancing||r->error[0]||r->turret_spawn_enabled||
    !r->program->turret_spawn_selected||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->turret_spawn=*h;r->turret_spawn_enabled=1;return 1;
}
typedef struct {GscRuntime*r;GscTurretSpawnHost host;int now;uint64_t program;} TurretSpawnLease;
static int turret_spawn_current(void*ctx){
 TurretSpawnLease*l=ctx;GscRuntime*r=l->r;
 return r->turret_spawn_busy&&r->now_ms==l->now&&gsc_runtime_program_identity(r)==l->program&&
  !memcmp(&r->turret_spawn,&l->host,sizeof(l->host))&&!r->error[0]&&console_context(r)==1;
}
static int turret_spawn_finish(TurretSpawnLease*l,const GscTurretSpawnTicket*input,int commit,int line){
 GscTurretSpawnTicket ticket=*input,expected=*input;char error[384];int ok;
 error[0]=0;
 ok=l->host.finish(l->host.user,&ticket,commit,error,sizeof(error),turret_spawn_current,l);
 error[sizeof(error)-1]=0;
 if(memcmp(&ticket,&expected,sizeof(ticket)))return rfail(l->r,line,"turret factory changed its finalization ticket");
 if(ok!=1)return rfail(l->r,line,"turret factory finalization refused: %s",error[0]?error:"host failed");
 return 1;
}
static int turret_spawn_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 GscTurretSpawnRequest q,expected;GscTurretSpawnTicket produced,ticket;
 TurretSpawnLease lease;char error[384];Value result;int n,k,ok,live;
 if(!r->turret_spawn_enabled||!out||!args||argc<3||argc>MAX_PARAMS)return rfail(r,line,"spawnTurret capability/arguments unavailable");
 if(r->turret_precache_busy||r->turret_spawn_busy||r->vehicle_spawn_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||
    r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"turret factory callback reentered");
 memset(&q,0,sizeof(q));
 if(args[0].kind!=V_STRING||args[2].kind!=V_STRING||!memchr(args[0].str,0,128)||!memchr(args[2].str,0,128))return rfail(r,line,"spawnTurret requires plain string getters 0 and 2");
 if(args[1].kind!=V_VECTOR)return rfail(r,line,"spawnTurret requires vector getter 1");
 memcpy(q.classname,args[0].str,128);memcpy(q.weapon,args[2].str,128);
 for(k=0;k<3;k++){if(!isfinite(args[1].vector[k]))return rfail(r,line,"spawnTurret vector is nonfinite");q.origin[k]=(float)args[1].vector[k];if(!isfinite(q.origin[k]))return rfail(r,line,"spawnTurret vector exceeds binary32");}
 if(!r->next_precache_item_operation||r->next_precache_item_operation==UINT64_MAX)return rfail(r,line,"shared weapon/item registration operation exhausted");
 q.precache_operation=r->next_precache_item_operation++;
 q.now_ms=r->now_ms;q.program=gsc_runtime_program_identity(r);expected=q;
 lease.r=r;lease.host=r->turret_spawn;lease.now=r->now_ms;lease.program=q.program;
 memset(&produced,0,sizeof(produced));error[0]=0;r->turret_spawn_busy=1;
 if(!turret_spawn_current(&lease)){r->turret_spawn_busy=0;return 0;}
 ok=lease.host.prepare(lease.host.user,&q,&produced,error,sizeof(error),turret_spawn_current,&lease);
 ticket=produced;error[sizeof(error)-1]=0;
 if(ok!=1||memcmp(&q,&expected,sizeof(q))||!turret_spawn_current(&lease)){
  rfail(r,line,"turret factory prepare refused or changed its request: %s",error[0]?error:"expired or invalid factory");goto cancel;
 }
 if(!ticket.operation||ticket.program!=q.program||!ticket.entity.epoch||!ticket.entity.generation){rfail(r,line,"turret factory returned invalid ticket");goto cancel;}
 live=state_entity_live(r,&ticket.entity);
 if(live!=1||!turret_spawn_current(&lease)){rfail(r,line,"turret factory returned stale entity");goto cancel;}
 if(!turret_spawn_finish(&lease,&ticket,1,line)||!turret_spawn_current(&lease))goto cancel;
 live=state_entity_live(r,&ticket.entity);
 if(live!=1||!turret_spawn_current(&lease)){rfail(r,line,"turret factory retired its result during commit");goto cancel;}
 memset(&result,0,sizeof(result));result.kind=V_ENTITY;result.entity=ticket.entity;
 r->turret_spawn_busy=0;*out=result;return 1;
cancel:
 if(ticket.operation)(void)turret_spawn_finish(&lease,&ticket,0,line);
 r->turret_spawn_busy=0;return 0;
}
