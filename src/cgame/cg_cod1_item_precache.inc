/* R121 private item consumer. Native item bits select the complete compiled
 * table; this port binds that table to a module/scene/fingerprint and requires
 * positive real registration handles. Publication is never a client ACK. */
#include "../game/bg_cod1_item_precache.h"
#include "../game/bg_cod1_soundalias.h"
#define CCI_ALIAS_TABLES 128
#define CCI_ALIAS_LIST_BYTES 8192
#define CCI_ALIAS_TABLE_BYTES 1048576
#define CCI_ALIAS_VARIANTS 4
typedef struct {
    uint64_t revision,epoch,operation_id;
    uint32_t fingerprint;
    int time;
    char map[sizeof(cgs.mapname)];
} cod1ItemClientLease_t;
typedef struct {
    int registered;
    qhandle_t models[2],icon,weapon;
    sfxHandle_t pickup[CCI_ALIAS_VARIANTS];
} cod1ItemClientMemo_t;
static struct {
    uint64_t revision,epoch,operation_id;
    uint32_t module,generation,fingerprint;
    int have_module,have_state,media_ready;
    unsigned char requested[COD1_ITEM_COUNT];
    uint64_t busy[COD1_ITEM_COUNT];
    cod1ItemClientMemo_t memo[COD1_ITEM_COUNT];
} cgCod1Items;
static void CCI_Advance(void) {
    if(cgCod1Items.revision!=UINT64_MAX)++cgCod1Items.revision;
}
static cod1ItemClientLease_t CCI_Lease(void) {
    cod1ItemClientLease_t lease;
    lease.revision=cgCod1Items.revision;lease.epoch=cgCod1Items.epoch;
    lease.operation_id=cgCod1Items.operation_id;lease.fingerprint=CI_Fingerprint();
    lease.time=cg.time;memcpy(lease.map,cgs.mapname,sizeof(lease.map));return lease;
}
static int CCI_OwnerCurrent(const cod1ItemClientLease_t *lease) {
    return lease && lease->revision && lease->revision!=UINT64_MAX &&
        lease->revision==cgCod1Items.revision && lease->epoch==cgCod1Items.epoch &&
        lease->operation_id==cgCod1Items.operation_id && lease->time==cg.time &&
        lease->fingerprint==CI_Fingerprint() &&
        !memcmp(lease->map,cgs.mapname,sizeof(lease->map));
}
/* Read the already integrated local gamestate without invoking a trap from a
 * lease predicate. CL's trusted checkpoint refuses replayed control strings. */
static int CCI_Module(uint32_t *module) {
    int offset=cgs.gameState.stringOffsets[CS_SYSTEMINFO];
    const char *text;char copied[MAX_INFO_STRING];size_t length;
    if(offset<0 || offset>=MAX_GAMESTATE_CHARS)return 0;
    text=cgs.gameState.stringData+offset;
    if(!memchr(text,0,(size_t)(MAX_GAMESTATE_CHARS-offset)))return 0;
    length=strlen(text);if(length>=sizeof(copied))return 0;
    memcpy(copied,text,length+1);
    return CA_ModuleIdToken(Info_ValueForKey(copied,COD1_AMBIENT_MODULE_CVAR),module);
}
static int CCI_Current(const cod1ItemClientLease_t *lease) {
    uint32_t module;
    return CCI_OwnerCurrent(lease) && cgCod1Items.media_ready &&
        cgCod1Items.have_module && cgCod1Items.have_state &&
        CCI_Module(&module) && module==cgCod1Items.module &&
        CI_Module(lease->epoch)==module && lease->fingerprint==cgCod1Items.fingerprint;
}
static int CCI_CallbackCurrent(void *user) {return CCI_Current((cod1ItemClientLease_t *)user);}
void CG_Cod1ItemPrecacheReset(void) {
    uint64_t revision=cgCod1Items.revision;
    if(revision!=UINT64_MAX)++revision;
    memset(&cgCod1Items,0,sizeof(cgCod1Items));cgCod1Items.revision=revision;
}
static void CCI_Retire(uint32_t module,int valid) {
    CCI_Advance();cgCod1Items.have_module=valid;
    cgCod1Items.have_state=0;cgCod1Items.epoch=0;cgCod1Items.operation_id=0;
    cgCod1Items.fingerprint=0;
    memset(cgCod1Items.requested,0,sizeof(cgCod1Items.requested));
    memset(cgCod1Items.busy,0,sizeof(cgCod1Items.busy));
    memset(cgCod1Items.memo,0,sizeof(cgCod1Items.memo));
    /* An invalid authority retires use, but retains the previous scene floor.
     * A verified new module establishes a separate generation realm. */
    if(valid && module!=cgCod1Items.module) {
        cgCod1Items.module=module;cgCod1Items.generation=0;
    }
}
static int CCI_Read(const cod1ItemClientLease_t *lease,const char *path,char **out,int *size) {
    fileHandle_t file=0;char *data=NULL;int length,valid;
    *out=NULL;*size=0;if(!CCI_Current(lease))return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    valid=CCI_Current(lease) && file && length>0 && length<=CCI_ALIAS_TABLE_BYTES;
    if(valid)data=(char *)calloc((size_t)length+1,1);
    if(data && CCI_Current(lease))trap_FS_Read(data,length,file);else valid=0;
    if(!CCI_Current(lease))valid=0;
    if(file)trap_FS_FCloseFile(file);
    if(!CCI_Current(lease))valid=0;
    if(!valid || !data || memchr(data,0,(size_t)length)) {free(data);return 0;}
    *out=data;*size=length;return 1;
}
static int CCI_Alias(const cod1ItemClientLease_t *lease,const char *alias,sfxHandle_t handles[CCI_ALIAS_VARIANTS]) {
    char list[CCI_ALIAS_LIST_BYTES],paths[CCI_ALIAS_TABLES][MAX_QPATH];
    cwSoundAliasVariant variants[CCI_ALIAS_VARIANTS+1];
    char *data=NULL,*name;size_t at,length;int count,i,j,size,n,selected;char swap[MAX_QPATH];
    if(!alias || !*alias)return 1;
    if(!CCI_Current(lease))return 0;
    memset(list,0,sizeof(list));count=trap_Cod1FS_GetFileList("soundaliases",".csv",list,sizeof(list));
    if(!CCI_Current(lease) || count<1 || count>CCI_ALIAS_TABLES)return 0;
    at=0;
    for(i=0;i<count;++i) {
        if(at>=sizeof(list) || !memchr(list+at,0,sizeof(list)-at))return 0;
        name=list+at;length=strlen(name);
        if(!length || length+sizeof("soundaliases/")>MAX_QPATH ||
           length<4 || Q_stricmp(name+length-4,".csv") ||
           strchr(name,'/') || strchr(name,'\\') || !Cod1PrecacheName(name,0))return 0;
        strcpy(paths[i],"soundaliases/");strcat(paths[i],name);at+=length+1;
    }
    /* Deterministic bounded table precedence is a selected port policy. */
    for(i=1;i<count;++i)for(j=i;j>0 && strcmp(paths[j-1],paths[j])>0;--j) {
        strcpy(swap,paths[j-1]);strcpy(paths[j-1],paths[j]);strcpy(paths[j],swap);
    }
    for(i=0;i<count;++i) {
        if(!CCI_Read(lease,paths[i],&data,&size)) {if(!CCI_Current(lease))return 0;continue;}
        n=CW_SoundAliasVariants(data,size,alias,variants,CCI_ALIAS_VARIANTS+1);free(data);data=NULL;
        if(!CCI_Current(lease))return 0;
        if(n>CCI_ALIAS_VARIANTS)return 0;
        selected=0;
        for(j=0;j<n;++j) {
            if(!CW_SoundAliasLoadSpecMatches(variants[j].loadspec,lease->map))continue;
            if(!CCI_Current(lease))return 0;
            handles[selected]=trap_S_RegisterSound(variants[j].path);
            if(!CCI_Current(lease) || handles[selected]<=0)return 0;
            ++selected;
        }
        if(selected)return 1;
    }
    return 0;
}
static int CCI_Register(const cod1ItemClientLease_t *lease,int index) {
    cod1ItemDescriptor_t descriptor;cod1ItemClientMemo_t candidate;int i,valid=0;
    if(!CCI_Current(lease))return 0;
    if(cgCod1Items.memo[index].registered)return 1;
    if(cgCod1Items.busy[index]==lease->revision)return 0;
    if(!CI_Descriptor(index,&descriptor))return 0;
    memset(&candidate,0,sizeof(candidate));cgCod1Items.busy[index]=lease->revision;
    for(i=0;i<2;++i) {
        if(!descriptor.model_present[i] || !descriptor.models[i][0])continue;
        if(!CCI_Current(lease))goto finished;
        candidate.models[i]=trap_R_RegisterModel(descriptor.models[i]);
        if(!CCI_Current(lease) || candidate.models[i]<=0)goto finished;
    }
    if(descriptor.icon_present && descriptor.icon[0]) {
        candidate.icon=trap_R_RegisterShaderNoMip(descriptor.icon);
        if(!CCI_Current(lease) || candidate.icon<=0)goto finished;
    }
    if(descriptor.pickup_present && descriptor.pickup_alias[0] &&
       !CCI_Alias(lease,descriptor.pickup_alias,candidate.pickup))goto finished;
    if(!CCI_Current(lease))goto finished;
    if(descriptor.type==1) {
        candidate.weapon=CG_Cod1PrecacheItemWeapon(descriptor.weapon_index,CCI_CallbackCurrent,(void *)lease);
        if(!CCI_Current(lease) || candidate.weapon<=0)goto finished;
    }
    if(!CCI_Current(lease))goto finished;
    candidate.registered=1;cgCod1Items.memo[index]=candidate;valid=1;
finished:
    if(cgCod1Items.busy[index]==lease->revision)cgCod1Items.busy[index]=0;
    return valid;
}
int CG_Cod1ItemPrecacheConfigChanged(int number) {
    cod1ItemClientLease_t lease;cod1ItemState_t state;const char *text;
    char copied[COD1_ITEM_STATE_CAPACITY];uint32_t module;size_t length;int i,ok=1;
    if(number!=CS_COD1_ITEM_PRECACHE && number!=CS_SYSTEMINFO)return 0;
    lease=CCI_Lease();if(!CCI_OwnerCurrent(&lease))return 0;
    if(!CCI_Module(&module)) {
        if(cgCod1Items.have_module || cgCod1Items.have_state)CCI_Retire(0,0);
        return 0;
    }
    if(!cgCod1Items.have_module || cgCod1Items.module!=module) {
        CCI_Retire(module,1);lease=CCI_Lease();if(!CCI_OwnerCurrent(&lease))return 0;
    }
    text=CG_ConfigString(CS_COD1_ITEM_PRECACHE);
    if(!CCI_OwnerCurrent(&lease) || !text)return 0;
    length=strlen(text);if(length>=sizeof(copied))return 0;
    memcpy(copied,text,length+1);
    if(!copied[0]) {CCI_Retire(module,1);return 0;}
    if(!CI_DecodeState(copied,&state) || CI_Module(state.epoch)!=module ||
       state.fingerprint!=lease.fingerprint || !CCI_OwnerCurrent(&lease))return 0;
    if(cgCod1Items.have_state && state.epoch==cgCod1Items.epoch) {
        if(state.operation_id<cgCod1Items.operation_id)return 0;
        /* The native publisher accumulates bits for a scene. The private
         * transport refuses contradictory bit removal; an empty string is
         * the explicit retirement boundary, retaining the generation floor. */
        for(i=1;i<COD1_ITEM_COUNT;++i)
            if(cgCod1Items.requested[i] && !state.registered[i])return 0;
        if(state.operation_id==cgCod1Items.operation_id) {
            if(memcmp(state.registered,cgCod1Items.requested,sizeof(state.registered)))return 0;
        } else {
            CCI_Advance();cgCod1Items.operation_id=state.operation_id;
            memcpy(cgCod1Items.requested,state.registered,sizeof(state.registered));
        }
    } else {
        if(CI_Generation(state.epoch)<=cgCod1Items.generation)return 0;
        CCI_Advance();cgCod1Items.epoch=state.epoch;cgCod1Items.generation=CI_Generation(state.epoch);
        cgCod1Items.operation_id=state.operation_id;cgCod1Items.fingerprint=state.fingerprint;
        cgCod1Items.have_state=1;memcpy(cgCod1Items.requested,state.registered,sizeof(state.registered));
        memset(cgCod1Items.memo,0,sizeof(cgCod1Items.memo));memset(cgCod1Items.busy,0,sizeof(cgCod1Items.busy));
    }
    lease=CCI_Lease();if(!CCI_Current(&lease))return 0;
    for(i=1;i<COD1_ITEM_COUNT;++i) {
        if(!state.registered[i])continue;
        if(!CCI_Register(&lease,i))ok=0;
        if(!CCI_Current(&lease))return 0;
    }
    return ok;
}
void CG_Cod1ItemPrecacheInit(void) {
    CCI_Advance();cgCod1Items.media_ready=1;
    (void)CG_Cod1ItemPrecacheConfigChanged(CS_COD1_ITEM_PRECACHE);
}
/* Cosmetic warmup consumes the established resource lane; it never grants an
 * inventory weapon or creates a request for an unregistered WeaponDef. */
int CG_Cod1ItemWeaponRequested(int definition) {
    cod1ItemClientLease_t lease=CCI_Lease();
    if(definition<0 || definition>=CW_Count() || !CCI_Current(&lease))return 0;
    return cgCod1Items.requested[definition+1] &&
        cgCod1Items.memo[definition+1].registered;
}
