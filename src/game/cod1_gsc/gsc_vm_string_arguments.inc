/* R165 per-invocation owned Value copies. The port stores strings/vectors
 * inline; object references retain the owning runtime's monotonic containers.
 * Ending this frame drops its aliases, never deletes a caller local/container.
 * No fake native pool IDs/refcounts, callbacks or global method registration. */
typedef struct {
    Value cells[MAX_PARAMS];int argc,failed;unsigned converted,parameter_error;
} StringArguments;
static int string_arguments_begin(GscRuntime *r,const Value *args,int argc,
    int line,StringArguments *scope) {
    if(!scope || argc<0 || argc>MAX_PARAMS || (argc && !args) || r->error[0] ||
       Cod1ModelStringOverlap(scope,sizeof(*scope),args,(size_t)argc*sizeof(*args)))
        return rfail(r,line,"invalid native string argument frame");
    memset(scope,0,sizeof(*scope));scope->argc=argc;
    if(argc)memcpy(scope->cells,args,(size_t)argc*sizeof(*args));
    return 1;
}
static int string_arguments_get(GscRuntime *r,StringArguments *scope,unsigned index,
    int line,const char **out) {
    GscStringCastValue value;GscStringCastResult result;Value *cell,copy;
    char error[128];int cast;
    if(!scope || !out || scope->failed || r->error[0])return 0;
    if(scope->argc<0 || scope->argc>MAX_PARAMS)goto unavailable;
    if(index>=(unsigned)scope->argc){scope->failed=1;
        return rfail(r,line,"parameter %d does not exist",(int)scalar_signed((uint32_t)index+1));}
    cell=&scope->cells[index];if(!animation_value_exportable(r,cell))goto unavailable;memset(&value,0,sizeof(value));
    switch(cell->kind) {
    case V_UNDEFINED:value.type=0;break;
    case V_STRING:value.type=1;memcpy(value.string,cell->str,sizeof(value.string));break;
    case V_LOCALIZED_STRING:value.type=2;memcpy(value.string,cell->str,sizeof(value.string));break;
    case V_NUMBER:
        if(cell->integer!=0 && cell->integer!=1)goto unavailable;
        if(cell->integer){value.type=5;
            if(cell->integer_exact)value.integer=cell->integer_value;
            else if(!Cod1ModelLegacyInt(cell->number,&value.integer))goto unavailable;
        } else{value.type=4;value.number=cell->number;}break;
    case V_VECTOR:value.type=3;memcpy(value.vector,cell->vector,sizeof(value.vector));break;
    /* Original raw Value.type7 owns all object-table references. Entity/array/
     * struct subtype names are not raw stack tags; CastString reports object.
     * The owned frame clears its alias on rejection. The runtime keeps roots. */
    case V_ENTITY:case V_ARRAY:case V_STRUCT:value.type=7;break;
    default:goto unavailable;
    }
    cast=GscStringCast(&value,&result,error,sizeof(error));
    if(!cast)goto unavailable;
    if(cast<0){cell->kind=V_UNDEFINED;scope->parameter_error=index+1;scope->failed=1;
        return rfail(r,line,"%s",error);}
    if(result.converted){memset(&copy,0,sizeof(copy));copy.kind=V_STRING;
        memcpy(copy.str,result.string,sizeof(copy.str));*cell=copy;scope->converted|=1u<<index;}
    *out=cell->str;return 1;
unavailable:
    scope->failed=1;return rfail(r,line,"native string conversion is outside the admitted value domain");
}
static void string_arguments_end(StringArguments *scope) {
    if(scope)memset(scope,0,sizeof(*scope));
}
