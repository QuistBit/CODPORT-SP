/* R107 genuine PCM/MP3 streamer adapter. This state is private to music track
 * zero; legacy starts keep unit alias gain and their previous loop policy. */
#include "../game/bg_cod1_music.h"
static struct {
    int active,starting,looping;
    float gain,fadeGain;
    int64_t fadeStart,fadeEnd,lastClock;
    fileHandle_t file;
} s_cod1Music;
static void S_Cod1MusicClose(void){
    streamingSound_t *ss=&streamingSounds[0];
    Sys_EnterCriticalSection(crit);
    if(ss->file){Sys_EndStreamedFile(ss->file);FS_FCloseFile(ss->file);
        ss->file=0;if(numStreamingSounds>0)--numStreamingSounds;}
    ss->kill=0;ss->loop[0]=0;s_rawend[0]=0;
    memset(&s_cod1Music,0,sizeof(s_cod1Music));Cvar_Set("s_currentMusic","");
    Sys_LeaveCriticalSection(crit);
}
static float S_Cod1MusicGain(void) {
    int64_t now=(int64_t)s_soundtime;
    if(!s_cod1Music.active||streamingSounds[0].file!=s_cod1Music.file)return 1.0f;
    if(!s_cod1Music.fadeEnd)return s_cod1Music.gain;
    if(now>=s_cod1Music.fadeEnd)return 0;
    if(now<=s_cod1Music.fadeStart)return s_cod1Music.fadeGain;
    return s_cod1Music.fadeGain*(float)((double)(s_cod1Music.fadeEnd-now)/
                                      (double)(s_cod1Music.fadeEnd-s_cod1Music.fadeStart));
}
/* Existing streamer expects the canonical PCM header emitted by CodMP3.
 * Validate this supported subset before any current-channel mutation. */
static unsigned int S_Cod1MusicU32(const unsigned char *p){
    return (unsigned int)p[0]|((unsigned int)p[1]<<8)|((unsigned int)p[2]<<16)|((unsigned int)p[3]<<24);
}
static int S_Cod1MusicMedia(const char *path){
    unsigned char header[44];fileHandle_t file=0;int length,got,channels,width;unsigned int rate,bytes;
    length=FS_FOpenFileRead(path,&file,qtrue);if(!file)return 0;
    got=FS_Read(header,44,file);FS_FCloseFile(file);
    if(got!=44||length<=44||length>COD_MP3_MAX_PCM||memcmp(header,"RIFF",4)||
       memcmp(header+8,"WAVEfmt ",8)||memcmp(header+36,"data",4)||
       S_Cod1MusicU32(header+16)!=16||header[20]!=1||header[21])return 0;
    channels=header[22]|(header[23]<<8);width=(header[34]|(header[35]<<8))/8;
    rate=S_Cod1MusicU32(header+24);bytes=S_Cod1MusicU32(header+40);
    return (channels==1||channels==2)&&(width==1||width==2)&&
        (header[34]==8||header[34]==16)&&!header[35]&&rate>0&&rate<=192000&&
        bytes==(unsigned int)(length-44)&&bytes%(unsigned int)(channels*width)==0&&
        S_Cod1MusicU32(header+4)==(unsigned int)(length-8)&&
        (header[32]|(header[33]<<8))==channels*width&&
        S_Cod1MusicU32(header+28)==rate*(unsigned int)(channels*width);
}
int S_Cod1ScriptMusic(const struct cod1MusicPlayback_s *input){
    cod1MusicPlayback event;char decoded[64],checked[64];streamingSound_t *ss=&streamingSounds[0];
    int64_t samples;
    if(!input||!snd.s_soundStarted||!crit||dma.speed<=0||s_soundtime<0)return 0;
    event=*input;
    if(event.kind==COD1_MUSIC_PLAY){
        if(event.transitionMs||!memchr(event.plan.path,0,sizeof(event.plan.path))||
           !CW_ViewPath(checked,event.plan.path,"sound/",0)||strcmp(checked,event.plan.path)||
           !isfinite(event.plan.gain)||event.plan.gain<0||event.plan.gain>4||
           (event.plan.looping!=0&&event.plan.looping!=1))return 0;
        /* Original music channel 32 ignores a new request while still busy. */
        if(ss->file&&!ss->kill)return 2;
        if(!S_Cod1MusicPath(event.plan.path,decoded,sizeof(decoded))||!S_Cod1MusicMedia(decoded))return 0;
        s_cod1Music.starting=1;s_cod1Music.looping=event.plan.looping;
        S_StartBackgroundTrack(decoded,decoded,0);s_cod1Music.starting=0;
        if(!ss->file)return 0;
        s_cod1Music.active=1;s_cod1Music.file=ss->file;s_cod1Music.gain=event.plan.gain*0.8f;
        s_cod1Music.fadeStart=s_cod1Music.fadeEnd=0;s_cod1Music.lastClock=(int64_t)s_soundtime;return 1;
    }
    if(event.kind!=COD1_MUSIC_STOP||event.transitionMs<0||event.plan.path[0]||
       event.plan.gain!=0||event.plan.looping)return 0;
    if(!ss->file)return 1;
    if(s_cod1Music.active&&(int64_t)s_soundtime<s_cod1Music.lastClock)return 0;
    if(!event.transitionMs){S_Cod1MusicClose();return 1;}
    /* A second positive original stop keeps the existing zero-target fade. */
    if(s_cod1Music.active&&ss->file==s_cod1Music.file&&s_cod1Music.fadeEnd)return 1;
    samples=((int64_t)dma.speed*(int64_t)event.transitionMs)/1000;
    if(samples<=0)samples=1;
    s_cod1Music.fadeGain=S_Cod1MusicGain();s_cod1Music.gain=s_cod1Music.fadeGain;
    s_cod1Music.active=1;s_cod1Music.file=ss->file;
    s_cod1Music.fadeStart=(int64_t)s_soundtime;s_cod1Music.fadeEnd=(int64_t)s_soundtime+samples;
    s_cod1Music.lastClock=(int64_t)s_soundtime;
    return 1;
}
