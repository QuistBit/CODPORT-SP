int gsc_program_select_model_part(GscProgram*g){
 if(!g||g->invalid||g->linked||g->model_part_selected||!g->names_syntax||!g->scalar_switch_syntax)return 0;
 g->model_part_selected=1;return 1;
}
int gsc_runtime_select_model_part(GscRuntime*r,const GscModelPartHost*h){
 if(!r||!h||!h->query||r->started||r->advancing||r->error[0]||r->model_part_enabled||!r->program->model_part_selected||
    !r->names_enabled||!r->scalar_switch_enabled||!r->results_enabled||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->model_part=*h;r->model_part_enabled=1;return 1;
}
static int model_part_current(void*ctx){GscRuntime*r=ctx;return !r->error[0]&&console_context(r)==1;}
static int model_part_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 GscModelPartRequest q,expected;GscModelPartHost host;char text[128],error[384];Value value;int ok;
 if(!r->model_part_enabled||!out||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"model part capability unavailable");
 if(argc<1||args[0].kind!=V_STRING||!memchr(args[0].str,0,128))return rfail(r,line,"getPartName GetString0 requires plain string");
 if(r->model_part_busy||r->weapon_classname_busy||r->weapon_model_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"model part callback reentered");
 memset(&q,0,sizeof(q));memcpy(q.model,args[0].str,128);q.now_ms=r->now_ms;
 /* Preserve native ordering: model resolution precedes the GetInt1 error.
  * All argument expressions already ran; only exact int32 payloads admitted. */
 if(argc>1&&args[1].kind==V_NUMBER&&args[1].integer==1){
  if(args[1].integer_exact){q.index=args[1].integer_value;q.index_valid=1;}
  else {double n=args[1].number;if(isfinite(args[1].number)&&n>=INT32_MIN&&n<=INT32_MAX&&trunc(n)==n){q.index=(int32_t)n;q.index_valid=1;}}
 }
 expected=q;host=r->model_part;memset(text,0xff,sizeof(text));error[0]=0;r->model_part_busy=1;
 if(console_context(r)!=1){r->model_part_busy=0;return 0;}
 ok=host.query(host.user,&q,text,error,sizeof(error),model_part_current,r);error[sizeof(error)-1]=0;
 r->model_part_busy=0;
 if(r->error[0]||!entity_task_current(r)||console_context(r)!=1)return 0;
 if(memcmp(&q,&expected,sizeof(q))||memcmp(&host,&r->model_part,sizeof(host)))return rfail(r,line,"model part host changed borrowed input/provider");
 if(ok!=1)return rfail(r,line,"getPartName refused: %s",error[0]?error:"host failed");
 if(!memchr(text,0,128)||!text[0])return rfail(r,line,"model part host returned invalid name");
 memset(&value,0,sizeof(value));value.kind=V_STRING;memcpy(value.str,text,128);*out=value;return 1;
}
