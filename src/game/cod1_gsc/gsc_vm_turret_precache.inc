/* Native311D0 gets string0 BEFORE checking the initial-level phase. Unknown
 * names are a loaded-weapon lookup miss, not an item-name-resolution error.
 * The selected domain is plain printable strings. Extra expressions evaluate. */
int gsc_program_select_turret_precache(GscProgram*g){if(!g||g->invalid||g->linked||g->turret_precache_selected)return 0;g->turret_precache_selected=1;return 1;}
int gsc_runtime_select_turret_precache(GscRuntime*r,const GscTurretPrecacheHost*h){
 if(!r||!h||!h->invoke||r->started||r->advancing||r->error[0]||r->turret_precache_enabled||!r->precache_item_enabled||!r->program->turret_precache_selected||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->turret_precache=*h;r->turret_precache_enabled=1;return 1;
}
typedef struct {GscRuntime*r;GscTurretPrecacheHost host;int now;uint64_t program;} TurretPrecacheLease;
static int turret_precache_current(void*ctx){TurretPrecacheLease*l=ctx;GscRuntime*r=l->r;
 return r->turret_precache_busy&&!r->error[0]&&r->precache_item_initial_window&&r->now_ms==l->now&&gsc_runtime_program_identity(r)==l->program&&!memcmp(&r->turret_precache,&l->host,sizeof(l->host))&&console_context(r)==1;
}
static int turret_precache_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 GscTurretPrecacheRequest q,expected;TurretPrecacheLease l;char error[384];unsigned i;int ok;
 if(!r->turret_precache_enabled||!out||argc<0||argc>MAX_PARAMS)return rfail(r,line,"precacheTurret capability unavailable");
 if(argc<1||!args||args[0].kind!=V_STRING||!memchr(args[0].str,0,128))return rfail(r,line,"precacheTurret requires plain string getter0");
 memset(&q,0,sizeof(q));for(i=0;args[0].str[i];i++)if((unsigned char)args[0].str[i]<32||(unsigned char)args[0].str[i]>126)return rfail(r,line,"precacheTurret string outside printable domain");
 memcpy(q.name,args[0].str,128);
 if(!r->precache_item_initial_window)return rfail(r,line,"precacheTurret must be called before any wait statements in the level script");
 if(r->turret_precache_busy||r->vehicle_spawn_busy||r->turret_spawn_busy||r->entity_field_busy||r->entity_method_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"precacheTurret callback reentered");
 if(!r->next_precache_item_operation||r->next_precache_item_operation==UINT64_MAX)return rfail(r,line,"shared weapon/item registration operation exhausted");
 q.operation=r->next_precache_item_operation++;q.program=gsc_runtime_program_identity(r);q.now_ms=r->now_ms;expected=q;
 l.r=r;l.host=r->turret_precache;l.now=r->now_ms;l.program=q.program;r->turret_precache_busy=1;error[0]=0;
 if(!turret_precache_current(&l)){r->turret_precache_busy=0;return 0;}
 ok=l.host.invoke(l.host.user,&q,error,sizeof(error),turret_precache_current,&l);error[sizeof(error)-1]=0;
 if(ok!=1||memcmp(&q,&expected,sizeof(q))||!turret_precache_current(&l)){r->turret_precache_busy=0;return rfail(r,line,"precacheTurret registration refused: %s",error[0]?error:"owner or request changed");}
 r->turret_precache_busy=0;memset(out,0,sizeof(*out));return 1;
}
