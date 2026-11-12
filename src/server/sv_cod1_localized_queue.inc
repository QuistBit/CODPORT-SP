/* All-recipient transaction for one bounded configstring command. The actual
 * queue allocator and Huffman message encoder run on owned copies first.
 * No live queue/table mutation occurs before every recipient admits it. */
#include <stdlib.h>
typedef struct C1LocalizedQueueStage_s {
 struct C1LocalizedQueueStage_s*next;
 client_t*live;
 client_t copy;
 char*commands[MAX_RELIABLE_COMMANDS];
 int lengths[MAX_RELIABLE_COMMANDS];
 char*bytes;
} C1LocalizedQueueStage;
static int C1QueueOverlap(const void*a,size_t n,const void*b,size_t m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n&&m&&(x<=y?y-x<n:x-y<m);
}
static int C1QueueStorage(client_t*c){
 const void*p[3];size_t n[3];int i,j,k;
 p[0]=c->reliableCommands.buf;n[0]=c->reliableCommands.bufSize;
 p[1]=c->reliableCommands.commands;n[1]=sizeof(char*)*MAX_RELIABLE_COMMANDS;
 p[2]=c->reliableCommands.commandLengths;n[2]=sizeof(int)*MAX_RELIABLE_COMMANDS;
 for(i=0;i<3;i++){
  if(!SV_Cod1LocalizedStorageSeparate(p[i],n[i]))return 0;
  if(C1QueueOverlap(p[i],n[i],svs.clients,sizeof(client_t)*sv_maxclients->integer))return 0;
  for(j=i+1;j<3;j++)if(C1QueueOverlap(p[i],n[i],p[j],n[j]))return 0;
  for(k=0;k<sv_maxclients->integer;k++){
   reliableCommands_t*r=&svs.clients[k].reliableCommands;if(&svs.clients[k]==c)continue;
   if((r->bufSize>0&&C1QueueOverlap(p[i],n[i],r->buf,r->bufSize))||
      C1QueueOverlap(p[i],n[i],r->commands,sizeof(char*)*MAX_RELIABLE_COMMANDS)||
      C1QueueOverlap(p[i],n[i],r->commandLengths,sizeof(int)*MAX_RELIABLE_COMMANDS))return 0;
  }
 }return 1;
}
qboolean SV_Cod1TryBroadcastLocalized(int index,const char*value){
 C1LocalizedQueueStage*head=NULL,*tail=NULL,*p,*next;char command[192];int i,j,ok=0;
 if(!value||index<CS_COD1_LOCALIZED_STRINGS||index>=CS_COD1_LOCALIZED_STRINGS+MAX_COD1_LOCALIZED_STRINGS||
    !svs.clients||!sv_maxclients||sv_maxclients->integer<1||sv_maxclients->integer>MAX_CLIENTS)return qfalse;
 for(i=0;i<128&&value[i];i++)if((unsigned char)value[i]<32||(unsigned char)value[i]>126||value[i]=='"'||value[i]=='\\'||value[i]=='%')return qfalse;
 if(i==128)return qfalse;
 Com_sprintf(command,sizeof(command),"cs %d \"%s\"",index,value);
 for(i=0;i<sv_maxclients->integer;i++){
  client_t*c=&svs.clients[i];uintptr_t begin,limit,at;int size;
  if(c->state<CS_PRIMED)continue;
  if(c->netchan.remoteAddress.type==NA_BOT||(c->gentity&&(c->gentity->r.svFlags&(SVF_BOT|SVF_CASTAI))))continue;
  size=c->reliableCommands.bufSize;begin=(uintptr_t)c->reliableCommands.buf;
  if(!begin||size<=0||size>MAX_RELIABLE_COMMANDS*MAX_STRING_CHARS||
     !c->reliableCommands.commands||!c->reliableCommands.commandLengths||
     (uintptr_t)size>UINTPTR_MAX-begin||!C1QueueStorage(c))goto end;
  limit=begin+(uintptr_t)size;at=(uintptr_t)c->reliableCommands.rover;
  if(at<begin||at>limit)goto end;
  p=(C1LocalizedQueueStage*)calloc(1,sizeof(*p));if(!p)goto end;
  if(tail)tail->next=p;else head=p;tail=p;
  p->live=c;p->copy=*c;p->bytes=(char*)malloc(size);if(!p->bytes)goto end;
  memcpy(p->bytes,c->reliableCommands.buf,size);
  p->copy.reliableCommands.buf=p->bytes;p->copy.reliableCommands.commands=p->commands;
  p->copy.reliableCommands.commandLengths=p->lengths;p->copy.reliableCommands.rover=p->bytes+(at-begin);
  for(j=0;j<MAX_RELIABLE_COMMANDS;j++){
   int n=c->reliableCommands.commandLengths[j];at=(uintptr_t)c->reliableCommands.commands[j];
   if(!at){if(n)goto end;p->commands[j]=NULL;}
   else{if(n<0||at<begin||at>=limit||(uintptr_t)n>=limit-at)goto end;p->commands[j]=p->bytes+(at-begin);}
   p->lengths[j]=n;
  }
  if(!SV_Cod1QueueChecked(&p->copy,command,1))goto end;
 }
 /* No callback/allocation/failable operation follows the first live copy.
  * Single-threaded server execution keeps the validated recipients stable. */
 for(p=head;p;p=p->next){
  reliableCommands_t*live=&p->live->reliableCommands;
  memcpy(live->buf,p->bytes,live->bufSize);
  memcpy(live->commandLengths,p->lengths,sizeof(p->lengths));
  for(j=0;j<MAX_RELIABLE_COMMANDS;j++)live->commands[j]=p->commands[j]?live->buf+(p->commands[j]-p->bytes):NULL;
  live->rover=live->buf+(p->copy.reliableCommands.rover-p->bytes);
  p->live->reliableSequence=p->copy.reliableSequence;
 }
 ok=1;
end:
 for(p=head;p;p=next){next=p->next;free(p->bytes);free(p);}
 return ok?qtrue:qfalse;
}
