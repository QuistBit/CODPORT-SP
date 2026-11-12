#ifndef G_COD1_NODE_PREPARE_H
#define G_COD1_NODE_PREPARE_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "cod1_gsc/gsc_node_script_types.h"
/* Native 6DC90/6D990 initialization after admitted spawn-field conversion.
 * Local string IDs preserve equality only, not the engine's global SL ABI.
 * Original node+34 opaque32 is deliberately zero: the genuine port reference
 * lives in a typed sidecar indexed by authored ID, surviving physical sorting.
 * This private candidate is not a live graph or traversal execution permit. */
typedef struct {
 uint32_t type,index; int32_t flags;
 uint16_t targetname,noteworthy,target,animscript;
 float origin[3],yaw; int hasYaw;
 GscNodeScriptRef script;
} C1NPInput;
static void C1NP_Word(unsigned char*p,uint16_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void C1NP_Dword(unsigned char*p,uint32_t n){C1NP_Word(p,(uint16_t)n);C1NP_Word(p+2,(uint16_t)(n>>16));}
/* GAME107D0 rounds each add/subtract through a float memory cell. Restrict
 * huge finite inputs before its potentially non-progressing loop; this is a
 * port admission refusal, not claimed original invalid-input behavior. */
#include "../qcommon/cod1_angle_normalize14.h"
static int C1NP_Yaw(float input,float*out){return C1AngleNormalize14(input,out);}

static int C1NP_Prepare(unsigned char out[116],const C1NPInput*input){
 static const uint16_t masks[18]={0,0,4,8,16,0,0,0,0,4,8,16,0,0,4,0,1,1};
 C1NPInput v;unsigned char staged[116]={0};uint32_t yawBits,type;uint16_t flags;float yaw=-1.0f;unsigned i;
 if(!out||!input)return 0;v=*input;
 if(v.type>17||v.index>=2048||(v.hasYaw!=0&&v.hasYaw!=1)||
    (!!v.script.cache!=!!v.script.entry)||(v.type!=16&&(v.script.cache||v.script.entry)))return 0;
 for(i=0;i<3;i++)if(!isfinite(v.origin[i]))return 0;
 if(v.hasYaw&&!C1NP_Yaw(v.yaw,&yaw))return 0;
 memcpy(&yawBits,&yaw,4);flags=(uint16_t)v.flags;
 if(yawBits==UINT32_C(0xbf800000))yawBits=0;else flags|=0x8000;
 flags&=(uint16_t)~masks[v.type];type=v.type;
 if(type==16){flags|=2;if(!v.animscript||!v.script.cache)type=0;}
 else if(type==17)flags|=2;
 if((flags&0x1c)==0x1c)flags&=(uint16_t)~0x1c;
 C1NP_Dword(staged+0x24,type);C1NP_Word(staged+0x28,(uint16_t)v.index);C1NP_Word(staged+0x2a,flags);
 C1NP_Word(staged+0x2c,v.targetname);C1NP_Word(staged+0x2e,v.noteworthy);C1NP_Word(staged+0x30,v.target);C1NP_Word(staged+0x32,v.animscript);
 memcpy(staged+0x38,v.origin,12);C1NP_Dword(staged+0x44,yawBits);
 memcpy(out,staged,sizeof(staged));return 1;
}
/* Allocated by the registry export; no pointer into the registry arena. */
#define C1NP_TOKEN_CAPACITY 8192u
typedef struct {
 uint64_t registryEpoch,sessionId;
 unsigned count,tokenCount;
 unsigned char *records;
 GscNodeScriptRef *scripts;
 char (*tokens)[128];
} C1NPPrepared;
#endif
