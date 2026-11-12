/* R120 real dual PCM/MP3 ambience streams, separate from music slot zero.
 * Alias identity/media substitutions and transactional prepare-before-replace
 * are port policy. Targets use the recovered 0.8 scalar and stored-target rate
 * rules with an output-sample clock; x87/Miles bit equivalence is not claimed.
 * Script STOP represents the proven empty-lookup/NULL sink no-op. RESET alone
 * performs private lifecycle cleanup. */
#include "../game/bg_cod1_ambient.h"
typedef struct {
    fileHandle_t file;
    int draining;
    cod1AmbientPlan plan;
    char alias[COD1_AMBIENT_ALIAS_BYTES];
    float start_gain,target;
    double rate_per_ms;
    int64_t start_clock;
} cod1AmbientSoundTrack_t;
typedef struct {
    uint64_t revision,epoch,operation_id,incarnation;
    int clock,rate,mute,started;
    uint32_t server_id;
    int reserve[2];
    void *critical;
} cod1AmbientSoundLease_t;
static struct {
    uint64_t revision,epoch,floor,operation_id,incarnation;
    int current,clock_rate,acquired,slot[2];
    uint32_t server_id;
    int64_t last_clock;
    cod1AmbientSoundTrack_t track[2];
} s_cod1Ambient;
/* Busy survives reentrant resets. Resets may retire an in-flight operation,
 * but a nested play cannot install a resource under its obsolete owner. */
static int s_cod1AmbientBusy;
static void SCA_Advance(uint64_t *value) {if(*value!=UINT64_MAX)++*value;}
static int SCA_Reserved(int index) {
    return s_cod1Ambient.acquired &&
        (index==s_cod1Ambient.slot[0] || index==s_cod1Ambient.slot[1]);
}
static int SCA_Relative(int index) {return index==s_cod1Ambient.slot[0]?0:1;}
static int SCA_Owns(int index) {
    return SCA_Reserved(index) &&
        (s_cod1Ambient.track[SCA_Relative(index)].file || s_cod1Ambient.track[SCA_Relative(index)].draining) &&
        s_cod1Ambient.track[SCA_Relative(index)].file==streamingSounds[index].file;
}
static cod1AmbientSoundLease_t SCA_Lease(void) {
    cod1AmbientSoundLease_t lease;
    lease.revision=s_cod1Ambient.revision;lease.epoch=s_cod1Ambient.epoch;
    lease.operation_id=s_cod1Ambient.operation_id;lease.incarnation=s_cod1Ambient.incarnation;
    lease.clock=s_soundtime;lease.rate=dma.speed;lease.mute=snd.s_soundMute;
    lease.reserve[0]=s_cod1Ambient.acquired?s_cod1Ambient.slot[0]:0;
    lease.reserve[1]=s_cod1Ambient.acquired?s_cod1Ambient.slot[1]:0;
    lease.server_id=CL_Cod1AmbientServerId();
    lease.started=snd.s_soundStarted;lease.critical=crit;return lease;
}
static int SCA_Current(const cod1AmbientSoundLease_t *lease) {
    int i;
    if(!lease || !lease->revision || lease->revision==UINT64_MAX ||
       !lease->incarnation || lease->incarnation==UINT64_MAX ||
       lease->revision!=s_cod1Ambient.revision || lease->epoch!=s_cod1Ambient.epoch ||
       lease->operation_id!=s_cod1Ambient.operation_id || lease->incarnation!=s_cod1Ambient.incarnation ||
       lease->clock!=s_soundtime || lease->rate!=dma.speed || lease->mute!=snd.s_soundMute ||
       lease->started!=snd.s_soundStarted || lease->critical!=crit || cls.state!=CA_ACTIVE ||
       !CL_Cod1AmbientAuthorityReady() || lease->server_id!=CL_Cod1AmbientServerId() ||
       lease->server_id!=s_cod1Ambient.server_id ||
       (lease->epoch && CA_ModuleId(lease->epoch)!=lease->server_id))return 0;
    for(i=0;i<2;++i) {
        streamingSound_t *ss;
        if(!s_cod1Ambient.acquired) {
            if(lease->reserve[i] && streamingSounds[lease->reserve[i]].file)return 0;
            continue;
        }
        if(lease->reserve[i]!=s_cod1Ambient.slot[i])return 0;
        ss=&streamingSounds[s_cod1Ambient.slot[i]];
        if(ss->file!=s_cod1Ambient.track[i].file || (ss->file &&
           (ss->kill || ss->entnum!=-1 || ss->channel!=CHAN_LOCAL || ss->attenuation)))return 0;
    }
    return 1;
}
static float SCA_Gain(int relative) {
    cod1AmbientSoundTrack_t *track=&s_cod1Ambient.track[relative];double gain;
    if(!s_cod1Ambient.acquired || !SCA_Owns(s_cod1Ambient.slot[relative]))return 0.0f;
    gain=(double)track->start_gain;
    if((int64_t)s_soundtime>track->start_clock && s_cod1Ambient.clock_rate>0)
        gain+=track->rate_per_ms*((double)((int64_t)s_soundtime-track->start_clock)*
            1000.0/(double)s_cod1Ambient.clock_rate);
    if(track->rate_per_ms>0.0 && gain>(double)track->target)gain=track->target;
    if(track->rate_per_ms<=0.0 && gain<(double)track->target)gain=track->target;
    if(gain<0.0)gain=0.0;
    return (float)gain;
}
static void SCA_Ramp(int relative,float target,int milliseconds,int fresh) {
    cod1AmbientSoundTrack_t *track=&s_cod1Ambient.track[relative];
    float previous=track->target,current=fresh?0.0f:SCA_Gain(relative);
    track->start_clock=(int64_t)s_soundtime;track->target=target;
    if(!milliseconds) {track->start_gain=target;track->rate_per_ms=0.0;return;}
    track->start_gain=current;
    track->rate_per_ms=((double)target-(fresh?0.0:(double)previous))/(double)milliseconds;
}
static void SCA_CloseTrack(int relative,cod1AmbientSoundLease_t *lease) {
    cod1AmbientSoundTrack_t *track=&s_cod1Ambient.track[relative];
    streamingSound_t *ss;fileHandle_t file=track->file;int index;
    if(!file && !track->draining)return;
    index=s_cod1Ambient.slot[relative];ss=&streamingSounds[index];
    /* Detach before any platform callback. Locally acquired cleanup ownership
     * remains valid after reset, while no later write can touch a new stream. */
    memset(track,0,sizeof(*track));SCA_Advance(&s_cod1Ambient.revision);
    if(lease)lease->revision=s_cod1Ambient.revision;
    if(ss->file==file) {
        ALab_ResetStream(index);
        ss->file=0;ss->kill=0;ss->loop[0]=0;
        s_rawend[index]=0;s_rawpainted[index]=0;
        memset(&s_rawVolume[index],0,sizeof(s_rawVolume[0]));
        if(file) {
            if(numStreamingSounds>0)--numStreamingSounds;
            Sys_EndStreamedFile(file);FS_FCloseFile(file);
        }
    }
}
static void S_Cod1AmbientClear(void) {
    cod1AmbientSoundLease_t lease;int i,previous_busy=s_cod1AmbientBusy;
    s_cod1AmbientBusy=1;SCA_Advance(&s_cod1Ambient.revision);
    for(i=0;i<2;++i) {
        lease=SCA_Lease();SCA_CloseTrack(i,&lease);
        if(s_cod1Ambient.revision!=lease.revision) {s_cod1AmbientBusy=previous_busy;return;}
    }
    s_cod1Ambient.acquired=0;s_cod1Ambient.slot[0]=s_cod1Ambient.slot[1]=0;s_cod1Ambient.current=1;
    s_cod1AmbientBusy=previous_busy;
}
void S_Cod1AmbientModuleReset(void) {
    /* Trusted module boundary invalidates every in-flight import and permits
     * a freshly loaded game/cgame pair to reuse a low scene/incarnation. */
    void *lock=crit;int previous_busy=s_cod1AmbientBusy;
    s_cod1AmbientBusy=1;SCA_Advance(&s_cod1Ambient.revision);
    s_cod1Ambient.epoch=s_cod1Ambient.floor=s_cod1Ambient.operation_id=s_cod1Ambient.incarnation=0;
    s_cod1Ambient.server_id=CL_Cod1AmbientServerId();
    if(lock)Sys_EnterCriticalSection(lock);
    S_Cod1AmbientClear();
    if(lock)Sys_LeaveCriticalSection(lock);
    s_cod1AmbientBusy=previous_busy;
}
static int SCA_PCM(const unsigned char header[44],int length,wavinfo_t *info) {
    unsigned int rate,bytes;int channels,width;
    if(length<=44 || length>COD_MP3_MAX_PCM || memcmp(header,"RIFF",4) ||
       memcmp(header+8,"WAVEfmt ",8) || memcmp(header+36,"data",4) ||
       S_Cod1MusicU32(header+16)!=16 || header[20]!=1 || header[21])return 0;
    channels=header[22]|(header[23]<<8);width=(header[34]|(header[35]<<8))/8;
    rate=S_Cod1MusicU32(header+24);bytes=S_Cod1MusicU32(header+40);
    if((channels!=1 && channels!=2) || (width!=1 && width!=2) ||
       (header[34]!=8 && header[34]!=16) || header[35] || !rate || rate>192000 ||
       bytes!=(unsigned int)(length-44) || bytes%(unsigned int)(channels*width) ||
       S_Cod1MusicU32(header+4)!=(unsigned int)(length-8) ||
       (header[32]|(header[33]<<8))!=channels*width ||
       S_Cod1MusicU32(header+28)!=rate*(unsigned int)(channels*width))return 0;
    memset(info,0,sizeof(*info));info->format=WAV_FORMAT_PCM;info->channels=channels;
    info->rate=(int)rate;info->width=width;info->samples=(int)bytes/(channels*width);return 1;
}
static int SCA_Path(const char *source,char result[MAX_QPATH],const cod1AmbientSoundLease_t *lease) {
    void *input=NULL;unsigned char *wav,header[44];fileHandle_t file=0;
    int length,size,existing,got;unsigned int checksum;wavinfo_t info;
    length=(int)strlen(source);if(length>=MAX_QPATH || !SCA_Current(lease))return 0;
    Q_strncpyz(result,source,MAX_QPATH);
    if(length<4 || Q_stricmp(source+length-4,".mp3"))return 1;
    length=FS_ReadFile(source,NULL);
    if(!SCA_Current(lease) || length<=0 || length>COD_MP3_MAX_INPUT)return 0;
    got=FS_ReadFile(source,&input);
    if(!SCA_Current(lease) || got!=length || !input) {if(input)FS_FreeFile(input);return 0;}
    checksum=Com_BlockChecksum(input,length);
    Com_sprintf(result,MAX_QPATH,"cod1cache/mp3_v1_%08x_%x.wav",checksum,length);
    existing=FS_FOpenFileRead(result,&file,qtrue);got=0;
    if(file && SCA_Current(lease))got=FS_Read(header,44,file);
    if(file)FS_FCloseFile(file);
    if(!SCA_Current(lease)) {FS_FreeFile(input);return 0;}
    if(got==44 && SCA_PCM(header,existing,&info)) {FS_FreeFile(input);return SCA_Current(lease);}
    wav=CodMP3_Decode((const unsigned char *)input,length,&size);FS_FreeFile(input);
    if(!SCA_Current(lease) || !wav) {free(wav);return 0;}
    file=FS_FOpenFileWrite(result);
    if(!file || !SCA_Current(lease)) {if(file)FS_FCloseFile(file);free(wav);return 0;}
    existing=FS_Write(wav,size,file);FS_FCloseFile(file);free(wav);
    return SCA_Current(lease) && existing==size;
}
static int SCA_Prepare(const cod1AmbientPlayback *event,streamingSound_t *pending,
                       const cod1AmbientSoundLease_t *lease) {
    unsigned char header[44];char decoded[MAX_QPATH];fileHandle_t file=0;int length,got=0;
    memset(pending,0,sizeof(*pending));
    if(!SCA_Path(event->plan.path,decoded,lease) || !SCA_Current(lease))return 0;
    length=FS_FOpenFileRead(decoded,&file,qtrue);
    if(file && SCA_Current(lease))got=FS_Read(header,44,file);
    if(!file || !SCA_Current(lease) || got!=44 || !SCA_PCM(header,length,&pending->info)) {
        if(file)FS_FCloseFile(file);return 0;
    }
    pending->file=file;pending->samples=pending->info.samples;
    pending->entnum=-1;pending->channel=CHAN_LOCAL;pending->attenuation=0;
    Q_strncpyz(pending->name,decoded,sizeof(pending->name));
    if(event->plan.looping)Q_strncpyz(pending->loop,decoded,sizeof(pending->loop));
    pending->looped=qtrue;return 1;
}
static int SCA_Reset(const cod1AmbientPlayback *event) {
    cod1AmbientSoundLease_t lease;int i,previous_busy=s_cod1AmbientBusy;void *lock;uint32_t id;uint64_t floor;
    if(!event->client_incarnation || event->transitionMs || event->alias[0] ||
       event->plan.path[0] || event->plan.gain!=0.0f || event->plan.looping ||
       (!CA_Epoch(event->epoch) && event->epoch) || !CL_Cod1AmbientAuthorityReady())return 0;
    id=CL_Cod1AmbientServerId();
    if(event->epoch && CA_ModuleId(event->epoch)!=id)return 0;
    floor=id==s_cod1Ambient.server_id?s_cod1Ambient.floor:0;
    if(event->epoch && event->epoch<floor)return 0;
    if(event->client_incarnation<=s_cod1Ambient.incarnation)return 0;
    s_cod1AmbientBusy=1;SCA_Advance(&s_cod1Ambient.revision);
    s_cod1Ambient.server_id=id;s_cod1Ambient.floor=floor;
    s_cod1Ambient.epoch=event->epoch;s_cod1Ambient.incarnation=event->client_incarnation;
    s_cod1Ambient.operation_id=event->operation_id;
    if(event->epoch>s_cod1Ambient.floor)s_cod1Ambient.floor=event->epoch;
    s_cod1Ambient.current=1;s_cod1Ambient.clock_rate=dma.speed;
    s_cod1Ambient.last_clock=(int64_t)s_soundtime;
    lock=crit;lease=SCA_Lease();if(lock)Sys_EnterCriticalSection(lock);
    if(s_cod1Ambient.revision!=lease.revision) {
        if(lock)Sys_LeaveCriticalSection(lock);s_cod1AmbientBusy=previous_busy;return 0;
    }
    for(i=0;i<2;++i) {
        lease=SCA_Lease();SCA_CloseTrack(i,&lease);
        if(s_cod1Ambient.revision!=lease.revision) {
            if(lock)Sys_LeaveCriticalSection(lock);s_cod1AmbientBusy=previous_busy;return 0;
        }
    }
    s_cod1Ambient.acquired=0;s_cod1Ambient.slot[0]=s_cod1Ambient.slot[1]=0;
    if(lock)Sys_LeaveCriticalSection(lock);
    s_cod1AmbientBusy=previous_busy;return 1;
}
int S_Cod1ScriptAmbient(const struct cod1AmbientPlayback_s *input) {
    cod1AmbientPlayback event;cod1AmbientSoundLease_t lease;streamingSound_t pending;
    cod1AmbientSoundTrack_t *track;char checked[CW_VIEW_PATH];int relative,index,i,count,started=0,result=0;
    float target;
    if(!input)return 0;event=*input;
    if(event.kind==COD1_AMBIENT_RESET)return SCA_Reset(&event);
    if(s_cod1AmbientBusy || s_cod1Ambient.revision==UINT64_MAX ||
       s_cod1Ambient.incarnation==UINT64_MAX || event.client_incarnation==UINT64_MAX ||
       !snd.s_soundStarted || !crit || snd.s_soundMute ||
       cls.state!=CA_ACTIVE || dma.speed<=0 || s_soundtime<0 || event.transitionMs<0 ||
       event.client_time<0 || !CA_Epoch(event.epoch) || !CL_Cod1AmbientAuthorityReady() ||
       CA_ModuleId(event.epoch)!=CL_Cod1AmbientServerId() ||
       s_cod1Ambient.server_id!=CL_Cod1AmbientServerId() ||
       event.epoch!=s_cod1Ambient.epoch || !event.client_incarnation ||
       event.client_incarnation!=s_cod1Ambient.incarnation || !event.operation_id ||
       event.operation_id<=s_cod1Ambient.operation_id)return 0;
    if(event.kind==COD1_AMBIENT_STOP) {
        if(event.alias[0] || event.plan.path[0] || event.plan.gain!=0.0f || event.plan.looping)return 0;
        /* Native API's empty name resolves NULL and the engine returns before
         * touching either active ambience stream. This is accepted intent. */
        s_cod1Ambient.operation_id=event.operation_id;return 1;
    }
    if(event.kind!=COD1_AMBIENT_PLAY || !memchr(event.alias,0,sizeof(event.alias)) ||
       !Cod1MusicAliasToken(event.alias) || !memchr(event.plan.path,0,sizeof(event.plan.path)) ||
       !CW_ViewPath(checked,event.plan.path,"sound/",0) || strcmp(checked,event.plan.path) ||
       !CQ_Finite(event.plan.gain) || event.plan.gain<0.0f || event.plan.gain>4.0f ||
       (event.plan.looping!=0 && event.plan.looping!=1))return 0;
    if((int64_t)s_soundtime<s_cod1Ambient.last_clock || s_cod1Ambient.clock_rate!=dma.speed)return 0;
    s_cod1AmbientBusy=1;s_cod1Ambient.operation_id=event.operation_id;lease=SCA_Lease();
    if(!s_cod1Ambient.acquired) {
        count=0;
        for(i=1;i<MAX_STREAMING_SOUNDS && count<2;++i)
            if(!streamingSounds[i].file)lease.reserve[count++]=i;
        if(count!=2)goto done;
    }
    if(!SCA_Current(&lease))goto done;
    relative=s_cod1Ambient.current;track=&s_cod1Ambient.track[relative];
    if((track->file || track->draining) && CW_AliasEqual(track->alias,event.alias)) {result=1;goto done;}
    target=event.plan.gain*0.8f;
    if((track->file || track->draining) && !strcmp(track->plan.path,event.plan.path) && track->plan.looping==event.plan.looping) {
        SCA_Ramp(relative,target,event.transitionMs,0);track->plan=event.plan;
        memcpy(track->alias,event.alias,sizeof(track->alias));result=1;goto done;
    }
    if(!SCA_Prepare(&event,&pending,&lease))goto done;
    if(!SCA_Current(&lease)) {FS_FCloseFile(pending.file);goto done;}
    Sys_EnterCriticalSection(crit);
    if(!SCA_Current(&lease))goto cleanup_locked;
    if(track->file || track->draining) {
        if(!event.transitionMs)SCA_CloseTrack(relative,&lease);
        else if(track->target>0.0f)SCA_Ramp(relative,0.0f,event.transitionMs,0);
    }
    if(!SCA_Current(&lease))goto cleanup_locked;
    relative=1-relative;SCA_CloseTrack(relative,&lease);
    if(!SCA_Current(&lease))goto cleanup_locked;
    Sys_BeginStreamedFile(pending.file,0x10000);started=1;
    if(!SCA_Current(&lease))goto cleanup_locked;
    if(!s_cod1Ambient.acquired) {
        s_cod1Ambient.slot[0]=lease.reserve[0];s_cod1Ambient.slot[1]=lease.reserve[1];
        s_cod1Ambient.acquired=1;SCA_Advance(&s_cod1Ambient.revision);lease.revision=s_cod1Ambient.revision;
    }
    index=s_cod1Ambient.slot[relative];streamingSounds[index]=pending;++numStreamingSounds;
    track=&s_cod1Ambient.track[relative];memset(track,0,sizeof(*track));
    track->file=pending.file;track->plan=event.plan;memcpy(track->alias,event.alias,sizeof(track->alias));
    s_rawend[index]=0;s_rawpainted[index]=0;
    memset(&s_rawVolume[index],0,sizeof(s_rawVolume[0]));
    s_cod1Ambient.current=relative;SCA_Ramp(relative,target,event.transitionMs,1);
    s_cod1Ambient.last_clock=(int64_t)s_soundtime;result=1;
    Sys_LeaveCriticalSection(lease.critical);
    if(!SCA_Current(&lease))result=0;
    goto done;
cleanup_locked:
    if(started)Sys_EndStreamedFile(pending.file);
    FS_FCloseFile(pending.file);Sys_LeaveCriticalSection(lease.critical);
done:
    s_cod1AmbientBusy=0;return result;
}
static void SCA_RawGain(int index) {
    double volume;int scaled;
    if(!SCA_Owns(index) || !s_volume || !s_mute)return;
    volume=(double)SCA_Gain(SCA_Relative(index))*(double)s_volume->value*(double)snd.volCurrent;
    if(s_mute->value || !(volume>=0.0 && volume<=(double)INT_MAX/256.0))volume=0.0;
    scaled=(int)(volume*256.0);s_rawVolume[index].left=s_rawVolume[index].right=scaled;
}
static int SCA_Tick(int index) {
    cod1AmbientSoundTrack_t *track;streamingSound_t *ss;fileHandle_t file;
    if(!SCA_Owns(index))return 0;
    track=&s_cod1Ambient.track[SCA_Relative(index)];
    ss=&streamingSounds[index];
    if(ss->kill==2 || (track->rate_per_ms<0.0 && track->target==0.0f && SCA_Gain(SCA_Relative(index))<=0.0f) ||
       (track->draining && s_rawend[index]<=s_soundtime)) {
        SCA_CloseTrack(SCA_Relative(index),NULL);return 1;
    }
    if(ss->kill==1 && track->file) {
        /* Natural EOF/read retirement retains the already queued PCM and its
         * gain until it drains. Explicit fades and lifecycle resets clear it. */
        file=track->file;track->file=0;track->draining=1;SCA_Advance(&s_cod1Ambient.revision);
        ss->file=0;ss->kill=0;ss->loop[0]=0;if(numStreamingSounds>0)--numStreamingSounds;
        Sys_EndStreamedFile(file);FS_FCloseFile(file);return 1;
    }
    SCA_RawGain(index);return 0;
}
static int SCA_Rewind(int index,const cod1AmbientSoundLease_t *lease) {
    streamingSound_t *ss=&streamingSounds[index];
    if(!SCA_Owns(index) || !SCA_Current(lease))return 0;
    Sys_StreamSeek(ss->file,44,FS_SEEK_SET);
    if(!SCA_Current(lease))return 0;
    ss->samples=ss->info.samples;ss->looped=qtrue;return 1;
}
