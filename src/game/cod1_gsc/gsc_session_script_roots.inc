/* Native GAME30050:2FE20 globals,2F940 map, then entity declarations.
 * This program-owned DATA cache does not admit any root for execution.
 * Port name handles index actual owned interned strings; native global string
 * allocator sharing/save lifetime remains a separate contract. */
static int session_roots_overlap(const void*a,size_t na,const void*b,size_t nb){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int session_script_roots_current(GscSession*s){
 GscSessionScriptRootsPlan copy;int ok;
 if(!s||s->abort_requested)return 0;if(!s->script_roots_enabled)return 1;
 if(s->script_roots_busy){s->abort_requested=1;session_terminal(s,"global/map declaration reentered");return 0;}
 copy=s->script_roots;s->script_roots_busy=1;ok=copy.current(copy.user,copy.epoch,copy.receipt);s->script_roots_busy=0;
 return ok==1&&!s->abort_requested&&!memcmp(&copy,&s->script_roots,sizeof(copy));
}
int gsc_session_select_script_roots(GscSession*s,const GscSessionScriptRootsPlan*input,char*error,size_t cap){
 GscSessionScriptRootsPlan copy;char raw[128],module[128];size_t n;int mp;
 if(!s||!input||session_roots_overlap(input,sizeof(*input),s,sizeof(*s))||
    session_roots_overlap(error,cap,s,sizeof(*s))||session_roots_overlap(error,cap,input,sizeof(*input)))return 0;
 if(s->busy||s->program||s->runtime||s->view.error[0]||s->script_roots_enabled||!s->modules_enabled||!s->function_refs_enabled)
  return session_reject(error,cap,"global/map roots require idle function-reference module Session");
 copy=*input;if(!copy.epoch||!copy.receipt||!copy.current||!memchr(copy.mapname,0,sizeof(copy.mapname))||!copy.mapname[0])
  return session_reject(error,cap,"global/map roots require bounded map identity and current declarations");
 n=strlen(copy.mapname);mp=n>=3&&(copy.mapname[0]=='m'||copy.mapname[0]=='M')&&(copy.mapname[1]=='p'||copy.mapname[1]=='P')&&(copy.mapname[2]=='/'||copy.mapname[2]=='\\');
 module[0]=0;
 if(!mp){
  /* Preserve full identity; refuse names the native64-byte maps/ buffer would truncate. */
  if(n+5>=64)return session_reject(error,cap,"map script path exceeds exact native buffer identity");
  snprintf(raw,sizeof(raw),"maps/%s",copy.mapname);
  if(!gsc_module_identity_canonical(raw,module,sizeof(module)))return session_reject(error,cap,"invalid map script module identity");
 }
 if(!session_enter(s,error,cap))return 0;
 s->script_roots=copy;strcpy(s->script_map_module,module);s->script_roots_enabled=1;
 if(!session_script_roots_current(s))return session_failed(s,"global/map declarations unavailable",error,cap);
 s->busy=0;return 1;
}
static uint16_t session_script_name_intern(char pool[GSC_GLOBAL_SCRIPT_ROOT_COUNT][64],unsigned*count,const char*name){
 unsigned n;size_t z;if(!pool||!count||!name||*count>GSC_GLOBAL_SCRIPT_ROOT_COUNT)return 0;
 for(z=0;z<64&&name[z];z++){}if(!z||z==64)return 0;
 for(n=0;n<*count;n++)if(!strcmp(pool[n],name))return (uint16_t)(n+1);
 if(n==GSC_GLOBAL_SCRIPT_ROOT_COUNT)return 0;
 memcpy(pool[n],name,z+1);*count=n+1;return (uint16_t)(n+1);
}
static int session_script_roots_build(GscSession*s,char*error,size_t cap){
 char pool[GSC_GLOBAL_SCRIPT_ROOT_COUNT][64],module[128];uint32_t functions[GSC_GLOBAL_SCRIPT_ROOT_COUNT],init,map;
 uint16_t names[GSC_GLOBAL_SCRIPT_ROOT_COUNT];unsigned n,count=0;uint64_t program;
 if(!s||!s->script_roots_enabled||s->script_roots_ready||s->runtime||!(program=gsc_program_identity(s->program)))
  return session_reject(error,cap,"global/map cache requires fresh linked program");
 memset(pool,0,sizeof(pool));
 for(n=0;n<GSC_GLOBAL_SCRIPT_ROOT_COUNT;n++){
  snprintf(module,sizeof(module),"animscripts/%s",gsc_global_script_names[n]);
  functions[n]=gsc_program_script_function_handle(s->program,module,"main");
  if(!functions[n]){if(error&&cap)snprintf(error,cap,"global script cache missing %s::main",module);return 0;}
  names[n]=session_script_name_intern(pool,&count,gsc_global_script_names[n]);if(!names[n])return session_reject(error,cap,"global script name pool unavailable");
 }
 init=gsc_program_script_function_handle(s->program,"animscripts/scripted","init");
 if(!init)return session_reject(error,cap,"global script cache missing animscripts/scripted::init");
 /* Native2F940 stores a possibly-zero map main and skips MP prefixes. */
 map=s->script_map_module[0]?gsc_program_script_function_handle(s->program,s->script_map_module,"main"):0;
 memcpy(s->script_name_pool,pool,sizeof(pool));s->script_name_count=count;
 for(n=0;n<GSC_GLOBAL_SCRIPT_ROOT_COUNT;n++){s->script_root_cache[n].function=functions[n];s->script_root_cache[n].name=names[n];s->script_root_cache[n].padding=0;}
 s->script_init_function=init;s->script_map_function=map;s->script_roots_program=program;s->script_roots_ready=1;return 1;
}
static int session_script_roots_cache_current(const GscSession*s){
 unsigned n;char module[128];uint16_t name;
 if(!s)return 0;if(!s->script_roots_enabled)return 1;
 if(!s->script_roots_ready||!s->script_roots_program||s->script_roots_program!=gsc_program_identity(s->program)||
    s->script_name_count!=GSC_GLOBAL_SCRIPT_ROOT_COUNT||
    (s->runtime&&s->script_roots_program!=gsc_runtime_program_identity(s->runtime)))return 0;
 for(n=0;n<GSC_GLOBAL_SCRIPT_ROOT_COUNT;n++){
  name=s->script_root_cache[n].name;
  if(!name||name>s->script_name_count||strcmp(s->script_name_pool[name-1],gsc_global_script_names[n]))return 0;
  snprintf(module,sizeof(module),"animscripts/%s",gsc_global_script_names[n]);
  if(!s->script_root_cache[n].function||s->script_root_cache[n].function!=gsc_program_script_function_handle(s->program,module,"main"))return 0;
 }
 return s->script_init_function&&s->script_init_function==gsc_program_script_function_handle(s->program,"animscripts/scripted","init")&&
  s->script_map_function==(s->script_map_module[0]?gsc_program_script_function_handle(s->program,s->script_map_module,"main"):0);
}
