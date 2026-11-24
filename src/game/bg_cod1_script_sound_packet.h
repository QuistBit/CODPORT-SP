#ifndef BG_COD1_SCRIPT_SOUND_PACKET_H
#define BG_COD1_SCRIPT_SOUND_PACKET_H
/* R194 paired SP transport. Float bits, bounded paths, map epoch and sequence;
 * this does not claim the retail snapshot layout or native channel numbering. */
#include "bg_cod1_script_quake.h"
#include "bg_cod1_script_sound.h"
#define C1S_ARGC 14
#define C1S_CAPACITY 512
/* Existing S_ThreadStartSoundExParams accepts alias gains through 2.0.
 * Preserve original CSV gains such as explo_rock volume1.5 -> gain1.2. */
#define C1S_GAIN_MAX 2.0f
#define C1S_ALIAS_GAIN 0.8f
typedef struct {
 uint64_t epoch,sequence;
 char path[COD1_SS_PATH_BYTES];
 uint32_t entity,channel;
 float origin[3],gain,pitch,dist_min,dist_max;
} Cod1SoundPacket;
static int C1S_Valid(const Cod1SoundPacket *s) {
 char path[COD1_SS_PATH_BYTES];int i;
 if(!s||!Cod1SS_Length(s->path,sizeof(s->path),NULL)||!s->epoch||!s->sequence||s->entity>=1024||s->channel>5||
    strncmp(s->path,"sound/",6)||!Cod1SS_Path(s->path+6,path)||strcmp(path,s->path))return 0;
 for(i=0;i<3;++i)if(!CQ_Finite(s->origin[i]))return 0;
 return CQ_Finite(s->gain)&&s->gain>=0&&s->gain<=C1S_GAIN_MAX&&CQ_Finite(s->pitch)&&s->pitch>0&&s->pitch<=4&&
  CQ_Finite(s->dist_min)&&s->dist_min>=0&&CQ_Finite(s->dist_max)&&(s->dist_max==0||s->dist_max>=s->dist_min);
}
static int C1S_Encode(char *out,size_t cap,const Cod1SoundPacket *s) {
 char b[C1S_CAPACITY];uint32_t f[9];size_t n,len;int i;
 if(!out||!C1S_Valid(s))return 0;
 memcpy(b,"c1s 1 ",6);CQ_WriteHex(b+6,16,s->epoch);b[22]=' ';CQ_WriteHex(b+23,16,s->sequence);
 n=39;b[n++]=' ';len=strlen(s->path);memcpy(b+n,s->path,len);n+=len;
 f[0]=s->entity;f[1]=s->channel;for(i=0;i<3;++i)f[2+i]=CQ_FloatBits(s->origin[i]);
 f[5]=CQ_FloatBits(s->gain);f[6]=CQ_FloatBits(s->pitch);f[7]=CQ_FloatBits(s->dist_min);f[8]=CQ_FloatBits(s->dist_max);
 for(i=0;i<9;++i){b[n++]=' ';CQ_WriteHex(b+n,8,f[i]);n+=8;}b[n]=0;
 if(cap<=n)return 0;memcpy(out,b,n+1);return 1;
}
static int C1S_Decode(int argc,const char *const *argv,Cod1SoundPacket *out) {
 Cod1SoundPacket s;uint64_t value;uint32_t f[9];int i;size_t n;
 if(!argv||!out||argc!=C1S_ARGC)return 0;
 for(i=0;i<argc;++i)if(!argv[i])return 0;
 if(strcmp(argv[0],"c1s")||strcmp(argv[1],"1")||!Cod1SS_Length(argv[4],sizeof(s.path),&n))return 0;
 memset(&s,0,sizeof(s));memcpy(s.path,argv[4],n+1);
 if(!CQ_ParseHex(argv[2],16,&s.epoch)||!CQ_ParseHex(argv[3],16,&s.sequence))return 0;
 for(i=0;i<9;++i){if(!CQ_ParseHex(argv[5+i],8,&value))return 0;f[i]=(uint32_t)value;}
 s.entity=f[0];s.channel=f[1];for(i=0;i<3;++i)s.origin[i]=CQ_FloatFromBits(f[2+i]);
 s.gain=CQ_FloatFromBits(f[5]);s.pitch=CQ_FloatFromBits(f[6]);s.dist_min=CQ_FloatFromBits(f[7]);s.dist_max=CQ_FloatFromBits(f[8]);
 if(!C1S_Valid(&s))return 0;*out=s;return 1;
}
#endif
