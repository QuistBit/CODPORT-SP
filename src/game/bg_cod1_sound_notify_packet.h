#ifndef BG_COD1_SOUND_NOTIFY_PACKET_H
#define BG_COD1_SOUND_NOTIFY_PACKET_H
/* Paired port protocol; deliberately separate from the retained c1s ABI. */
#include "bg_cod1_script_sound_packet.h"
#include "bg_cod1_sound_notify.h"
#define C1N_ARGC 16
#define C1N_CAPACITY (C1S_CAPACITY+32)
#define C1D_ARGC 8
#define C1D_CAPACITY 128
typedef struct {Cod1SoundPacket sound;uint64_t session;uint32_t generation;} C1NPacket;
typedef struct {C1NIdentity identity;int duration;} C1DReport;
static int C1N_EncodePacket(char *out,size_t cap,const C1NPacket *p){
 char b[C1N_CAPACITY];size_t n;
 if(!out||!p||!p->session||!p->generation||!C1S_Encode(b,sizeof(b),&p->sound))return 0;
 b[2]='n';n=strlen(b);b[n++]=' ';CQ_WriteHex(b+n,16,p->session);n+=16;
 b[n++]=' ';CQ_WriteHex(b+n,8,p->generation);n+=8;b[n]=0;
 if(cap<=n)return 0;memcpy(out,b,n+1);return 1;
}
static int C1N_DecodePacket(int argc,const char *const *argv,C1NPacket *out){
 const char *a[C1S_ARGC];C1NPacket p;uint64_t gen;int i;
 if(!argv||!out||argc!=C1N_ARGC)return 0;
 for(i=0;i<argc;i++)if(!argv[i])return 0;
 if(strcmp(argv[0],"c1n"))return 0;
 memset(&p,0,sizeof(p));for(i=0;i<C1S_ARGC;i++)a[i]=argv[i];a[0]="c1s";
 if(!C1S_Decode(C1S_ARGC,a,&p.sound)||!CQ_ParseHex(argv[14],16,&p.session)||!p.session||
    !CQ_ParseHex(argv[15],8,&gen)||!gen)return 0;
 p.generation=(uint32_t)gen;*out=p;return 1;
}
static C1NIdentity C1N_PacketIdentity(const C1NPacket *p){
 C1NIdentity id;memset(&id,0,sizeof(id));id.epoch=p->sound.epoch;id.session=p->session;
 id.sequence=p->sound.sequence;id.entity=p->sound.entity;id.generation=p->generation;return id;
}
static int C1D_Encode(char *out,size_t cap,const C1DReport *r){
 char b[C1D_CAPACITY];size_t n=6;const C1NIdentity *id;
 if(!out||!r||!C1N_Identity(&r->identity)||r->duration<0||r->duration>C1N_MAX_DURATION_MS)return 0;
 id=&r->identity;memcpy(b,"c1d 1 ",6);
 CQ_WriteHex(b+n,16,id->epoch);n+=16;b[n++]=' ';CQ_WriteHex(b+n,16,id->session);n+=16;
 b[n++]=' ';CQ_WriteHex(b+n,8,id->entity);n+=8;b[n++]=' ';CQ_WriteHex(b+n,8,id->generation);n+=8;
 b[n++]=' ';CQ_WriteHex(b+n,16,id->sequence);n+=16;b[n++]=' ';CQ_WriteHex(b+n,8,(uint32_t)r->duration);n+=8;b[n]=0;
 if(cap<=n)return 0;memcpy(out,b,n+1);return 1;
}
static int C1D_Decode(int argc,const char *const *argv,C1DReport *out){
 C1DReport r;uint64_t v;int i;
 if(!argv||!out||argc!=C1D_ARGC)return 0;
 for(i=0;i<argc;i++)if(!argv[i])return 0;
 if(strcmp(argv[0],"c1d")||strcmp(argv[1],"1"))return 0;
 memset(&r,0,sizeof(r));
 if(!CQ_ParseHex(argv[2],16,&r.identity.epoch)||!CQ_ParseHex(argv[3],16,&r.identity.session)||
    !CQ_ParseHex(argv[4],8,&v))return 0;r.identity.entity=(uint32_t)v;
 if(!CQ_ParseHex(argv[5],8,&v))return 0;r.identity.generation=(uint32_t)v;
 if(!CQ_ParseHex(argv[6],16,&r.identity.sequence)||!CQ_ParseHex(argv[7],8,&v)||v>C1N_MAX_DURATION_MS)return 0;
 r.duration=(int)v;if(!C1N_Identity(&r.identity))return 0;*out=r;return 1;
}
#endif
