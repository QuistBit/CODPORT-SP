/* Owned PCM loops. Retail443010 refresh preserves the stream;4443D0 retires
 * unrefreshed handles. This is a port implementation, not Miles/attenuation
 * numeric parity. It emits no duration, completion notify or one-shot event. */
#include "../game/bg_cod1_loop_sound.h"
#include "snd_cod1_stream_wav.h"
#include <limits.h>
#if RAND_MAX != 32767
#error Native loop rand contract requires 15-bit CRT rand
#endif
typedef struct {int active,begun,effectiveRate,outputRate,lastClock;float gain;fileHandle_t file;C1VPCM pcm;cod1LoopStreamRequest request;} SCLTrack;
static SCLTrack sclTracks[MAX_STREAMING_SOUNDS];
static uint64_t sclRevision=1,sclIncarnation,sclEpoch,sclEndedFrame,sclLastSequence[1024];
static uint32_t sclServer;static int sclBusy;
static C1LPacket sclDeferred[1024];static uint64_t sclDeferredFrame[1024];
typedef struct {uint64_t revision,incarnation,epoch;uint32_t server;int clock,rate,mute,started;void*lock;fileHandle_t file;uint32_t position;vec3_t listener;} SCLLease;
static uint64_t SCL_Epoch(void){uint64_t e=0;int at=cl.gameState.stringOffsets[CS_COD1_SCRIPT_EFFECTS];if(at<=0||at>(int)sizeof(cl.gameState.stringData)-COD1_SCRIPT_QUAKE_METADATA_CAPACITY)return 0;return CQ_DecodeMetadata(cl.gameState.stringData+at,&e)?e:0;}
static void SCL_Advance(void){if(sclRevision!=UINT64_MAX)++sclRevision;}
static int SCL_Reserved(int i){return i>0&&i<MAX_STREAMING_SOUNDS&&sclTracks[i].active;}
static SCLLease SCL_Lease(void){SCLLease l;memset(&l,0,sizeof(l));l.revision=sclRevision;l.incarnation=sclIncarnation;l.epoch=SCL_Epoch();l.server=CL_Cod1AmbientServerId();l.clock=s_soundtime;l.rate=dma.speed;l.mute=snd.s_soundMute;l.started=snd.s_soundStarted;l.lock=crit;memcpy(l.listener,listener_origin,sizeof(l.listener));return l;}
static int SCL_Current(void*u){SCLLease*l=u;return l&&l->revision==sclRevision&&sclRevision!=UINT64_MAX&&l->incarnation==sclIncarnation&&l->epoch&&l->epoch==sclEpoch&&l->epoch==SCL_Epoch()&&l->server==sclServer&&l->server==CL_Cod1AmbientServerId()&&l->clock==s_soundtime&&l->rate==dma.speed&&l->mute==snd.s_soundMute&&l->started==snd.s_soundStarted&&l->lock==crit&&!memcmp(l->listener,listener_origin,sizeof(l->listener))&&crit&&snd.s_soundStarted&&cls.state==CA_ACTIVE&&CL_Cod1AmbientAuthorityReady();}
static int SCL_Position(SCLLease*l,uint32_t at){unsigned char discard[8192];uint32_t n;if(!SCL_Current(l)||at>INT_MAX)return 0;if(at<l->position){if(FS_Seek(l->file,0,FS_SEEK_SET)<0||!SCL_Current(l))return 0;l->position=0;}while(l->position<at){n=at-l->position;if(n>sizeof(discard))n=sizeof(discard);if(FS_Read(discard,(int)n,l->file)!=(int)n||!SCL_Current(l))return 0;l->position+=n;}return 1;}
static int SCL_ReadAt(void*u,uint32_t at,void*out,size_t n){SCLLease*l=u;if(n>INT_MAX||!SCL_Position(l,at))return 0;if(FS_Read(out,(int)n,l->file)!=(int)n||!SCL_Current(l))return 0;l->position+=(uint32_t)n;return 1;}
static int SCL_Close(int i){SCLTrack old;streamingSound_t*ss;uint64_t revision;int exact;
 if(!SCL_Reserved(i))return 1;old=sclTracks[i];ss=&streamingSounds[i];exact=ss->file==old.file;
 memset(&sclTracks[i],0,sizeof(sclTracks[i]));SCL_Advance();revision=sclRevision;
 if(exact){ALab_ResetStream(i);memset(ss,0,sizeof(*ss));s_rawend[i]=s_rawpainted[i]=0;memset(s_rawVolume+i,0,sizeof(s_rawVolume[i]));if(numStreamingSounds>0)--numStreamingSounds;}
 if(old.file){if(old.begun)Sys_EndStreamedFile(old.file);FS_FCloseFile(old.file);}
 return exact&&revision==sclRevision&&revision!=UINT64_MAX;
}
static int SCL_Clear(void){int i,ok=1;SCL_Advance();memset(sclDeferredFrame,0,sizeof(sclDeferredFrame));for(i=1;i<MAX_STREAMING_SOUNDS;i++)if(!SCL_Close(i))ok=0;return ok&&sclRevision!=UINT64_MAX;}
static void S_Cod1LoopClear(void){int old=sclBusy;sclBusy=1;SCL_Clear();sclBusy=old;}
void S_Cod1LoopModuleReset(void){void*lock=crit;int old=sclBusy;uint64_t op;sclBusy=1;SCL_Advance();op=sclRevision;if(lock)Sys_EnterCriticalSection(lock);if(op==sclRevision&&SCL_Clear()){sclIncarnation=sclEpoch=sclEndedFrame=0;sclServer=0;memset(sclLastSequence,0,sizeof(sclLastSequence));}if(lock)Sys_LeaveCriticalSection(lock);sclBusy=old;}
static int SCL_SameOwner(const C1LPacket*a,const C1LPacket*b){return a->sound.epoch==b->sound.epoch&&a->entity_epoch==b->entity_epoch&&a->generation==b->generation&&a->sound.entity==b->sound.entity;}
int S_Cod1LoopStream(const struct cod1LoopStreamRequest_s*input){
 cod1LoopStreamRequest r;SCLLease l;C1VFile io;C1VPCM pcm;streamingSound_t pending;SCLTrack*t;
 fileHandle_t file=0;void*lock;int i,slot=0,replace=0,length,begun=0,ok=0,rate;uint64_t after;float gain,pitch;
 if(!input)return 0;r=*input;if(!C1L_RequestValid(&r)||!crit||!snd.s_soundStarted||cls.state!=CA_ACTIVE||!CL_Cod1AmbientAuthorityReady())return 0;
 if(r.operation==C1L_RESET){
  int old=sclBusy;uint64_t op;SCLLease before=SCL_Lease();
  if(r.incarnation<=sclIncarnation||r.packet.sound.epoch!=before.epoch)return 0;
  sclBusy=1;SCL_Advance();op=sclRevision;lock=crit;Sys_EnterCriticalSection(lock);
  if(op==sclRevision&&before.server==CL_Cod1AmbientServerId()&&before.epoch==SCL_Epoch()&&SCL_Clear()){
   /* Clear invokes file services; validate the captured scene again before
    * installing its new incarnation. Revision alone is not scene authority. */
   sclIncarnation=r.incarnation;sclEpoch=r.packet.sound.epoch;sclServer=before.server;sclEndedFrame=0;memset(sclLastSequence,0,sizeof(sclLastSequence));
   before.incarnation=sclIncarnation;before.revision=sclRevision;ok=SCL_Current(&before);
  }
  op=sclRevision;Sys_LeaveCriticalSection(lock);if(op!=sclRevision||!SCL_Current(&before))ok=0;sclBusy=old;return ok;
 }
 if(sclBusy||r.incarnation!=sclIncarnation||r.packet.sound.epoch!=sclEpoch||sclEpoch!=SCL_Epoch()||sclServer!=CL_Cod1AmbientServerId())return 0;
 sclBusy=1;lock=crit;l=SCL_Lease();Sys_EnterCriticalSection(lock);if(!SCL_Current(&l))goto done;
 if(r.operation==C1L_END_FRAME){if(r.frame<=sclEndedFrame)goto done;for(i=1;i<MAX_STREAMING_SOUNDS;i++)if(SCL_Reserved(i)&&sclTracks[i].request.frame!=r.frame){after=sclRevision;if(!SCL_Close(i)||sclRevision!=after+1)goto done;l.revision=sclRevision;}if(SCL_Current(&l)){for(i=0;i<1024;i++)if(sclDeferredFrame[i]!=r.frame)sclDeferredFrame[i]=0;sclEndedFrame=r.frame;ok=1;}goto done;}
 for(i=1;i<MAX_STREAMING_SOUNDS;i++)if(SCL_Reserved(i)&&sclTracks[i].request.packet.sound.entity==r.packet.sound.entity){replace=i;break;}
 if(r.operation==C1L_STOP){if(!replace){i=r.packet.sound.entity;if(sclDeferredFrame[i]&&SCL_SameOwner(&sclDeferred[i],&r.packet)&&sclDeferred[i].sound.sequence==r.packet.sound.sequence){sclDeferredFrame[i]=0;ok=1;}goto done;}if(!SCL_SameOwner(&sclTracks[replace].request.packet,&r.packet)||sclTracks[replace].request.packet.sound.sequence!=r.packet.sound.sequence)goto done;ok=SCL_Close(replace);if(ok)l.revision=sclRevision;sclDeferredFrame[r.packet.sound.entity]=0;goto done;}
 if(snd.s_soundMute||dma.speed<=0||r.frame<=sclEndedFrame)goto done;
 if(replace){t=&sclTracks[replace];if(r.packet.sound.sequence<t->request.packet.sound.sequence)goto done;
  if(r.packet.sound.sequence==t->request.packet.sound.sequence&&memcmp(&r.packet,&t->request.packet,sizeof(r.packet)))goto done;
 }else if(r.packet.sound.sequence<=sclLastSequence[r.packet.sound.entity]&&
  !(sclDeferredFrame[r.packet.sound.entity]&&r.packet.sound.sequence==sclLastSequence[r.packet.sound.entity]&&!memcmp(&r.packet,&sclDeferred[r.packet.sound.entity],sizeof(r.packet))))goto done;
 if(r.packet.native_parameters&&!C1L_WithinDistance(l.listener,r.packet.sound.origin,r.packet.sound.dist_max)){
  if(replace){after=sclRevision;if(!SCL_Close(replace)||sclRevision!=after+1)goto done;l.revision=sclRevision;}
  if(!SCL_Current(&l))goto done;i=r.packet.sound.entity;sclDeferred[i]=r.packet;sclDeferredFrame[i]=r.frame;sclLastSequence[i]=r.packet.sound.sequence;ok=C1L_OUT_OF_RANGE;goto done;
 }
 if(replace){t=&sclTracks[replace];
  if(SCL_SameOwner(&r.packet,&t->request.packet)&&!strcmp(r.packet.alias,t->request.packet.alias)&&!strcmp(r.packet.sound.path,t->request.packet.sound.path)){
   if(streamingSounds[replace].file!=t->file||t->outputRate!=dma.speed||s_soundtime<t->lastClock)goto done;
   /* Original refresh443010 uses min-volume blend and retains the initial
    * pitch/sample cursor; fresh random pitch belongs only to creation. */
   t->request=r;t->gain=r.packet.refresh_gain;sclLastSequence[r.packet.sound.entity]=r.packet.sound.sequence;ok=1;goto done;
  }
 }
 gain=r.packet.sound.gain;pitch=r.packet.sound.pitch;
 if(r.packet.native_parameters){C1LParameters p;unsigned g=(unsigned)rand(),v=(unsigned)rand();
  /* Shared client-thread CRT state: never seed/reset it per owner/frame/map.
   * Native443440 draws only after in-range refresh miss, before slot/open. */
  if(!C1L_ResolveDraws(&r.packet.range,&r.packet.range,0,g,v,&p))goto done;gain=p.gain;pitch=p.pitch;
 }
 slot=replace;for(i=MAX_STREAMING_SOUNDS-1;!slot&&i>0;i--)if(!SCA_Reserved(i)&&!SCV_Reserved(i)&&!SCL_Reserved(i)&&!streamingSounds[i].file)slot=i;if(!slot)goto done;
 length=FS_FOpenFileRead(r.packet.sound.path,&file,qtrue);l.file=file;if(!file||length<44||!SCL_Current(&l))goto done;
 io.user=&l;io.current=SCL_Current;io.read_at=SCL_ReadAt;if(!C1V_Inspect(&io,(uint32_t)length,&pcm)||!SCL_Current(&l))goto done;
 rate=(int)((double)pcm.rate*pitch+.5);if(rate<1000||rate>768000||!SCL_Position(&l,pcm.offset)||!SCL_Current(&l))goto done;
 memset(&pending,0,sizeof(pending));pending.file=file;pending.info.format=1;pending.info.channels=pcm.channels;pending.info.width=pcm.width;pending.info.rate=pcm.rate;pending.info.samples=pcm.frames;pending.samples=pcm.frames;pending.entnum=r.packet.sound.entity;pending.channel=CHAN_AUTO;Q_strncpyz(pending.name,r.packet.sound.path,sizeof(pending.name));
 begun=1;Sys_BeginStreamedFile(file,0x10000);if(!SCL_Current(&l))goto done;
 if(replace){after=sclRevision;if(!SCL_Close(replace)||sclRevision!=after+1)goto done;l.revision=sclRevision;}
 if(!SCL_Current(&l)||SCA_Reserved(slot)||SCV_Reserved(slot)||SCL_Reserved(slot)||streamingSounds[slot].file)goto done;
 streamingSounds[slot]=pending;s_rawend[slot]=s_soundtime;s_rawpainted[slot]=0;
 t=sclTracks+slot;memset(t,0,sizeof(*t));t->active=t->begun=1;t->file=file;t->pcm=pcm;t->request=r;t->gain=gain;t->effectiveRate=rate;t->outputRate=dma.speed;t->lastClock=s_soundtime;sclLastSequence[r.packet.sound.entity]=r.packet.sound.sequence;sclDeferredFrame[r.packet.sound.entity]=0;++numStreamingSounds;file=0;begun=0;ok=1;
 done:if(file){if(begun)Sys_EndStreamedFile(file);FS_FCloseFile(file);}after=sclRevision;Sys_LeaveCriticalSection(lock);if(after!=sclRevision||!SCL_Current(&l))ok=0;sclBusy=0;return ok;
}
static void SCL_Gains(const SCLTrack*t,float*left,float*right){const Cod1SoundPacket*p=&t->request.packet.sound;int l,r;vec3_t origin;float gain=t->gain*s_volume->value*snd.volCurrent,range=S_Cod1AliasRtcwRange(p->dist_min,p->dist_max,SOUND_RANGE_DEFAULT);memcpy(origin,p->origin,sizeof(origin));S_SpatializeOrigin(origin,255,&l,&r,range);if(s_mute->value)gain=0;*left=l/255.f*gain;*right=r/255.f*gain;}
static int SCL_Tick(int i){
 SCLTrack*t;streamingSound_t*ss;SCLLease l;unsigned char raw[8192];int frames,bytes,got,oldEnd;float lv,rv;
 if(!SCL_Reserved(i))return 0;if(sclBusy)return 1;t=sclTracks+i;ss=streamingSounds+i;
 if(ss->file!=t->file||ss->kill||snd.s_soundMute||s_soundtime<0||s_soundtime>INT_MAX-MAX_RAW_SAMPLES||s_soundtime<t->lastClock||dma.speed!=t->outputRate||!snd.s_soundStarted||!CL_Cod1AmbientAuthorityReady()||cls.state!=CA_ACTIVE||sclEpoch!=SCL_Epoch()||sclServer!=CL_Cod1AmbientServerId()){SCL_Close(i);return 1;}
 t->lastClock=s_soundtime;SCL_Gains(t,&lv,&rv);s_rawVolume[i].left=(int)(256*lv);s_rawVolume[i].right=(int)(256*rv);
 sclBusy=1;l=SCL_Lease();if(s_rawend[i]<s_soundtime)s_rawend[i]=s_soundtime;
 while(s_rawend[i]<s_soundtime+MAX_RAW_SAMPLES){
  if(!ss->samples){
   /* Rewind only at genuine EOF. Large ZIP seeks are avoided: zero rewind,
    * bounded sequential skip to the validated PCM chunk, then streaming IO. */
   t->begun=0;Sys_EndStreamedFile(t->file);if(!SCL_Current(&l))break;
   l.file=t->file;l.position=t->pcm.offset+t->pcm.bytes;if(!SCL_Position(&l,t->pcm.offset)){if(SCL_Current(&l))SCL_Close(i);break;}
   t->begun=1;Sys_BeginStreamedFile(t->file,0x10000);if(!SCL_Current(&l))break;ss->samples=t->pcm.frames;
  }
  frames=(int)(((int64_t)(MAX_RAW_SAMPLES-(s_rawend[i]-s_soundtime))*t->effectiveRate)/dma.speed);if(frames<=0)break;
  if(frames>ss->samples)frames=ss->samples;if(frames>(int)sizeof(raw)/(ss->info.width*ss->info.channels))frames=sizeof(raw)/(ss->info.width*ss->info.channels);bytes=frames*ss->info.width*ss->info.channels;
  got=Sys_StreamedRead(raw,1,bytes,t->file);if(!SCL_Current(&l))break;if(got!=bytes){SCL_Close(i);break;}
  S_ByteSwapRawSamples(frames,ss->info.width,ss->info.channels,(short*)raw);SCL_Gains(t,&lv,&rv);oldEnd=s_rawend[i];
  S_RawSamples(frames,t->effectiveRate,ss->info.width,ss->info.channels,raw,lv,rv,i);if(!SCL_Current(&l))break;
  if(s_rawend[i]<=oldEnd){SCL_Close(i);break;}s_rawpainted[i]=qtrue;ss->samples-=frames;
 }
 sclBusy=0;return 1;
}
