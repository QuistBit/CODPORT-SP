/* Common constructor orchestration uses the restricted copied service table.
 * Legacy entry points construct this private adapter around their existing
 * runtime argument; a Session provider never receives that pointer. */
static int ActorLegacyCurrent(void *context){return context!=NULL;}
static uint64_t ActorLegacyProgram(void *context){return gsc_runtime_program_identity(context);}
static int ActorLegacyFunction(void *context,const char *module,const char *function){return gsc_runtime_script_function_available(context,module,function);}
static int ActorLegacyStorage(void *context,const void *p,size_t n){return gsc_runtime_storage_separate(context,p,n);}
static int ActorLegacyAvailable(void *context,int kind){
    return kind==GSC_ACTOR_BOOTSTRAP_AVAILABLE_RESOURCES?gsc_runtime_resource_bootstrap_available(context):
        kind==GSC_ACTOR_BOOTSTRAP_AVAILABLE_SPAWNER_TEAM?gsc_runtime_spawner_team_available(context):0;
}
static int ActorLegacyError(void *context,char *error,size_t size){
    const char *text;if(!context||!error||!size)return 0;text=gsc_runtime_error(context);EntityError(error,size,text);return 1;
}
static int ActorLegacyHooks(void *context,const GscEntityHookBootstrapEntry *entries,size_t count,
    const GscEntityHookBootstrapHost *host,GscEntityHookBootstrapResult *out,char *error,size_t size){
    return gsc_start_entity_hooks(context,entries,count,host,out,error,size);
}
static int ActorLegacyEntry(void *context,const GscPreparedEntityEntry *entry,const GscPreparedEntityEntryHost *host,
    GscPreparedEntityEntryRun *out,char *error,size_t size){return gsc_execute_prepared_entity_entry(context,entry,host,out,error,size);}
static GscActorBootstrapServices ActorLegacyServices(const GscRuntime *runtime){
    GscActorBootstrapServices s;memset(&s,0,sizeof(s));s.context=(void *)runtime;s.current=ActorLegacyCurrent;
    s.program_identity=ActorLegacyProgram;s.script_function_available=ActorLegacyFunction;s.storage_separate=ActorLegacyStorage;
    s.available=ActorLegacyAvailable;s.copy_error=ActorLegacyError;s.start_entity_hooks=ActorLegacyHooks;
    s.execute_prepared_entity_entry=ActorLegacyEntry;return s;
}
static int ActorServicesValid(const GscActorBootstrapServices *s){
    return s&&s->context&&s->current&&s->program_identity&&s->script_function_available&&s->storage_separate&&
        s->available&&s->copy_error&&s->start_entity_hooks&&s->execute_prepared_entity_entry&&s->current(s->context)==1;
}
static int ActorServicesFailed(const GscActorBootstrapServices *s){
    char error[256];error[0]=0;return !s->copy_error(s->context,error,sizeof(error))||error[0];
}
