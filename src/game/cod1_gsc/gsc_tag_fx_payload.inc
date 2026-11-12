#ifndef GSC_TAG_FX_PAYLOAD_INCLUDED
#define GSC_TAG_FX_PAYLOAD_INCLUDED
/* R124 copied tagged-FX request. model_identity is an opaque nonwrapping
 * provider cookie, not a recovered retail field or a wire/entity ABI change. */
static int tag_fx_ref_valid(const GscEntityRef *e) {
    return e && e->epoch && e->generation && e->slot<GSC_TAG_FX_ENTITY_LIMIT;
}
static int tag_fx_ref_same(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch && a->generation==b->generation && a->slot==b->slot;
}
static int tag_fx_name_valid(const char *text,size_t size) {
    size_t n;
    if(!text || !memchr(text,0,size))return 0;
    for(n=0;text[n];n++)if((unsigned char)text[n]<32 ||
        (unsigned char)text[n]>126 || text[n]=='"')return 0;
    return 1;
}
static int tag_fx_query_valid(const GscTagFxQuery *q) {
    if(!q || !q->operation_id || q->operation_id==UINT64_MAX ||
       !tag_fx_ref_valid(&q->entity) || q->index<1 || q->index>=64 ||
       !memchr(q->model_name,0,sizeof(q->model_name)))return 0;
    if(q->phase==GSC_TAG_FX_MODEL)
        return q->model_present==0 && !q->model_identity && q->bone_index==-1 &&
            !q->tag[0] && !q->model_name[0];
    return q->phase==GSC_TAG_FX_BONE && q->model_present==1 && q->model_identity &&
        q->model_identity!=UINT64_MAX && q->bone_index==-1 &&
        tag_fx_name_valid(q->tag,sizeof(q->tag));
}
static int tag_fx_reply_valid(const GscTagFxQuery *before,const GscTagFxQuery *after) {
    if(!tag_fx_query_valid(before) || before->phase!=after->phase ||
       before->operation_id!=after->operation_id || before->index!=after->index ||
       !tag_fx_ref_same(&before->entity,&after->entity) ||
       memcmp(before->tag,after->tag,sizeof(before->tag)) ||
       !memchr(after->model_name,0,sizeof(after->model_name)))return 0;
    if(before->phase==GSC_TAG_FX_MODEL)
        return (after->model_present==0 || after->model_present==1) &&
            (after->model_present?(after->model_identity && after->model_identity!=UINT64_MAX):!after->model_identity) &&
            after->bone_index==-1;
    return after->model_present==before->model_present &&
        after->model_identity==before->model_identity &&
        !memcmp(before->model_name,after->model_name,sizeof(before->model_name)) &&
        after->bone_index>=-1 && after->bone_index<GSC_TAG_FX_BONE_LIMIT;
}
static int tag_fx_event_valid(const GscTagFxEvent *e) {
    char expected[GSC_TAG_FX_TOKEN_CAPACITY];int n;
    if(!e || !e->operation_id || e->operation_id==UINT64_MAX ||
       !tag_fx_ref_valid(&e->entity) || e->index<1 || e->index>=64 ||
       !e->model_identity || e->model_identity==UINT64_MAX ||
       !tag_fx_name_valid(e->tag,sizeof(e->tag)) ||
       !memchr(e->token,0,sizeof(e->token)))return 0;
    n=snprintf(expected,sizeof(expected),"%02d%s",(int)e->index,e->tag);
    return n>=0 && (size_t)n<sizeof(expected) && !strcmp(expected,e->token);
}
static int tag_fx_event_same(const GscTagFxEvent *a,const GscTagFxEvent *b) {
    return a->operation_id==b->operation_id && a->index==b->index &&
        a->model_identity==b->model_identity && tag_fx_ref_same(&a->entity,&b->entity) &&
        !memcmp(a->tag,b->tag,sizeof(a->tag)) && !memcmp(a->token,b->token,sizeof(a->token));
}
#endif
