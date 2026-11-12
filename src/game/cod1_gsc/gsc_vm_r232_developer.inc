static int developer_runtime_current(GscRuntime *r) {
    GscDeveloperScriptSelection copy;const GscProgram *program;uint64_t identity;int now,ok;
    if(!r||!r->program)return 0;
    if(!r->program->developer_selected)return 1;
    if(r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,0,"developer setting certificate reentered");
    program=r->program;
    if(program->developer_value_only&&!r->developer_admitted)return rfail(r,0,"developer setting needs runtime admission");
    copy=program->developer_value_only?r->developer_admission:program->developer;identity=r->array_token;now=r->now_ms;
    r->entity_certificate_busy=1;ok=copy.certificate.current(copy.certificate.receipt);r->entity_certificate_busy=0;
    if(r->error[0])return 0;
    if(ok!=1||r->program!=program||r->array_token!=identity||r->now_ms!=now||
       !program->developer_selected||memcmp(program->developer_value_only?&r->developer_admission:&program->developer,&copy,sizeof(copy)))
        return rfail(r,0,"developer_script setting certificate retired or changed");
    return 1;
}

int gsc_runtime_admit_developer_script(GscRuntime*r,const GscDeveloperScriptSelection*input){
 GscDeveloperScriptSelection copy;const GscProgram*p;uint64_t identity;int now,ok;
 if(!r||!input||r->error[0]||r->started||r->entity_prepare_busy||r->entity_certificate_busy||
    !r->program||!r->program->developer_value_only||r->developer_admitted||
    runtime_private_overlap(r,input,sizeof(*input)))return 0;
 copy=*input;p=r->program;identity=r->array_token;now=r->now_ms;
 if(copy.enabled!=p->developer.enabled||!copy.certificate.receipt||!copy.certificate.current)return 0;
 r->entity_certificate_busy=1;ok=copy.certificate.current(copy.certificate.receipt);r->entity_certificate_busy=0;
 if(ok!=1||r->error[0]||r->started||r->program!=p||r->array_token!=identity||r->now_ms!=now||r->developer_admitted||memcmp(input,&copy,sizeof(copy)))return 0;
 r->developer_admission=copy;r->developer_admitted=1;return 1;
}
