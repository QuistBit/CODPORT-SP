/* R149 pure getentbynum pool read. Retail tests a 2048-record native pool's
 * inuse byte, then publishes the record's entity-slot word. The port returns
 * only an actual current epoch/slot/generation allocation. Its smaller pool
 * is explicit capacity policy, never a synthetic original inuse assertion. */
int G_Cod1EntityByNumber(int32_t slot,GscEntityRef *out,int *found,
    char *error,size_t size) {
    uint64_t epoch=G_Cod1EntityMapEpoch();
    GscEntityRef ref;int result=0,entityCount=level.num_entities;
    if(EntityCollectionOverlap(out,sizeof(*out),error,size) ||
       EntityCollectionOverlap(found,sizeof(*found),error,size))return 0;
    if(!out || !found || EntityCollectionOverlap(out,sizeof(*out),found,sizeof(*found))) {
        EntityError(error,size,"getentbynum requires separate copied outputs");return 0;
    }
    if(!G_Cod1EntityCollectionReady(epoch,entityCount) || level.restarted) {
        EntityError(error,size,"getentbynum requires a ready fresh CoD SP map");return 0;
    }
    memset(&ref,0,sizeof(ref));
    if((uint32_t)slot<2048 && (uint32_t)slot<MAX_GENTITIES && g_entities[slot].inuse) {
        if(!G_Cod1EntityCapture(&g_entities[slot],&ref) || ref.slot!=(uint32_t)slot ||
           !EntityValidate(NULL,&ref)) {
            EntityError(error,size,"getentbynum active allocation has inconsistent identity");return 0;
        }
        result=1;
    }
    /* No callback or trap occurs between read and copied publication. */
    *out=ref;*found=result;return 1;
}
