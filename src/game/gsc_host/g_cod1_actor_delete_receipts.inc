/* R204 monotonic script-delete receipts. Included in the actual BSP owner TU.
 * Cleanup retirement is never substituted for an authorized script tombstone. */
static int Cod1ActorDeleteRowLive(uint32_t i) {
    const cod1ActorBootstrapRow_t *row;cod1ActorLeaseSnapshot_t actor;
    if(i>=cod1ActorPrefix.placedRows||!Cod1ActorPrefixCoverage(cod1ActorPrefix.epoch)||
       cod1ActorPrefixBusy)return 0;
    row=&cod1ActorPrefix.rows[i];
    return !row->retired&&row->accounted&&row->startupConsumed&&
        row->stage==COD1_ACTOR_ENTRY_ENTERED&&row->entryStatus==GSC_ENTITY_ENTRY_RETURNED&&
        row->lease.serial&&G_Cod1EntitySameAllocation(&row->entity)&&
        G_Cod1ActorReadLease(&row->lease,&actor,NULL,0)&&
        Cod1AiRefEqual(&row->entity,&actor.lease.entity);
}
static Cod1ActorDeleteRecord *Cod1ActorDeleteFind(const cod1ActorDeleteReceipt_t *input) {
    Cod1ActorDeleteRecord *record;
    if(!input||Cod1ActorPrefixPrivate(input,sizeof(*input))||!Cod1ActorPrefixOwner()||
       input->epoch!=cod1ActorPrefix.epoch||input->receipt!=cod1ActorPrefix.receipt||
       input->row>=cod1ActorPrefix.placedRows||!input->transaction)return NULL;
    record=&cod1ActorDeletes[input->row];
    return !memcmp(input,&record->receipt,sizeof(*input))?record:NULL;
}
int G_Cod1ActorBootstrapDeleteCandidate(const GscEntityRef *input,cod1ActorLease_t *out) {
    uint32_t i;
    if(!input||!out||Cod1ActorPrefixPrivate(input,sizeof(*input))||
       Cod1ActorPrefixPrivate(out,sizeof(*out))||Cod1AiOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    for(i=0;i<cod1ActorPrefix.placedRows;++i)if(Cod1AiRefEqual(input,&cod1ActorPrefix.rows[i].entity)) {
        if(cod1ActorDeletes[i].state!=COD1_ACTOR_DELETE_LIVE||!Cod1ActorDeleteRowLive(i))return 0;
        *out=cod1ActorPrefix.rows[i].lease;return 1;
    }
    return 0;
}
int G_Cod1ActorBootstrapDeleteAdmit(const GscEntityRef *input,const cod1ActorLease_t *lease,
    cod1ActorDeleteReceipt_t *out) {
    cod1ActorLease_t current;cod1ActorDeleteReceipt_t copy;uint32_t i;
    if(!input||!lease||!out||Cod1ActorPrefixPrivate(input,sizeof(*input))||
       Cod1ActorPrefixPrivate(lease,sizeof(*lease))||Cod1ActorPrefixPrivate(out,sizeof(*out))||
       Cod1AiOverlap(input,sizeof(*input),out,sizeof(*out))||
       Cod1AiOverlap(lease,sizeof(*lease),out,sizeof(*out))||cod1ActorDeleteSerial==UINT64_MAX||
       !G_Cod1ActorBootstrapDeleteCandidate(input,&current)||memcmp(lease,&current,sizeof(current)))return 0;
    for(i=0;i<cod1ActorPrefix.placedRows;++i)if(Cod1AiRefEqual(input,&cod1ActorPrefix.rows[i].entity))break;
    if(i==cod1ActorPrefix.placedRows)return 0;
    memset(&copy,0,sizeof(copy));copy.epoch=cod1ActorPrefix.epoch;copy.receipt=cod1ActorPrefix.receipt;
    copy.transaction=++cod1ActorDeleteSerial;copy.row=i;copy.entity=*input;copy.lease=*lease;
    cod1ActorDeletes[i].receipt=copy;cod1ActorDeletes[i].state=COD1_ACTOR_DELETE_ADMITTED;
    *out=copy;return 1;
}
int G_Cod1ActorBootstrapDeleteCurrent(const cod1ActorDeleteReceipt_t *input) {
    Cod1ActorDeleteRecord *record=Cod1ActorDeleteFind(input);
    return record&&(record->state==COD1_ACTOR_DELETE_ADMITTED||record->state==COD1_ACTOR_DELETE_NOTIFIED)&&
        Cod1ActorDeleteRowLive(input->row);
}
int G_Cod1ActorBootstrapDeleteNotified(const cod1ActorDeleteReceipt_t *input) {
    Cod1ActorDeleteRecord *record=Cod1ActorDeleteFind(input);
    if(!record||record->state!=COD1_ACTOR_DELETE_ADMITTED||record->notified||
       !Cod1ActorDeleteRowLive(input->row))return 0;
    /* Latch before calling the irreversible VM delivery. Failed notification
     * is terminal too; ordinary delete must never replay that borrowed call. */
    record->notified=1;record->state=COD1_ACTOR_DELETE_NOTIFIED;return 1;
}
int G_Cod1ActorBootstrapDeleteCommit(const cod1ActorDeleteReceipt_t *input) {
    Cod1ActorDeleteRecord *record=Cod1ActorDeleteFind(input);cod1ActorBootstrapRow_t *row;
    if(!record||record->state!=COD1_ACTOR_DELETE_NOTIFIED||!record->notified||
       !cod1ActorPrefix.runComplete||cod1ActorPrefix.closed||cod1ActorPrefix.invalid||
       !G_Cod1EntityDeleteRetiredIdentity(&input->entity,&input->lease))return 0;
    row=&cod1ActorPrefix.rows[input->row];
    if(!Cod1AiRefEqual(&row->entity,&input->entity)||memcmp(&row->lease,&input->lease,sizeof(row->lease))||
       row->retired||!row->accounted)return 0;
    /* No traps, VM current callback or script execution occurs between the
     * real free proof and this permanent original-owner tombstone. */
    record->state=COD1_ACTOR_DELETE_RETIRED;row->retired=1;return 1;
}
void G_Cod1ActorBootstrapDeleteFail(const cod1ActorDeleteReceipt_t *input) {
    Cod1ActorDeleteRecord *record=Cod1ActorDeleteFind(input);
    if(record&&record->state!=COD1_ACTOR_DELETE_RETIRED)record->state=COD1_ACTOR_DELETE_FAILED;
}
int G_Cod1ActorBootstrapDeleteRead(cod1ActorDeleteSnapshot_t *out,char *error,size_t size) {
    cod1ActorDeleteSnapshot_t copy;uint32_t i;
    if(!out||Cod1ActorPrefixPrivate(out,sizeof(*out))||Cod1ActorPrefixPrivate(error,size)||
       Cod1AiOverlap(out,sizeof(*out),error,size))return 0;
    if(!Cod1ActorPrefixOwner())return Cod1AiError(error,size,"delete catalog has no current original map owner");
    memset(&copy,0,sizeof(copy));copy.epoch=cod1ActorPrefix.epoch;copy.receipt=cod1ActorPrefix.receipt;
    copy.count=cod1ActorPrefix.placedRows;
    for(i=0;i<copy.count;++i){const Cod1ActorDeleteRecord *d=&cod1ActorDeletes[i];
        copy.rows[i].entity=cod1ActorPrefix.rows[i].entity;copy.rows[i].lease=cod1ActorPrefix.rows[i].lease;
        copy.rows[i].transaction=d->receipt.transaction;copy.rows[i].state=d->state;
        if(d->notified)++copy.notifiedRows;
        if(Cod1ActorDeleteAuthorized(i))++copy.retiredRows;
        if(d->state==COD1_ACTOR_DELETE_FAILED)++copy.failedRows;
    }
    *out=copy;return 1;
}
