/* Included after entity_method_finish. No engine pointers or usercmd values
 * cross this boundary. Every request/result is staged and checked. */
int gsc_program_select_player_natives(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->player_natives_selected||!g->names_syntax)return 0;
    g->player_natives_selected=1;return 1;
}
int gsc_program_select_player_normal_health(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->player_natives_selected||g->player_normal_health_selected)return 0;
    g->player_normal_health_selected=1;return 1;
}
int gsc_program_select_player_viewmodel(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->player_natives_selected||g->player_viewmodel_selected)return 0;
    g->player_viewmodel_selected=1;return 1;
}
int gsc_program_select_player_max_ammo(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->player_natives_selected||g->player_max_ammo_selected)return 0;
    g->player_max_ammo_selected=1;return 1;
}
int gsc_program_select_player_stance_permissions(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->player_natives_selected||g->player_stance_permissions_selected)return 0;
    g->player_stance_permissions_selected=1;return 1;
}
int gsc_runtime_select_player_natives(GscRuntime *r,const GscPlayerNativeHost *host) {
    if(!r||!host||!host->invoke||r->started||r->advancing||r->error[0]||
       !r->program->player_natives_selected||r->player_natives_enabled||
       !r->entities_enabled||!r->state_enabled||!r->results_enabled||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->player_natives=*host;r->player_natives_enabled=1;return 1;
}
static int player_native_current(void *context) {
    GscRuntime *r=context;
    return r&&!r->error[0]&&entity_task_current(r);
}
static int player_native_invoke(GscRuntime *r,const Value *receiver,int descriptor,
                                 const Value *args,int argc,int line,Value *out) {
    GscPlayerNativeRequest request,original_request;
    GscPlayerNativeResult result;
    Value staged;
    GscEntityRef owner;
    char error[384];
    int kind=player_method_kind(descriptor),finished,ok,expected;
    if(!r->player_natives_enabled||!kind||!receiver||receiver->kind!=V_ENTITY||
       argc!=player_method_arity(descriptor)||(argc&&!args))
        return rfail(r,line,"player native capability, receiver or arity unavailable");
    if(r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,line,"player native callback reentered");
    memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));
    memset(&staged,0,sizeof(staged));owner=receiver->entity;
    request.player=owner;request.kind=kind;request.now_ms=r->now_ms;
    if(kind>=GSC_PLAYER_ALLOW_STAND&&kind<=GSC_PLAYER_ALLOW_PRONE){
        if(!r->program->player_stance_permissions_selected||
           !scalar_get_integer(r,&args[0],line,&request.value))return 0;
    }
    if(kind==GSC_PLAYER_SET_ANGLES){
        if(args[0].kind!=V_VECTOR)return rfail(r,line,"setPlayerAngles requires a vector");
        memcpy(request.angles_bits,args[0].vector,sizeof(request.angles_bits));
        {unsigned i;for(i=0;i<3;i++)if((request.angles_bits[i]&0x7f800000u)==0x7f800000u)
            return rfail(r,line,"setPlayerAngles requires finite coordinates");}
    }
    if(kind==GSC_PLAYER_GET_SLOT_WEAPON||kind==GSC_PLAYER_HAS_WEAPON||kind==GSC_PLAYER_GIVE_WEAPON||
       kind==GSC_PLAYER_TAKE_WEAPON||kind==GSC_PLAYER_SWITCH_TO_WEAPON||
       kind==GSC_PLAYER_SET_AMMO_CLIP||kind==GSC_PLAYER_SET_AMMO_STOCK||
       kind==GSC_PLAYER_GET_FRACTION_MAX_AMMO||kind==GSC_PLAYER_GIVE_MAX_AMMO){
        if(args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str))||!args[0].str[0])
            return rfail(r,line,"player weapon name requires a nonempty plain string");
        memcpy(request.weapon,args[0].str,sizeof(request.weapon));
    }
    if(kind==GSC_PLAYER_SET_AMMO_CLIP||kind==GSC_PLAYER_SET_AMMO_STOCK||
       kind==GSC_PLAYER_FREEZE_CONTROLS){
        int index=kind==GSC_PLAYER_FREEZE_CONTROLS?0:1;
        if(!scalar_get_integer(r,&args[index],line,&request.value))return 0;
        if(request.value<0||(kind==GSC_PLAYER_FREEZE_CONTROLS&&request.value>1))
            return rfail(r,line,"player native integer is outside its supported range");
    }
    if(kind==GSC_PLAYER_SHELLSHOCK){
        if(args[0].kind!=V_STRING||!memchr(args[0].str,0,sizeof(args[0].str)))
            return rfail(r,line,"shellshock requires a registered effect name");
        memcpy(request.weapon,args[0].str,sizeof(request.weapon));
        if(!scalar_float(r,args+1,line,&request.number))return 0;
    }
    if(kind==GSC_PLAYER_SET_VIEWMODEL) {
        if(!r->program->player_viewmodel_selected||args[0].kind!=V_STRING||
           !memchr(args[0].str,0,sizeof(args[0].str)))return rfail(r,line,"setViewmodel requires selected capability and plain model string");
        memcpy(request.weapon,args[0].str,sizeof(request.weapon));
    }
    if(kind==GSC_PLAYER_SET_NORMAL_HEALTH||kind==GSC_PLAYER_GET_NORMAL_HEALTH) {
        if(!r->program->player_normal_health_selected)return rfail(r,line,"normal health capability unavailable");
        if(kind==GSC_PLAYER_SET_NORMAL_HEALTH) {
            if(args[0].kind!=V_NUMBER||!isfinite(args[0].number)||args[0].number<=0.0f)
                return rfail(r,line,"setnormalhealth requires a finite positive number");
            request.number=args[0].number;
        }
    }
    r->entity_method_busy=1;finished=entity_method_finish(r,&owner,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    memcpy(&original_request,&request,sizeof(request));
    error[0]=0;
    ok=r->player_natives.invoke(r->player_natives.user,&request,&result,
        error,sizeof(error),player_native_current,r);
    error[sizeof(error)-1]=0;
    if(r->error[0]){r->entity_method_busy=0;return 0;}
    if(!entity_task_current(r)){r->entity_method_busy=0;return !r->error[0];}
    if(ok!=1){r->entity_method_busy=0;return rfail(r,line,"player native refused: %s",error[0]?error:"host callback failed");}
    finished=entity_method_finish(r,&owner,line);r->entity_method_busy=0;
    if(finished<0)return finished==-2;
    if(memcmp(&request,&original_request,sizeof(request)))return rfail(r,line,"player native mutated its borrowed request");
    expected=player_method_result(descriptor);
    if(result.kind!=expected)return rfail(r,line,"player native returned the wrong result kind");
    if(expected==GSC_PLAYER_RESULT_INT){
        if(result.value!=0&&result.value!=1)return rfail(r,line,"player predicate returned a non-boolean result");
        scalar_integer(&staged,result.value);
    }else if(expected==GSC_PLAYER_RESULT_FLOAT){
        if(!isfinite(result.number)||(kind!=GSC_PLAYER_GET_NORMAL_HEALTH&&(result.number<0.0f||result.number>1.0f)))
            return rfail(r,line,"player ammo fraction returned an invalid finite ratio");
        staged.kind=V_NUMBER;staged.number=result.number;staged.integer=0;
    }else if(expected==GSC_PLAYER_RESULT_STRING){
        if(!memchr(result.text,0,sizeof(result.text))||!result.text[0])
            return rfail(r,line,"player native returned an invalid weapon string");
        if(kind==GSC_PLAYER_GET_STANCE&&strcmp(result.text,"stand")&&strcmp(result.text,"crouch")&&strcmp(result.text,"prone"))
            return rfail(r,line,"player stance host returned an unsupported stance");
        staged.kind=V_STRING;memcpy(staged.str,result.text,sizeof(staged.str));
    }
    if(out)*out=staged;
    return 1;
}
