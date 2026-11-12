#ifndef SND_COD1_STREAM_WAV_H
#define SND_COD1_STREAM_WAV_H
/* R230 bounded RIFF inspection through positioned file reads. No PCM is
 * cached here: the accepted data offset belongs to the actual stream reader.
 * Output is published only after complete validation and a live owner lease. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {void *user;int (*current)(void *);int (*read_at)(void *,uint32_t,void *,size_t);} C1VFile;
typedef struct {uint32_t offset,bytes,frames,rate;unsigned channels,width;} C1VPCM;
static uint32_t C1V_LE32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static unsigned C1V_LE16(const unsigned char *p){return (unsigned)p[0]|((unsigned)p[1]<<8);}
static int C1V_Read(const C1VFile *f,uint32_t at,void *out,size_t n){return f->current(f->user)==1&&f->read_at(f->user,at,out,n)==1&&f->current(f->user)==1;}
static int C1V_Inspect(const C1VFile *f,uint32_t length,C1VPCM *out){
 unsigned char b[16];uint32_t end,at,size,next,fmt=0,data=0;unsigned chunks=0;C1VPCM v;
 if(!f||!out||!f->current||!f->read_at||length<44||!C1V_Read(f,0,b,12)||memcmp(b,"RIFF",4)||memcmp(b+8,"WAVE",4))return 0;
 end=C1V_LE32(b+4);if(end<36||end>length-8)return 0;end+=8;at=12;memset(&v,0,sizeof(v));
 while(at<end){
  if(++chunks>4096||end-at<8||!C1V_Read(f,at,b,8))return 0;
  size=C1V_LE32(b+4);if(size>end-at-8)return 0;next=at+8+size;
  if(size&1){if(next==end)return 0;++next;}
  if(!memcmp(b,"fmt ",4)){
   if(fmt||size<16||!C1V_Read(f,at+8,b,16)||C1V_LE16(b)!=1)return 0;
   v.channels=C1V_LE16(b+2);v.rate=C1V_LE32(b+4);v.width=C1V_LE16(b+14);
   if((v.channels!=1&&v.channels!=2)||(v.width!=8&&v.width!=16)||v.rate<8000||v.rate>192000)return 0;
   v.width/=8;if(C1V_LE16(b+12)!=v.channels*v.width||C1V_LE32(b+8)!=v.rate*v.channels*v.width)return 0;fmt=1;
  }else if(!memcmp(b,"data",4)){
   if(data||!size)return 0;v.offset=at+8;v.bytes=size;data=1;
  }
  at=next;
 }
 if(!fmt||!data||at!=end||v.bytes%(v.channels*v.width))return 0;
 v.frames=v.bytes/(v.channels*v.width);if(!v.frames||f->current(f->user)!=1)return 0;
 *out=v;return 1;
}
/* Round up: a real final sample must not produce an early completion event.
 * Bounds permit later resampling with integer effective source rate. */
static int C1V_Duration(const C1VPCM *v,uint32_t effectiveRate){uint64_t ms;if(!v||!v->frames||effectiveRate<1000||effectiveRate>768000)return -1;ms=((uint64_t)v->frames*1000+effectiveRate-1)/effectiveRate;return ms<=600000?(int)ms:-1;}
#endif
