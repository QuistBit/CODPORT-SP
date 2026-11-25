/* Three actual function handles per aitype, in native main/precache/spawner
 * order. Other implicit roots (including turret animscripts) are not aitypes.
 * Declaration/map admission remains independent from these compiled DATA. */
static const char *const session_actor_cache_labels[3]={"main","precache","spawner"};
static int session_actor_cache_build(GscSession*s,char*error,size_t cap){
 uint32_t staged[GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY][3];unsigned n,k;uint64_t program;
 if(!s||!s->actor_program_enabled||s->actor_cache_ready||s->runtime||
    !(program=gsc_program_identity(s->program)))return session_reject(error,cap,"actor cache requires fresh linked program");
 memset(staged,0,sizeof(staged));
 for(n=0;n<s->actor_program.module_count;n++)if(!strncmp(s->actor_program.module_roots[n],"aitype/",7)){
  gsc_program_script_function_handles(s->program,s->actor_program.module_roots[n],session_actor_cache_labels,staged[n],3);
  for(k=0;k<3;k++)if(!staged[n][k]){
   if(error&&cap)snprintf(error,cap,"actor program cache missing %s::%s",s->actor_program.module_roots[n],session_actor_cache_labels[k]);return 0;
  }
 }
 /* Resolve the complete cohort before publishing; no callbacks here. */
 memcpy(s->actor_function_cache,staged,sizeof(staged));s->actor_cache_program=program;s->actor_cache_ready=1;return 1;
}
static int session_actor_cache_current(const GscSession*s){
 unsigned n,k;
 if(!s)return 0;if(!s->actor_program_enabled)return 1;
 if(!s->actor_cache_ready||!s->actor_cache_program||s->actor_cache_program!=gsc_program_identity(s->program)||
    (s->runtime&&s->actor_cache_program!=gsc_runtime_program_identity(s->runtime)))return 0;
 for(n=0;n<s->actor_program.module_count;n++){
  uint32_t expected[3]={0,0,0};
  int aitype=!strncmp(s->actor_program.module_roots[n],"aitype/",7);
  if(aitype)gsc_program_script_function_handles(s->program,s->actor_program.module_roots[n],session_actor_cache_labels,expected,3);
  for(k=0;k<3;k++)if(s->actor_function_cache[n][k]!=expected[k]||(!expected[k]&&aitype))return 0;
 }
 return 1;
}
static int session_actor_cached_function(const GscSession*s,const char*module,const char*function){
 unsigned n,k;
 if(!s||!module||!function||!session_actor_cache_current(s))return 0;
 if(!s->actor_program_enabled||strncmp(module,"aitype/",7))return 1;
 for(n=0;n<s->actor_program.module_count;n++)if(!strcmp(module,s->actor_program.module_roots[n])){
  for(k=0;k<3;k++)if(!strcmp(function,session_actor_cache_labels[k]))
   return s->actor_function_cache[n][k]!=0; /* Full cache validation above, no intervening callbacks. */
  return 1;
 }
 return 0;
}
