#ifndef G_COD1_DYNAMIC_PATH_CORE_H
#define G_COD1_DYNAMIC_PATH_CORE_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/* Private DATA candidate operations, not a GAME/native capability or proof of
 * a live graph. Callers must supply finalized native-order nodes/12-byte links.
 * Never adopt authored getnode records or an empty synthetic graph as authority.
 * Mutations, including partial failures, affect only the caller's candidate;
 * a future live owner must validate, commit/rollback and retire that candidate.
 * Original: 20072390/72420 edge partition, 724E0 owner records,
 * 725D0 reconnect, 726C0 trace/disconnect. All unrelated edge bytes are retained. */
typedef struct {uint16_t target;uint8_t blockers,flags;uint32_t payload[2];} C1DPLink;
typedef struct {uint32_t first;uint16_t active,count;float origin[3];} C1DPNode;
typedef struct {uint16_t source,target,previous,next;} C1DPRecord;
typedef struct {uint32_t flags;uint16_t head,entity;int32_t time;} C1DPOwner;
typedef struct {C1DPNode*nodes;C1DPLink*links;C1DPRecord*records;C1DPOwner*owners;uint32_t node_count,link_count,record_count,owner_count;} C1DPState;
typedef int (*C1DPTrace)(void*,unsigned,unsigned,const float*,const float*,const float*,const float*,unsigned,unsigned);
enum {C1DP_OK=1,C1DP_BAD_DATA=-1,C1DP_NO_RECORD=-2,C1DP_COUNT_OVERFLOW=-3,C1DP_TRACE_FAILED=-4,C1DP_METHOD_REFUSED=-5};
static int C1DP_Layout(const C1DPState*s){uint32_t i,j,end=0;
 if(!s||!s->nodes||!s->links||!s->records||!s->owners||!s->node_count||s->node_count>2048||s->record_count<2||s->record_count>65535||!s->owner_count||s->owner_count>2048)return 0;
 for(i=0;i<s->node_count;i++){C1DPNode*n=s->nodes+i;if(n->active>n->count||n->count>32767||n->first!=end||n->count>s->link_count-end)return 0;
  for(j=0;j<n->count;j++){C1DPLink*l=s->links+n->first+j;if(l->target>=s->node_count||(j<n->active?l->blockers!=0:l->blockers==0))return 0;}end+=n->count;}
 if(end!=s->link_count)return 0;
 for(i=0;i<s->record_count;i++)if(s->records[i].next>=s->record_count||s->records[i].previous>=s->record_count)return 0;
 for(i=0;i<s->owner_count;i++)if(s->owners[i].head>=s->record_count||s->owners[i].entity>=2048)return 0;
 return 1;
}
static int C1DP_Block(C1DPState*s,unsigned ni,unsigned li){C1DPNode*n=s->nodes+ni;C1DPLink*l=s->links+n->first+li,tmp;
 if(l->blockers==255)return C1DP_COUNT_OVERFLOW; /* Native calls a diagnostic import after byte wrap; its continuation is unproven. Candidate refuses before wrap. */
 ++l->blockers;if(l->blockers==1){if(!n->active)return C1DP_BAD_DATA;--n->active;tmp=*l;*l=s->links[n->first+n->active];s->links[n->first+n->active]=tmp;}return C1DP_OK;
}
static int C1DP_Unblock(C1DPState*s,unsigned ni,unsigned li){C1DPNode*n=s->nodes+ni;C1DPLink*l=s->links+n->first+li,tmp;
 if(!l->blockers)return C1DP_BAD_DATA;--l->blockers;if(!l->blockers){if(n->active>=n->count)return C1DP_BAD_DATA;tmp=*l;*l=s->links[n->first+n->active];s->links[n->first+n->active]=tmp;++n->active;}return C1DP_OK;
}
static int C1DP_AddRecord(C1DPState*s,unsigned oi,unsigned ni,unsigned li){C1DPOwner*o=s->owners+oi;C1DPRecord*r=s->records;unsigned k=r[0].next,h=o->head;int ok;
 if(!k)return C1DP_NO_RECORD;if(k>=s->record_count||h>=s->record_count||r[k].next>=s->record_count)return C1DP_BAD_DATA;
 r[0].next=r[k].next;r[r[k].next].previous=0;r[k].source=(uint16_t)ni;r[k].target=s->links[s->nodes[ni].first+li].target;
 if(!h){o->head=(uint16_t)k;r[k].previous=r[k].next=(uint16_t)k;}
 else {r[k].previous=(uint16_t)h;r[k].next=r[h].next;r[r[k].next].previous=(uint16_t)k;r[h].next=(uint16_t)k;}
 ok=C1DP_Block(s,ni,li);return ok;
}
static int C1DP_Reconnect(C1DPState*s,unsigned oi){C1DPOwner*o=s->owners+oi;C1DPRecord*r=s->records;unsigned h=o->head,k,tail,freeTail,steps=0,j;int ok;
 o->flags|=0x4000u;if(!h)return C1DP_OK;
 if(h>=s->record_count)return C1DP_BAD_DATA;tail=r[h].previous;freeTail=r[0].previous;
 if(tail>=s->record_count||freeTail>=s->record_count)return C1DP_BAD_DATA;
 o->head=0;r[tail].next=0;r[freeTail].next=(uint16_t)h;r[0].previous=(uint16_t)tail;r[h].previous=(uint16_t)freeTail;
 for(k=h;k;k=r[k].next){C1DPNode*n;
  if(k>=s->record_count||++steps>=s->record_count||r[k].source>=s->node_count||r[k].target>=s->node_count)return C1DP_BAD_DATA;
  n=s->nodes+r[k].source;
  for(j=n->active;j<n->count;j++)if(s->links[n->first+j].target==r[k].target){ok=C1DP_Unblock(s,r[k].source,j);if(ok!=C1DP_OK)return ok;break;}
 }return C1DP_OK;
}
static int C1DP_Disconnect(C1DPState*s,unsigned oi,int32_t time,C1DPTrace trace,void*ctx){
 static const float mins[3]={-15,-15,18},maxs[3]={15,15,72};unsigned i;int j,hit,ok;C1DPOwner*o=s->owners+oi;
 if(!trace)return C1DP_TRACE_FAILED;ok=C1DP_Reconnect(s,oi);if(ok!=C1DP_OK)return ok;
 o->flags&=~0x4000u;o->time=time;
 for(i=0;i<s->node_count;i++)for(j=(int)s->nodes[i].count-1;j>=0;j--){unsigned target=s->links[s->nodes[i].first+j].target;
  if(target>=s->node_count)return C1DP_BAD_DATA;
  hit=trace(ctx,i,target,s->nodes[i].origin,s->nodes[target].origin,mins,maxs,o->entity,0x02820011u);
  if(hit!=0&&hit!=1)return C1DP_TRACE_FAILED;
  if(hit){ok=C1DP_AddRecord(s,oi,i,(unsigned)j);if(ok!=C1DP_OK)return ok;}
 }return C1DP_OK;
}
static int C1DP_CandidateTransition(C1DPState*s,unsigned owner,int disconnect,int32_t time,C1DPTrace trace,void*ctx){
 if(!C1DP_Layout(s)||owner>=s->owner_count||(disconnect!=0&&disconnect!=1))return C1DP_BAD_DATA;
 if(!(s->owners[owner].flags&0x1000u))return C1DP_METHOD_REFUSED;
 return disconnect?C1DP_Disconnect(s,owner,time,trace,ctx):C1DP_Reconnect(s,owner);
}
/* A malformed owner ring may fail after several edits. Run the existing
 * transition on scratch arrays and publish only a completely valid result. */
static int C1DP_ConnectOwned(C1DPState *live,unsigned index){
    C1DPState next;
    int result=C1DP_BAD_DATA;
    if(!C1DP_Layout(live)||index>=live->owner_count)return C1DP_BAD_DATA;
    if(!(live->owners[index].flags&0x1000u))return C1DP_METHOD_REFUSED;
    if(!live->owners[index].head)return C1DP_Reconnect(live,index);
    next=*live;
    next.nodes=malloc(sizeof(*next.nodes)*next.node_count);
    next.links=malloc(sizeof(*next.links)*next.link_count);
    next.records=malloc(sizeof(*next.records)*next.record_count);
    next.owners=malloc(sizeof(*next.owners)*next.owner_count);
    if(next.nodes&&next.links&&next.records&&next.owners){
        memcpy(next.nodes,live->nodes,sizeof(*next.nodes)*next.node_count);
        memcpy(next.links,live->links,sizeof(*next.links)*next.link_count);
        memcpy(next.records,live->records,sizeof(*next.records)*next.record_count);
        memcpy(next.owners,live->owners,sizeof(*next.owners)*next.owner_count);
        result=C1DP_CandidateTransition(&next,index,0,0,NULL,NULL);
        if(result==C1DP_OK && !C1DP_Layout(&next))result=C1DP_BAD_DATA;
        if(result==C1DP_OK){
            memcpy(live->nodes,next.nodes,sizeof(*next.nodes)*next.node_count);
            memcpy(live->links,next.links,sizeof(*next.links)*next.link_count);
            memcpy(live->records,next.records,sizeof(*next.records)*next.record_count);
            live->owners[index]=next.owners[index];
        }
    }
    free(next.nodes);free(next.links);free(next.records);free(next.owners);
    return result;
}
#endif
