#include "../g_cod1_animation_start_origin_session.h"
#include "../../renderer/cod1_actor_start_sample14.h"
#include "../../qcommon/cod1_native_start_origin.h"

typedef struct {
    cod1StartOriginRef_t ref;
    ca14StartSample sample;
    uint32_t parent;
    size_t sourceOffset;
} Cod1StartOriginRow;
typedef struct {
    uint64_t epoch, receipt;
    size_t count;
    Cod1StartOriginRow rows[COD1_START_ORIGIN_MAX];
} Cod1StartOriginBank;
static Cod1StartOriginBank cod1StartOrigins, cod1StartOriginsStage;
static int StartOriginTypedQuery(void *,const GscStartOriginRequest *,
    GscStartOriginReply *,char *,size_t,int (*)(void *),void *);
static const GscStartOriginHost cod1StartOriginTypedHost={NULL,StartOriginTypedQuery};
static int StartOriginNamespaceTypedQuery(void *,const GscStartOriginRequest *,
    GscStartOriginReply *,char *,size_t,int (*)(void *),void *);
static const GscStartOriginHost cod1NamespaceStartOriginTypedHost={NULL,StartOriginNamespaceTypedQuery};
static uint64_t cod1StartOriginReceiptSerial, cod1StartOriginOperationSerial;
static struct { int active, cancelled; uint64_t serial; } cod1StartOriginGuard;

static void StartOriginResourcesReset(void) {
    if (cod1StartOriginGuard.active) cod1StartOriginGuard.cancelled = 1;
    memset(&cod1StartOrigins, 0, sizeof(cod1StartOrigins));
    memset(&cod1StartOriginsStage, 0, sizeof(cod1StartOriginsStage));
    /* Never erase serials or an in-flight guard while its frame is live. */
}

static int StartOriginPrivate(const void *p, size_t n) {
    return ActorOverlap(p,n,&cod1StartOriginTypedHost,sizeof(cod1StartOriginTypedHost)) ||
        ActorOverlap(p,n,&cod1NamespaceStartOriginTypedHost,sizeof(cod1NamespaceStartOriginTypedHost)) ||
        AmmoAnglesPrivate(p, n) ||
        ActorOverlap(p,n,&cod1StartOrigins,sizeof(cod1StartOrigins)) ||
        ActorOverlap(p,n,&cod1StartOriginsStage,sizeof(cod1StartOriginsStage)) ||
        ActorOverlap(p,n,&cod1StartOriginGuard,sizeof(cod1StartOriginGuard)) ||
        ActorOverlap(p,n,&cod1StartOriginReceiptSerial,sizeof(cod1StartOriginReceiptSerial)) ||
        ActorOverlap(p,n,&cod1StartOriginOperationSerial,sizeof(cod1StartOriginOperationSerial)) ||
        ActorOverlap(p,n,&animDataSerial,sizeof(animDataSerial)) ||
        ActorOverlap(p,n,&animDataEpoch,sizeof(animDataEpoch)) ||
        ActorOverlap(p,n,&animDataReceipt,sizeof(animDataReceipt)) ||
        ActorOverlap(p,n,&animDataPending,sizeof(animDataPending)) ||
        ActorOverlap(p,n,&animDataRuntime,sizeof(animDataRuntime)) ||
        ActorOverlap(p,n,&animDataBusy,sizeof(animDataBusy)) ||
        ActorOverlap(p,n,&animationRefsSerial,sizeof(animationRefsSerial)) ||
        ActorOverlap(p,n,&animationRefsEpoch,sizeof(animationRefsEpoch)) ||
        ActorOverlap(p,n,&animationRefsReceipt,sizeof(animationRefsReceipt)) ||
        ActorOverlap(p,n,&animationRefsPending,sizeof(animationRefsPending)) ||
        ActorOverlap(p,n,&animationRefsRuntime,sizeof(animationRefsRuntime)) ||
        ActorOverlap(p,n,&animationRefsBusy,sizeof(animationRefsBusy));
}

static int StartOriginName(const char *name, size_t *length, int tree) {
    size_t n;
    if (!name) return 0;
    for (n=0; n<64 && name[n]; ++n) {
        unsigned char c=(unsigned char)name[n];
        if (!((c>='a'&&c<='z') || (!tree&&c>='A'&&c<='Z') ||
              (c>='0'&&c<='9') || c=='_')) return 0;
    }
    if (!n || n==64) return 0;
    *length=n+1; return 1;
}
static int StartOriginNameEqual(const char *a, const char *b) {
    for (;;) {
        unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;
        if (x>='A' && x<='Z') x+=32;
        if (y>='A' && y<='Z') y+=32;
        if (x!=y) return 0;
        if (!x) return 1;
    }
}

static int StartOriginEnter(uint64_t *serial, char *error, size_t size) {
    if (cod1StartOriginGuard.active) {
        cod1StartOriginGuard.cancelled=1;
        EntityError(error,size,"start-origin resource operation reentered"); return 0;
    }
    if (cod1StartOriginOperationSerial==UINT64_MAX) {
        EntityError(error,size,"start-origin operation identity exhausted"); return 0;
    }
    *serial=++cod1StartOriginOperationSerial;
    cod1StartOriginGuard.active=1; cod1StartOriginGuard.cancelled=0;
    cod1StartOriginGuard.serial=*serial; return 1;
}
static int StartOriginMapAvailable(void) {
    return cod1EntityEpoch!=UINT64_MAX && cod1EntityReady &&
        !level.spawning && !level.restarted && ActorMapAvailable();
}
static int StartOriginOperationCurrent(uint64_t serial, uint64_t epoch) {
    return epoch && StartOriginMapAvailable() && G_Cod1EntityMapEpoch()==epoch && cod1StartOriginGuard.active &&
        !cod1StartOriginGuard.cancelled && cod1StartOriginGuard.serial==serial;
}
static void StartOriginLeave(uint64_t serial) {
    if (cod1StartOriginGuard.serial==serial) {
        cod1StartOriginGuard.active=0; cod1StartOriginGuard.cancelled=0;
    }
}

int G_Cod1SelectStartOriginResources(const cod1StartOriginResourceInput_t *input,
    size_t count, char *error, size_t size) {
    uint64_t serial,epoch,receipt;
    size_t i,j,treeLength,nameLength;
    GscAnimationCatalog *catalog=NULL;
    GscAnimationCatalogEntry entry;
    int ok=0,found;
    if ((size&&!error) || StartOriginPrivate(error,size)) return 0;
    if (!input || !count || count>COD1_START_ORIGIN_MAX) {
        EntityError(error,size,"start-origin resource input unavailable"); return 0;
    }
    if (StartOriginPrivate(input,count*sizeof(*input)) ||
        ActorOverlap(input,count*sizeof(*input),error,size)) return 0;
    /* Admit every source range before diagnostics, stage writes or parsing. */
    for (i=0;i<count;++i) {
        if (!input[i].tree || !input[i].name || !input[i].atr_bytes ||
            !input[i].xanim_bytes || !input[i].atr_size || input[i].atr_size>131072 ||
            !input[i].xanim_size || input[i].xanim_size>CA14_ACTOR_CLIP_LIMIT) return 0;
        if (!StartOriginName(input[i].tree,&treeLength,1) ||
            !StartOriginName(input[i].name,&nameLength,0)) return 0;
        if (StartOriginPrivate(input[i].tree,treeLength) ||
            StartOriginPrivate(input[i].name,nameLength) ||
            StartOriginPrivate(input[i].atr_bytes,input[i].atr_size) ||
            StartOriginPrivate(input[i].xanim_bytes,input[i].xanim_size) ||
            ActorOverlap(input[i].tree,treeLength,error,size) ||
            ActorOverlap(input[i].name,nameLength,error,size) ||
            ActorOverlap(input[i].atr_bytes,input[i].atr_size,error,size) ||
            ActorOverlap(input[i].xanim_bytes,input[i].xanim_size,error,size)) return 0;
    }
    if (!StartOriginEnter(&serial,error,size)) return 0;
    epoch=G_Cod1EntityMapEpoch();
    if (!StartOriginOperationCurrent(serial,epoch) || cod1StartOrigins.receipt ||
        cod1StartOriginReceiptSerial==UINT64_MAX) goto done;
    receipt=++cod1StartOriginReceiptSerial;
    memset(&cod1StartOriginsStage,0,sizeof(cod1StartOriginsStage));
    for (i=0;i<count;++i) {
        Cod1StartOriginRow *row=&cod1StartOriginsStage.rows[i];
        for (j=0;j<i;++j) if (!strcmp(input[i].tree,input[j].tree)) {
            if (input[i].atr_size!=input[j].atr_size ||
                memcmp(input[i].atr_bytes,input[j].atr_bytes,input[i].atr_size) ||
                StartOriginNameEqual(input[i].name,input[j].name)) goto done;
        }
        catalog=gsc_animation_catalog_parse(input[i].tree,input[i].atr_bytes,
                                             input[i].atr_size,error,size);
        if (!catalog || !StartOriginOperationCurrent(serial,epoch)) goto done;
        found=0;
        for (j=0;j<gsc_animation_catalog_count(catalog);++j) {
            if (!gsc_animation_catalog_entry(catalog,j,&entry)) goto done;
            if (StartOriginNameEqual(entry.name,input[i].name)) {found=1;break;}
        }
        if (!found || (entry.flags&GSC_ANIMATION_NODE_BRANCH) ||
            !CA14_StartSampleLoad(input[i].xanim_bytes,input[i].xanim_size,&row->sample) ||
            !StartOriginOperationCurrent(serial,epoch)) goto done;
        row->ref.epoch=epoch; row->ref.receipt=receipt; row->ref.row=(uint32_t)i;
        row->ref.ordinal=entry.ordinal; row->ref.flags=entry.flags;
        strcpy(row->ref.tree,input[i].tree); strcpy(row->ref.name,entry.name);
        row->parent=entry.parent; row->sourceOffset=entry.source_offset;
        gsc_animation_catalog_free(catalog); catalog=NULL;
    }
    cod1StartOriginsStage.epoch=epoch; cod1StartOriginsStage.receipt=receipt;
    cod1StartOriginsStage.count=count;
    if (!StartOriginOperationCurrent(serial,epoch)) goto done;
    cod1StartOrigins=cod1StartOriginsStage; ok=1;
 done:
    if (catalog) gsc_animation_catalog_free(catalog);
    memset(&cod1StartOriginsStage,0,sizeof(cod1StartOriginsStage));
    StartOriginLeave(serial);
    if (!ok) EntityError(error,size,"start-origin resource admission refused");
    return ok;
}

int G_Cod1StartOriginResourceRef(const char *tree, const char *name,
    cod1StartOriginRef_t *out, char *error, size_t size) {
    uint64_t serial,epoch;
    size_t i,treeLength,nameLength;
    cod1StartOriginRef_t staged;
    int ok=0;
    if ((size&&!error) || StartOriginPrivate(error,size) || StartOriginPrivate(out,sizeof(*out)) ||
        ActorOverlap(out,sizeof(*out),error,size)) return 0;
    if (!tree || !name || !out || !StartOriginName(tree,&treeLength,1) ||
        !StartOriginName(name,&nameLength,0)) return 0;
    if (ActorOverlap(tree,treeLength,error,size) || ActorOverlap(name,nameLength,error,size) ||
        ActorOverlap(tree,treeLength,out,sizeof(*out)) ||
        ActorOverlap(name,nameLength,out,sizeof(*out))) return 0;
    if (!StartOriginEnter(&serial,error,size)) return 0;
    epoch=G_Cod1EntityMapEpoch();
    if (!StartOriginOperationCurrent(serial,epoch) || cod1StartOrigins.epoch!=epoch ||
        !cod1StartOrigins.receipt) goto done;
    for (i=0;i<cod1StartOrigins.count;++i) {
        staged=cod1StartOrigins.rows[i].ref;
        if (!strcmp(tree,staged.tree) && StartOriginNameEqual(name,staged.name)) {
            *out=staged; ok=1; break;
        }
    }
 done:
    StartOriginLeave(serial);
    if (!ok) EntityError(error,size,"start-origin selected leaf unavailable");
    return ok;
}

static int StartOriginQueryCurrent(uint64_t serial, const Cod1StartOriginRow *row) {
    return StartOriginOperationCurrent(serial,row->ref.epoch) &&
        cod1StartOrigins.epoch==row->ref.epoch && cod1StartOrigins.receipt==row->ref.receipt &&
        row->ref.row<cod1StartOrigins.count &&
        !memcmp(row,&cod1StartOrigins.rows[row->ref.row],sizeof(*row));
}

int G_Cod1GetStartOrigin(const cod1StartOriginRef_t *input, const float origin[3],
    const float angles[3], float out[3], char *error, size_t size,
    int (*current)(void *), void *context) {
    cod1StartOriginRef_t ref;
    Cod1StartOriginRow row;
    float copiedOrigin[3],copiedAngles[3],staged[3];
    uint64_t serial;
    int ok=0;
    if ((size&&!error) || StartOriginPrivate(error,size) || StartOriginPrivate(out,sizeof(staged)) ||
        ActorOverlap(input,sizeof(*input),error,size) ||
        ActorOverlap(origin,sizeof(copiedOrigin),error,size) ||
        ActorOverlap(angles,sizeof(copiedAngles),error,size) ||
        ActorOverlap(out,sizeof(staged),error,size) ||
        ActorOverlap(input,sizeof(*input),out,sizeof(staged)) ||
        ActorOverlap(origin,sizeof(copiedOrigin),out,sizeof(staged)) ||
        ActorOverlap(angles,sizeof(copiedAngles),out,sizeof(staged))) return 0;
    if (!input || !origin || !angles || !out || !current) {
        EntityError(error,size,"start-origin query requires copied resource and caller");return 0;
    }
    ref=*input; memcpy(copiedOrigin,origin,sizeof(copiedOrigin));
    memcpy(copiedAngles,angles,sizeof(copiedAngles));
    if (!StartOriginEnter(&serial,error,size)) return 0;
    if (ref.row>=cod1StartOrigins.count || ref.row>=COD1_START_ORIGIN_MAX) goto done;
    row=cod1StartOrigins.rows[ref.row];
    if (memcmp(&ref,&row.ref,sizeof(ref)) || !StartOriginQueryCurrent(serial,&row)) goto done;
    if (current(context)!=1 || !StartOriginQueryCurrent(serial,&row) ||
        StartOriginPrivate(out,sizeof(staged)) || StartOriginPrivate(error,size) ||
        memcmp(input,&ref,sizeof(ref)) || memcmp(origin,copiedOrigin,sizeof(copiedOrigin)) ||
        memcmp(angles,copiedAngles,sizeof(copiedAngles))) goto done;
    if (!C1NSO_Transform(copiedOrigin,copiedAngles,row.sample.translation,staged)) goto done;
    if (current(context)!=1 || !StartOriginQueryCurrent(serial,&row) ||
        StartOriginPrivate(out,sizeof(staged)) || StartOriginPrivate(error,size) ||
        memcmp(input,&ref,sizeof(ref)) || memcmp(origin,copiedOrigin,sizeof(copiedOrigin)) ||
        memcmp(angles,copiedAngles,sizeof(copiedAngles))) goto done;
    memcpy(out,staged,sizeof(staged)); ok=1;
 done:
    StartOriginLeave(serial);
    /* A caller fence may have rebound the actual active client pool. Never
     * diagnose through a range that became protected during that callback. */
    if (!ok && !StartOriginPrivate(error,size)) EntityError(error,size,"start-origin query refused");
    return ok;
}

/* A pure map/resource receipt, deliberately independent of the synchronous
 * operation guard: Runtime and Session must inspect it inside that guard. */
static int StartOriginMapCertificateCurrent(uint64_t receipt) {
    return receipt && StartOriginMapAvailable() &&
        cod1StartOrigins.epoch==G_Cod1EntityMapEpoch() &&
        cod1StartOrigins.receipt==receipt && cod1StartOrigins.count &&
        cod1StartOrigins.count<=COD1_START_ORIGIN_MAX;
}
int G_Cod1StartOriginMapCertificate(GscMapExecutionCertificate *out,
    char *error,size_t size) {
    GscMapExecutionCertificate staged;
    if(!out || (size&&!error) || StartOriginPrivate(out,sizeof(*out)) ||
       StartOriginPrivate(error,size) || ActorOverlap(out,sizeof(*out),error,size)) return 0;
    staged.receipt=cod1StartOrigins.receipt;
    staged.current=StartOriginMapCertificateCurrent;
    if(!staged.current(staged.receipt)) {
        EntityError(error,size,"start-origin map resource certificate unavailable");return 0;
    }
    *out=staged;return 1;
}
typedef struct {
    const GscStartOriginRequest *input;
    GscStartOriginRequest copied;
    GscStartOriginReply *out;
    char *error;size_t size;
    uint64_t receipt;
    int (*current)(void *);void *context;
} Cod1StartOriginTypedLease;
static int StartOriginTypedPure(const Cod1StartOriginTypedLease *lease) {
    return lease && StartOriginMapCertificateCurrent(lease->receipt) &&
        !StartOriginPrivate(lease->out,sizeof(*lease->out)) &&
        !StartOriginPrivate(lease->error,lease->size) &&
        !memcmp(lease->input,&lease->copied,sizeof(lease->copied));
}
static int StartOriginTypedCurrent(void *context) {
    Cod1StartOriginTypedLease *lease=(Cod1StartOriginTypedLease *)context;
    return StartOriginTypedPure(lease) && lease->current(lease->context)==1 &&
        StartOriginTypedPure(lease);
}
static int StartOriginTypedQuery(void *user,const GscStartOriginRequest *input,
    GscStartOriginReply *out,char *error,size_t size,
    int (*current)(void *),void *context) {
    Cod1StartOriginTypedLease lease;
    cod1StartOriginRef_t ref;
    GscStartOriginReply staged;
    (void)user;
    if(!input || !out || !current || (size&&!error) ||
       StartOriginPrivate(input,sizeof(*input)) || StartOriginPrivate(out,sizeof(*out)) ||
       StartOriginPrivate(error,size) || ActorOverlap(input,sizeof(*input),out,sizeof(*out)) ||
       ActorOverlap(input,sizeof(*input),error,size) || ActorOverlap(out,sizeof(*out),error,size)) return 0;
    memset(&lease,0,sizeof(lease));lease.input=input;lease.copied=*input;
    lease.out=out;lease.error=error;lease.size=size;lease.current=current;lease.context=context;
    if(memcmp(lease.copied.tree,"generic_human",14) ||
       memcmp(lease.copied.name,"training_guy_practicejump",26) ||
       !lease.copied.runtime_identity || !lease.copied.catalog_identity ||
       lease.copied.now_ms<0 || !lease.copied.ordinal || lease.copied.flags) goto refuse;
    if(!G_Cod1StartOriginResourceRef(lease.copied.tree,lease.copied.name,&ref,error,size)) return 0;
    lease.receipt=ref.receipt;
    if(ref.ordinal!=lease.copied.ordinal || ref.flags!=lease.copied.flags ||
       !StartOriginTypedPure(&lease)) goto refuse;
    /* Only the actual R208 transform's two fences invoke the caller. */
    if(!G_Cod1GetStartOrigin(&ref,lease.copied.origin,lease.copied.angles,
        staged.vector,error,size,StartOriginTypedCurrent,&lease) ||
        !StartOriginTypedPure(&lease)) goto refuse;
    *out=staged;return 1;
refuse:
    if(!StartOriginPrivate(error,size))EntityError(error,size,"typed start-origin query refused");
    return 0;
}
const GscStartOriginHost *G_Cod1StartOriginQueryHost(void) {
    return &cod1StartOriginTypedHost;
}

#include "g_cod1_namespace_start_origin.inc"
