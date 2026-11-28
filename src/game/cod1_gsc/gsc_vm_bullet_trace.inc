/* GAME34830 five-key array. Only qualified world/miss replies publish here. */
int gsc_program_select_bullet_trace(GscProgram*g){if(!g||g->invalid||g->linked||g->bullet_trace_selected)return 0;g->bullet_trace_selected=1;return 1;}
int gsc_runtime_select_bullet_trace(GscRuntime*r,const GscBulletTraceHost*h){
 if(!r||!h||!h->query||r->started||r->advancing||r->error[0]||r->bullet_trace_enabled||!r->program->bullet_trace_selected||!r->arrays_enabled||!r->state_enabled||runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->bullet_trace=*h;r->bullet_trace_enabled=1;return 1;
}
typedef struct {GscRuntime*r;GscBulletTraceHost host;int now;uint64_t program;} BulletTraceLease;
static int bullet_trace_current(void*ctx){BulletTraceLease*l=ctx;GscRuntime*r=l->r;
 return r->bullet_trace_busy&&r->now_ms==l->now&&gsc_runtime_program_identity(r)==l->program&&!memcmp(&r->bullet_trace,&l->host,sizeof(l->host))&&!r->error[0]&&console_context(r)==1;
}
static int bullet_trace_call(GscRuntime*r,const Value*args,int argc,int line,Value*out){
 static const char*keys[5]={"fraction","position","entity","normal","surfacetype"};
 static const char*surfaces[24]={"default","bark","brick","carpet","cloth","concrete","dirt","flesh","foliage","glass","grass","gravel","ice","metal","mud","paper","plaster","rock","sand","snow","water","wood","asphalt","none"};
 GscBulletTraceRequest q,expected;GscBulletTraceReply result;BulletTraceLease lease;Value staged;char error[384];int n,k,ok,valid;
 if(!r->bullet_trace_enabled||!out||!args||argc<4||argc>MAX_PARAMS)return rfail(r,line,"bulletTrace capability/arguments unavailable");
 if(r->bullet_trace_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"bulletTrace callback reentered");
 if(args[0].kind!=V_VECTOR||args[1].kind!=V_VECTOR||args[2].kind!=V_NUMBER||!isfinite(args[2].number)||args[3].kind!=V_UNDEFINED)return rfail(r,line,"bulletTrace requires vectors, numeric boolean, undefined exclusion");
 if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count<5)return rfail(r,line,"bulletTrace array arena limit");
 memset(&q,0,sizeof(q));q.actors=args[2].number!=0;q.now_ms=r->now_ms;q.program=gsc_runtime_program_identity(r);
 for(k=0;k<3;k++){if(!isfinite(args[0].vector[k])||!isfinite(args[1].vector[k])||fabs(args[0].vector[k])>1000000||fabs(args[1].vector[k])>1000000)return rfail(r,line,"bulletTrace vector outside finite domain");q.start[k]=(float)args[0].vector[k];q.end[k]=(float)args[1].vector[k];}
 expected=q;lease.r=r;lease.host=r->bullet_trace;lease.now=r->now_ms;lease.program=q.program;
 memset(&result,0xff,sizeof(result));error[0]=0;r->bullet_trace_busy=1;r->entity_method_busy=1;
 if(!bullet_trace_current(&lease)){r->bullet_trace_busy=0;r->entity_method_busy=0;return 0;}
 ok=lease.host.query(lease.host.user,&q,&result,error,sizeof(error),bullet_trace_current,&lease);error[sizeof(error)-1]=0;
 valid=bullet_trace_current(&lease);r->bullet_trace_busy=0;r->entity_method_busy=0;
 if(ok!=1||!valid||memcmp(&q,&expected,sizeof(q)))return rfail(r,line,"bulletTrace query refused or retired: %s",error[0]?error:"invalid provider lease");
 if(!isfinite(result.fraction)||result.fraction<0||result.fraction>1||result.surface>23||result.entity_kind>1||(result.fraction==1?(result.surface!=23||result.entity_kind!=0):result.surface==23))return rfail(r,line,"invalid bulletTrace world/miss reply");
 for(k=0;k<3;k++)if(!isfinite(result.position[k])||!isfinite(result.normal[k])||fabs(result.position[k])>1000000||fabs(result.normal[k])>1.00001)return rfail(r,line,"nonfinite or invalid bulletTrace vectors");
 /* No callbacks follow this point. Preserve the undefined entity entry. */
 if(!array_new(r,line,&staged))return 0;
 for(n=0;n<5;n++){ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)staged.array;entry.key.kind=V_STRING;strcpy(entry.key.string,keys[n]);
  if(n==0){entry.value.kind=V_NUMBER;entry.value.number=result.fraction;}
  else if(n==1||n==3){entry.value.kind=V_VECTOR;for(k=0;k<3;k++)entry.value.vector[k]=n==1?result.position[k]:result.normal[k];}
  else if(n==4){entry.value.kind=V_STRING;strcpy(entry.value.str,surfaces[result.surface]);}
  array_entry_append55(r,&entry);
 }
 r->arrays[staged.array-1].count=5;r->arrays[staged.array-1].numeric_count=0;*out=staged;return 1;
}
