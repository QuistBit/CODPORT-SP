#ifndef BG_COD1_LOOP_RECEIPT_H
#define BG_COD1_LOOP_RECEIPT_H
#include "bg_cod1_loop_sound.h"
/* Port delivery receipt, never a script notification or sound duration.
 * status: accepted live state, rejected command, loss after acceptance, or
 * verified retirement while the client has no active snapshot. */
enum {C1P_REJECTED=0,C1P_ACCEPTED=1,C1P_LOST=2,C1P_SUSPENDED=3,C1P_DEFERRED=4};
#define C1P_ARGC 9
#define C1P_CAPACITY 128
typedef struct {uint64_t epoch,sequence,entity_epoch;uint32_t entity,generation,live,status;} C1PReceipt;
static int C1P_Valid(const C1PReceipt*p){return p&&p->epoch&&p->sequence&&p->entity_epoch&&p->entity<1024&&p->generation&&p->live<=1&&p->status<=4&&(p->status<C1P_LOST||p->live);}
static C1PReceipt C1P_From(const C1LPacket*p,unsigned status){C1PReceipt r;memset(&r,0,sizeof(r));r.epoch=p->sound.epoch;r.sequence=p->sound.sequence;r.entity_epoch=p->entity_epoch;r.entity=p->sound.entity;r.generation=p->generation;r.live=p->live;r.status=status;return r;}
static int C1P_Matches(const C1PReceipt*r,const C1LPacket*p){return C1P_Valid(r)&&r->epoch==p->sound.epoch&&r->sequence==p->sound.sequence&&r->entity_epoch==p->entity_epoch&&r->entity==p->sound.entity&&r->generation==p->generation&&r->live==(unsigned)p->live;}
static int C1P_Encode(char*out,size_t cap,const C1PReceipt*p){char b[C1P_CAPACITY];uint64_t v[7];int i,w;size_t at=6;if(!out||!C1P_Valid(p))return 0;memcpy(b,"c1p 3 ",6);v[0]=p->epoch;v[1]=p->sequence;v[2]=p->entity_epoch;v[3]=p->entity;v[4]=p->generation;v[5]=p->live;v[6]=p->status;for(i=0;i<7;i++){w=i<3?16:8;CQ_WriteHex(b+at,w,v[i]);at+=w;b[at++]=i==6?0:' ';}if(cap<at)return 0;memcpy(out,b,at);return 1;}
static int C1P_Decode(int argc,const char*const*a,C1PReceipt*out){C1PReceipt r;uint64_t v[7];int i;if(!out||!a||argc!=C1P_ARGC)return 0;for(i=0;i<argc;i++)if(!a[i])return 0;if(strcmp(a[0],"c1p")||strcmp(a[1],"3"))return 0;for(i=0;i<7;i++)if(!CQ_ParseHex(a[i+2],i<3?16:8,v+i))return 0;memset(&r,0,sizeof(r));r.epoch=v[0];r.sequence=v[1];r.entity_epoch=v[2];r.entity=(uint32_t)v[3];r.generation=(uint32_t)v[4];r.live=(uint32_t)v[5];r.status=(uint32_t)v[6];if(!C1P_Valid(&r))return 0;*out=r;return 1;}
/* Sender receipt mailbox. Queueing/sending is not acceptance. Retain exact
 * tombstone identity after entity free; recipient/scene validation is owned
 * by the GAME adapter and is mandatory at Begin/Report. */
typedef struct {int pending,accepted,failed,suspended,deferred;uint64_t recipient;C1LPacket sent;} C1PDelivery;
static int C1P_Begin(C1PDelivery*d,uint64_t recipient,const C1LPacket*p){C1PDelivery n;if(!d||!recipient||!C1L_Valid(p))return 0;if(d->sent.sound.epoch==p->sound.epoch&&p->sound.sequence<=d->sent.sound.sequence)return 0;memset(&n,0,sizeof(n));n.pending=1;n.recipient=recipient;n.sent=*p;*d=n;return 1;}
static int C1P_Report(C1PDelivery*d,uint64_t recipient,const C1PReceipt*r){
 if(!d||!recipient||recipient!=d->recipient||!C1P_Matches(r,&d->sent))return 0;
 if(r->status==C1P_SUSPENDED){if(d->failed||d->suspended||(!d->pending&&!d->accepted&&!d->deferred))return 0;d->pending=d->accepted=d->deferred=0;d->suspended=1;return 1;}
 if(r->status==C1P_DEFERRED){if(d->failed||d->suspended||d->deferred||(!d->pending&&!d->accepted))return 0;d->pending=d->accepted=0;d->deferred=1;return 1;}
 if(r->status==C1P_LOST){if(!d->accepted||d->failed)return 0;d->accepted=0;d->failed=1;return 1;}
 if((!d->pending&&!d->deferred)||d->accepted||d->failed||d->suspended)return 0;d->pending=d->deferred=0;d->accepted=r->status==C1P_ACCEPTED;d->failed=r->status==C1P_REJECTED;return 1;
}
#endif
