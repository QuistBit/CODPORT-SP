/* Allocation authority for models and origins born through the real GSC spawn host.
 * This receipt certifies lifetime only, never a model resource, pose, collision
 * or method result. Motion and model changes retain identity; free/reuse and
 * an observed role change cannot revive a former thread. */
typedef struct {GscEntityRef owner;int born,broken,kind;unsigned nativeContents;} ScriptModelThreadOwner;
static ScriptModelThreadOwner scriptModelThreads[MAX_GENTITIES];
static int ScriptModelThreadBody(const GscEntityRef *ref){
    const gentity_t *e;
    if(!ref||!ActorOwnerAllowed(ref,0))return 0;
    e=g_entities+ref->slot;
    if(!e->classname||e->client||e->aiCharacter||e->r.bmodel||e->r.contents||e->neverFree||
       e->s.eType!=ET_GENERAL||e->touch||e->use)return 0;
    if(!strcmp(e->classname,"script_model")&&e->s.density==COD1_SCRIPT_MODEL_NET_MARKER)return 1;
    if(!strcmp(e->classname,"script_origin")&&!e->s.density&&!e->s.modelindex&&(!e->model||!e->model[0]))return 2;
    return 0;
}
static void ScriptModelThreadBorn(const GscEntityRef *ref){
    ScriptModelThreadOwner *row;int kind;
    if(!ref||ref->slot>=MAX_GENTITIES||!(kind=ScriptModelThreadBody(ref)))return;
    row=scriptModelThreads+ref->slot;
    /* A second birth cannot replace/revive an existing receipt for this allocation. */
    if(row->born&&!memcmp(&row->owner,ref,sizeof(*ref)))return;
    memset(row,0,sizeof(*row));row->owner=*ref;row->born=1;row->kind=kind;row->nativeContents=kind==1?0x2080u:0;
}
static int ScriptModelThreadCurrent(const GscEntityRef *ref,uint64_t serial){
    ScriptModelThreadOwner *row;
    if(!ref||!serial||serial!=ref->generation||ref->slot>=MAX_GENTITIES)return 0;
    row=scriptModelThreads+ref->slot;
    if(!row->born||row->broken||memcmp(&row->owner,ref,sizeof(*ref)))return 0;
    if(ScriptModelThreadBody(ref)!=row->kind){row->broken=1;return 0;}
    return 1;
}

/* Actual supported spawn(model/origin) constructors397F0/39640 preserve
 * C4=2080/0 under the admitted transform/render-only body. No geometry grant. */
int G_Cod1RuntimeScriptMask(const GscEntityRef*ref,unsigned*out){
 ScriptModelThreadOwner*r;
 if(!ref||!out||(G_Cod1EntityStorageProtected(out,sizeof(*out))||ActorOverlap(out,sizeof(*out),scriptModelThreads,sizeof(scriptModelThreads)))||!ScriptModelThreadCurrent(ref,ref->generation))return 0;
 r=scriptModelThreads+ref->slot;if(r->nativeContents!=(r->kind==1?0x2080u:0))return 0;*out=r->nativeContents;return 1;
}
