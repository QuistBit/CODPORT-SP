/* R127 paired request decoder/cache only. No current arbitrary actor/model
 * pose exists here. Admitted requests are consumed without a visual or replay
 * queue; the entity origin and first-person viewmodel are never substitutes.
 * Each request supplies its raw dictionary entry, so late clients need no
 * aggregate configstring table or unreliable bootstrap ordering. */
static struct {
    uint64_t epoch,map_epoch,sequence;
    char tokens[COD1_TAG_FX_SLOTS][COD1_TAG_FX_TOKEN_CAPACITY];
    int have_time,last_time;
} cgCod1TagFx;
static int cgCod1TagFxBusy,cgCod1TagFxCancelled;

static void CFT_Reset(qboolean full) {
    if(full)memset(&cgCod1TagFx,0,sizeof(cgCod1TagFx));
    /* A soft visual/clock reset retains scene replay and entry identities. */
    else {cgCod1TagFx.have_time=0;cgCod1TagFx.last_time=0;}
}
static int CFT_Admit(const cod1TagFx_t *event) {
    int index;
    if(!CT_Validate(event) || event->epoch!=cgCod1ScriptFx.epoch || cg.time<0 ||
       (cgCod1TagFx.epoch==event->epoch && event->sequence<=cgCod1TagFx.sequence))return 0;
    if(cgCod1TagFx.epoch==event->epoch &&
       (event->map_epoch!=cgCod1TagFx.map_epoch ||
        (cgCod1TagFx.have_time && cg.time<cgCod1TagFx.last_time)))return 0;
    index=(int)event->dictionary_slot;
    if(cgCod1TagFx.epoch==event->epoch && cgCod1TagFx.tokens[index][0] &&
       strcmp(cgCod1TagFx.tokens[index],event->token))return 0;
    /* The transport token keeps raw case/spelling. A slot cannot be rebound
     * within the scene, even while its actor or model is unsupported. */
    if(cgCod1TagFx.epoch!=event->epoch) {
        CFT_Reset(qtrue);cgCod1TagFx.epoch=event->epoch;
        cgCod1TagFx.map_epoch=event->map_epoch;
    }
    strcpy(cgCod1TagFx.tokens[index],event->token);
    cgCod1TagFx.sequence=event->sequence;cgCod1TagFx.have_time=1;
    cgCod1TagFx.last_time=cg.time;return 1;
}
void CG_Cod1TagFxCommand(void) {
    /* One extra byte makes an overlong argument fail instead of admitting a
     * valid maximum-length prefix after trap_Argv truncation. */
    char copied[COD1_TAG_FX_ARGC][COD1_TAG_FX_HEX_CAPACITY+1];
    const char *arguments[COD1_TAG_FX_ARGC];
    cod1ScriptFxClientLease_t lease;
    cod1TagFx_t event;
    int i,count;
    if(!cgCod1ScriptFx.epoch)return;
    if(cgCod1TagFxBusy){cgCod1TagFxCancelled=1;return;}
    cgCod1TagFxBusy=1;cgCod1TagFxCancelled=0;
    lease=CFS_Lease(0);count=trap_Argc();
    if(cgCod1TagFxCancelled || !CFS_Current(&lease) || count!=COD1_TAG_FX_ARGC)goto leave;
    for(i=0;i<count;++i) {
        memset(copied[i],0,sizeof(copied[i]));
        trap_Argv(i,copied[i],sizeof(copied[i]));
        if(cgCod1TagFxCancelled || !CFS_Current(&lease) || !memchr(copied[i],0,sizeof(copied[i])))goto leave;
        arguments[i]=copied[i];
    }
    if(!CT_DecodeCommand(count,arguments,&event) || cgCod1TagFxCancelled ||
       !CFS_Current(&lease) || !CFT_Admit(&event))goto leave;
    /* Publication was consumed. Future rendering requires a separate proved
     * current actor generation/model cookie/tag pose boundary. */
leave:
    cgCod1TagFxBusy=0;
}
