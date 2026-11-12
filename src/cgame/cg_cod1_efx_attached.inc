/* Separate mode7 current-world attachment service. The inspected ENGINE
 * retains owner/tag and resolves a CURRENT matrix at each update. Sampling,
 * budgets and visual interpolation remain this port's declared FX policy;
 * retail A8 client dispatch/render parity is unavailable and not claimed. */
static int ceAttachedBusy;
void CG_Cod1EfxAttachedRetire(void){int i;for(i=0;i<CE_LIVE;i++)if(ceState.live[i].used&&ceState.live[i].worldOwner)ceState.live[i].used=0;}
void CG_Cod1EfxAttachedResolver(int(*resolver)(int,uint64_t,uint32_t,float[16])){ceWorldResolver=resolver;}
static int ceAttachedOwn(int id,uint64_t cache,uint64_t ticket){return cache==ceScriptCacheGeneration&&id>0&&id<=ceState.cacheCount&&ceState.cache[id-1].mode==7&&ceState.cache[id-1].state==1&&ceState.cache[id-1].scriptTicket==ticket;}
int CG_Cod1EfxAttachedRegister(const char*name,cod1ScriptFxAdmission_t admission,void*user){
 ceScriptLease lease;ceCached*c=NULL;char path[CEFX_PATH],error[256],*text=NULL;int i,j,id=0,n,ok=0;uint64_t ticket=0;
 if(ceAttachedBusy)return 0;ceAttachedBusy=1;lease.admission=admission;lease.user=user;lease.generation=ceScriptCacheGeneration;lease.time=cg.time;
 if(!ceScriptCurrent(&lease)||!ceScriptRendererReady(&lease)||!cePath(name,path,1))goto done;
 for(i=0;i<ceState.cacheCount;i++)if(ceState.cache[i].mode==7&&!strcmp(path,ceState.cache[i].path)){if(ceState.cache[i].state==2){ok=i+1;goto done;}if(ceState.cache[i].state!=0)goto done;id=i+1;break;}
 if(!id){if(ceState.cacheCount>=CE_CACHE)goto done;id=++ceState.cacheCount;}
 ceScriptAdvance(&ceScriptTicket);ticket=ceScriptTicket;memset(ceState.cache+id-1,0,sizeof(ceCached));strcpy(ceState.cache[id-1].path,path);ceState.cache[id-1].mode=7;ceState.cache[id-1].state=1;ceState.cache[id-1].scriptTicket=ticket;
 c=calloc(1,sizeof(*c));if(!c)goto done;c->def=malloc(sizeof(*c->def));if(!c->def)goto done;c->mode=7;c->scriptTicket=ticket;strcpy(c->path,path);
 n=ceScriptRead(path,&text,&lease);if(n<1||!ceParseMode(text,n,c->def,error,7)){if(ceScriptCurrent(&lease))CG_Printf("CoD1 attached FX: resource refused %s\n",path);goto done;}
 for(i=0;i<c->def->count;i++)for(j=0;j<c->def->prim[i].shaderCount;j++){
  if(!ceScriptCurrent(&lease)||!ceAttachedOwn(id,lease.generation,ticket))goto done;c->shaders[i][j]=trap_R_RegisterShader(c->def->prim[i].shaders[j]);
  if(!ceScriptCurrent(&lease)||!ceAttachedOwn(id,lease.generation,ticket)||!c->shaders[i][j])goto done;
 }
 if(!ceScriptCurrent(&lease)||!ceAttachedOwn(id,lease.generation,ticket))goto done;c->state=2;ceState.cache[id-1]=*c;c->def=NULL;ok=id;
 done:free(text);if(c){free(c->def);free(c);}if(!ok&&ticket&&ceAttachedOwn(id,lease.generation,ticket))ceState.cache[id-1].state=0;ceAttachedBusy=0;return ok;
}
int CG_Cod1EfxAttachedPlay(int id,int owner,uint64_t epoch,uint32_t generation,cod1ScriptFxAdmission_t admission,void*user){
 ceScriptLease lease;uint64_t scene,ticket;float matrix[16];vec3_t basis[3];ceCached*c;int i,j,count,result=0;
 if(ceAttachedBusy||owner<0||owner>=MAX_GENTITIES||!epoch||!generation||id<1||id>ceState.cacheCount)return 0;ceAttachedBusy=1;
 lease.admission=admission;lease.user=user;lease.generation=ceScriptCacheGeneration;lease.time=cg.time;scene=ceScriptSceneGeneration;ticket=ceState.cache[id-1].scriptTicket;
 if(!ceScriptCurrent(&lease)||cg.time<0||ceState.cache[id-1].mode!=7||ceState.cache[id-1].state!=2||!ticket)goto done;
 if(!ceWorldResolve(owner,epoch,generation,matrix)||!ceScriptCurrent(&lease)||scene!=ceScriptSceneGeneration||id>ceState.cacheCount||ceState.cache[id-1].scriptTicket!=ticket||ceState.cache[id-1].mode!=7||ceState.cache[id-1].state!=2)goto done;
 trap_Cvar_Update(&ceEnabled);if(!ceScriptCurrent(&lease)||scene!=ceScriptSceneGeneration)goto done;if(!ceEnabled.integer){result=1;goto done;}
 if(!ceScriptRendererReady(&lease)||scene!=ceScriptSceneGeneration||id>ceState.cacheCount||ceState.cache[id-1].scriptTicket!=ticket||ceState.cache[id-1].state!=2)goto done;
 ceCheckClock();if(scene!=ceScriptSceneGeneration)goto done;c=ceState.cache+id-1;for(i=0;i<3;i++)VectorCopy(matrix+i*4,basis[i]);
 ceWorldSpawning.owner=owner+1;ceWorldSpawning.epoch=epoch;ceWorldSpawning.generation=generation;
 for(i=0;i<c->def->count;i++){const cePrimitive*p=c->def->prim+i;count=(int)floorf(ceSample(p->count)+.5f);for(j=0;j<count;j++){float delay=ceSample(p->delay);ceMake(id,i,matrix+12,basis,cg.time+delay,0,NULL);}}
 memset(&ceWorldSpawning,0,sizeof(ceWorldSpawning));++ceState.played;result=1;
 done:ceAttachedBusy=0;return result;
}
