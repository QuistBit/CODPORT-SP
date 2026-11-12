/* Public localized strings have map lifetime. Only SERVER owns their exact
 * receipts across GAME DLL reload. A fresh GAME/Session acquires a distinct
 * publication lease; stale/foreign public bytes are never adopted or erased.
 * Port lifecycle policy; native ordinals/ASCII lookup remain in GAME/VM. */
static struct {
 int valid,held,module;
 char records[MAX_COD1_LOCALIZED_STRINGS][128];
 struct {
  int armed,active,used,server_id,maxclients,challenge,connect_time;
  cod1LocalizedStartup_t request;
  client_t*client;sharedEntity_t*entity;playerState_t*player;
  netadr_t address;
 } startup;
} c1LocalizedMap;
static uint64_t c1LocalizedSerial;
void SV_Cod1LocalizedClearMap(void){memset(&c1LocalizedMap,0,sizeof(c1LocalizedMap));}
void SV_Cod1LocalizedBeginGameModule(void){c1LocalizedMap.module=1;c1LocalizedMap.held=0;memset(&c1LocalizedMap.startup,0,sizeof(c1LocalizedMap.startup));}
void SV_Cod1LocalizedEndGameModule(void){c1LocalizedMap.module=0;c1LocalizedMap.held=0;memset(&c1LocalizedMap.startup,0,sizeof(c1LocalizedMap.startup));}
static int C1LocalizedCopy(char out[128],const char*text){
 size_t n;const unsigned char*p=(const unsigned char*)text;if(!p)return 0;
 for(n=0;n<128&&p[n];n++)if(p[n]<32||p[n]>126||p[n]=='"'||p[n]=='\\'||p[n]=='%')return 0;
 if(n==128)return 0;memcpy(out,text,n+1);return 1;
}
static int C1LocalizedCurrent(void){
 int i;for(i=0;i<MAX_COD1_LOCALIZED_STRINGS;i++){
  const char*p=sv.configstrings[CS_COD1_LOCALIZED_STRINGS+i];
  if(!p||strcmp(p,c1LocalizedMap.valid?c1LocalizedMap.records[i]:""))return 0;
 }return 1;
}
static int C1LocalizedLoading(void){
 return c1LocalizedMap.module&&sv.state==SS_LOADING&&sv_gametype&&sv_gametype->integer==GT_SINGLE_PLAYER;
}
static int C1LocalizedStartupCurrent(void){
 client_t*c;
 if(!c1LocalizedMap.module||!c1LocalizedMap.valid||!c1LocalizedMap.held||
    !c1LocalizedMap.startup.armed||!c1LocalizedMap.startup.active||!c1LocalizedMap.startup.used||
    sv.state!=SS_GAME||!sv_gametype||sv_gametype->integer!=GT_SINGLE_PLAYER||
    !sv_maxclients||sv_maxclients->integer<1||sv_maxclients->integer>MAX_CLIENTS||
    sv_maxclients->integer!=c1LocalizedMap.startup.maxclients||!svs.clients||
    !sv.gentities||sv.gentitySize<(int)sizeof(sharedEntity_t)||sv.num_entities<1||
    !sv.gameClients||sv.gameClientSize<(int)sizeof(playerState_t)||
    sv.serverId!=c1LocalizedMap.startup.server_id||!C1LocalizedCurrent())return 0;
 c=&svs.clients[0];
 return c==c1LocalizedMap.startup.client&&c->state==CS_ACTIVE&&
    c->challenge==c1LocalizedMap.startup.challenge&&c->lastConnectTime==c1LocalizedMap.startup.connect_time&&
    !memcmp(&c->netchan.remoteAddress,&c1LocalizedMap.startup.address,sizeof(netadr_t))&&
    c->netchan.remoteAddress.type!=NA_BOT&&c->gentity==c1LocalizedMap.startup.entity&&
    c->gentity==SV_GentityNum(0)&&SV_GameClientNum(0)==c1LocalizedMap.startup.player&&
    c->gentity->s.number==0&&c->gentity->s.eType==ET_PLAYER&&c->gentity->r.linked&&
    !(c->gentity->r.svFlags&(SVF_BOT|SVF_CASTAI))&&c1LocalizedMap.startup.player->clientNum==0;
}
static int C1LocalizedPublishing(void){return C1LocalizedLoading()||C1LocalizedStartupCurrent();}
static int C1LocalizedCommit(int index,const char*value){
 size_t chars=1,n;int i;char*owned;
 if(!C1LocalizedPublishing())return 0;
 if(!strcmp(sv.configstrings[index],value))return 1;
 for(i=0;i<MAX_CONFIGSTRINGS;i++){
  const char*p=i==index?value:sv.configstrings[i];if(!p)return 0;if(!*p)continue;n=strlen(p);
  if(n>=BIG_INFO_STRING||n+1>MAX_GAMESTATE_CHARS-chars)return 0;chars+=n+1;
 }
 /* Allocation precedes publication. All-client queue failure is unchanged. */
 owned=CopyString(value);
 if(!owned)return 0;
 if(!C1LocalizedPublishing()){Z_Free(owned);return 0;}
 if((sv.restarting||sv.state==SS_GAME)&&!SV_Cod1TryBroadcastLocalized(index,value)){Z_Free(owned);return 0;}
 Z_Free(sv.configstrings[index]);sv.configstrings[index]=owned;return 1;
}
static int C1LocalizedOverlap(const void*p,size_t n,const void*q,size_t z){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return p&&q&&n&&z&&(a<=b?b-a<n:a-b<z);
}
int SV_Cod1LocalizedStorageSeparate(const void*p,size_t n){
 int i;
 if(!p||!n||C1LocalizedOverlap(p,n,&c1LocalizedMap,sizeof(c1LocalizedMap))||
    C1LocalizedOverlap(p,n,&c1LocalizedSerial,sizeof(c1LocalizedSerial))||
    C1LocalizedOverlap(p,n,&sv,sizeof(sv))||C1LocalizedOverlap(p,n,&svs,sizeof(svs)))return 0;
 for(i=0;i<MAX_CONFIGSTRINGS;i++)if(sv.configstrings[i]&&C1LocalizedOverlap(p,n,sv.configstrings[i],strlen(sv.configstrings[i])+1))return 0;
 if(sv.gentities&&sv.gentitySize>0&&sv.num_entities>0&&sv.num_entities<=MAX_GENTITIES&&
    (size_t)sv.gentitySize<=SIZE_MAX/(size_t)sv.num_entities&&
    C1LocalizedOverlap(p,n,sv.gentities,(size_t)sv.gentitySize*sv.num_entities))return 0;
 if(sv.gameClients&&sv.gameClientSize>0&&sv_maxclients&&sv_maxclients->integer>0&&sv_maxclients->integer<=MAX_CLIENTS&&
    (size_t)sv.gameClientSize<=SIZE_MAX/(size_t)sv_maxclients->integer&&
    C1LocalizedOverlap(p,n,sv.gameClients,(size_t)sv.gameClientSize*sv_maxclients->integer))return 0;
 return 1;
}
static int C1LocalizedOutput(char*out,int cap){
 int i;if(!out||cap!=128||C1LocalizedOverlap(out,128,&c1LocalizedMap,sizeof(c1LocalizedMap))||
  C1LocalizedOverlap(out,128,&c1LocalizedSerial,sizeof(c1LocalizedSerial))||
  C1LocalizedOverlap(out,128,&sv,sizeof(sv))||C1LocalizedOverlap(out,128,&svs,sizeof(svs)))return 0;
 for(i=0;i<MAX_CONFIGSTRINGS;i++)if(sv.configstrings[i]&&C1LocalizedOverlap(out,128,sv.configstrings[i],strlen(sv.configstrings[i])+1))return 0;
 if(sv_maxclients&&svs.clients){
  if(sv_maxclients->integer<1||sv_maxclients->integer>MAX_CLIENTS)return 0;
  if(C1LocalizedOverlap(out,128,svs.clients,sizeof(client_t)*sv_maxclients->integer))return 0;
  for(i=0;i<sv_maxclients->integer;i++){
   reliableCommands_t*r=&svs.clients[i].reliableCommands;
   if(r->bufSize>0&&C1LocalizedOverlap(out,128,r->buf,r->bufSize))return 0;
   if(C1LocalizedOverlap(out,128,r->commands,sizeof(char*)*MAX_RELIABLE_COMMANDS)||
      C1LocalizedOverlap(out,128,r->commandLengths,sizeof(int)*MAX_RELIABLE_COMMANDS))return 0;
  }
 }
 return 1;
}
qboolean SV_Cod1LocalizedAcquire(char*out,int cap){
 char marker[128];uint64_t next;
 if(!C1LocalizedOutput(out,cap)||!C1LocalizedLoading()||c1LocalizedMap.held||
    c1LocalizedSerial==UINT64_MAX||!C1LocalizedCurrent())return qfalse;
 next=c1LocalizedSerial+1;Com_sprintf(marker,sizeof(marker),"cod1loc2 %llu",(unsigned long long)next);
 if(!C1LocalizedCommit(CS_COD1_LOCALIZED_STRINGS,marker))return qfalse;
 c1LocalizedSerial=next;c1LocalizedMap.valid=c1LocalizedMap.held=1;
 memset(&c1LocalizedMap.startup,0,sizeof(c1LocalizedMap.startup));
 strcpy(c1LocalizedMap.records[0],marker);strcpy(out,marker);return qtrue;
}
qboolean SV_Cod1LocalizedRelease(const char*owner){
 char marker[128];
 if(!C1LocalizedCopy(marker,owner)||!c1LocalizedMap.valid||!c1LocalizedMap.held||
    strcmp(marker,c1LocalizedMap.records[0]))return qfalse;
 /* Retiring a capability never changes public bytes; even a foreign table
  * replacement must not prevent local retirement. Reacquire revalidates ALL. */
 c1LocalizedMap.held=0;c1LocalizedMap.startup.active=0;return qtrue;
}
qboolean SV_Cod1LocalizedStartup(const cod1LocalizedStartup_t*input){
 cod1LocalizedStartup_t q;client_t*c;
 if(!input)return qfalse;memcpy(&q,input,sizeof(q));
 if(q.version!=COD1_LOCALIZED_STARTUP_VERSION||q.bytes!=sizeof(q)||q.reserved||
    !q.session_id||!q.map_epoch||q.player_slot||!memchr(q.owner,0,sizeof(q.owner))||
    !c1LocalizedMap.module||!c1LocalizedMap.valid||!c1LocalizedMap.held||
    strcmp(q.owner,c1LocalizedMap.records[0]))return qfalse;
 if(q.operation==COD1_LOCALIZED_STARTUP_ARM){
  if(q.player_generation||!C1LocalizedLoading()||!C1LocalizedCurrent()||c1LocalizedMap.startup.armed)return qfalse;
  c1LocalizedMap.startup.request=q;c1LocalizedMap.startup.server_id=sv.serverId;c1LocalizedMap.startup.armed=1;return qtrue;
 }
 if(!c1LocalizedMap.startup.armed||q.session_id!=c1LocalizedMap.startup.request.session_id||
    q.map_epoch!=c1LocalizedMap.startup.request.map_epoch||strcmp(q.owner,c1LocalizedMap.startup.request.owner))return qfalse;
 if(q.operation==COD1_LOCALIZED_STARTUP_CANCEL){c1LocalizedMap.startup.active=0;c1LocalizedMap.startup.used=1;return qtrue;}
 if(q.operation==COD1_LOCALIZED_STARTUP_END){
  if(!c1LocalizedMap.startup.active||q.player_generation!=c1LocalizedMap.startup.request.player_generation)return qfalse;
  c1LocalizedMap.startup.active=0;return qtrue;
 }
 if(q.operation!=COD1_LOCALIZED_STARTUP_BEGIN||!q.player_generation||c1LocalizedMap.startup.used||
    sv.state!=SS_GAME||!sv_gametype||sv_gametype->integer!=GT_SINGLE_PLAYER||
    !sv_maxclients||sv_maxclients->integer<1||sv_maxclients->integer>MAX_CLIENTS||!svs.clients||!C1LocalizedCurrent()||
    sv.serverId!=c1LocalizedMap.startup.server_id||
    !sv.gentities||sv.gentitySize<(int)sizeof(sharedEntity_t)||sv.num_entities<1||
    !sv.gameClients||sv.gameClientSize<(int)sizeof(playerState_t))return qfalse;
 c=&svs.clients[0];
 if(c->state!=CS_ACTIVE||!c->gentity||c->gentity!=SV_GentityNum(0)||
    c->gentity->s.number||c->gentity->s.eType!=ET_PLAYER||!c->gentity->r.linked||
    c->netchan.remoteAddress.type==NA_BOT||c->gentity->r.svFlags&(SVF_BOT|SVF_CASTAI)||
    SV_GameClientNum(0)->clientNum)return qfalse;
 c1LocalizedMap.startup.request=q;c1LocalizedMap.startup.used=c1LocalizedMap.startup.active=1;
 c1LocalizedMap.startup.server_id=sv.serverId;c1LocalizedMap.startup.client=c;
 c1LocalizedMap.startup.maxclients=sv_maxclients->integer;
 c1LocalizedMap.startup.entity=c->gentity;c1LocalizedMap.startup.player=SV_GameClientNum(0);
 c1LocalizedMap.startup.challenge=c->challenge;c1LocalizedMap.startup.connect_time=c->lastConnectTime;
 c1LocalizedMap.startup.address=c->netchan.remoteAddress;return qtrue;
}
qboolean SV_Cod1TrySetLocalizedConfigstring(int index,const char*val,const char*expected,const char*owner){
 char value[128],prior[128],marker[128];int i,slot;
 if(index<=CS_COD1_LOCALIZED_STRINGS||index>=CS_COD1_LOCALIZED_STRINGS+MAX_COD1_LOCALIZED_STRINGS)return qfalse;
 slot=index-CS_COD1_LOCALIZED_STRINGS;
 if(!C1LocalizedCopy(value,val)||!value[0]||
    !C1LocalizedCopy(prior,expected)||!C1LocalizedCopy(marker,owner)||!C1LocalizedPublishing()||
    !c1LocalizedMap.valid||!c1LocalizedMap.held||strcmp(marker,c1LocalizedMap.records[0])||
    !C1LocalizedCurrent()||strcmp(prior,c1LocalizedMap.records[slot]))return qfalse;
 /* Existing ordinals are immutable map data, including their first spelling. */
 if(prior[0])return !strcmp(value,prior)?qtrue:qfalse;
 for(i=1;i<slot;i++)if(!c1LocalizedMap.records[i][0]||!Q_stricmp(c1LocalizedMap.records[i],value))return qfalse;
 if(!C1LocalizedCommit(index,value))return qfalse;
 strcpy(c1LocalizedMap.records[slot],value);return qtrue;
}
