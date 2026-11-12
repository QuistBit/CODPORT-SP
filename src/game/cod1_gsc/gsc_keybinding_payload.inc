/* R135 bounded copied getkeybinding transport; no borrowed engine strings. */
static int keybinding_command_valid(const char text[COD1_KEYBINDING_NAME_CAP]) {
    size_t n;
    if(!text||!memchr(text,0,COD1_KEYBINDING_NAME_CAP))return 0;
    for(n=0;text[n];n++)if((unsigned char)text[n]<32||(unsigned char)text[n]>126)return 0;
    return 1;
}
static int keybinding_text_valid(const char text[COD1_KEYBINDING_NAME_CAP]) {
    return text&&memchr(text,0,COD1_KEYBINDING_NAME_CAP)!=NULL;
}
static int keybinding_filled_valid(const GscKeyBindingRequest *q) {
    if(!q||q->abi!=COD1_KEYBINDING_WIRE_ABI||!q->operation_id||
       !keybinding_command_valid(q->command)||q->cg_time<0||!q->cgame_generation||
       q->count<0||q->count>2||!keybinding_text_valid(q->key1)||!keybinding_text_valid(q->key2))return 0;
    return q->count==0?(!strcmp(q->key1,"KEY_UNBOUND")&&!q->key2[0]):
        q->count==1?(q->key1[0]&&!q->key2[0]):(q->key1[0]&&q->key2[0]);
}
static int keybinding_request_valid(const GscKeyBindingRequest *q) {
    size_t n;
    if(!q||q->abi!=COD1_KEYBINDING_WIRE_ABI||!q->operation_id||!keybinding_command_valid(q->command))return 0;
    if(q->phase==COD1_KEYBINDING_PHASE_VALIDATE)return keybinding_filled_valid(q);
    if(q->phase!=COD1_KEYBINDING_PHASE_QUERY||q->cg_time!=-1||q->count!=-1||q->cgame_generation)return 0;
    for(n=0;n<COD1_KEYBINDING_NAME_CAP;n++)if(q->key1[n]||q->key2[n])return 0;
    return 1;
}
static int keybinding_intent_same(const GscKeyBindingRequest *a,const GscKeyBindingRequest *b) {
    return a&&b&&a->abi==b->abi&&a->phase==b->phase&&a->operation_id==b->operation_id&&
        !memcmp(a->command,b->command,sizeof(a->command));
}
static int keybinding_filled_same(const GscKeyBindingRequest *a,const GscKeyBindingRequest *b) {
    return keybinding_intent_same(a,b)&&a->cg_time==b->cg_time&&a->count==b->count&&
        a->cgame_generation==b->cgame_generation&&!memcmp(a->key1,b->key1,sizeof(a->key1))&&
        !memcmp(a->key2,b->key2,sizeof(a->key2));
}
static int keybinding_reply_valid(const GscKeyBindingRequest *before,const GscKeyBindingRequest *after) {
    return keybinding_request_valid(before)&&keybinding_intent_same(before,after)&&
        keybinding_filled_valid(after)&&(before->phase==COD1_KEYBINDING_PHASE_QUERY||keybinding_filled_same(before,after));
}
static int keybinding_no_alias(const GscKeyBindingRequest *query,const char *error,size_t capacity) {
    uintptr_t a,b;
    if(!query||!error||!capacity)return 1;
    a=(uintptr_t)query;b=(uintptr_t)error;
    return b>=a?b-a>=sizeof(*query):a-b>=capacity;
}
