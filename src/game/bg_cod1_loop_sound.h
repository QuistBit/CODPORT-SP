#ifndef BG_COD1_LOOP_SOUND_H
#define BG_COD1_LOOP_SOUND_H
/* Paired port transport, NOT the original entity snapshot layout. Native
 * evidence: loop alias persists per allocation; client refresh keeps a handle,
 * missing refresh retires it. No sounddone, duration or notify is represented. */
#include "bg_cod1_script_sound_packet.h"
#include "bg_cod1_loop_parameters.h"
#define C1L_SOUND_ARGC 23
#define C1L_SOUND_CAPACITY 768
/* native_parameters==1 carries ranges, not a claimed playback gain/pitch.
 * Zero gain/pitch are canonical unused fields in that mode. Mode0 retains
 * the explicit resolved-packet adapter for bounded component compatibility. */
typedef struct {Cod1SoundPacket sound;uint64_t entity_epoch;uint32_t generation;int live;char alias[128];float refresh_gain;unsigned native_parameters;C1LRange range;} C1LPacket;
static int C1L_Valid(const C1LPacket*p){
 Cod1SoundPacket zero;
 if(!p||!p->sound.epoch||!p->sound.sequence||!p->entity_epoch||!p->generation||p->sound.entity>=1024||(p->live!=0&&p->live!=1))return 0;
 if(p->native_parameters>1)return 0;
 if(p->live){Cod1SoundPacket v=p->sound;C1LRange z;memset(&z,0,sizeof(z));
  if(p->native_parameters){if(!C1L_RangeValid(&p->range)||CQ_FloatBits(v.gain)||CQ_FloatBits(v.pitch)||CQ_FloatBits(p->refresh_gain)!=CQ_FloatBits((float)((double)p->range.volume_min*(double).8f)))return 0;v.pitch=1;}
  else if(memcmp(&z,&p->range,sizeof(z)))return 0;
  return C1S_Valid(&v)&&!v.channel&&Cod1SS_Token(p->alias,sizeof(p->alias))&&CQ_Finite(p->refresh_gain)&&p->refresh_gain>=0&&p->refresh_gain<=C1S_GAIN_MAX;
 }
 memset(&zero,0,sizeof(zero));zero.epoch=p->sound.epoch;zero.sequence=p->sound.sequence;zero.entity=p->sound.entity;
 {C1LRange z;memset(&z,0,sizeof(z));return !memcmp(&zero,&p->sound,sizeof(zero))&&!p->alias[0]&&CQ_FloatBits(p->refresh_gain)==0&&!p->native_parameters&&!memcmp(&z,&p->range,sizeof(z));}
}
static int C1L_Encode(char*out,size_t cap,const C1LPacket*p){
 char text[C1L_SOUND_CAPACITY];uint64_t fields[6];float vals[12];size_t at=6,n;int i;
 if(!out||!C1L_Valid(p))return 0;memcpy(text,"c1o 3 ",6);
 fields[0]=p->sound.epoch;fields[1]=p->sound.sequence;fields[2]=p->entity_epoch;fields[3]=p->sound.entity;fields[4]=p->generation;fields[5]=p->live;
 for(i=0;i<6;i++){int w=i<3?16:8;CQ_WriteHex(text+at,w,fields[i]);at+=w;text[at++]=' ';}
 n=p->live?strlen(p->alias):1;memcpy(text+at,p->live?p->alias:"-",n);at+=n;text[at++]=' ';
 n=p->live?strlen(p->sound.path):1;memcpy(text+at,p->live?p->sound.path:"-",n);at+=n;
 memcpy(vals,p->sound.origin,12);vals[3]=p->sound.gain;vals[4]=p->sound.pitch;vals[5]=p->sound.dist_min;vals[6]=p->sound.dist_max;
 vals[7]=p->refresh_gain;memcpy(vals+8,&p->range,sizeof(p->range));for(i=0;i<12;i++){text[at++]=' ';CQ_WriteHex(text+at,8,CQ_FloatBits(vals[i]));at+=8;}text[at++]=' ';CQ_WriteHex(text+at,8,p->native_parameters);at+=8;text[at]=0;
 if(at>=cap)return 0;memcpy(out,text,at+1);return 1;
}
static int C1L_Decode(int argc,const char*const*a,C1LPacket*out){
 C1LPacket p;uint64_t f[6],v;float vals[12];size_t n;int i;
 if(!a||!out||argc!=C1L_SOUND_ARGC)return 0;for(i=0;i<argc;i++)if(!a[i])return 0;
 if(strcmp(a[0],"c1o")||strcmp(a[1],"3"))return 0;memset(&p,0,sizeof(p));
 for(i=0;i<6;i++)if(!CQ_ParseHex(a[i+2],i<3?16:8,&f[i]))return 0;
 p.sound.epoch=f[0];p.sound.sequence=f[1];p.entity_epoch=f[2];p.sound.entity=(uint32_t)f[3];p.generation=(uint32_t)f[4];p.live=(int)f[5];
 if(p.live==1){if(!Cod1SS_Length(a[8],sizeof(p.alias),&n))return 0;memcpy(p.alias,a[8],n+1);if(!Cod1SS_Length(a[9],sizeof(p.sound.path),&n))return 0;memcpy(p.sound.path,a[9],n+1);}
 else if(strcmp(a[8],"-")||strcmp(a[9],"-"))return 0;
 for(i=0;i<12;i++){if(!CQ_ParseHex(a[10+i],8,&v))return 0;vals[i]=CQ_FloatFromBits((uint32_t)v);}
 memcpy(p.sound.origin,vals,12);p.sound.gain=vals[3];p.sound.pitch=vals[4];p.sound.dist_min=vals[5];p.sound.dist_max=vals[6];
 p.refresh_gain=vals[7];memcpy(&p.range,vals+8,sizeof(p.range));if(!CQ_ParseHex(a[22],8,&v))return 0;p.native_parameters=(unsigned)v;if(!C1L_Valid(&p))return 0;*out=p;return 1;
}
enum {C1L_RESET=1,C1L_UPDATE=2,C1L_STOP=3,C1L_END_FRAME=4};
enum {C1L_PLAYING=1,C1L_OUT_OF_RANGE=2};
typedef struct cod1LoopStreamRequest_s {unsigned operation;uint64_t incarnation,frame;C1LPacket packet;} cod1LoopStreamRequest;
static int C1L_RequestValid(const cod1LoopStreamRequest*r){
 if(!r||!r->incarnation||r->incarnation==UINT64_MAX||!r->packet.sound.epoch)return 0;
 if(r->operation==C1L_RESET)return !r->frame;
 if(r->operation==C1L_END_FRAME)return r->frame&&r->frame!=UINT64_MAX;
 return (r->operation==C1L_UPDATE||r->operation==C1L_STOP)&&r->frame&&r->frame!=UINT64_MAX&&r->packet.live&&C1L_Valid(&r->packet);
}
#endif
