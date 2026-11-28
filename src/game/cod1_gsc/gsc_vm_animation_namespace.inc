/* Selected declared-symbol metadata only. Catalog ordinals are never native
 * tree/node IDs, and no entry here certifies a loaded clip or playback. */
static const char *const animation_namespace_trees[GSC_ANIMATION_NAMESPACE_CATALOGS]={
    "generic_human","animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"
};
static const char *const r232_animation_namespace_trees[4]={"generic_human","mg42","animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"};
static int animation_namespace_count(const GscProgram *g){return g->map_animation_names.count?(int)g->map_animation_names.count:g->training_rigs_selected?4:g->turret_animations_selected?2:GSC_ANIMATION_NAMESPACE_CATALOGS;}
static const char *animation_namespace_name(const GscProgram *g,int n){return g->map_animation_names.count?g->map_animation_names.names[n]:g->turret_animations_selected?r232_animation_namespace_trees[n]:animation_namespace_trees[n];}
static int animation_namespace_tree_index_for_program(const GscProgram *g,const char *name){int n;for(n=0;n<animation_namespace_count(g);n++)if(!strcmp(name,animation_namespace_name(g,n)))return n;return -1;}
static int animation_namespace_kind(int kind){return kind==V_ANIMATION_TREE||kind==V_NAMESPACED_ANIMATION;}
static int animation_namespace_tree_index(const char *name){int n;for(n=0;n<GSC_ANIMATION_NAMESPACE_CATALOGS;++n)if(!strcmp(name,animation_namespace_trees[n]))return n;return -1;}
int gsc_program_select_animation_namespace(GscProgram *g){
    if(!g||g->invalid||g->linked||g->module_count||g->expr_count||g->func_count||g->string_count||
       !g->anim_data_selected||g->animation_namespace_selected||g->animation_refs_selected||
       g->start_origin_selected||g->animation_length_selected||g->animation_methods_selected)return 0;
    g->animation_namespace_selected=1;return 1;
}
/* Independent immutable declared namespace; no actor, playback or resource
 * sampling capability follows from selecting these names. */
int gsc_program_select_map_animation_namespace(GscProgram*g,const GscMapAnimationNames*input){
 GscMapAnimationNames copy;unsigned i,j,k;
 if(!g||!input||g->map_animation_names.count||g->turret_animations_selected||g->training_rigs_selected)return 0;
 copy=*input;if(!copy.count||copy.count>GSC_MAP_ANIMATION_CATALOGS)return 0;
 for(i=0;i<copy.count;i++){
  if(!copy.names[i][0]||!memchr(copy.names[i],0,64))return 0;
  for(k=0;copy.names[i][k];k++)if(!((copy.names[i][k]>='a'&&copy.names[i][k]<='z')||(copy.names[i][k]>='0'&&copy.names[i][k]<='9')||copy.names[i][k]=='_'))return 0;
  for(j=0;j<i;j++)if(!strcmp(copy.names[i],copy.names[j]))return 0;
 }
 if(!gsc_program_select_animation_namespace(g))return 0;
 g->map_animation_names=copy;g->native_animation_roots_selected=1;return 1;
}
static int animation_namespace_scope(const GscRuntime *r){
    const AnimationNamespace *space;
    if(!r||!r->animation_namespace_enabled||!r->program||r->program!=r->animation_namespace.program||!r->program->animation_namespace_selected||
       !r->anim_data_enabled||r->animation_refs_enabled||r->error[0])return 0;
    space=&r->animation_namespace;
    return space->identity&&space->runtime_identity==r->array_token&&
        space->map_certificate.receipt==r->anim_data.receipt&&space->map_certificate.current==r->anim_data.current&&
        space->map_certificate.receipt&&space->map_certificate.current;
}
static int animation_namespace_value_valid(const GscRuntime *r,const Value *v){
    if(!v||!animation_namespace_kind(v->kind)||!animation_namespace_scope(r)||
       v->object_token!=r->animation_namespace.identity||!v->object)return 0;
    return v->kind==V_ANIMATION_TREE?v->object<=(uint32_t)animation_namespace_count(r->program):
        v->object<=(uint32_t)r->animation_namespace.entry_count;
}
static int animation_namespace_context(const GscProgram *g,const Expr *e){
    const StringLiteral *context=NULL;int n;
    if(e->owner_module<0||e->owner_module>=g->module_count)return -1;
    for(n=0;n<g->string_count;++n){const StringLiteral *s=&g->strings[n];
        if(s->module==e->owner_module&&s->owner_kind==GSC_STRING_OWNER_ANIMTREE&&
           s->source_offset<e->audit_offset&&(!context||s->source_offset>context->source_offset))context=s;
    }
    return string_literal_decoded(g,context)?
        animation_namespace_tree_index_for_program(g,context->decoded):-1;
}
static int animation_namespace_quiescent(const GscRuntime *r){
    return r&&!r->started&&!r->advancing&&!r->executing_task&&!r->task_count&&!r->error[0]&&
        !r->animation_namespace_busy&&!r->entity_entry_depth&&!r->entity_prepare_busy&&!r->entity_certificate_busy&&
        !r->entity_sequence_busy&&!r->entity_inheritance_busy&&!r->entity_field_busy&&!r->entity_method_busy;
}
static int animation_namespace_catalog_valid(const GscAnimationCatalog *c,int index,const GscProgram *g){
    uint32_t n;unsigned k;
    if(!c||!c->token||!c->count||c->count>GSC_ANIMATION_CATALOG_CAPACITY||
       !memchr(c->tree,0,sizeof(c->tree))||strcmp(c->tree,animation_namespace_name(g,index)))return 0;
    for(n=0;n<c->count;++n){const GscAnimationCatalogEntry *e=&c->entries[n];
        if(e->ordinal!=n+1||e->parent>=e->ordinal||!e->name[0]||!memchr(e->name,0,sizeof(e->name))||
           (e->flags&~(GSC_ANIMATION_NODE_BRANCH|GSC_ANIMATION_NODE_COMPLETE|GSC_ANIMATION_NODE_LOOPSYNC|GSC_ANIMATION_NODE_NONLOOPSYNC))||
           (!(e->flags&GSC_ANIMATION_NODE_BRANCH)&&e->flags)||
           ((e->flags&GSC_ANIMATION_NODE_LOOPSYNC)&&(e->flags&GSC_ANIMATION_NODE_NONLOOPSYNC)))return 0;
        for(k=0;e->name[k];++k)if(!animation_catalog_id((unsigned char)e->name[k]))return 0;
        if(e->parent&&!(c->entries[e->parent-1].flags&GSC_ANIMATION_NODE_BRANCH))return 0;
    }
    return 1;
}
/* Native455940 installs the implicit root independently of ATR declarations.
 * Portable root metadata is explicit; it is never a fabricated catalog row. */
int gsc_program_select_r233_native_animation_roots(GscProgram*g){if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||!g->function_admission_selected||g->native_animation_roots_selected)return 0;g->native_animation_roots_selected=1;return 1;}
static int animation_namespace_capacity(const GscProgram*g){return g->native_animation_roots_selected?GSC_R233_ANIMATION_NAMESPACE_CAPACITY:GSC_ANIMATION_REFERENCE_CAPACITY;}
static int animation_namespace_bind_atom(const GscProgram *program,const GscAnimationCatalog *const *catalogs,AnimationNamespace *staged,int n){
    const Expr *e=&program->expr[n];AnimationNamespaceAtom *atom;int tree,found,k,j;
    if(e->kind!=E_ANIM_REF&&e->kind!=E_ANIMTREE_VALUE)return 1;
    if(staged->atom_count>=animation_namespace_capacity(program)||(tree=animation_namespace_context(program,e))<0)return 0;
    atom=&staged->atoms[staged->atom_count++];atom->expr=n;
    if(e->kind==E_ANIMTREE_VALUE){atom->kind=V_ANIMATION_TREE;atom->object=(uint32_t)tree+1;return 1;}
    found=-1;
    for(k=0;k<(int)catalogs[tree]->count;k++)if(animation_catalog_name_equal(catalogs[tree]->entries[k].name,e->name)){
        if(found>=0)return 0;found=k;
    }
    if(program->native_animation_roots_selected&&!strcmp(e->name,"root")){
        if(found>=0)return 0; /* An explicit conflicting declaration is held. */
        for(j=0;j<staged->entry_count;j++)if(staged->entries[j].catalog==(uint32_t)tree&&staged->entries[j].declaration.ordinal==0)break;
        if(j==staged->entry_count){if(j>=animation_namespace_capacity(program))return 0;memset(staged->entries+j,0,sizeof(staged->entries[j]));staged->entries[j].catalog=(uint32_t)tree;staged->entries[j].declaration.flags=GSC_ANIMATION_NODE_BRANCH|GSC_ANIMATION_NODE_IMPLICIT_ROOT;strcpy(staged->entries[j].declaration.name,"root");staged->entry_count++;}
        atom->kind=V_NAMESPACED_ANIMATION;atom->object=(uint32_t)j+1;return 1;
    }
    if(found<0)return 0;
    for(j=0;j<staged->entry_count;j++)if(staged->entries[j].catalog==(uint32_t)tree&&staged->entries[j].declaration.ordinal==(uint32_t)found+1)break;
    if(j==staged->entry_count){if(j>=animation_namespace_capacity(program))return 0;
        staged->entries[j].catalog=(uint32_t)tree;staged->entries[j].declaration=catalogs[tree]->entries[found];staged->entry_count++;
    }
    atom->kind=V_NAMESPACED_ANIMATION;atom->object=(uint32_t)j+1;return 1;
}
static int animation_namespace_select_full(GscRuntime *r,const GscAnimationNamespaceSelection *input,const GscAnimationCatalog *wire,const GscMapAnimationNamespaceSelection *map){
    static uint64_t serial;static int selecting;
    GscAnimationNamespaceSelection selection;AnimationNamespace *staged=NULL;
    const GscProgram *program;GscMapExecutionCertificate certificate;uint64_t runtime_identity;
    int n,k,accepted;unsigned char admitted[MAX_FUNCS],required[MAX_FUNCS];int scoped;
    const void *descriptor=map?(const void*)map:(const void*)input;size_t descriptor_size=map?sizeof(*map):sizeof(*input);
    if(selecting||serial==UINT64_MAX||!descriptor||(map&&input)||!animation_namespace_quiescent(r)||
       !r->program->animation_namespace_selected||!r->anim_data_enabled||r->animation_refs_enabled||
       r->animation_namespace_enabled||runtime_private_overlap(r,descriptor,descriptor_size)||
       (!!map!=!!r->program->map_animation_names.count))goto refused;
    const GscAnimationCatalog *catalogs[GSC_MAP_ANIMATION_CATALOGS]={0};
    if(map){GscMapAnimationNamespaceSelection copy=*map;for(n=0;n<GSC_MAP_ANIMATION_CATALOGS;n++)catalogs[n]=copy.catalogs[n];certificate=copy.map_certificate;}
    else{selection=*input;for(n=0;n<GSC_ANIMATION_NAMESPACE_CATALOGS;n++)catalogs[n]=selection.catalogs[n];catalogs[3]=wire;certificate=selection.map_certificate;}
    if(!certificate.receipt||!certificate.current||certificate.receipt!=r->anim_data.receipt||certificate.current!=r->anim_data.current)goto refused;
    staged=GSC_RUNTIME_CALLOC(1,sizeof(*staged));if(!staged)goto refused;staged->runtime_identity=r->array_token;staged->program=r->program;staged->map_certificate=certificate;
    for(n=0;n<animation_namespace_count(r->program);++n){const GscAnimationCatalog *c=catalogs[n];
        if(!c||runtime_private_overlap(r,c,sizeof(*c))||
           Cod1ModelStringOverlap(c,sizeof(*c),descriptor,descriptor_size)||!animation_namespace_catalog_valid(c,n,r->program))goto refused;
        for(k=0;k<n;++k)if(Cod1ModelStringOverlap(c,sizeof(*c),catalogs[k],sizeof(*c)))goto refused;
        strcpy(staged->catalogs[n].tree,c->tree);staged->catalogs[n].declared_count=c->count;staged->catalogs[n].source_token=c->token;
    }
    /* Legacy profiles still bind the whole retained graph. The additive
     * function-admission profile certifies each COMPLETE function's metadata,
     * before it can enter; unavailable future bodies remain real retained IR. */
    scoped=r->program->function_admission_selected&&r->program->turret_animations_selected;
    if(scoped){int f,pass;
        memcpy(admitted,r->function_admission,sizeof(admitted));memset(required,0,sizeof(required));
        if(!function_admission_collect(r->program,r->program->entry_fn,admitted,required,NULL,0))goto refused;
        /* Current direct/spawn closure owns capacity before future callbacks. */
        for(pass=1;pass>=0;pass--)for(f=0;f<r->program->func_count;f++)if(admitted[f]&&required[f]==pass){
            const Func *fn=&r->program->funcs[f];int atoms=staged->atom_count,entries=staged->entry_count;
            if(fn->expr_start<0||fn->expr_end<fn->expr_start||fn->expr_end>r->program->expr_count)goto refused;
            for(n=fn->expr_start;n<fn->expr_end;n++)if(!developer_expr_hidden(r->program,n)&&
                !animation_namespace_bind_atom(r->program,catalogs,staged,n)){
                admitted[f]=0;staged->atom_count=atoms;staged->entry_count=entries;break;
            }
        }
        for(n=0;n<r->program->expr_count;n++)if((r->program->expr[n].kind==E_ANIM_REF||r->program->expr[n].kind==E_ANIMTREE_VALUE)&&!developer_expr_hidden(r->program,n)){
            for(f=0;f<r->program->func_count;f++)if(n>=r->program->funcs[f].expr_start&&n<r->program->funcs[f].expr_end)break;
            if(f==r->program->func_count&&!animation_namespace_bind_atom(r->program,catalogs,staged,n))goto refused;
        }
        if(!function_admission_current_mask(r->program,r->program->entry_fn,admitted,NULL,0))goto refused;
    }else for(n=0;n<r->program->expr_count;n++)if(!animation_namespace_bind_atom(r->program,catalogs,staged,n))goto refused;
    /* No borrowed catalog/input access follows the callback. */
    program=r->program;runtime_identity=r->array_token;selecting=1;r->animation_namespace_busy=1;r->entity_certificate_busy=1;
    accepted=certificate.current(certificate.receipt);r->entity_certificate_busy=0;r->animation_namespace_busy=0;selecting=0;
    if(accepted!=1||!animation_namespace_quiescent(r)||r->program!=program||r->array_token!=runtime_identity||
       !r->anim_data_enabled||r->animation_refs_enabled||r->animation_namespace_enabled||
       r->anim_data.receipt!=certificate.receipt||r->anim_data.current!=certificate.current||!r->program->animation_namespace_selected)goto refused;
    staged->identity=++serial;r->animation_namespace=*staged;GSC_RUNTIME_FREE(staged);staged=NULL;if(scoped)memcpy(r->function_admission,admitted,sizeof(admitted));r->animation_namespace_enabled=1;return 1;
refused:GSC_RUNTIME_FREE(staged);return 0;
}

static int animation_namespace_select(GscRuntime*r,const GscAnimationNamespaceSelection*input,const GscAnimationCatalog*wire){return animation_namespace_select_full(r,input,wire,NULL);}
int gsc_runtime_select_map_animation_namespace(GscRuntime*r,const GscMapAnimationNamespaceSelection*input){return animation_namespace_select_full(r,NULL,NULL,input);}

/* Whole-program selection appends atoms in increasing expression order.
 * Scoped admission uses two passes and may be unsorted; preserve its
 * first-match scan. Neither path bypasses the resource certificate. */
static int animation_namespace_atom_find56(const AnimationNamespace *space,int expression,int sorted){
    int n;
    if(sorted){
        int lo=0,hi=space->atom_count;
        while(lo<hi){
            int mid=lo+(hi-lo)/2;
            if(space->atoms[mid].expr<expression)lo=mid+1;
            else hi=mid;
        }
        return lo<space->atom_count&&space->atoms[lo].expr==expression?lo:-1;
    }
    for(n=0;n<space->atom_count;++n)if(space->atoms[n].expr==expression)return n;
    return -1;
}
static int animation_namespace_reference_value(GscRuntime *r,const Expr *e,int expression,int line,Value *out){
    int n;Value value;
    if(!e||!out||!animation_namespace_scope(r)||(e->kind!=E_ANIM_REF&&e->kind!=E_ANIMTREE_VALUE))return rfail(r,line,"animation namespace metadata authority unavailable");
    if(!anim_data_current(r,r->executing_task))return 0;
    n=animation_namespace_atom_find56(&r->animation_namespace,expression,
        !(r->program->function_admission_selected&&r->program->turret_animations_selected));
    if(n>=0){
        memset(&value,0,sizeof(value));value.kind=r->animation_namespace.atoms[n].kind;
        value.object=r->animation_namespace.atoms[n].object;value.object_token=r->animation_namespace.identity;*out=value;return 1;
    }
    return rfail(r,line,"animation namespace atom has no copied declaration");
}
static int animation_namespace_compare(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out){
    uint32_t left,right;
    if((op!=T_EQ&&op!=T_NE)||a->kind!=b->kind||!animation_namespace_value_valid(r,a)||!animation_namespace_value_valid(r,b))
        return rfail(r,line,"animation metadata equality requires two current values of the same kind");
    left=a->object;right=b->object;
    if(!anim_data_current(r,r->executing_task))return 0;
    scalar_integer(out,op==T_EQ?left==right:left!=right);return 1;
}

int gsc_runtime_select_animation_namespace(GscRuntime *r,const GscAnimationNamespaceSelection *input){
    return animation_namespace_select(r,input,NULL);
}
