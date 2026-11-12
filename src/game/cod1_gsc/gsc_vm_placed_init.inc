/* A fixed canonical initial invocation, not the factory-issued spawn lane.
 * Exact Session/cache/row admission happens above. Map and Actor authorities
 * are copied into the root and its real children; no facade pointer survives. */
static int execute_placed_init_selected(GscRuntime*r,const GscEntityRef*input,
 const GscEntityExecutionCertificate*authority,const GscMapExecutionCertificate*mapInput,
 const GscPlacedInitReturnHost*hostInput,int cohort,GscPlacedInitRun*out,char*error,size_t cap){
 GscEntityRef row;GscEntityExecutionCertificate cert;GscMapExecutionCertificate map;
 GscEntityScriptEntry entry;GscPlacedInitRun result;PlacedInitReturnScope pop;GscPlacedInitReturnHost hostCopy;uint64_t instance,program,catalog;int ok;
 if(!r||!input||!authority||!mapInput||!out||
    (hostInput&&(!gsc_runtime_storage_separate(r,hostInput,sizeof(*hostInput))||
     Cod1ModelStringOverlap(hostInput,sizeof(*hostInput),out,sizeof(*out))||Cod1ModelStringOverlap(hostInput,sizeof(*hostInput),error,cap)||
     Cod1ModelStringOverlap(hostInput,sizeof(*hostInput),input,sizeof(*input))||Cod1ModelStringOverlap(hostInput,sizeof(*hostInput),authority,sizeof(*authority))||Cod1ModelStringOverlap(hostInput,sizeof(*hostInput),mapInput,sizeof(*mapInput))))||!gsc_runtime_storage_separate(r,input,sizeof(*input))||
    !gsc_runtime_storage_separate(r,authority,sizeof(*authority))||!gsc_runtime_storage_separate(r,mapInput,sizeof(*mapInput))||
    !gsc_runtime_storage_separate(r,out,sizeof(*out))||!gsc_runtime_storage_separate(r,error,cap)||
    Cod1ModelStringOverlap(out,sizeof(*out),input,sizeof(*input))||Cod1ModelStringOverlap(out,sizeof(*out),authority,sizeof(*authority))||
    Cod1ModelStringOverlap(out,sizeof(*out),mapInput,sizeof(*mapInput))||Cod1ModelStringOverlap(error,cap,input,sizeof(*input))||
    Cod1ModelStringOverlap(error,cap,authority,sizeof(*authority))||Cod1ModelStringOverlap(error,cap,mapInput,sizeof(*mapInput))||
    Cod1ModelStringOverlap(error,cap,out,sizeof(*out)))return 0;
 memset(&hostCopy,0,sizeof(hostCopy));if(hostInput)hostCopy=*hostInput;
 row=*input;cert=*authority;map=*mapInput;memset(&result,0,sizeof(result));*out=result;
 if(!cert.serial||!cert.current||!state_ref_equal(&cert.owner,&row)||!map.receipt||!map.current||
    !r->started||r->error[0]||r->advancing||r->executing_task||r->entity_entry_depth||r->entity_sequence_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
    r->r243_controller_enabled||r->placed_return||r->placed_init_initial_scope||(!cohort&&gsc_active_threads(r)))
  return inherited_entry_error(error,cap,"placed INIT requires quiescent first-row scope; owned controller return profile remains open");
 instance=gsc_runtime_instance_identity(r);program=gsc_runtime_program_identity(r);catalog=r->animation_namespace.identity;
 if(!instance||!program||!catalog||!gsc_runtime_script_function_available(r,"animscripts/init","main"))
  return inherited_entry_error(error,cap,"placed INIT original root or animation namespace unavailable");
 memset(&entry,0,sizeof(entry));entry.entity=row;strcpy(entry.module,"animscripts/init");strcpy(entry.function,"main");
 memset(&pop,0,sizeof(pop));
 if(hostInput){
  if(!gsc_runtime_storage_separate(r,hostInput,sizeof(*hostInput))||!hostInput->invoke||!r->r243_thread_start_enabled||!r->r237_frame_buckets_enabled)return inherited_entry_error(error,cap,"placed return requires genuine native inline/frame profile");
  pop.r=r;pop.host=hostCopy;pop.certificate=cert;pop.map=map;r->placed_return=&pop;
 }
 r->entity_sequence_busy=1;r->placed_init_initial_scope=1;
 ok=execute_entity_values_owned_map_r243(r,&entry,&cert,&map,NULL,0,&result.entry,NULL,hostInput?&pop:NULL);
 r->placed_return=NULL;r->placed_init_initial_scope=0;r->entity_sequence_busy=0;
 result.entry_scope_released=!r->executing_task&&!r->advancing&&!r->entity_entry_depth&&!r->execution_budget;
 if(hostInput){result.root_pop_observed=pop.completed;if(!pop.completed||!pop.popped||!result.entry_scope_released||result.entry.status!=GSC_ENTITY_ENTRY_RETURNED)ok=rfail(r,0,"placed INIT did not complete its actual root pop");}
 memset(&pop,0,sizeof(pop));
 result.runtime_identity=instance;result.program_identity=program;result.catalog_identity=catalog;
 result.active_children=(uint32_t)gsc_active_threads(r);*out=result;
 if((hostInput&&memcmp(hostInput,&hostCopy,sizeof(hostCopy)))||!ok||r->error[0]||result.entry.status==GSC_ENTITY_ENTRY_CANCELLED||
    gsc_runtime_instance_identity(r)!=instance||gsc_runtime_program_identity(r)!=program||r->animation_namespace.identity!=catalog||
    memcmp(input,&row,sizeof(row))||memcmp(authority,&cert,sizeof(cert))||memcmp(mapInput,&map,sizeof(map)))
  return inherited_entry_error(error,cap,r->error[0]?r->error:"placed INIT context retired or input changed");
 return 1;
}

int gsc_execute_placed_init_initial(GscRuntime*r,const GscEntityRef*e,const GscEntityExecutionCertificate*c,const GscMapExecutionCertificate*m,GscPlacedInitRun*out,char*error,size_t cap){return execute_placed_init_selected(r,e,c,m,NULL,0,out,error,cap);}
int gsc_execute_placed_init_with_return(GscRuntime*r,const GscEntityRef*e,const GscEntityExecutionCertificate*c,const GscMapExecutionCertificate*m,const GscPlacedInitReturnHost*h,GscPlacedInitRun*out,char*error,size_t cap){return h&&execute_placed_init_selected(r,e,c,m,h,0,out,error,cap);}
