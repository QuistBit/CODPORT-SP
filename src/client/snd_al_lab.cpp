// Optional OpenAL/EAX adapter -> existing engine DMA output. Errors retain the base mixer.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "al_eal/EalMan.h"
#include "snd_al_lab.h"
static_assert(sizeof(EAXListenerProperties)==112,"EAX3 listener ABI required");
static_assert(sizeof(EAXSourceProperties)==72,"EAX3 source ABI required");
namespace {
constexpr int NVOICE=224,NSTREAM=12;
HMODULE dll;ALCdevice *device;ALCcontext *context;int sampleRate;void(*logger)(const char*);
bool active=false,faulted=false,effects=true,haveData=false,defaultOnly=false;int environment=-999,loopFrame=0;
long long rendered=0;int started=0,streamed=0,errors=0;std::string description,mapName;
EalMan eal;EAXListenerProperties defaultEnv{},quietEnv{},appliedEnv{};float listener[3]{};
GUID listenerGuid={0xA8FA6882,0xB476,0x11D3,{0xBD,0xB9,0,0xC0,0xF0,0x2D,0xDF,0x87}};
GUID sourceGuid={0xA8FA6881,0xB476,0x11D3,{0xBD,0xB9,0,0xC0,0xF0,0x2D,0xDF,0x87}};
using EaxFn=ALenum(AL_APIENTRY*)(const GUID*,ALuint,ALuint,ALvoid*,ALuint);EaxFn eaxSet,eaxGet;
#define API(n) decltype(&n) q##n
API(alcGetProcAddress);API(alcCreateContext);API(alcMakeContextCurrent);API(alcDestroyContext);API(alcCloseDevice);API(alcGetIntegerv);API(alcGetError);
API(alGetProcAddress);API(alIsExtensionPresent);API(alGetError);API(alGetString);API(alGenSources);API(alDeleteSources);API(alGenBuffers);API(alDeleteBuffers);API(alBufferData);API(alSourcei);API(alSourcef);API(alSource3f);API(alSourcePlay);API(alSourceStop);API(alGetSourcei);API(alSourceQueueBuffers);API(alSourceUnqueueBuffers);API(alListener3f);API(alListenerfv);API(alListenerf);API(alDistanceModel);API(alDopplerFactor);
LPALCRENDERSAMPLESSOFT renderSamples;
struct Voice{ALuint source=0,buffer=0;const void *key=nullptr;int token=0,entity=-1,seen=0;EAXSourceProperties acoustic{};bool props=false;};
struct Stream{ALuint source=0;int end=0,channels=0;std::vector<ALuint> buffers;EAXSourceProperties acoustic{};bool props=false;};
std::array<Voice,NVOICE> voices;std::array<Stream,NSTREAM> streams;
void log(const std::string &s){if(logger)logger(("[OpenAL lab] "+s+"\n").c_str());}
void fault(const char *where,int err){faulted=true;++errors;if(errors<12)log(std::string(where)+" failed "+std::to_string(err)+"; retaining base mixer until sound restart");}
// The optional backend must not unwind a C++ exception through engine C code.
// This handles C++ exceptions only; native access violations remain failures.
void exceptionFault() noexcept {
 faulted=true;++errors;
 if(logger){try{logger("[OpenAL] allocation/processing failure; retaining base mixer until sound restart\n");}catch(...){}}
}
bool check(const char *where){ALenum err=qalGetError();if(err)fault(where,err);return !faulted;}
EMPoint ealPoint(const float *p){return {p[0],p[2],p[1]};}
EAXSourceProperties sourceProps(const float *p,bool wet){
 EAXSourceProperties a{};a.flOcclusionLFRatio=.25f;a.flOcclusionRoomRatio=.5f;a.flOcclusionDirectRatio=1;a.flExclusionLFRatio=1;a.flDopplerFactor=1;a.flAirAbsorptionFactor=1;a.dwFlags=7;
 if(!wet||!effects){a.lRoom=-10000;return a;}
 for(int i=0;i<3;i++)if(!std::isfinite(p[i])){fault("source coordinates",-1);return a;}
 if(haveData&&!defaultOnly){auto point=ealPoint(p);EMPoint virt{};int result=eal.GetSourceDynamicAttributes(0,point,a.lObstruction,a.flObstructionLFRatio,a.lOcclusion,a.flOcclusionLFRatio,a.flOcclusionRoomRatio,virt,0);if(result)fault("EAL source lookup",result);}
 return a;
}
void acoustic(ALuint src,EAXSourceProperties &last,bool &valid,const float *p,bool wet){auto a=sourceProps(p,wet);if(faulted)return;if(!valid||std::memcmp(&a,&last,sizeof(a))){auto err=eaxSet(&sourceGuid,1,src,&a,sizeof(a));if(err){fault("EAX source",err);}else{last=a;valid=true;}}}
void stopStream(Stream &s){if(s.source){qalSourceStop(s.source);qalDeleteSources(1,&s.source);}if(!s.buffers.empty())qalDeleteBuffers(static_cast<ALsizei>(s.buffers.size()),s.buffers.data());s=Stream{};}
uint32_t u32(const unsigned char *p){uint32_t n;std::memcpy(&n,p,4);return n;}
bool emptyEal(const unsigned char *p,size_t n,EAXListenerProperties &out){
 if(n<12||std::memcmp(p,"RIFF",4)||std::memcmp(p+8,"eal ",4)||u32(p+4)>n-8)return false;
 bool env=false,geometry=false;int names=-1;
 for(size_t at=12;at+8<=n;){size_t len=u32(p+at+4),off=at+8;if(len>n-off)return false;
  if(!std::memcmp(p+at,"denv",4)&&len==sizeof(out)){std::memcpy(&out,p+off,len);env=true;}
  if(!std::memcmp(p+at,"LIST",4)&&len>=4){
   bool gp=!std::memcmp(p+off,"gemp",4),ep=!std::memcmp(p+off,"envp",4);
   for(size_t sub=off+4;sub+8<=off+len;){size_t sl=u32(p+sub+4),so=sub+8;if(sl>off+len-so)return false;
    if(ep&&!std::memcmp(p+sub,"num ",4)&&sl==4)names=static_cast<int>(u32(p+so));
    if(gp&&!std::memcmp(p+sub,"gema",4)&&sl==28)geometry=u32(p+so)==0&&u32(p+so+4)==0&&u32(p+so+8)==0xffffffff&&u32(p+so+12)==0&&u32(p+so+16)==0;
    sub=so+sl+(sl&1);
   }
  }at=off+len+(len&1);
 }return env&&geometry&&names==0;
}
}
extern "C" int ALab_Active(){return active&&!faulted;}
extern "C" int ALab_MonotonicClock(){return active;}
extern "C" int ALab_Init(int rate,int hrtf,void(*output)(const char*))try {
 if(dll||device||context)ALab_Shutdown();logger=output;sampleRate=rate;faulted=false;errors=0;char path[MAX_PATH];auto pathLength=GetModuleFileNameA(nullptr,path,MAX_PATH);if(!pathLength||pathLength>=MAX_PATH)return 0;auto slash=std::strrchr(path,'\\');if(!slash||static_cast<size_t>(slash+1-path)+sizeof("soft_oal.dll")>sizeof(path))return 0;std::memcpy(slash+1,"soft_oal.dll",sizeof("soft_oal.dll"));
 dll=LoadLibraryExA(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll){log("Cannot load stand soft_oal.dll");return 0;}
#define LOAD(n) q##n=reinterpret_cast<decltype(&n)>(GetProcAddress(dll,#n));if(!q##n){log("Missing " #n);ALab_Shutdown();return 0;}
 LOAD(alcGetProcAddress);LOAD(alcCreateContext);LOAD(alcMakeContextCurrent);LOAD(alcDestroyContext);LOAD(alcCloseDevice);LOAD(alcGetIntegerv);LOAD(alcGetError);
 LOAD(alGetProcAddress);LOAD(alIsExtensionPresent);LOAD(alGetError);LOAD(alGetString);LOAD(alGenSources);LOAD(alDeleteSources);LOAD(alGenBuffers);LOAD(alDeleteBuffers);LOAD(alBufferData);LOAD(alSourcei);LOAD(alSourcef);LOAD(alSource3f);LOAD(alSourcePlay);LOAD(alSourceStop);LOAD(alGetSourcei);LOAD(alSourceQueueBuffers);LOAD(alSourceUnqueueBuffers);LOAD(alListener3f);LOAD(alListenerfv);LOAD(alListenerf);LOAD(alDistanceModel);LOAD(alDopplerFactor);
 auto open=reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(qalcGetProcAddress(nullptr,"alcLoopbackOpenDeviceSOFT"));renderSamples=reinterpret_cast<LPALCRENDERSAMPLESSOFT>(qalcGetProcAddress(nullptr,"alcRenderSamplesSOFT"));if(!open||!renderSamples){ALab_Shutdown();return 0;}
 device=open(nullptr);if(!device){ALab_Shutdown();return 0;}
 ALCint attrs[]={ALC_FORMAT_CHANNELS_SOFT,ALC_STEREO_SOFT,ALC_FORMAT_TYPE_SOFT,ALC_SHORT_SOFT,ALC_FREQUENCY,rate,0x20003,4,ALC_HRTF_SOFT,hrtf?ALC_TRUE:ALC_FALSE,0};
 context=qalcCreateContext(device,attrs);if(!context||!qalcMakeContextCurrent(context)||!qalIsExtensionPresent("EAX3.0")){ALab_Shutdown();return 0;}
 eaxSet=reinterpret_cast<EaxFn>(qalGetProcAddress("EAXSet"));eaxGet=reinterpret_cast<EaxFn>(qalGetProcAddress("EAXGet"));if(!eaxSet||!eaxGet){ALab_Shutdown();return 0;}
 if(eaxGet(&listenerGuid,1,0,&quietEnv,sizeof(quietEnv))){ALab_Shutdown();return 0;}quietEnv.lRoom=-10000;appliedEnv=quietEnv;if(eaxSet(&listenerGuid,1,0,&quietEnv,sizeof(quietEnv))){ALab_Shutdown();return 0;}
 qalDistanceModel(AL_NONE);qalDopplerFactor(0);qalListenerf(0x20004,.0254f); // AL_METERS_PER_UNIT
 if(!check("init")||qalcGetError(device)!=ALC_NO_ERROR){ALab_Shutdown();return 0;}
 active=true;rendered=0;started=streamed=0;haveData=false;environment=-999;mapName.clear();
 ALCint status=0;qalcGetIntegerv(device,ALC_HRTF_STATUS_SOFT,1,&status);log(std::string(qalGetString(AL_VERSION))+" loopback -> DMA, HRTF="+std::to_string(status));return 1;
} catch (...) { exceptionFault();ALab_Shutdown();return 0; }
extern "C" void ALab_StopVoice(int slot){if(!active||slot<0||slot>=NVOICE)return;auto &v=voices[slot];if(v.source){qalSourceStop(v.source);qalDeleteSources(1,&v.source);}if(v.buffer)qalDeleteBuffers(1,&v.buffer);v=Voice{};}
extern "C" void ALab_Clear(int raw,int music){if(!active)return;for(int i=0;i<NVOICE;i++)ALab_StopVoice(i);for(int i=0;i<NSTREAM;i++)if((i&&raw)||(!i&&music))stopStream(streams[i]);}
extern "C" void ALab_Shutdown(){if(active)ALab_Clear(1,1);active=false;if(context){qalcMakeContextCurrent(nullptr);qalcDestroyContext(context);context=nullptr;}if(device){qalcCloseDevice(device);device=nullptr;}if(dll){FreeLibrary(dll);dll=nullptr;}eal.FreeDataSet(0);}
extern "C" int ALab_LoadEal(const void *data,int bytes,const char *name)try {
 if(!active||faulted)return 0;haveData=false;defaultOnly=false;environment=-999;mapName=name?name:"";eal.FreeDataSet(0);
 if(data&&bytes>0){if(emptyEal(static_cast<const unsigned char*>(data),bytes,defaultEnv)){defaultOnly=haveData=true;log(mapName+": default-only EAL accepted");return 2;}
  std::string copy(static_cast<const char*>(data),bytes);std::istringstream stream(copy,std::ios::in|std::ios::binary);int result=eal.LoadDataSetFromMemory(stream);if(result==0&&eal.GetEnvironmentAttributes(-1,defaultEnv)==0){haveData=true;log(mapName+": EalMan dataset accepted");return 1;}log(mapName+": EAL rejected "+std::to_string(result));fault("EAL load",result);return 0;
 }else log(mapName+": no EAL, positional audio with dry environment");
 appliedEnv=quietEnv;if(auto err=eaxSet(&listenerGuid,1,0,&quietEnv,sizeof(quietEnv)))fault("dry environment",err);return 0;
} catch (...) { exceptionFault();return 0; }
extern "C" void ALab_Listener(const float *p,const float *forward,const float *up,int enabled)try {
 if(!active||faulted)return;for(int i=0;i<3;i++)if(!std::isfinite(p[i])||!std::isfinite(forward[i])||!std::isfinite(up[i])){fault("listener coordinates",-1);return;}effects=enabled!=0;std::copy(p,p+3,listener);qalListener3f(AL_POSITION,p[0],p[1],p[2]);float orientation[6]={forward[0],forward[1],forward[2],up[0],up[1],up[2]};if(forward[0]*forward[0]+forward[1]*forward[1]+forward[2]*forward[2]<.5f){orientation[0]=1;orientation[1]=orientation[2]=orientation[3]=orientation[4]=0;orientation[5]=1;}qalListenerfv(AL_ORIENTATION,orientation);
 int id=-1;EAXListenerProperties env=quietEnv;
 if(haveData&&effects){env=defaultEnv;if(!defaultOnly){auto point=ealPoint(p);int result=eal.GetListenerDynamicAttributes(0,point,id,EMFLAG_LOCKPOSITION);if(!result)result=eal.GetEnvironmentAttributes(id,env);if(result){fault("EAL listener lookup",result);return;}}}
 if(std::memcmp(&env,&appliedEnv,sizeof(env))){auto err=eaxSet(&listenerGuid,1,0,&env,sizeof(env));if(!err)appliedEnv=env;else{fault("EAX environment",err);return;}}
 if(id!=environment){environment=id;EAXListenerProperties readback{};auto readStatus=eaxGet(&listenerGuid,1,0,&readback,sizeof(readback));bool equal=!readStatus&&!std::memcmp(&readback,&env,sizeof(env));if(!equal)fault("EAX environment readback",readStatus?readStatus:-1);
  std::ostringstream line;line<<mapName<<": environment "<<id<<" listener_game="<<p[0]<<","<<p[1]<<","<<p[2]<<" eax_readback_equal="<<equal<<" room="<<env.lRoom<<" decay="<<env.flDecayTime;log(line.str());}
} catch (...) { exceptionFault();return; }
extern "C" int ALab_NeedsVoice(int slot,const void *key,int token){return active&&!faulted&&slot>=0&&slot<NVOICE&&(!voices[slot].source||voices[slot].key!=key||voices[slot].token!=token);}
extern "C" void ALab_Voice(int slot,const void *key,int token,const short *pcm,int frames,int rate,int loop,const float *p,int relative,int wet,float gain,float pitch,int offset)try {
 if(!active||faulted||slot<0||slot>=NVOICE)return;auto &v=voices[slot];if(ALab_NeedsVoice(slot,key,token)){
  if(!pcm||frames<=0||frames>INT_MAX/2){fault("voice PCM unavailable",-1);return;}
  int entity=v.entity,seen=v.seen;ALab_StopVoice(slot);v.entity=entity;v.seen=seen;
  qalGenSources(1,&v.source);qalGenBuffers(1,&v.buffer);qalBufferData(v.buffer,AL_FORMAT_MONO16,pcm,frames*2,rate);qalSourcei(v.source,AL_BUFFER,v.buffer);qalSourcei(v.source,AL_LOOPING,loop?AL_TRUE:AL_FALSE);if(offset>0&&offset<frames)qalSourcei(v.source,AL_SAMPLE_OFFSET,offset);
  if(!check("voice upload"))return;v.key=key;v.token=token;++started;
 }
 qalSourcei(v.source,AL_SOURCE_RELATIVE,relative?AL_TRUE:AL_FALSE);qalSource3f(v.source,AL_POSITION,relative?0:p[0],relative?0:p[1],relative?0:p[2]);qalSourcef(v.source,AL_ROLLOFF_FACTOR,0);qalSourcef(v.source,AL_PITCH,pitch);qalSourcef(v.source,AL_GAIN,std::max(0.f,std::min(gain,4.f)));acoustic(v.source,v.acoustic,v.props,relative?listener:p,wet!=0);
 ALint state;qalGetSourcei(v.source,AL_SOURCE_STATE,&state);if(state==AL_INITIAL)qalSourcePlay(v.source);check("voice");
} catch (...) { exceptionFault();return; }
extern "C" void ALab_BeginLoops(){++loopFrame;}
extern "C" void ALab_ResetStream(int index){if(active&&index>=0&&index<NSTREAM)stopStream(streams[index]);}
extern "C" int ALab_LoopSlot(int entity,const void *key)try {if(!active||faulted)return -1;int slot=-1;for(int i=96;i<NVOICE;i++){auto &v=voices[i];if(v.key==key&&v.entity==entity&&v.seen!=loopFrame){slot=i;break;}if(!v.source&&slot<0)slot=i;}if(slot>=0){voices[slot].seen=loopFrame;voices[slot].entity=entity;}else fault("loop capacity",-1);return slot;} catch (...) { exceptionFault();return -1; }
extern "C" void ALab_EndLoops(){for(int i=96;i<NVOICE;i++)if(voices[i].source&&voices[i].seen!=loopFrame)ALab_StopVoice(i);}
extern "C" void ALab_Stream(int index,int start,int end,const int *raw,int mask,int channels,const float *p,int relative,int wet,float gain)try {
 if(!active||faulted||index<0||index>=NSTREAM)return;
 if((channels!=1&&channels!=2)||(channels==2&&!relative)){fault("unsupported spatial stereo stream",-1);return;}
 auto &s=streams[index];if(end<=start){if(s.source&&end<s.end)stopStream(s);return;}
 if(s.source&&(end<s.end||start>s.end||s.channels!=channels))stopStream(s);
 if(!s.source){qalGenSources(1,&s.source);qalSourcei(s.source,AL_SOURCE_RELATIVE,relative?1:0);if(!check("stream source"))return;s.end=start;s.channels=channels;++streamed;}
 ALint processed=0;qalGetSourcei(s.source,AL_BUFFERS_PROCESSED,&processed);while(processed-->0){ALuint b;qalSourceUnqueueBuffers(s.source,1,&b);qalDeleteBuffers(1,&b);s.buffers.erase(std::remove(s.buffers.begin(),s.buffers.end(),b),s.buffers.end());}
 if(s.end<start)s.end=start;
 while(s.end<end){int count=std::min(2048,end-s.end);std::vector<short> samples(count*channels);for(int j=0;j<count;j++){int at=(s.end+j)&mask;samples[j*channels]=static_cast<short>(raw[at*2]>>8);if(channels==2)samples[j*2+1]=static_cast<short>(raw[at*2+1]>>8);}s.buffers.reserve(s.buffers.size()+1);ALuint b=0;qalGenBuffers(1,&b);qalBufferData(b,channels==1?AL_FORMAT_MONO16:AL_FORMAT_STEREO16,samples.data(),static_cast<int>(samples.size()*2),sampleRate);if(!check("stream buffer upload")){if(b)qalDeleteBuffers(1,&b);return;}qalSourceQueueBuffers(s.source,1,&b);if(!check("stream queue")){if(b)qalDeleteBuffers(1,&b);return;}s.buffers.push_back(b);s.end+=count;}
 qalSourcei(s.source,AL_SOURCE_RELATIVE,relative?1:0);qalSource3f(s.source,AL_POSITION,relative?0:p[0],relative?0:p[1],relative?0:p[2]);qalSourcef(s.source,AL_GAIN,std::max(0.f,std::min(gain,4.f)));qalSourcef(s.source,AL_ROLLOFF_FACTOR,0);acoustic(s.source,s.acoustic,s.props,relative?listener:p,wet!=0);
 ALint state=0,queued=0;qalGetSourcei(s.source,AL_SOURCE_STATE,&state);qalGetSourcei(s.source,AL_BUFFERS_QUEUED,&queued);if(state!=AL_PLAYING&&queued)qalSourcePlay(s.source);check("stream");
} catch (...) { exceptionFault();return; }
extern "C" void ALab_Render(int *pairs,int frames)try {if(!active||faulted||frames<=0)return;std::vector<short> out(frames*2);renderSamples(device,out.data(),frames);if(auto err=qalcGetError(device))fault("render ALC",err);if(!check("render"))return;for(int i=0;i<frames*2;i++)pairs[i]=static_cast<int>(out[i])*256;rendered+=frames;} catch (...) { exceptionFault();return; }
// A fresh backend owns no queued audio. Initial DMA position belongs to the
// engine; do not synthesize elapsed silence before the first source/block.
extern "C" void ALab_Skip(int frames)try {if(!active||faulted)return;if(rendered==0&&started==0&&streamed==0)return;if(frames>sampleRate){fault("catch-up exceeds one second budget",-1);return;}short discard[2048];while(frames>0){int n=std::min(frames,1024);renderSamples(device,discard,n);if(auto err=qalcGetError(device))fault("skip ALC",err);if(!check("skip"))return;frames-=n;rendered+=n;}} catch (...) { exceptionFault();return; }
extern "C" const char *ALab_Info()try {description="OpenAL lab active="+std::to_string(active&&!faulted)+" faulted="+std::to_string(faulted)+" map="+mapName+" defaultOnly="+std::to_string(defaultOnly)+" voicesStarted="+std::to_string(started)+" streamsStarted="+std::to_string(streamed)+" frames="+std::to_string(rendered)+" errors="+std::to_string(errors);return description.c_str();} catch (...) { exceptionFault();return "OpenAL disabled: C++ allocation/processing failure"; }
