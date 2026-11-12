/* R173 externally owned portable checkpoint. Array IDs below are owner-local
 * graph indices, never runtime handles/native IDs. No Value or host pointer is
 * copied into the graph. Runtime reconstruction stamps fresh private cookies. */
enum { GAME_PORTABLE_INT=1,GAME_PORTABLE_FLOAT,GAME_PORTABLE_STRING,
    GAME_PORTABLE_LOCALIZED,GAME_PORTABLE_VECTOR,GAME_PORTABLE_ARRAY };
typedef struct { int kind;int32_t integer;float number,vector[3];char string[128];uint32_t array; } GamePortableValue;
typedef struct { int count,numeric_count; } GamePortableArray;
typedef struct { int owner;ArrayKey key;GamePortableValue value; } GamePortableEntry;
typedef struct { int array_count,entry_count;GamePortableArray arrays[GSC_ARRAY_CONTAINER_CAPACITY];GamePortableEntry entries[GSC_ARRAY_ENTRY_CAPACITY]; } GamePortableGraph;
struct GscGameArrayOwner { GamePortableGraph *graph; };
static int game_owner_error(char *error,size_t size,const char *message) {
    if(error&&size)str_copy(error,size,message);
    return 0;
}
static int game_owner_arguments(const GscGameArrayOwner *owner,const GscRuntime *r,char *error,size_t size) {
    if(!owner||!r||(size&&!error)||
       runtime_private_overlap(r,owner,sizeof(*owner))||
       Cod1ModelStringOverlap(error,size,owner,sizeof(*owner))||
       runtime_private_overlap(r,error,size)||
       (owner->graph&&Cod1ModelStringOverlap(error,size,owner->graph,sizeof(*owner->graph))))return 0;
    return 1;
}
static int game_owner_key_valid(const ArrayKey *key) {
    return key->kind==V_NUMBER?(key->number>=0&&key->number<=GSC_ARRAY_NUMERIC_KEY_MAX):
        key->kind==V_STRING&&memchr(key->string,0,sizeof(key->string))!=NULL;
}
static int game_portable_value_valid(const GamePortableValue *v,int arrays) {
    int k;
    switch(v->kind) {
    case GAME_PORTABLE_INT:return 1;
    case GAME_PORTABLE_FLOAT:return isfinite(v->number);
    case GAME_PORTABLE_STRING:case GAME_PORTABLE_LOCALIZED:return memchr(v->string,0,sizeof(v->string))!=NULL;
    case GAME_PORTABLE_VECTOR:for(k=0;k<3;k++)if(!isfinite(v->vector[k]))return 0;return 1;
    case GAME_PORTABLE_ARRAY:return v->array&&v->array<=(uint32_t)arrays;
    default:return 0;
    }
}
/* All graph validation precedes a restore arena write or owner pointer swap.
 * Kahn's walk detects cycles, and an independent root walk rejects orphans. */
static int game_portable_graph_valid(const GamePortableGraph *g) {
    int counts[GSC_ARRAY_CONTAINER_CAPACITY],numeric[GSC_ARRAY_CONTAINER_CAPACITY];
    int maximum[GSC_ARRAY_CONTAINER_CAPACITY],indegree[GSC_ARRAY_CONTAINER_CAPACITY];
    int queue[GSC_ARRAY_CONTAINER_CAPACITY],head,tail,n,k,processed;
    unsigned char seen[GSC_ARRAY_CONTAINER_CAPACITY];
    if(!g||g->array_count<1||g->array_count>GSC_ARRAY_CONTAINER_CAPACITY||
       g->entry_count<0||g->entry_count>GSC_ARRAY_ENTRY_CAPACITY)return 0;
    memset(counts,0,sizeof(counts));memset(numeric,0,sizeof(numeric));
    memset(indegree,0,sizeof(indegree));memset(seen,0,sizeof(seen));
    for(n=0;n<GSC_ARRAY_CONTAINER_CAPACITY;n++)maximum[n]=-1;
    for(n=0;n<g->entry_count;n++) {
        const GamePortableEntry *e=&g->entries[n];int owner=e->owner-1;
        if(owner<0||owner>=g->array_count||!game_owner_key_valid(&e->key)||
           !game_portable_value_valid(&e->value,g->array_count))return 0;
        for(k=0;k<n;k++)if(g->entries[k].owner==e->owner&&array_key_equal(&g->entries[k].key,&e->key))return 0;
        counts[owner]++;
        if(e->key.kind==V_NUMBER){numeric[owner]++;if(maximum[owner]<e->key.number)maximum[owner]=e->key.number;}
        if(e->value.kind==GAME_PORTABLE_ARRAY)indegree[e->value.array-1]++;
    }
    head=tail=0;
    for(n=0;n<g->array_count;n++) {
        if(g->arrays[n].count!=counts[n]||g->arrays[n].numeric_count!=numeric[n]||numeric[n]!=maximum[n]+1)return 0;
        if(!indegree[n])queue[tail++]=n+1;
    }
    processed=0;
    while(head<tail) {
        int owner=queue[head++];processed++;
        for(n=0;n<g->entry_count;n++)if(g->entries[n].owner==owner&&g->entries[n].value.kind==GAME_PORTABLE_ARRAY) {
            int child=(int)g->entries[n].value.array-1;
            if(!--indegree[child])queue[tail++]=child+1;
        }
    }
    if(processed!=g->array_count)return 0;
    head=0;tail=1;queue[0]=1;seen[0]=1;
    while(head<tail) {
        int owner=queue[head++];
        for(n=0;n<g->entry_count;n++)if(g->entries[n].owner==owner&&g->entries[n].value.kind==GAME_PORTABLE_ARRAY) {
            int child=(int)g->entries[n].value.array-1;
            if(!seen[child]){seen[child]=1;queue[tail++]=child+1;}
        }
    }
    return tail==g->array_count;
}
GscGameArrayOwner *gsc_game_array_owner_new(void) {
    return GSC_RUNTIME_CALLOC(1,sizeof(GscGameArrayOwner));
}
void gsc_game_array_owner_free(GscGameArrayOwner *owner) {
    if(owner){GSC_RUNTIME_FREE(owner->graph);GSC_RUNTIME_FREE(owner);}
}
int gsc_game_array_owner_clear(GscGameArrayOwner *owner) {
    if(!owner)return 0;
    GSC_RUNTIME_FREE(owner->graph);owner->graph=NULL;return 1;
}
/* 1=portable, 2=pruned, 0=unavailable. No entity validation or host calls. */
static int game_owner_value(const GscRuntime *r,const Value *v,GamePortableValue *out,
    uint32_t *map,uint32_t *source_ids,int *arrays,char *error,size_t size) {
    double number;int k;
    switch(v->kind) {
    case V_NUMBER:
        if((v->integer!=0&&v->integer!=1)||(v->integer_exact!=0&&v->integer_exact!=1)||
           (!v->integer&&v->integer_exact))return game_owner_error(error,size,"game owner contains an invalid scalar");
        if(v->integer) {
            out->kind=GAME_PORTABLE_INT;
            if(v->integer_exact)out->integer=v->integer_value;
            else {number=(double)v->number;if(!isfinite(v->number)||number<INT32_MIN||number>INT32_MAX||trunc(number)!=number)return game_owner_error(error,size,"game owner contains an invalid integer");out->integer=(int32_t)number;}
        } else {if(!isfinite(v->number))return game_owner_error(error,size,"game owner contains a nonfinite float");out->kind=GAME_PORTABLE_FLOAT;memcpy(&out->number,&v->number,sizeof(out->number));}
        return 1;
    case V_STRING:case V_LOCALIZED_STRING:
        if(!memchr(v->str,0,sizeof(v->str)))return game_owner_error(error,size,"game owner contains an unterminated string");
        out->kind=v->kind==V_STRING?GAME_PORTABLE_STRING:GAME_PORTABLE_LOCALIZED;memcpy(out->string,v->str,sizeof(out->string));return 1;
    case V_VECTOR:
        for(k=0;k<3;k++)if(!isfinite(v->vector[k]))return game_owner_error(error,size,"game owner contains a nonfinite vector");
        out->kind=GAME_PORTABLE_VECTOR;memcpy(out->vector,v->vector,sizeof(out->vector));return 1;
    case V_ARRAY:
        if(!game_array_value_valid(r,v))return game_owner_error(error,size,"game owner array ownership is unavailable");
        if(!map[v->array]){if(*arrays>=GSC_ARRAY_CONTAINER_CAPACITY)return game_owner_error(error,size,"game owner array capacity exceeded");map[v->array]=(uint32_t)++*arrays;source_ids[*arrays-1]=v->array;}
        out->kind=GAME_PORTABLE_ARRAY;out->array=map[v->array];return 1;
    case V_ENTITY:
        if(!v->entity.epoch||!v->entity.generation)return game_owner_error(error,size,"game owner contains an invalid entity identity");
        return 2;
    case V_STRUCT:if(!struct_value_valid(r,v))return game_owner_error(error,size,"game owner contains an invalid structure identity");return 2;
    case V_LEVEL:return 2;
    /* Native raw9 is function and is removed; raw8 is key/value, not a
     * portable script function. Never dereference/carry a scoped code ID. */
    case V_FUNCTION:return 2;
    default:return game_owner_error(error,size,"game owner contains an unsupported value");
    }
}
int gsc_game_array_owner_capture(GscGameArrayOwner *owner,const GscRuntime *r,char *error,size_t size) {
    GamePortableGraph *candidate,*old;uint32_t map[GSC_ARRAY_CONTAINER_CAPACITY+1],source_ids[GSC_ARRAY_CONTAINER_CAPACITY];
    int arrays=1,local,n,k;
    if(!game_owner_arguments(owner,r,error,size))return 0;
    if(!r->game_array_enabled||r->advancing||r->executing_task||r->error[0]||
       r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||
       r->array_count<1||r->array_count>GSC_ARRAY_CONTAINER_CAPACITY||
       r->array_entry_count<0||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY||
       !game_array_value_valid(r,&r->game_array_root))
        return game_owner_error(error,size,"game owner capture requires a quiescent selected runtime");
    candidate=GSC_RUNTIME_CALLOC(1,sizeof(*candidate));
    if(!candidate)return game_owner_error(error,size,"game owner allocation failed");
    memset(map,0,sizeof(map));memset(source_ids,0,sizeof(source_ids));map[r->game_array_root.array]=1;source_ids[0]=r->game_array_root.array;
    for(local=1;local<=arrays;local++) {
        uint32_t source=source_ids[local-1];const ArrayContainer *a=&r->arrays[source-1];
        int count=0,numeric=0,kept_numeric=0,maximum=-1;
        if(a->keybinding){game_owner_error(error,size,"game owner contains a keybinding array");goto failed;}
        if(a->count<0||a->count>GSC_ARRAY_ENTRY_CAPACITY||a->numeric_count<0||a->numeric_count>GSC_ARRAY_NUMERIC_KEY_MAX+1)goto malformed;
        for(n=0;n<r->array_entry_count;n++) {
            const ArrayEntry *e=&r->array_entries[n];GamePortableValue value;int kept;
            if(e->owner!=(int)source)continue;
            if(!game_owner_key_valid(&e->key))goto malformed;
            for(k=0;k<n;k++)if(r->array_entries[k].owner==e->owner&&array_key_equal(&r->array_entries[k].key,&e->key))goto malformed;
            count++;
            if(e->key.kind==V_NUMBER){numeric++;if(e->key.number>=a->numeric_count)goto malformed;}
            memset(&value,0,sizeof(value));kept=game_owner_value(r,&e->value,&value,map,source_ids,&arrays,error,size);
            if(!kept)goto failed;
            if(kept==2)continue;
            if(candidate->entry_count>=GSC_ARRAY_ENTRY_CAPACITY)goto malformed;
            candidate->entries[candidate->entry_count].owner=local;
            candidate->entries[candidate->entry_count].key=e->key;
            candidate->entries[candidate->entry_count++].value=value;candidate->arrays[local-1].count++;
            if(e->key.kind==V_NUMBER){kept_numeric++;if(maximum<e->key.number)maximum=e->key.number;}
        }
        if(count!=a->count||numeric!=a->numeric_count)goto malformed;
        if(kept_numeric!=maximum+1){game_owner_error(error,size,"game owner pruning would leave numeric holes");goto failed;}
        candidate->arrays[local-1].numeric_count=kept_numeric;
    }
    candidate->array_count=arrays;
    if(!game_portable_graph_valid(candidate))goto malformed;
    old=owner->graph;owner->graph=candidate;GSC_RUNTIME_FREE(old);if(error&&size)error[0]=0;return 1;
malformed:game_owner_error(error,size,"game owner graph is invalid");
failed:GSC_RUNTIME_FREE(candidate);return 0;
}
static int game_owner_fresh_target(const GscRuntime *r) {
    int n;
    if(!game_array_selection_ready(r)||r->game_array_enabled||r->task_count||r->struct_count||
       r->array_count||r->array_entry_count||r->executing_task||r->entity_entry_depth||
       r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy)return 0;
    for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used)return 0;
    return 1;
}
static void game_portable_restore_value(const GamePortableValue *v,uint64_t token,Value *out) {
    memset(out,0,sizeof(*out));
    switch(v->kind) {
    case GAME_PORTABLE_INT:scalar_integer(out,v->integer);break;
    case GAME_PORTABLE_FLOAT:out->kind=V_NUMBER;memcpy(&out->number,&v->number,sizeof(out->number));break;
    case GAME_PORTABLE_STRING:case GAME_PORTABLE_LOCALIZED:out->kind=v->kind==GAME_PORTABLE_STRING?V_STRING:V_LOCALIZED_STRING;memcpy(out->str,v->string,sizeof(out->str));break;
    case GAME_PORTABLE_VECTOR:out->kind=V_VECTOR;memcpy(out->vector,v->vector,sizeof(out->vector));break;
    case GAME_PORTABLE_ARRAY:out->kind=V_ARRAY;out->array=v->array;out->object_token=token;break;
    }
}
int gsc_runtime_restore_game_array(GscRuntime *r,const GscGameArrayOwner *owner,char *error,size_t size) {
    const GamePortableGraph *g;Value root;int n;
    if(!game_owner_arguments(owner,r,error,size))return 0;
    if(!game_owner_fresh_target(r))return game_owner_error(error,size,"game owner restore requires a fresh unselected runtime");
    g=owner->graph;
    if(g&&!game_portable_graph_valid(g))return game_owner_error(error,size,"game owner graph is invalid");
    /* Validation completed. This callback-free, allocation-free commit has no
     * failure point; every destination arena was required to be empty. */
    if(g) {
        for(n=0;n<g->array_count;n++){r->arrays[n].count=g->arrays[n].count;r->arrays[n].numeric_count=g->arrays[n].numeric_count;r->arrays[n].keybinding=0;}
        for(n=0;n<g->entry_count;n++){ArrayEntry *e=&r->array_entries[n];e->owner=g->entries[n].owner;e->key=g->entries[n].key;game_portable_restore_value(&g->entries[n].value,r->array_token,&e->value);}
        r->array_count=g->array_count;r->array_entry_count=g->entry_count;
    } else {memset(&r->arrays[0],0,sizeof(r->arrays[0]));r->array_count=1;}
    memset(&root,0,sizeof(root));root.kind=V_ARRAY;root.array=1;root.object_token=r->array_token;
    r->game_array_root=root;r->game_array_enabled=1;if(error&&size)error[0]=0;return 1;
}
