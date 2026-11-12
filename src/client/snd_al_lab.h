#ifndef COD1_AL_LAB_H
#define COD1_AL_LAB_H
#ifdef __cplusplus
extern "C" {
#endif
int ALab_Init(int rate,int hrtf,void (*log)(const char *));
void ALab_Shutdown(void);
int ALab_Active(void);
/* Clock ownership lasts through runtime fallback until sound shutdown. */
int ALab_MonotonicClock(void);
int ALab_LoadEal(const void *data,int bytes,const char *name);
void ALab_Listener(const float *position,const float *forward,const float *up,int effects);
int ALab_NeedsVoice(int slot,const void *key,int token);
void ALab_Voice(int slot,const void *key,int token,const short *pcm,int frames,int rate,int loop,const float *position,int relative,int wet,float gain,float pitch,int offset);
void ALab_StopVoice(int slot);
void ALab_Clear(int streams,int music);
void ALab_ResetStream(int index);
void ALab_BeginLoops(void);
int ALab_LoopSlot(int entity,const void *key);
void ALab_EndLoops(void);
void ALab_Stream(int index,int start,int end,const int *raw,int mask,int channels,const float *position,int relative,int wet,float gain);
void ALab_Render(int *stereoPairs,int frames);
void ALab_Skip(int frames);
const char *ALab_Info(void);
#ifdef __cplusplus
}
#endif
#endif
