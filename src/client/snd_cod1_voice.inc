/* R230 actual owned PCM voice streams. No RegisterSound cache/fallback.
 * Dedicated slot ownership is maintained until raw output drains. Alias
 * attenuation uses the established port curve; native curve parity is not claimed. */
#include "../game/bg_cod1_voice_stream.h"
#include "snd_cod1_stream_wav.h"
#include <limits.h>
typedef struct {int active,draining,primed,effectiveRate,lastClock,outputRate;fileHandle_t file;cod1VoiceRequest request;} SCVTrack;
static SCVTrack scvTracks[MAX_STREAMING_SOUNDS];
typedef struct {C1NIdentity identity;int status;} SCVResult;
static SCVResult scvResults[1024];
static uint64_t scvRevision=1,scvIncarnation,scvEpoch,scvSequence;
static uint32_t scvModule;
/* GSC effects have their own scene epoch (often2); it is not the high32
 * module field used by the unrelated ambient event protocol. */
static uint64_t SCV_EffectsEpoch(void){uint64_t epoch=0;int at=cl.gameState.stringOffsets[CS_COD1_SCRIPT_EFFECTS];if(at<=0||at>(int)sizeof(cl.gameState.stringData)-COD1_SCRIPT_QUAKE_METADATA_CAPACITY)return 0;return CQ_DecodeMetadata(cl.gameState.stringData+at,&epoch)?epoch:0;}
static int scvBusy;
typedef struct {uint64_t revision,incarnation,epoch;uint32_t server;int clock,rate,mute,started;void *lock;fileHandle_t file;uint32_t position;} SCVLease;
static void SCV_Advance(void){if(scvRevision!=UINT64_MAX)++scvRevision;}
static int SCV_Reserved(int i){return i>0&&i<MAX_STREAMING_SOUNDS&&scvTracks[i].active;}
static SCVLease SCV_Lease(void){SCVLease l;l.revision=scvRevision;l.incarnation=scvIncarnation;l.server=CL_Cod1AmbientServerId();l.epoch=SCV_EffectsEpoch();l.clock=s_soundtime;l.rate=dma.speed;l.mute=snd.s_soundMute;l.started=snd.s_soundStarted;l.lock=crit;l.file=0;l.position=0;return l;}
static int SCV_Current(void *u){SCVLease *l=u;return l&&l->revision==scvRevision&&scvRevision!=UINT64_MAX&&l->incarnation==scvIncarnation&&l->server==CL_Cod1AmbientServerId()&&l->epoch&&l->epoch==SCV_EffectsEpoch()&&l->clock==s_soundtime&&l->rate==dma.speed&&l->mute==snd.s_soundMute&&l->started==snd.s_soundStarted&&l->lock==crit&&crit&&snd.s_soundStarted&&cls.state==CA_ACTIVE&&CL_Cod1AmbientAuthorityReady();}
/* The inherited ZIP backend aborts on nonzero seeks >=65536. Traverse the
 * original RIFF chunks through bounded sequential reads; only rewind at zero. */
static int SCV_Position(SCVLease *l,uint32_t at){unsigned char discard[8192];uint32_t n;if(!SCV_Current(l)||at>INT_MAX)return 0;if(at<l->position){if(FS_Seek(l->file,0,FS_SEEK_SET)<0||!SCV_Current(l))return 0;l->position=0;}while(l->position<at){n=at-l->position;if(n>sizeof(discard))n=sizeof(discard);if(FS_Read(discard,(int)n,l->file)!=(int)n||!SCV_Current(l))return 0;l->position+=n;}return 1;}
static int SCV_ReadAt(void *u,uint32_t at,void *out,size_t n){SCVLease *l=u;if(n>INT_MAX||!SCV_Position(l,at))return 0;if(FS_Read(out,(int)n,l->file)!=(int)n||!SCV_Current(l))return 0;l->position+=(uint32_t)n;return 1;}
static int SCV_Close(int i){fileHandle_t f;streamingSound_t *ss;uint64_t retired;if(!SCV_Reserved(i))return 1;ss=&streamingSounds[i];f=scvTracks[i].file;{SCVResult *result=&scvResults[scvTracks[i].request.packet.sound.entity];result->identity=C1N_PacketIdentity(&scvTracks[i].request.packet);result->status=-1;}memset(&scvTracks[i],0,sizeof(scvTracks[i]));SCV_Advance();retired=scvRevision;if(ss->file==f){ALab_ResetStream(i);memset(ss,0,sizeof(*ss));s_rawend[i]=s_rawpainted[i]=0;memset(&s_rawVolume[i],0,sizeof(s_rawVolume[i]));if(f&&numStreamingSounds>0)--numStreamingSounds;}if(f){Sys_EndStreamedFile(f);FS_FCloseFile(f);}return retired==scvRevision&&retired!=UINT64_MAX;}
static int SCV_ClearOwned(void){int i;SCV_Advance();for(i=1;i<MAX_STREAMING_SOUNDS;i++)if(!SCV_Close(i))return 0;return scvRevision!=UINT64_MAX;}
static void S_Cod1VoiceClear(void){int old=scvBusy;scvBusy=1;SCV_ClearOwned();scvBusy=old;}
void S_Cod1VoiceModuleReset(void){void *lock=crit;int old=scvBusy;uint64_t operation;scvBusy=1;SCV_Advance();operation=scvRevision;if(lock)Sys_EnterCriticalSection(lock);if(operation==scvRevision&&SCV_ClearOwned()){scvIncarnation=scvEpoch=scvSequence=0;scvModule=0;memset(scvResults,0,sizeof(scvResults));}if(lock)Sys_LeaveCriticalSection(lock);scvBusy=old;}
int S_Cod1VoiceStream(const struct cod1VoiceRequest_s *input){
 cod1VoiceRequest r;SCVLease lease;C1VFile io;C1VPCM pcm;streamingSound_t pending;fileHandle_t file=0;int i,index=0,replace=0,length,duration=-1,rate;uint64_t after;void *lock;
 if(!input)return -1;r=*input;if(!C1V_RequestValid(&r)||!crit||!snd.s_soundStarted||cls.state!=CA_ACTIVE||!CL_Cod1AmbientAuthorityReady())return -1;
 if(r.operation!=C1V_STREAM_RESET&&(r.incarnation!=scvIncarnation||r.packet.sound.epoch!=scvEpoch||(r.packet.sound.epoch!=SCV_EffectsEpoch()||scvModule!=CL_Cod1AmbientServerId())))return -1;
 if(r.operation==C1V_STREAM_RESET){int previous=scvBusy;uint64_t operation;if(r.incarnation<=scvIncarnation||r.packet.sound.epoch!=SCV_EffectsEpoch())return -1;lock=crit;scvBusy=1;SCV_Advance();operation=scvRevision;Sys_EnterCriticalSection(lock);if(operation==scvRevision&&SCV_ClearOwned()){scvIncarnation=r.incarnation;scvEpoch=r.packet.sound.epoch;scvModule=CL_Cod1AmbientServerId();scvSequence=0;memset(scvResults,0,sizeof(scvResults));duration=0;}operation=scvRevision;Sys_LeaveCriticalSection(lock);if(operation!=scvRevision)duration=-1;scvBusy=previous;return duration;}
 if(r.operation==C1V_STREAM_STATUS){C1NIdentity id=C1N_PacketIdentity(&r.packet);SCVResult result=scvResults[id.entity];return !memcmp(&id,&result.identity,sizeof(id))?result.status:-1;}
 if(scvBusy||(r.operation==C1V_STREAM_PLAY&&(snd.s_soundMute||dma.speed<=0)))return -1;
 scvBusy=1;lock=crit;lease=SCV_Lease();Sys_EnterCriticalSection(lock);if(!SCV_Current(&lease))goto done;
 for(i=1;i<MAX_STREAMING_SOUNDS;i++)if(SCV_Reserved(i)&&scvTracks[i].request.packet.sound.entity==r.packet.sound.entity){replace=i;break;}
 if(r.operation==C1V_STREAM_STOP){if(!replace||memcmp(&scvTracks[replace].request.packet,&r.packet,sizeof(r.packet)))goto done;SCV_Close(replace);duration=0;goto done;}
 if(r.packet.sound.sequence<=scvSequence)goto done;
 index=replace;for(i=MAX_STREAMING_SOUNDS-1;!index&&i>0;i--)if(!SCA_Reserved(i)&&!SCV_Reserved(i)&&!SCL_Reserved(i)&&!streamingSounds[i].file)index=i;
 if(!index)goto done;
 length=FS_FOpenFileRead(r.packet.sound.path,&file,qtrue);lease.file=file;if(!file||length<44||!SCV_Current(&lease))goto done;
 io.user=&lease;io.current=SCV_Current;io.read_at=SCV_ReadAt;if(!C1V_Inspect(&io,(uint32_t)length,&pcm)||!SCV_Current(&lease))goto done;
 rate=(int)((double)pcm.rate*r.packet.sound.pitch+0.5);duration=C1V_Duration(&pcm,rate);if(duration<=0||duration>C1N_MAX_DURATION_MS){duration=-1;goto done;}
 if(!SCV_Position(&lease,pcm.offset)||!SCV_Current(&lease)){duration=-1;goto done;}
 memset(&pending,0,sizeof(pending));pending.file=file;pending.info.format=1;pending.info.channels=pcm.channels;pending.info.width=pcm.width;pending.info.rate=pcm.rate;pending.info.samples=pcm.frames;pending.samples=pcm.frames;pending.entnum=r.packet.sound.entity;pending.channel=CHAN_VOICE;Q_strncpyz(pending.name,r.packet.sound.path,sizeof(pending.name));
 Sys_BeginStreamedFile(file,0x10000);if(!SCV_Current(&lease)){Sys_EndStreamedFile(file);duration=-1;goto done;}
 if(replace){after=scvRevision;SCV_Close(replace);if(scvRevision!=after+1){Sys_EndStreamedFile(file);duration=-1;goto done;}lease.revision=scvRevision;}
 if(!SCV_Current(&lease)){Sys_EndStreamedFile(file);duration=-1;goto done;}
 streamingSounds[index]=pending;s_rawend[index]=s_soundtime;s_rawpainted[index]=0;scvTracks[index].active=1;scvTracks[index].file=file;scvTracks[index].request=r;scvTracks[index].effectiveRate=rate;scvTracks[index].lastClock=s_soundtime;scvTracks[index].outputRate=dma.speed;scvSequence=r.packet.sound.sequence;file=0;++numStreamingSounds;scvResults[r.packet.sound.entity].identity=C1N_PacketIdentity(&r.packet);scvResults[r.packet.sound.entity].status=1;
 done:if(file)FS_FCloseFile(file);after=scvRevision;Sys_LeaveCriticalSection(lock);if(after!=scvRevision)duration=-1;scvBusy=0;return duration;
}
/* Native444B30 scans active/paused voices for alias master35; native444DCD
 * and444E6F cap only slave36 after distance gain, before channel/global gain.
 * Selected owned voices only; this does not admit loaded/ambient master rows. */
static int SCV_MasterActive(void){int i;for(i=1;i<MAX_STREAMING_SOUNDS;i++){SCVTrack *t=&scvTracks[i];if(SCV_Reserved(i)&&t->request.mix_mode==1&&streamingSounds[i].file==t->file&&!streamingSounds[i].kill&&(!t->draining||s_rawend[i]>s_soundtime)&&t->request.incarnation==scvIncarnation&&t->request.packet.sound.epoch==scvEpoch&&scvEpoch==SCV_EffectsEpoch()&&scvModule==CL_Cod1AmbientServerId())return 1;}return 0;}
/* Unpublished GSC actors have no CG_SetEntitySoundPosition update.
 * A voice packet carries the GAME owner's origin from this exact utterance.
 * Only use the interpolated entity position for a current snapshot owner. */
static const float *SCV_Origin(const SCVTrack *track) {
    const C1NPacket *packet = &track->request.packet;
    int first = 0, count = cl.snap.numEntities;
    if (!cl.snap.valid || count < 0 || count > MAX_ENTITIES_IN_SNAPSHOT ||
        cl.snap.parseEntitiesNum < 0 || cl.parseEntitiesNum < cl.snap.parseEntitiesNum ||
        cl.parseEntitiesNum - cl.snap.parseEntitiesNum >= MAX_PARSE_ENTITIES ||
        cl.parseEntitiesNum - cl.snap.parseEntitiesNum < count)
        return packet->sound.origin;
    /* Snapshot entities are sorted by entity number by the delta decoder. */
    while (first < count) {
        int mid = first + (count - first) / 2;
        const entityState_t *entity = &cl.parseEntities[(cl.snap.parseEntitiesNum + mid) & (MAX_PARSE_ENTITIES - 1)];
        if (entity->number < (int)packet->sound.entity)
            first = mid + 1;
        else if (entity->number > (int)packet->sound.entity)
            count = mid;
        else {
            if (entity->cod1TurretActorVersion &&
                (uint32_t)entity->cod1TurretActorGeneration != packet->generation)
                return packet->sound.origin;
            return snd.entityPositions[entity->number];
        }
    }
    return packet->sound.origin;
}
static void SCV_Gains(const SCVTrack *t,float *left,float *right){const Cod1SoundPacket *p=&t->request.packet.sound;float range,gain,atten,scale=1;vec3_t delta,origin;int l,r;
 memcpy(origin,SCV_Origin(t),sizeof(origin));
 range=S_Cod1AliasRtcwRange(p->dist_min,p->dist_max,SOUND_RANGE_DEFAULT);S_SpatializeOrigin(origin,255,&l,&r,range);
 gain=p->gain;
 if(t->request.mix_mode==2&&SCV_MasterActive()) {VectorSubtract(origin,listener_origin,delta);atten=VectorNormalize(delta)-range*.064f;if(atten<0)atten=0;atten=1-atten/range;if(atten<0)atten=0;
  if(gain*atten>t->request.slave_cap*C1S_ALIAS_GAIN&&gain*atten>0)scale=t->request.slave_cap*C1S_ALIAS_GAIN/(gain*atten);
 }
 gain*=scale*s_volume->value*snd.volCurrent;if(s_mute->value)gain=0;*left=(float)l/255*gain;*right=(float)r/255*gain;
}
static int SCV_Tick(int i){
 SCVTrack *t;streamingSound_t *ss;SCVLease lease;unsigned char raw[8192];int frames,bytes,got;float lv,rv;const Cod1SoundPacket *p;
 if(!SCV_Reserved(i))return 0;if(scvBusy)return 1;t=&scvTracks[i];ss=&streamingSounds[i];
 if(ss->file!=t->file||ss->kill||snd.s_soundMute==1||s_soundtime<t->lastClock||dma.speed!=t->outputRate||!snd.s_soundStarted||cls.state!=CA_ACTIVE||(t->request.packet.sound.epoch!=SCV_EffectsEpoch()||scvModule!=CL_Cod1AmbientServerId())){SCV_Close(i);return 1;}
 /* Already submitted device samples cannot contain a newly accepted voice. */
 if(!t->primed){
  s_rawend[i]=s_paintedtime>s_soundtime?s_paintedtime:s_soundtime;
  t->primed=1;
 }
 t->lastClock=s_soundtime;SCV_Gains(t,&lv,&rv);s_rawVolume[i].left=(int)(256*lv);s_rawVolume[i].right=(int)(256*rv);
 if(t->draining){if(s_rawend[i]<=s_soundtime){C1NIdentity id=C1N_PacketIdentity(&t->request.packet);if(SCV_Close(i))scvResults[id.entity].status=0;}return 1;}
 scvBusy=1;lease=SCV_Lease();p=&t->request.packet.sound;if(s_rawend[i]<s_soundtime)s_rawend[i]=s_soundtime;
 while(ss->samples>0&&s_rawend[i]<s_soundtime+MAX_RAW_SAMPLES){
  frames=(int)(((int64_t)(MAX_RAW_SAMPLES-(s_rawend[i]-s_soundtime))*t->effectiveRate)/dma.speed);if(frames<=0)break;
  if(frames>ss->samples)frames=ss->samples;if(frames>(int)sizeof(raw)/(ss->info.width*ss->info.channels))frames=sizeof(raw)/(ss->info.width*ss->info.channels);bytes=frames*ss->info.width*ss->info.channels;
  got=Sys_StreamedRead(raw,1,bytes,ss->file);if(!SCV_Current(&lease)){scvBusy=0;return 1;}if(got!=bytes){SCV_Close(i);break;}
  S_ByteSwapRawSamples(frames,ss->info.width,ss->info.channels,(short *)raw);
  SCV_Gains(t,&lv,&rv);
  S_RawSamples(frames,t->effectiveRate,ss->info.width,ss->info.channels,raw,lv,rv,i);if(!SCV_Current(&lease)){scvBusy=0;return 1;}
  s_rawpainted[i]=qtrue;ss->samples-=frames;if(!ss->samples)t->draining=1;
 }
 scvBusy=0;return 1;
}
