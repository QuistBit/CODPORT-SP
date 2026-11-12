/* Early byte ownership, late execution authority. No Actor/map certificate is
 * manufactured to compile a declaration. Original catalogs remain immutable. */
static int session_turret_program_current(GscSession*s){
 GscSessionTurretProgramPlan copy;GscProgram*p;GscRuntime*r;uint64_t id;int state,ok;
 if(!s||s->abort_requested||s->view.error[0])return 0;if(!s->turret_program_enabled)return 1;
 if(!s->busy||s->turret_program_busy){s->abort_requested=1;session_terminal(s,"turret declaration reentered");return 0;}
 copy=s->turret_program;p=s->program;r=s->runtime;id=s->view.session_id;state=s->view.state;
 s->turret_program_busy=1;ok=copy.current(copy.user,copy.epoch,copy.receipt);s->turret_program_busy=0;
 return ok==1&&!s->abort_requested&&!s->view.error[0]&&p==s->program&&r==s->runtime&&id==s->view.session_id&&state==s->view.state&&!memcmp(&copy,&s->turret_program,sizeof(copy));
}
int gsc_session_select_turret_program(GscSession*s,const GscSessionTurretProgramPlan*input,char*error,size_t cap){
 static const char*const names[2]={"generic_human","mg42"};
 GscSessionTurretProgramPlan copy;GscAnimationCatalog*c[2]={NULL,NULL};unsigned char*bytes[2]={NULL,NULL};char why[256]={0};int i,ok=0;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;
 for(i=0;i<2;i++)if(!copy.catalogs[i].atr_bytes||!copy.catalogs[i].atr_size||copy.catalogs[i].atr_size>GSC_SESSION_MAX_SOURCE||
  session_animation_namespace_private(s,copy.catalogs[i].atr_bytes,copy.catalogs[i].atr_size)||
  session_animation_namespace_overlap(input,sizeof(*input),copy.catalogs[i].atr_bytes,copy.catalogs[i].atr_size)||
  session_animation_namespace_overlap(error,cap,copy.catalogs[i].atr_bytes,copy.catalogs[i].atr_size))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(!s->host.enabled||!s->dynamic_randomfloat_enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
    s->animation_namespace_enabled||s->start_origin_enabled||s->turret_program_enabled||s->developer_selected||
    !copy.epoch||!copy.receipt||!copy.current||(copy.developer_enabled!=0&&copy.developer_enabled!=1))
  return session_leave_rejected(s,"turret declaration requires idle exclusive two-ATR DATA plan",error,cap);
 for(i=0;i<2;i++){
  bytes[i]=GSC_SESSION_MALLOC(copy.catalogs[i].atr_size);if(!bytes[i])goto done;
  memcpy(bytes[i],copy.catalogs[i].atr_bytes,copy.catalogs[i].atr_size);
  c[i]=gsc_animation_catalog_parse(names[i],bytes[i],copy.catalogs[i].atr_size,why,sizeof(why));if(!c[i]||!gsc_animation_catalog_count(c[i]))goto done;
 }
 /* Publish owned payloads before calling out. No borrowed byte pointer survives. */
 s->turret_program=copy;
 for(i=0;i<2;i++){
  s->turret_program_bytes[i]=bytes[i];bytes[i]=NULL;s->turret_program_sizes[i]=copy.catalogs[i].atr_size;
  s->turret_program.catalogs[i].atr_bytes=NULL;
  s->animation_namespace_catalogs[i]=c[i];c[i]=NULL;
 }
 s->turret_program_enabled=s->animation_namespace_enabled=s->turret_animations_enabled=1;
 s->developer.enabled=copy.developer_enabled;s->developer_selected=1;
 if(!session_turret_program_current(s)||memcmp(input,&copy,sizeof(copy)))return session_failed(s,"turret declaration expired during selection",error,cap);
 ok=1;
done:
 for(i=0;i<2;i++){GSC_SESSION_FREE(bytes[i]);gsc_animation_catalog_free(c[i]);}s->busy=0;
 return ok?1:session_reject(error,cap,why[0]?why:"turret declaration parse/allocation failed");
}
static int session_turret_admission_bytes(GscSession*s,const GscSessionTurretProgramAdmission*a){
 int i;
 for(i=0;i<2;i++)if(a->animations.catalogs[i].atr_size!=s->turret_program_sizes[i]||
  memcmp(a->animations.catalogs[i].atr_bytes,s->turret_program_bytes[i],s->turret_program_sizes[i]))return 0;
 return 1;
}
int gsc_session_admit_turret_program(GscSession*s,uint64_t id,const GscSessionTurretProgramAdmission*input,char*error,size_t cap){
 GscSessionTurretProgramAdmission copy;int i;
 if(!s||(cap&&!error)||session_animation_namespace_private(s,error,cap)||(input&&(session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_overlap(input,sizeof(*input),error,cap))))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(!session_id_matches(s,id))return session_leave_rejected(s,"stale turret admission Session ID",error,cap);
 if(s->view.state!=GSC_SESSION_COMPILED||!s->program||s->runtime||!s->turret_program_enabled||s->turret_program_admitted||s->turret_admission_attempted)
  return session_leave_rejected(s,"turret resource admission outside compiled boundary",error,cap);
 s->turret_admission_attempted=1;
 if(!input)return session_failed(s,"turret resource admission missing",error,cap);copy=*input;
 if(copy.epoch!=s->turret_program.epoch||copy.developer.enabled!=s->turret_program.developer_enabled||
    !copy.animations.host.validate||!copy.animations.host.invoke||!copy.animations.map_certificate.receipt||!copy.animations.map_certificate.current||
    !copy.developer.certificate.receipt||!copy.developer.certificate.current)return session_failed(s,"turret resource admission differs from plan",error,cap);
 for(i=0;i<2;i++)if(!copy.animations.catalogs[i].atr_bytes||copy.animations.catalogs[i].atr_size!=s->turret_program_sizes[i]||
  session_animation_namespace_private(s,copy.animations.catalogs[i].atr_bytes,copy.animations.catalogs[i].atr_size)||
  session_animation_namespace_overlap(error,cap,copy.animations.catalogs[i].atr_bytes,copy.animations.catalogs[i].atr_size)||
  session_animation_namespace_overlap(input,sizeof(*input),copy.animations.catalogs[i].atr_bytes,copy.animations.catalogs[i].atr_size))
   return session_failed(s,"turret resource admission invalid bytes",error,cap);
 if(!session_turret_program_current(s)||!session_turret_admission_bytes(s,&copy)||
    !session_animation_namespace_certificate(s,&copy.animations.map_certificate)||
    !session_animation_namespace_certificate(s,&copy.developer.certificate)||
    !session_turret_program_current(s)||!session_turret_admission_bytes(s,&copy)||memcmp(input,&copy,sizeof(copy)))
  return session_failed(s,"turret resource bytes/settings retired during admission",error,cap);
 s->animation_namespace_map=copy.animations.map_certificate;s->turret_animations=copy.animations.host;
 s->developer=copy.developer;s->turret_program_admitted=1;s->busy=0;return 1;
}
