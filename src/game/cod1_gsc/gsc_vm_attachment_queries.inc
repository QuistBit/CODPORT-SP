/* R203 typed read-only attachment queries. Results are staged until both
 * component/map and full entity identities remain current. */
int gsc_program_select_attachment_queries(GscProgram *g){
 if(!g||g->invalid||g->linked||g->attachment_queries_selected||!g->names_syntax)return 0;
 g->attachment_queries_selected=1;return 1;
}
int gsc_runtime_select_attachment_queries(GscRuntime *r,const GscAttachmentQueryHost *host){
 if(!r||!host||!host->invoke||r->started||r->advancing||r->executing_task||r->error[0]||
    !r->program->attachment_queries_selected||r->attachment_queries_enabled||!r->entities_enabled||
    !r->state_enabled||!r->results_enabled||!r->names_enabled||!gsc_runtime_storage_separate(r,host,sizeof(*host)))return 0;
 r->attachment_queries=*host;r->attachment_queries_enabled=1;return 1;
}
static int attachment_query_current(void *context){GscRuntime *r=context;return r&&!r->error[0]&&entity_task_current(r);}
static int attachment_query_invoke(GscRuntime *r,const Value *receiver,int descriptor,const Value *args,int argc,int line,Value *out){
 GscAttachmentQueryRequest request,expected;GscAttachmentQueryResult result;GscEntityRef owner;Value staged;char why[384];int kind=attachment_query_kind(descriptor),ok,finished;
 if(!r->attachment_queries_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||
    argc!=(kind==GSC_ATTACH_QUERY_SIZE?0:1)||(argc&&!args))return rfail(r,line,"attachment query requires selected capability, entity and exact arity");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"attachment query callback reentered");
 memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));result.kind=-1;memset(&staged,0,sizeof(staged));owner=receiver->entity;
 request.entity=owner;request.now_ms=r->now_ms;request.kind=kind;
 if(kind!=GSC_ATTACH_QUERY_SIZE){int32_t index;if(!scalar_get_integer(r,&args[0],line,&index))return 0;request.index=index;if(request.index<0||request.index>=7)return rfail(r,line,"attachment index is outside 0..6");}
 r->entity_method_busy=1;finished=entity_method_finish(r,&owner,line);if(finished<0){r->entity_method_busy=0;return finished==-2;}
 expected=request;why[0]=0;ok=r->attachment_queries.invoke(r->attachment_queries.user,&request,&result,why,sizeof(why),attachment_query_current,r);why[sizeof(why)-1]=0;
 if(r->error[0]){r->entity_method_busy=0;return 0;}
 if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
 if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"attachment query refused: %s",why[0]?why:"host callback failed");}
 finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;if(finished<0)return finished==-2;
 if(memcmp(&request,&expected,sizeof(request)))return rfail(r,line,"attachment query mutated borrowed input");
 if(kind==GSC_ATTACH_QUERY_SIZE){if(result.kind!=GSC_ATTACH_RESULT_INTEGER||result.integer<0||result.integer>7)return rfail(r,line,"attachment size result is invalid");scalar_integer(&staged,result.integer);}
 else{if(result.kind!=GSC_ATTACH_RESULT_STRING||!memchr(result.string,0,sizeof(result.string))||(kind==GSC_ATTACH_QUERY_MODEL_NAME&&!result.string[0]))return rfail(r,line,"attachment name result is invalid");staged.kind=V_STRING;memcpy(staged.str,result.string,sizeof(staged.str));}
 if(out)*out=staged;return 1;
}
