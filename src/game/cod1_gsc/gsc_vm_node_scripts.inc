/* GAME2FC49..2FCC4 resolves compiled main before trap96 publication; trap97
 * only retrieves that identity. Cache DATA never enables traverse execution. */
void gsc_runtime_node_scripts_retire(GscRuntime*r){
 if(!r)return;r->node_script_cache=0;r->node_script_count=0;memset(r->node_scripts,0,sizeof(r->node_scripts));
}
int gsc_runtime_node_scripts_build(GscRuntime*r,const GscNodeScriptNames*input){
 static uint64_t serial;GscNodeScriptNames names;FunctionIdentity identities[GSC_NODE_SCRIPT_CAPACITY];unsigned i,j;char module[128];int m,f;
 if(!r||!input||!gsc_runtime_storage_separate(r,input,sizeof(*input))||!r->program||!r->program->linked||
    !r->function_refs_enabled||!r->array_token||!r->program->function_program_token||r->error[0]||r->r243_time_root_teardown||
    r->started||r->advancing||r->executing_task||r->node_script_cache)return 0;
 names=*input;if(!names.count||names.count>GSC_NODE_SCRIPT_CAPACITY)return 0;
 memset(identities,0,sizeof(identities));
 for(i=0;i<names.count;i++){
  if(!gsc_node_script_name_valid(names.names[i]))return 0;
  for(j=0;j<i;j++)if(!strcmp(names.names[i],names.names[j]))return 0;
  snprintf(module,sizeof(module),"animscripts/traverse/%s",names.names[i]);m=find_module(r->program,module);
  if(m<0||(f=find_func(r->program,m,"main"))<0||r->program->funcs[f].module!=m)return 0;
  identities[i].program=r->program->function_program_token;identities[i].runtime=r->array_token;identities[i].module=m;identities[i].function=f;
 }
 if(serial==UINT64_MAX)return 0;
 /* No callbacks between final resolution and whole-table publication. */
 for(i=0;i<names.count;i++){memcpy(r->node_scripts[i].name,names.names[i],64);r->node_scripts[i].identity=identities[i];}
 r->node_script_count=names.count;r->node_script_cache=++serial;return 1;
}
int gsc_runtime_node_script_current(const GscRuntime*r,GscNodeScriptRef ref){
 const FunctionIdentity*id;
 if(!r||!r->program||!r->program->linked||r->error[0]||r->r243_time_root_teardown||!ref.cache||ref.cache!=r->node_script_cache||!ref.entry||ref.entry>r->node_script_count)return 0;
 id=&r->node_scripts[(unsigned)ref.entry-1].identity;
 return id->program&&id->program==r->program->function_program_token&&id->runtime&&id->runtime==r->array_token&&
  id->module>=0&&id->module<r->program->module_count&&id->function>=0&&id->function<r->program->func_count&&r->program->funcs[id->function].module==id->module;
}
GscNodeScriptRef gsc_runtime_node_script_lookup(const GscRuntime*r,const char*name){
 GscNodeScriptRef ref={0,0};unsigned i;
 if(!r||!name||!gsc_runtime_storage_separate(r,name,64)||!gsc_node_script_name_valid(name))return ref;
 for(i=0;i<r->node_script_count;i++)if(!strcmp(name,r->node_scripts[i].name)){
  ref.cache=r->node_script_cache;ref.entry=i+1;if(gsc_runtime_node_script_current(r,ref))return ref;break;
 }ref.cache=ref.entry=0;return ref;
}
