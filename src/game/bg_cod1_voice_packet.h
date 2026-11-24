#ifndef BG_COD1_VOICE_PACKET_H
#define BG_COD1_VOICE_PACKET_H
#include "bg_cod1_voice_stream.h"
#define COD1_LOADED_CAPTION_COMMAND "c1j"
#define C1V_ARGC (C1N_ARGC+3)
#define C1V_CAPACITY (C1N_CAPACITY+COD1_SS_TEXT_BYTES+20)
typedef struct {C1NPacket sound;char subtitle[COD1_SS_TEXT_BYTES];unsigned mix_mode;float slave_cap;} C1VPacket;
static int C1V_Subtitle(const char *s){return Cod1SS_Token(s,COD1_SS_TEXT_BYTES)&&!strncmp(s,"SUBTITLE_",9)&&s[9];}
static int C1V_MixValid(unsigned mode,float cap){return mode<=2&&CQ_Finite(cap)&&cap>=0&&(mode==2||cap==0);}
static int C1V_EncodePacket(char *out,size_t cap,const C1VPacket *p){char text[C1V_CAPACITY];size_t n,k;if(!out||!p||!C1V_MixValid(p->mix_mode,p->slave_cap)||p->sound.sound.channel!=3||(p->subtitle[0]&&!C1V_Subtitle(p->subtitle))||!C1N_EncodePacket(text,sizeof(text),&p->sound))return 0;text[2]='v';n=strlen(text);k=p->subtitle[0]?strlen(p->subtitle):1;if(n+1+k+18>=cap)return 0;text[n++]=' ';memcpy(text+n,p->subtitle[0]?p->subtitle:"-",k);n+=k;text[n++]=' ';CQ_WriteHex(text+n,8,p->mix_mode);n+=8;text[n++]=' ';CQ_WriteHex(text+n,8,CQ_FloatBits(p->slave_cap));n+=8;text[n]=0;memcpy(out,text,n+1);return 1;}
static int C1V_DecodePacket(int argc,const char *const *argv,C1VPacket *out){const char *a[C1N_ARGC];C1VPacket p;uint64_t mode,bits;int i;if(!argv||!out||argc!=C1V_ARGC||!argv[0]||strcmp(argv[0],"c1v")||(strcmp(argv[C1N_ARGC],"-")&&!C1V_Subtitle(argv[C1N_ARGC])))return 0;for(i=0;i<C1N_ARGC;i++)a[i]=argv[i];a[0]="c1n";memset(&p,0,sizeof(p));if(!C1N_DecodePacket(C1N_ARGC,a,&p.sound)||p.sound.sound.channel!=3||!CQ_ParseHex(argv[C1N_ARGC+1],8,&mode)||!CQ_ParseHex(argv[C1N_ARGC+2],8,&bits))return 0;p.mix_mode=(unsigned)mode;p.slave_cap=CQ_FloatFromBits((uint32_t)bits);if(!C1V_MixValid(p.mix_mode,p.slave_cap))return 0;if(strcmp(argv[C1N_ARGC],"-"))strcpy(p.subtitle,argv[C1N_ARGC]);*out=p;return 1;}
/* Loaded speech uses the same owner and localized token fields, with no stream mix mode. */
static int C1V_EncodeLoadedCaption(char *out,size_t cap,const C1VPacket *p){
 if(!p || !C1V_Subtitle(p->subtitle) || p->mix_mode || !C1V_EncodePacket(out,cap,p))return 0;
 out[2]=COD1_LOADED_CAPTION_COMMAND[2];
 return 1;
}
static int C1V_DecodeLoadedCaption(int argc,const char *const *argv,C1VPacket *out){
 const char *fields[C1V_ARGC];C1VPacket packet;int i;
 if(!out || !argv || argc!=C1V_ARGC || !argv[0] || strcmp(argv[0],COD1_LOADED_CAPTION_COMMAND))return 0;
 for(i=1;i<argc;i++)fields[i]=argv[i];
 fields[0]="c1v";
 if(!C1V_DecodePacket(argc,fields,&packet) || !C1V_Subtitle(packet.subtitle) || packet.mix_mode)return 0;
 *out=packet;
 return 1;
}
/* c1f client drain/failure: duration zero cancels, positive confirms a real
 * drained stream. c1x server stop: zero duration, matching full owner only. */
static int C1V_ControlEncode(char *out,size_t cap,const C1DReport *r,char command){if(command!='f'&&command!='x')return 0;if(!C1D_Encode(out,cap,r))return 0;out[2]=command;return 1;}
static int C1V_ControlDecode(int argc,const char *const *argv,C1DReport *out,char command){const char *a[C1D_ARGC];char name[4]={'c','1',0,0};int i;if(!argv||!out||argc!=C1D_ARGC||(command!='f'&&command!='x'))return 0;name[2]=command;if(!argv[0]||strcmp(argv[0],name))return 0;for(i=0;i<C1D_ARGC;i++)a[i]=argv[i];a[0]="c1d";return C1D_Decode(argc,a,out);}
#endif
