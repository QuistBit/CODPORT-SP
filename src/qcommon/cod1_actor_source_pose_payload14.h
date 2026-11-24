#ifndef COD1_ACTOR_SOURCE_POSE_PAYLOAD14_H
#define COD1_ACTOR_SOURCE_POSE_PAYLOAD14_H
#include "cod1_actor_source_pose14.h"
/* Versioned animation payload: sixty weights, sixty phases, sixty source
 * ordinals, count, six native prone-controller words, then three zero words. All ordinal floats are exact small
 * integers, so the existing float delta codec carries them losslessly. */
#define C1ASP14_WORDS 190u
static float C1ASP14_Integer(unsigned v){unsigned bits=0,e=0,copy=v;float f;
 if(v){while(copy>>1){copy>>=1;e++;}bits=((127+e)<<23)|((v-(1u<<e))<<(23-e));}memcpy(&f,&bits,4);return f;
}
static int C1ASP14_ReadInteger(float f,unsigned maximum,unsigned*out){
 unsigned bits=C1ASP14_Bits(f),exponent=(bits>>23)&255,shift,value;
 if(!bits){*out=0;return 1;}
 if(bits&0x80000000u||exponent<127||exponent>138)return 0;
 shift=150-exponent;if(bits&((1u<<shift)-1u))return 0;
 value=((bits&0x7fffffu)|0x800000u)>>shift;if(value>maximum)return 0;*out=value;return 1;
}
static int C1ASP14_EncodePayloadInner(const c1asp14Packet_t*p,float out[C1ASP14_WORDS]){
 c1asp14Decoded_t d;float v[C1ASP14_WORDS]={0};unsigned i;
 if(!p||!out||C1AB14_Overlap(p,sizeof(*p),out,sizeof(v))||!C1ASP14_DecodeInner(p,&d))return 0;
 for(i=0;i<p->count;i++){v[i]=p->weight[i];v[60+i]=p->phase[i];v[120+i]=C1ASP14_Integer(p->source[i]);}
 v[180]=C1ASP14_Integer(p->count);memcpy(v+181,p->controller,24);memcpy(out,v,sizeof(v));return 1;
}
static int C1ASP14_DecodePayloadInner(const float input[C1ASP14_WORDS],c1asp14Packet_t*out){
 c1asp14Packet_t p;c1asp14Decoded_t d;unsigned i;
 if(!input||!out||C1AB14_Overlap(input,C1ASP14_WORDS*4,out,sizeof(*out)))return 0;
 memset(&p,0,sizeof(p));if(!C1ASP14_ReadInteger(input[180],C1ASP14_MAX,&p.count))return 0;
 for(i=0;i<C1ASP14_MAX;i++){p.weight[i]=input[i];p.phase[i]=input[60+i];if(!C1ASP14_ReadInteger(input[120+i],C1ASP14_CATALOG-1,p.source+i))return 0;}
 memcpy(p.controller,input+181,24);
 for(i=187;i<C1ASP14_WORDS;i++)if(C1ASP14_Bits(input[i]))return 0;
 if(!C1ASP14_DecodeInner(&p,&d))return 0;*out=p;return 1;
}
static int C1ASP14_EncodePayload(const c1asp14Packet_t*p,float out[C1ASP14_WORDS]){int ok;C1ASP14_BEGIN;ok=C1ASP14_EncodePayloadInner(p,out);C1ASP14_END;return ok;}
static int C1ASP14_DecodePayload(const float input[C1ASP14_WORDS],c1asp14Packet_t*out){int ok;C1ASP14_BEGIN;ok=C1ASP14_DecodePayloadInner(input,out);C1ASP14_END;return ok;}
#endif
