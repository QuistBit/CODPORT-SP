/* Included after registry helpers. Acquisition is explicit; no GAME profile
 * is enabled here. Authored custom fields remain in the existing catalog. */
#include "cod1_gsc/gsc_session.h"
#include "cod1_gsc/gsc_postcohort_private.h"
void G_Cod1NodePreparedFree(C1NPPrepared*p){
 if(!p)return;free(p->records);free(p->scripts);free(p->tokens);free(p);
}
static int Cod1NodePreparedToken(C1NPPrepared*p,const char*value,uint16_t*out){
 unsigned i;size_t n;
 if(!value){*out=0;return 1;}
 /* Current admitted profile excludes escapes/empty/extended bytes whose
  * SL/string-conversion semantics must not be guessed from raw BSP text. */
 if(!Cod1NodeText(value,128,&n)||!n)return 0;
 for(i=0;i<n;i++)if((unsigned char)value[i]<32||(unsigned char)value[i]>126||value[i]=='\\')return 0;
 for(i=0;i<p->tokenCount;i++)if(!strcmp(p->tokens[i],value)){*out=(uint16_t)(i+1);return 1;}
 if(p->tokenCount>=C1NP_TOKEN_CAPACITY)return 0;
 strcpy(p->tokens[p->tokenCount++],value);*out=(uint16_t)p->tokenCount;return 1;
}
static int Cod1NodePreparedOwner(uint64_t epoch){return !cod1NodeCancelled&&epoch==cod1NodeEpoch&&Cod1NodeAvailable();}
static int Cod1NodePreparedRefs(C1NPPrepared*p,GscSession*s){
 GscMapExecutionCertificate map;unsigned i;
 map.receipt=p->registryEpoch;map.current=Cod1NodeScriptMapCurrent;
 if(!gsc_session_node_script_map_matches(s,p->sessionId,&map)||!Cod1NodePreparedOwner(p->registryEpoch))return 0;
 for(i=0;i<p->count;i++)if(p->scripts[i].cache){
  if(!gsc_session_node_script_current(s,p->sessionId,p->scripts[i])||!Cod1NodePreparedOwner(p->registryEpoch))return 0;
 }return Cod1NodePreparedOwner(p->registryEpoch);
}
int G_Cod1NodePreparedCurrent(C1NPPrepared*p,GscSession*s,uint64_t sessionId){
 int ok;if(cod1NodeBusy){cod1NodeCancelled=1;return 0;}
 if(!p||!s||!sessionId||p->sessionId!=sessionId||p->count!=(unsigned)cod1NodeCount||!p->count||p->count>2048||
    !p->scripts||p->registryEpoch!=cod1NodeEpoch||!Cod1NodeAvailable())return 0;
 cod1NodeBusy=1;cod1NodeCancelled=0;ok=Cod1NodePreparedRefs(p,s);cod1NodeBusy=0;return ok;
}
static int Cod1NodePreparedActorCurrent(C1NPPrepared*p,GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row){
 GscMapExecutionCertificate map;unsigned i;int ok=0;
 if(cod1NodeBusy){cod1NodeCancelled=1;return 0;}
 if(!p||!s||!id||p->sessionId!=id||p->count!=(unsigned)cod1NodeCount||!p->count||p->count>2048||
    !p->scripts||p->registryEpoch!=cod1NodeEpoch||!Cod1NodeAvailable())return 0;
 cod1NodeBusy=1;cod1NodeCancelled=0;map.receipt=p->registryEpoch;map.current=Cod1NodeScriptMapCurrent;
 if(!gsc_session_actor_graph_map_matches(s,id,receipt,row,&map)||!Cod1NodePreparedOwner(p->registryEpoch))goto done;
 for(i=0;i<p->count;i++)if(p->scripts[i].cache){
  if(!gsc_session_actor_graph_script_current(s,id,receipt,row,p->scripts[i])||!Cod1NodePreparedOwner(p->registryEpoch))goto done;
 }ok=Cod1NodePreparedOwner(p->registryEpoch);
done:cod1NodeBusy=0;return ok;
}
static int Cod1NodePreparedPostCohortCurrent(C1NPPrepared*p,GscSession*s,uint64_t id,uint64_t receipt){
 GscMapExecutionCertificate map;unsigned i;int ok=0;
 if(cod1NodeBusy){cod1NodeCancelled=1;return 0;}
 if(!p||!s||!id||p->sessionId!=id||p->count!=(unsigned)cod1NodeCount||!p->count||p->count>2048||
    !p->scripts||p->registryEpoch!=cod1NodeEpoch||!Cod1NodeAvailable())return 0;
 cod1NodeBusy=1;cod1NodeCancelled=0;map.receipt=p->registryEpoch;map.current=Cod1NodeScriptMapCurrent;
 if(!gsc_session_postcohort_graph_map_matches(s,id,receipt,&map)||!Cod1NodePreparedOwner(p->registryEpoch))goto done;
 for(i=0;i<p->count;i++)if(p->scripts[i].cache){
  if(!gsc_session_postcohort_graph_script_current(s,id,receipt,p->scripts[i])||!Cod1NodePreparedOwner(p->registryEpoch))goto done;
 }ok=Cod1NodePreparedOwner(p->registryEpoch);
done:cod1NodeBusy=0;return ok;
}
C1NPPrepared*G_Cod1NodePreparedBuild(GscSession*s,uint64_t sessionId){
 C1NPPrepared*p=NULL;C1NPInput in;unsigned i,j;const char*name;GscSessionSnapshot view;
 if(cod1NodeBusy){cod1NodeCancelled=1;return NULL;}
 if(!s||!sessionId||!Cod1NodeAvailable()||!cod1NodeCount)return NULL;
 cod1NodeBusy=1;cod1NodeCancelled=0;
 p=(C1NPPrepared*)calloc(1,sizeof(*p));if(!p)goto fail;
 p->registryEpoch=cod1NodeEpoch;p->sessionId=sessionId;p->count=(unsigned)cod1NodeCount;
 p->records=(unsigned char*)calloc(p->count,116);p->scripts=(GscNodeScriptRef*)calloc(p->count,sizeof(*p->scripts));
 p->tokens=calloc(C1NP_TOKEN_CAPACITY,sizeof(*p->tokens));if(!p->records||!p->scripts||!p->tokens)goto fail;
 for(i=0;i<p->count;i++){
  const Cod1NodeRecord*r=&cod1Nodes[i];
  if(!Cod1NodePreparedOwner(p->registryEpoch))goto fail;
  /* Known built-in radius conversion and alternate field spelling have not
   * been admitted here. Refuse before publishing a misleading zero field. */
  for(j=0;j<(unsigned)r->fields;j++){
   const char*k=cod1NodeArena+r->key[j];const unsigned char*q=(const unsigned char*)k;
   for(;*q;q++)if(*q>='A'&&*q<='Z')goto fail;
   if(!strcmp(k,"radius")||!strcmp(k,"type"))goto fail;
  }
  memset(&in,0,sizeof(in));in.type=r->type;in.index=i;in.flags=r->spawnflags;memcpy(in.origin,r->origin,12);
  in.hasYaw=Cod1NodeRaw(r,"angles")!=NULL;in.yaw=r->angles[1];
  if(!Cod1NodePreparedToken(p,Cod1NodeRaw(r,"targetname"),&in.targetname)||
     !Cod1NodePreparedToken(p,Cod1NodeRaw(r,"script_noteworthy"),&in.noteworthy)||
     !Cod1NodePreparedToken(p,Cod1NodeRaw(r,"target"),&in.target)||
     !Cod1NodePreparedToken(p,Cod1NodeRaw(r,"animscript"),&in.animscript))goto fail;
  name=Cod1NodeRaw(r,"animscript");
  if(in.type==16&&name){
   in.script=gsc_session_node_script_lookup(s,sessionId,name);
   if(!Cod1NodePreparedOwner(p->registryEpoch))goto fail;
   /* A true cache miss is type0; a failed/retired Session is transaction
    * failure and must not be disguised as a successful invalid node. */
   if(!gsc_session_snapshot(s,&view)||view.session_id!=sessionId||!view.has_runtime||
      (view.state!=GSC_SESSION_PREPARED&&view.state!=GSC_SESSION_RUNNING&&view.state!=GSC_SESSION_COMPLETED))goto fail;
  }
  if(!C1NP_Prepare(p->records+(size_t)i*116,&in))goto fail;
  p->scripts[i]=in.script;
 }
 if(!Cod1NodePreparedRefs(p,s))goto fail;
 cod1NodeBusy=0;return p;
fail:G_Cod1NodePreparedFree(p);cod1NodeBusy=0;return NULL;
}
