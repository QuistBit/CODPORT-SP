#ifndef BG_COD1_VEHICLE_RESOURCES14_H
#define BG_COD1_VEHICLE_RESOURCES14_H
#include "bg_cod1_vehicle14.h"
#include "g_cod1_script_sound_core.h"
/* GAME 3E290/3E4E0: preserve VFS order, parse <=32 definitions, then
 * register their six alias names in the shared 1..255 sound-index domain.
 * Registration does not play/resolve PCM or turn a missing alias into audio.
 * Malformed input is a failed transaction here, never a partially ready bank. */
typedef struct {unsigned count;C1V14Definition definitions[32];char aliases[256][128];} C1VR14Bank;
typedef struct {void*user;int(*current)(void*);
 int(*list)(void*,char*,size_t);int(*read)(void*,const char*,char**,size_t*);void(*release)(void*,char*);
} C1VR14Files;
typedef struct {const C1VR14Files*borrowed;C1VR14Files files;C1VR14Bank*bank;const char(*input)[128];char expected[256][128];} C1VR14Lease;
static int C1VR14_Current(void*user){C1VR14Lease*l=user;
 return !memcmp(l->borrowed,&l->files,sizeof(l->files))&&!memcmp(l->input,l->expected,sizeof(l->expected))&&l->files.current(l->files.user)==1&&
  !memcmp(l->borrowed,&l->files,sizeof(l->files))&&!memcmp(l->input,l->expected,sizeof(l->expected));}
static int C1VR14_Get(void*u,unsigned i,char*out,size_t cap){C1VR14Lease*l=u;
 if(!C1VR14_Current(l)||i<1||i>255||cap<128)return 0;memcpy(out,l->bank->aliases[i],128);return 1;}
static int C1VR14_Set(void*u,unsigned i,const char*name){C1VR14Lease*l=u;
 if(!C1VR14_Current(l)||i<1||i>255||!Cod1ScriptSoundText(name))return 0;strcpy(l->bank->aliases[i],name);return 1;}
static int C1VR14_Sound(void*u,const char*name,unsigned*out){Cod1ScriptSoundOps ops;memset(&ops,0,sizeof(ops));ops.user=u;ops.get_index=C1VR14_Get;ops.set_index=C1VR14_Set;
 return Cod1ScriptSoundIndex(&ops,name,out)==COD1_SS_OK;}
static int C1VR14_Load(const C1VR14Files*files,const char aliases[256][128],C1VR14Bank*scratch,C1VR14Bank*out){
 C1VR14Lease lease;C1V14SoundHost sound;C1V14Definition resolved;char list[4096],path[80],names[32][64];
 int count,i,ok;size_t at=0,n,bytes;char*data;
 if(!files||!aliases||!scratch||!out||!files->current||!files->list||!files->read||!files->release||
  C1V14_Overlap(files,sizeof(*files),scratch,sizeof(*scratch))||C1V14_Overlap(files,sizeof(*files),out,sizeof(*out))||
  C1V14_Overlap(aliases,256*128,scratch,sizeof(*scratch))||C1V14_Overlap(aliases,256*128,out,sizeof(*out))||C1V14_Overlap(scratch,sizeof(*scratch),out,sizeof(*out)))return 0;
 memset(&lease,0,sizeof(lease));lease.borrowed=files;lease.files=*files;lease.bank=scratch;lease.input=aliases;memcpy(lease.expected,aliases,sizeof(lease.expected));
 for(i=0;i<256;i++)if(!Cod1ScriptSoundText(aliases[i]))return 0;
 if(!C1VR14_Current(&lease))return 0;
 memset(scratch,0,sizeof(*scratch));memcpy(scratch->aliases,aliases,sizeof(scratch->aliases));memset(list,0xff,sizeof(list));
 count=lease.files.list(lease.files.user,list,sizeof(list));
 if(!C1VR14_Current(&lease)||count<0||count>32)return 0;
 for(i=0;i<count;i++){
  if(at>=sizeof(list)||!C1V14_Text(list+at,sizeof(list)-at,&n)||!n||n>=64||strchr(list+at,'/')||strchr(list+at,'\\')||strstr(list+at,".."))return 0;
  memcpy(names[i],list+at,n+1);at+=n+1;
 }
 for(i=0;i<count;i++){
  strcpy(path,"vehicles/");strcat(path,names[i]);data=NULL;bytes=0;
  if(!C1VR14_Current(&lease))return 0;
  ok=lease.files.read(lease.files.user,path,&data,&bytes);
  if(ok!=1||!data||!C1VR14_Current(&lease)){if(data)lease.files.release(lease.files.user,data);return 0;}
  ok=C1V14_Parse(names[i],data,bytes,scratch->definitions+i);
  lease.files.release(lease.files.user,data);
  if(!ok||!C1VR14_Current(&lease))return 0;
 }
 sound.user=&lease;sound.current=C1VR14_Current;sound.sound=C1VR14_Sound;
 for(i=0;i<count;i++){
  if(!C1V14_ResolveSounds(scratch->definitions+i,&resolved,&sound))return 0;
  scratch->definitions[i]=resolved;
 }
 if(!C1VR14_Current(&lease))return 0;
 scratch->count=(unsigned)count;*out=*scratch;return 1;
}
#endif
