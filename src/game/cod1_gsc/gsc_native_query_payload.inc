/* New optional R128 query transport. This is a port-owned synchronous copy,
 * not the retail script VM representation or a persistent SAVE record. */
static int native_query_empty_bytes(const char *value,size_t size) {
    size_t n;for(n=0;n<size;n++)if(value[n])return 0;return 1;
}
/* R148 canonical difficulty transport. Retail returns these four literal
 * names; the port refuses an invalid cached index and noncanonical replies. */
static int native_query_difficulty_name_valid(const char value[128]) {
    static const char *names[4]={"easy","medium","hard","fu"};
    size_t n,length;
    for(n=0;n<4;++n) {
        length=strlen(names[n]);
        if(!memcmp(value,names[n],length) &&
           native_query_empty_bytes(value+length,128-length))return 1;
    }
    return 0;
}
static int native_query_request_valid(const GscNativeQuery *q) {
    return q && q->operation_id &&
        ((q->kind>=GSC_QUERY_CVAR_STRING&&q->kind<=GSC_QUERY_CVAR_FLOAT) ||
         q->kind==GSC_QUERY_LEVEL_TIME || q->kind==GSC_QUERY_DIFFICULTY) && memchr(q->name,0,sizeof(q->name)) &&
        !q->integer_value && q->float_value==0 && !q->string_value[0] &&
        ((q->kind!=GSC_QUERY_LEVEL_TIME && q->kind!=GSC_QUERY_DIFFICULTY) ||
         (native_query_empty_bytes(q->name,sizeof(q->name)) &&
          native_query_empty_bytes(q->string_value,sizeof(q->string_value))));
}
static int native_query_reply_valid(const GscNativeQuery *before,const GscNativeQuery *after) {
    if(!native_query_request_valid(before) || !after ||
       before->operation_id!=after->operation_id || before->kind!=after->kind ||
       memcmp(before->name,after->name,sizeof(before->name)))return 0;
    if(after->kind==GSC_QUERY_CVAR_STRING)return !after->integer_value &&
        after->float_value==0 && memchr(after->string_value,0,sizeof(after->string_value));
    if(after->kind==GSC_QUERY_DIFFICULTY)return !after->integer_value &&
        after->float_value==0 && native_query_difficulty_name_valid(after->string_value);
    if(after->kind==GSC_QUERY_LEVEL_TIME)return after->float_value==0 &&
        native_query_empty_bytes(after->string_value,sizeof(after->string_value));
    if(after->kind==GSC_QUERY_CVAR_INTEGER)return after->float_value==0 &&
        !after->string_value[0];
    return !after->integer_value && !after->string_value[0] && isfinite(after->float_value);
}
