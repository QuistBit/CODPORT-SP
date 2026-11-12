/* Early name ownership is independent of the late registry map certificate.
 * Compilation may inspect names; cache publication still requires fresh registry
 * admission and a real runtime. Replacement never retains that admission. */
static int session_node_program_current(GscSession*s){
 GscSessionNodeProgramPlan copy;GscProgram*p;GscRuntime*r;uint64_t id;int state,ok;
 if(!s||s->abort_requested||s->view.error[0])return 0;if(!s->node_program_enabled)return 1;
 if(!s->busy||s->node_program_busy){s->abort_requested=1;session_terminal(s,"node program declaration reentered");return 0;}
 copy=s->node_program;p=s->program;r=s->runtime;id=s->view.session_id;state=s->view.state;
 s->node_program_busy=1;ok=copy.current(copy.user,copy.epoch,copy.receipt);s->node_program_busy=0;
 return ok==1&&!s->abort_requested&&!s->view.error[0]&&p==s->program&&r==s->runtime&&id==s->view.session_id&&state==s->view.state&&!memcmp(&copy,&s->node_program,sizeof(copy));
}
static int session_node_names_valid(const GscNodeScriptNames*n){unsigned i,j;
 if(!n||!n->count||n->count>GSC_NODE_SCRIPT_CAPACITY)return 0;
 for(i=0;i<n->count;i++){if(!gsc_node_script_name_valid(n->names[i]))return 0;
  for(j=0;j<i;j++)if(!strcmp(n->names[i],n->names[j]))return 0;}return 1;
}
static int session_node_names_equal(const GscNodeScriptNames*a,const GscNodeScriptNames*b){unsigned i;
 if(!session_node_names_valid(a)||!session_node_names_valid(b)||a->count!=b->count)return 0;
 for(i=0;i<a->count;i++)if(strcmp(a->names[i],b->names[i]))return 0;return 1;
}
/* Independent map selection, explicit acquisition and published phases.
 * busy excludes reentry; only actual map certificate + loaded cache establish
 * DATA ownership. No actor-bootstrap scope or execution admission is borrowed. */
static int session_node_scripts_current(GscSession*s){
 GscMapExecutionCertificate cert;GscProgram*p;GscRuntime*r;uint64_t id;int phase,state,ok;
 if(!s)return 0;if(!s->node_scripts_enabled)return 1;
 if(!s->busy||s->abort_requested||s->view.error[0]||!s->node_script_phase||s->node_script_map_busy)return 0;
 if(s->node_program_enabled){
  if(!session_node_program_current(s))return 0;
  if(!s->node_program_admitted)return s->node_script_phase==2&&!s->runtime&&
   (s->view.state==GSC_SESSION_EMPTY||s->view.state==GSC_SESSION_COMPILED);
 }
 cert=s->node_script_selection.map_certificate;if(!cert.receipt||!cert.current)return 0;
 p=s->program;r=s->runtime;id=s->view.session_id;phase=s->node_script_phase;state=s->view.state;
 s->node_script_map_busy=1;ok=cert.current(cert.receipt);s->node_script_map_busy=0;
 return ok==1&&!s->abort_requested&&!s->view.error[0]&&p==s->program&&r==s->runtime&&id==s->view.session_id&&phase==s->node_script_phase&&state==s->view.state&&
  cert.receipt==s->node_script_selection.map_certificate.receipt&&cert.current==s->node_script_selection.map_certificate.current;
}
int gsc_session_select_node_program(GscSession*s,const GscSessionNodeProgramPlan*input,char*error,size_t cap){
 GscSessionNodeProgramPlan copy;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 if(s->busy||s->view.error[0]||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->node_scripts_enabled||s->node_program_enabled||
    !s->modules_enabled||!s->function_refs_enabled||!s->lifetime_enabled||!copy.epoch||!copy.receipt||!copy.current||!session_node_names_valid(&copy.scripts))
  return session_reject(error,cap,"node program requires idle module/function Session and current unique declarations");
 if(!session_enter(s,error,cap))return 0;
 s->node_program=copy;s->node_program_enabled=1;s->node_script_selection.scripts=copy.scripts;s->node_scripts_enabled=1;
 if(!session_node_program_current(s))return session_failed(s,"node declarations unavailable",error,cap);
 s->busy=0;return 1;
}
int gsc_session_admit_node_scripts(GscSession*s,uint64_t id,const GscSessionNodeScriptAdmission*input,char*error,size_t cap){
 GscSessionNodeScriptAdmission copy;
 if(!s||(cap&&!error)||session_animation_namespace_private(s,error,cap)||(input&&(session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_overlap(input,sizeof(*input),error,cap))))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(!session_id_matches(s,id))return session_leave_rejected(s,"stale node admission Session ID",error,cap);
 if(s->view.state!=GSC_SESSION_COMPILED||!s->program||s->runtime||!s->node_program_enabled||s->node_program_admitted||s->node_admission_attempted)
  return session_leave_rejected(s,"node registry admission outside compiled boundary",error,cap);
 s->node_admission_attempted=1;
 if(!input)return session_failed(s,"node registry admission missing",error,cap);copy=*input;
 if(copy.epoch!=s->node_program.epoch||!copy.selection.map_certificate.receipt||!copy.selection.map_certificate.current||
    !session_node_names_equal(&copy.selection.scripts,&s->node_program.scripts)||!session_node_program_current(s))
  return session_failed(s,"node registry differs from compiled declarations",error,cap);
 s->node_script_selection=copy.selection;s->node_program_admitted=1;
 if(!session_node_scripts_current(s)||!session_node_program_current(s))return session_failed(s,"node registry retired during admission",error,cap);
 s->busy=0;return 1;
}
int gsc_session_select_node_scripts(GscSession*s,const GscSessionNodeScriptSelection*input,char*error,size_t cap){
 GscSessionNodeScriptSelection copy;unsigned i,j;
 if(!s||!input||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 if(s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->node_scripts_enabled||!s->modules_enabled||!s->function_refs_enabled||!s->lifetime_enabled||
    !copy.map_certificate.receipt||!copy.map_certificate.current||!copy.scripts.count||copy.scripts.count>GSC_NODE_SCRIPT_CAPACITY)return session_reject(error,cap,"node scripts require independent map selection and empty module/function Session");
 for(i=0;i<copy.scripts.count;i++){
  if(!gsc_node_script_name_valid(copy.scripts.names[i]))return session_reject(error,cap,"invalid traverse script name");
  for(j=0;j<i;j++)if(!strcmp(copy.scripts.names[i],copy.scripts.names[j]))return session_reject(error,cap,"duplicate traverse script name");
 }
 if(!session_enter(s,error,cap))return 0;s->node_script_selection=copy;s->node_scripts_enabled=1;s->node_script_phase=1;
 if(!session_node_scripts_current(s))return session_failed(s,"node script selection map retired",error,cap);
 s->node_script_phase=0;s->busy=0;return 1;
}
static int session_node_scripts_publish(GscSession*s){
 if(!s->node_scripts_enabled)return 1;
 if(s->node_script_phase!=2||(s->node_program_enabled&&!s->node_program_admitted)||!session_node_scripts_current(s)||!gsc_runtime_node_scripts_build(s->runtime,&s->node_script_selection.scripts)||!session_node_scripts_current(s))return 0;
 s->node_script_phase=3;return 1;
}
static int session_node_script_query_begin(GscSession*s,uint64_t id){
 if(!s||!s->node_scripts_enabled||s->node_script_phase!=3||id!=s->view.session_id||!s->runtime||
    (s->view.state!=GSC_SESSION_PREPARED&&s->view.state!=GSC_SESSION_RUNNING&&s->view.state!=GSC_SESSION_COMPLETED))return 0;
 if(!session_enter(s,NULL,0))return 0;
 if(!session_node_scripts_current(s)){session_failed(s,"node script map retired",NULL,0);return 0;}return 1;
}
int gsc_session_node_script_map_matches(GscSession*s,uint64_t id,const GscMapExecutionCertificate*map){
 GscMapExecutionCertificate copy;
 if(!s||!map||session_animation_namespace_private(s,map,sizeof(*map)))return 0;copy=*map;
 if(!s->node_scripts_enabled||copy.receipt!=s->node_script_selection.map_certificate.receipt||
    copy.current!=s->node_script_selection.map_certificate.current)return 0;
 if(!session_node_script_query_begin(s,id))return 0;s->busy=0;return 1;
}
GscNodeScriptRef gsc_session_node_script_lookup(GscSession*s,uint64_t id,const char*name){
 GscNodeScriptRef ref={0,0};char copy[64]={0};unsigned i;
 if(!s||!name||session_animation_namespace_private(s,name,64)||!gsc_node_script_name_valid(name))return ref;
 for(i=0;i<63&&name[i];i++)copy[i]=name[i];
 if(!session_node_script_query_begin(s,id))return ref;
 ref=gsc_runtime_node_script_lookup(s->runtime,copy);
 if(!session_node_scripts_current(s)){session_failed(s,"node script map retired during lookup",NULL,0);ref.cache=ref.entry=0;return ref;}
 s->busy=0;return ref;
}
int gsc_session_node_script_current(GscSession*s,uint64_t id,GscNodeScriptRef ref){
 int ok;if(!session_node_script_query_begin(s,id))return 0;ok=gsc_runtime_node_script_current(s->runtime,ref);
 if(!session_node_scripts_current(s)){session_failed(s,"node script map retired during validation",NULL,0);return 0;}s->busy=0;return ok;
}
