int gsc_program_select_entity_links(GscProgram*g){
 if(!g||g->invalid||g->linked||g->entity_links_selected||!g->names_syntax)return 0;
 g->entity_links_selected=1;return 1;
}
int gsc_runtime_select_entity_links(GscRuntime*r,const GscEntityLinkHost*h){
 if(!r||!h||!h->invoke||r->started||r->advancing||r->error[0]||r->entity_links_enabled||
    !r->program->entity_links_selected||!r->entities_enabled||!r->state_enabled||
    runtime_private_overlap(r,h,sizeof(*h)))return 0;
 r->entity_links=*h;r->entity_links_enabled=1;return 1;
}
static int entity_link_call(GscRuntime*r,GscEntityLinkRequest*q,int line){
 GscEntityLinkRequest borrowed=*q;char error[384];int ok,finished;
 error[0]=0;ok=r->entity_links.invoke(r->entity_links.user,&borrowed,error,sizeof(error),player_native_current,r);error[sizeof(error)-1]=0;
 if(r->error[0])return 0;
 if(!entity_task_current(r))return !r->error[0];
 if(memcmp(&borrowed,q,sizeof(borrowed)))return rfail(r,line,"link host changed its borrowed request");
 if(ok!=1)return rfail(r,line,"entity link refused: %s",error[0]?error:"host callback failed");
 finished=entity_method_finish(r,&q->child,line);return finished==1||finished==-2;
}
static int entity_link_invoke(GscRuntime*r,const Value*receiver,int descriptor,const Value*args,int argc,int line){
 GscEntityLinkRequest q;int result=0,finished,k;
 if(!r->entity_links_enabled||!entity_link_method(descriptor)||!receiver||receiver->kind!=V_ENTITY||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"entity link capability/receiver unavailable");
 if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"entity link callback reentered");
 memset(&q,0,sizeof(q));q.child=receiver->entity;q.now_ms=r->now_ms;q.argc=argc;r->entity_method_busy=1;
 finished=entity_method_finish(r,&q.child,line);if(finished<0){result=finished==-2;goto done;}
 if(descriptor==ENTITY_UNLINK_METHOD){q.operation=GSC_ENTITY_UNLINK;result=entity_link_call(r,&q,line);goto done;}
 /* Original31591 checks entity pointer type before receiver support, but
  * resolves the actual parent and reads optional getters after that support. */
 if(argc<1||args[0].kind!=V_ENTITY){rfail(r,line,"linkto argument0 is not an entity");goto done;}
 q.parent=args[0].entity;q.operation=GSC_ENTITY_LINK_CHECK;
 if(!entity_link_call(r,&q,line))goto done;
 if(!entity_task_current(r)){result=!r->error[0];goto done;}
 finished=entity_method_finish(r,&q.parent,line);if(finished<0){result=finished==-2;goto done;}
 if(argc>=2){if(args[1].kind!=V_STRING||!memchr(args[1].str,0,sizeof(args[1].str))){rfail(r,line,"linkto GetString1 requires plain string");goto done;}memcpy(q.tag,args[1].str,sizeof(q.tag));}
 if(argc>2){
  if(args[2].kind!=V_VECTOR){rfail(r,line,"linkto GetVector2 requires vector");goto done;}
  if(argc<4||args[3].kind!=V_VECTOR){rfail(r,line,"linkto GetVector3 requires vector");goto done;}
  q.explicit_offsets=1;
  for(k=0;k<3;k++){q.origin[k]=args[2].vector[k];q.angles[k]=args[3].vector[k];if(!isfinite(q.origin[k])||!isfinite(q.angles[k])){rfail(r,line,"linkto offsets must be finite");goto done;}}
 }
 /* Extra arguments after3 have already evaluated, but native reads none. */
 q.operation=GSC_ENTITY_LINK_ATTACH;result=entity_link_call(r,&q,line);
done:r->entity_method_busy=0;return result;
}
