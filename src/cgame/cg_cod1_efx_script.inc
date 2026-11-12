/* R115 script-only entry into the real EFX cache/scheduler. Included once by
 * cg_cod1_efx.h, after the common executor. No grenade compatibility fallback.
 * Mode 6 uses the existing bounded world executor; renderer/sampling/motion
 * and basis construction are declared port policy, not original FX parity. */
typedef struct {
    cod1ScriptFxAdmission_t admission;
    void *user;
    uint64_t generation;
    int time;
} ceScriptLease;

uint64_t CG_Cod1EfxGeneration(void) { return ceScriptCacheGeneration; }

static int ceScriptCurrent(const ceScriptLease *lease) {
    return lease && lease->time==cg.time && lease->generation==ceScriptCacheGeneration &&
        lease->admission && lease->admission(lease->user) &&
        lease->time==cg.time && lease->generation==ceScriptCacheGeneration;
}

void CG_Cod1EfxScriptClear(void) {
    int i;
    ceScriptAdvance(&ceScriptSceneGeneration);
    for(i=0;i<CE_LIVE;++i)
        if(ceState.live[i].used && ceState.live[i].script)ceState.live[i].used=0;
    /* A cancelled unpublished registrar is reusable. A completed failed
     * resource remains failed, avoiding repeated FS reads on each event. */
    for(i=0;i<ceState.cacheCount;++i)
        if(ceState.cache[i].mode==6 && ceState.cache[i].state==1)
            ceState.cache[i].state=0;
}

static int ceScriptRendererReady(const ceScriptLease *lease) {
    char value[32];
    if(!ceScriptCurrent(lease))return 0;
    memset(value,0,sizeof(value));
    trap_Cvar_VariableStringBuffer("r_cod1EfxShaders",value,sizeof(value));
    if(!ceScriptCurrent(lease))return 0;
    return atoi(value)>0;
}

static int ceScriptRead(const char *path,char **out,const ceScriptLease *lease) {
    fileHandle_t file=0;
    char *bytes=NULL;
    int size=-1,current;
    *out=NULL;
    if(!ceScriptCurrent(lease))return -1;
    size=trap_FS_FOpenFile(path,&file,FS_READ);
    current=ceScriptCurrent(lease);
    if(!file)return -1;
    if(!current || size<=0 || size>CEFX_FILE_LIMIT)goto close;
    bytes=(char *)malloc((size_t)size+1);
    if(!bytes || !ceScriptCurrent(lease))goto close;
    trap_FS_Read(bytes,size,file);
    if(!ceScriptCurrent(lease))goto close;
    bytes[size]=0;
close:
    /* The file is owned by this acquisition. Closing it is cleanup even if
     * its scene lease was cancelled; no stale cache mutation follows. */
    current=ceScriptCurrent(lease);
    trap_FS_FCloseFile(file);
    current=current && ceScriptCurrent(lease);
    if(!current || !bytes) { free(bytes);return -1; }
    *out=bytes;return size;
}

static int ceScriptOwns(int id,const char *path,uint64_t generation,uint64_t ticket) {
    return generation==ceScriptCacheGeneration && id>0 && id<=ceState.cacheCount &&
        ceState.cache[id-1].mode==6 && ceState.cache[id-1].state==1 &&
        ceState.cache[id-1].scriptTicket==ticket &&
        !strcmp(ceState.cache[id-1].path,path);
}

static int ceScriptRegister(const char *name,int depth,const ceScriptLease *lease) {
    char path[CEFX_PATH],error[256],*text=NULL;
    ceCached *candidate=NULL;
    uint64_t ticket;
    int id=0,i,j,size,failed=0;
    if(!ceScriptCurrent(lease) || !cePath(name,path,1))return 0;
    if(depth>CE_DEPTH) {
        CG_Printf("CoD1 SCRIPT_FX_R115: dependency depth refused: %s\n",path);
        return 0;
    }
    for(i=0;i<ceState.cacheCount;++i)
        if(ceState.cache[i].mode==6 && !strcmp(path,ceState.cache[i].path)) {
            if(ceState.cache[i].state==2)return i+1;
            if(ceState.cache[i].state==-1)return 0;
            if(ceState.cache[i].state==1) {
                CG_Printf("CoD1 SCRIPT_FX_R115: cyclic/reentrant dependency refused: %s\n",path);
                return 0;
            }
            id=i+1;break;
        }
    if(!id) {
        if(ceState.cacheCount>=CE_CACHE) {
            CG_Printf("CoD1 SCRIPT_FX_R115: shared definition cache full: %s\n",path);
            return 0;
        }
        id=++ceState.cacheCount;
    }
    ceScriptAdvance(&ceScriptTicket);ticket=ceScriptTicket;
    memset(&ceState.cache[id-1],0,sizeof(ceState.cache[id-1]));
    strcpy(ceState.cache[id-1].path,path);
    ceState.cache[id-1].mode=6;ceState.cache[id-1].state=1;
    ceState.cache[id-1].scriptTicket=ticket;
    candidate=(ceCached *)calloc(1,sizeof(*candidate));
    if(!candidate) { failed=1;goto finish; }
    strcpy(candidate->path,path);candidate->mode=6;candidate->scriptTicket=ticket;
    size=ceScriptRead(path,&text,lease);
    if(size<0) {
        if(ceScriptCurrent(lease)) {
            failed=1;
            CG_Printf("CoD1 SCRIPT_FX_R115: missing/invalid resource refused: %s\n",path);
        }
        goto finish;
    }
    candidate->def=(ceDefinition *)malloc(sizeof(*candidate->def));
    if(!candidate->def) { failed=1;goto finish; }
    if(!ceParseMode(text,(size_t)size,candidate->def,error,6)) {
        failed=1;
        CG_Printf("CoD1 SCRIPT_FX_R115: definition refused %s: %s\n",path,error);
        goto finish;
    }
    free(text);text=NULL;
    for(i=0;i<candidate->def->count;++i) {
        const cePrimitive *p=&candidate->def->prim[i];
        for(j=0;j<p->shaderCount;++j) {
            if(!ceScriptCurrent(lease))goto finish;
            candidate->shaders[i][j]=trap_R_RegisterShader(p->shaders[j]);
            if(!ceScriptCurrent(lease))goto finish;
            if(!candidate->shaders[i][j]) {
                failed=1;
                CG_Printf("CoD1 SCRIPT_FX_R115: shader refused %s in %s\n",p->shaders[j],path);
                goto finish;
            }
        }
        for(j=0;j<p->playCount;++j) {
            candidate->child[i][j]=ceScriptRegister(p->play[j],depth+1,lease);
            if(!ceScriptCurrent(lease))goto finish;
            if(!candidate->child[i][j]) { failed=1;goto finish; }
        }
        for(j=0;j<p->emitCount;++j) {
            candidate->emitted[i][j]=ceScriptRegister(p->emit[j],depth+1,lease);
            if(!ceScriptCurrent(lease))goto finish;
            if(!candidate->emitted[i][j]) { failed=1;goto finish; }
        }
        for(j=0;j<p->modelCount;++j) {
            if(!ceScriptCurrent(lease))goto finish;
            candidate->models[i][j]=trap_R_RegisterModel(p->models[j]);
            if(!ceScriptCurrent(lease))goto finish;
            if(!candidate->models[i][j]) {
                failed=1;
                CG_Printf("CoD1 SCRIPT_FX_R115: model refused %s in %s\n",p->models[j],path);
                goto finish;
            }
        }
    }
    if(!ceScriptCurrent(lease) || !ceScriptOwns(id,path,lease->generation,ticket))goto finish;
    candidate->state=2;
    ceState.cache[id-1]=*candidate;
    candidate->def=NULL;free(candidate);return id;
finish:
    free(text);
    if(candidate) { free(candidate->def);free(candidate); }
    if(ceScriptOwns(id,path,lease->generation,ticket))
        ceState.cache[id-1].state=failed && ceScriptCurrent(lease)?-1:0;
    return 0;
}

int CG_Cod1EfxScriptRegister(const char *path,cod1ScriptFxAdmission_t admission,void *user) {
    ceScriptLease lease;
    lease.admission=admission;lease.user=user;lease.generation=ceScriptCacheGeneration;lease.time=cg.time;
    if(!ceScriptCurrent(&lease) || !ceScriptRendererReady(&lease))return 0;
    return ceScriptRegister(path,0,&lease);
}

int CG_Cod1EfxScriptPlay(int id,const float origin[3],int directed,
    const float direction[3],cod1ScriptFxAdmission_t admission,void *user) {
    ceScriptLease lease;
    vec3_t copiedOrigin,copiedDirection,basis[3];
    int i;
    lease.admission=admission;lease.user=user;lease.generation=ceScriptCacheGeneration;lease.time=cg.time;
    if(!ceScriptCurrent(&lease) || !origin || (directed!=0 && directed!=1) ||
       id<=0 || id>ceState.cacheCount || ceState.cache[id-1].mode!=6 ||
       ceState.cache[id-1].state!=2 || !ceState.cache[id-1].def || cg.time<0)return 0;
    for(i=0;i<3;++i) {
        if(!CQ_Finite(origin[i]) || (directed && (!direction || !CQ_Finite(direction[i]))))return 0;
        copiedOrigin[i]=origin[i];copiedDirection[i]=directed?direction[i]:0.f;
    }
    if(!ceScriptCurrent(&lease))return 0;
    trap_Cvar_Update(&ceEnabled);
    if(!ceScriptCurrent(&lease))return 0;
    /* A disabled visual channel consumes the already admitted command and
     * does not queue an effect to replay when it is enabled later. */
    if(!ceEnabled.integer)return 1;
    if(!ceScriptRendererReady(&lease))return 0;
    if(id>ceState.cacheCount || ceState.cache[id-1].mode!=6 || ceState.cache[id-1].state!=2)return 0;
    /* Original snapshots reconstruct bytedirs*length. The directional basis
     * below normalizes it using the bounded port helper; undirected uses the
     * identity basis. The unobserved original renderer basis is not claimed. */
    if(directed) {
        double square=0.0,length;
        vec3_t unit;
        for(i=0;i<3;++i)square+=(double)copiedDirection[i]*copiedDirection[i];
        length=sqrt(square);
        if(!(length>0.0))return 0;
        for(i=0;i<3;++i)unit[i]=(float)((double)copiedDirection[i]/length);
        ceBasis(unit,basis);
    } else AxisClear(basis);
    if(!ceScriptCurrent(&lease))return 0;
    ceCheckClock();
    ceSpawn(id,copiedOrigin,basis,cg.time,0);
    ++ceState.played;return 1;
}

/* A full tag frame contains roll which cannot be reconstructed from its
 * forward direction. This is a private port scheduler entry, not an actor
 * binding or a recovered retail client/renderer contract. Accepted axes keep
 * their supplied values; the tolerance only refuses malformed frames. */
#define CE_SCRIPT_FRAME_EPSILON 0.001
static int ceScriptBasisBusy,ceScriptBasisCancelled;

static int ceScriptBasisFrame(const float origin[3],const float basis[3][3],
    vec3_t copiedOrigin,vec3_t copiedBasis[3]) {
    double dot,determinant;
    int i,j,k;
    if(!origin || !basis)return 0;
    /* Copy borrowed input before the first admission or engine callback. */
    memcpy(copiedOrigin,origin,sizeof(vec3_t));
    memcpy(copiedBasis,basis,3*sizeof(vec3_t));
    for(i=0;i<3;++i) {
        if(!CQ_Finite(copiedOrigin[i]))return 0;
        for(j=0;j<3;++j)if(!CQ_Finite(copiedBasis[i][j]))return 0;
    }
    for(i=0;i<3;++i)for(j=i;j<3;++j) {
        dot=0.0;
        for(k=0;k<3;++k)dot+=(double)copiedBasis[i][k]*copiedBasis[j][k];
        if(fabs(dot-(i==j?1.0:0.0))>CE_SCRIPT_FRAME_EPSILON)return 0;
    }
    determinant=(double)copiedBasis[0][0]*
        ((double)copiedBasis[1][1]*copiedBasis[2][2]-(double)copiedBasis[1][2]*copiedBasis[2][1])-
        (double)copiedBasis[0][1]*
        ((double)copiedBasis[1][0]*copiedBasis[2][2]-(double)copiedBasis[1][2]*copiedBasis[2][0])+
        (double)copiedBasis[0][2]*
        ((double)copiedBasis[1][0]*copiedBasis[2][1]-(double)copiedBasis[1][1]*copiedBasis[2][0]);
    return fabs(determinant-1.0)<=CE_SCRIPT_FRAME_EPSILON;
}

/* Cache slots are independently rebindable. The completed registrar's exact
 * ticket, cache generation and live-scene generation all belong to this
 * attempt. A callback can clear a scene without clearing the resource cache. */
static int ceScriptBasisPure(const ceScriptLease *lease,int id,uint64_t scene,
    uint64_t ticket) {
    return ceScriptBasisBusy && !ceScriptBasisCancelled && lease &&
        lease->time==cg.time && cg.time>=0 &&
        lease->generation==ceScriptCacheGeneration && scene==ceScriptSceneGeneration &&
        !(ceState.clockValid && cg.time<ceState.clock) &&
        id>0 && id<=ceState.cacheCount && ceState.cache[id-1].mode==6 &&
        ceState.cache[id-1].state==2 && ceState.cache[id-1].def && ticket &&
        ceState.cache[id-1].scriptTicket==ticket;
}
static int ceScriptBasisCurrent(const ceScriptLease *lease,int id,uint64_t scene,
    uint64_t ticket) {
    return ceScriptBasisPure(lease,id,scene,ticket) &&
        ceScriptCurrent(lease) && ceScriptBasisPure(lease,id,scene,ticket);
}

int CG_Cod1EfxScriptPlayBasis(int id,const float origin[3],
    const float basis[3][3],cod1ScriptFxAdmission_t admission,void *user) {
    ceScriptLease lease;
    vec3_t copiedOrigin,copiedBasis[3];
    char renderer[32];
    uint64_t scene,ticket;
    int result=0;
    if(ceScriptBasisBusy) {ceScriptBasisCancelled=1;return 0;}
    ceScriptBasisBusy=1;ceScriptBasisCancelled=0;
    if(!ceScriptBasisFrame(origin,basis,copiedOrigin,copiedBasis) ||
        id<=0 || id>ceState.cacheCount)goto leave;
    lease.admission=admission;lease.user=user;
    lease.generation=ceScriptCacheGeneration;lease.time=cg.time;
    scene=ceScriptSceneGeneration;ticket=ceState.cache[id-1].scriptTicket;
    if(!ceScriptBasisCurrent(&lease,id,scene,ticket))goto leave;
    trap_Cvar_Update(&ceEnabled);
    if(!ceScriptBasisCurrent(&lease,id,scene,ticket))goto leave;
    if(!ceEnabled.integer) {result=1;goto leave;}
    memset(renderer,0,sizeof(renderer));
    trap_Cvar_VariableStringBuffer("r_cod1EfxShaders",renderer,sizeof(renderer));
    if(!ceScriptBasisCurrent(&lease,id,scene,ticket) || atoi(renderer)<=0)goto leave;
    /* The final admission may itself be reentrant. Its post-check above is
     * pure. ceCheckClock/ceSpawn and nested runners have no engine callbacks
     * in this executor; the copied frame commits in that interval. Particle
     * pools/budgets can still drop visuals, just as for the existing entry. */
    ceCheckClock();
    if(!ceScriptBasisPure(&lease,id,scene,ticket))goto leave;
    ceSpawn(id,copiedOrigin,copiedBasis,cg.time,0);
    ++ceState.played;result=1;
leave:
    ceScriptBasisBusy=0;
    return result;
}
#undef CE_SCRIPT_FRAME_EPSILON
