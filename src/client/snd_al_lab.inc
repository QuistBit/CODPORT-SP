/* Engine-side optional OpenAL adapter. The retained mixer still computes speech amplitude;
 * only the final PCM is replaced by individually positioned OpenAL sources. */
static cvar_t *alLabHrtf,*alLabEffects;
static char alLabMap[MAX_QPATH];
static void S_AL_Log(const char *text){Com_Printf("%s",text);}
static void S_AL_Info_f(void){Com_Printf("%s fade=%.3f target=%.3f volume=%.3f mute=%d clock=%d painted=%d\n",ALab_Info(),snd.volCurrent,snd.volTarget,s_volume->value,snd.s_soundMute,s_soundtime,s_paintedtime);}
static void S_AL_LabInit(void){
 alLabHrtf=Cvar_Get("s_alHrtf","1",CVAR_ARCHIVE|CVAR_LATCH);alLabEffects=Cvar_Get("s_alEffects","1",CVAR_ARCHIVE);alLabMap[0]=0;
 if(S_Cod1SoundMenuUseOpenAL()&&!ALab_Init(dma.speed,alLabHrtf->integer,S_AL_Log))Com_Printf("OpenAL/EAL init failed; keeping base mixer\n");
 Cmd_AddCommand("s_al_info",S_AL_Info_f);
}
static void S_AL_LabShutdown(void){ALab_Shutdown();Cmd_RemoveCommand("s_al_info");}
static short *S_AL_PCM(sfx_t *sfx){short *pcm;int copied=0;const sndBuffer *part;if(!sfx||sfx->soundLength<=0||sfx->soundCompressionMethod!=0)return NULL;pcm=Hunk_AllocateTempMemory(sfx->soundLength*2);for(part=sfx->soundData;part&&copied<sfx->soundLength;part=part->next){int count=sfx->soundLength-copied;if(count>SND_CHUNK_SIZE)count=SND_CHUNK_SIZE;memcpy(pcm+copied,part->sndChunk,count*2);copied+=count;}if(copied!=sfx->soundLength){Hunk_FreeTempMemory(pcm);return NULL;}return pcm;}
void S_AL_RenderBlock(portable_samplepair_t *paint,int frames){
 int i,slot;float global=s_mute->integer?0:s_volume->value*snd.volCurrent;
 const char *map=Cvar_VariableString("mapname");
 if(!ALab_Active())return;
 if(Q_stricmp(map,alLabMap)){void *data=NULL;char path[MAX_QPATH];int bytes;ALab_Clear(1,1);Q_strncpyz(alLabMap,map,sizeof(alLabMap));Com_sprintf(path,sizeof(path),"eagle/%s.eal",map);bytes=FS_ReadFile(path,&data);ALab_LoadEal(data,bytes,map);if(data)FS_FreeFile(data);}
 ALab_Listener(listener_origin,listener_axis[0],listener_axis[2],alLabEffects->integer);
 for(i=0;i<MAX_CHANNELS;i++){channel_t *ch=&s_channels[i];short *pcm=NULL;const float *origin;float gain;int local,wet;
  if(!ch->thesfx||ch->startSample==START_SAMPLE_IMMEDIATE)continue;
  origin=ch->fixed_origin?ch->origin:snd.entityPositions[ch->entnum];local=ch->entnum==listener_number;wet=ch->entchannel!=CHAN_LOCAL&&ch->entchannel!=CHAN_LOCAL_SOUND&&ch->entchannel!=CHAN_ANNOUNCER;
  gain=local?(float)ch->leftvol/256*1.41421356f:(float)(ch->leftvol+ch->rightvol)/256*.70710678f;gain*=global;
  if(ALab_NeedsVoice(i,ch->thesfx,ch->startSample))pcm=S_AL_PCM(ch->thesfx);
  ALab_Voice(i,ch->thesfx,ch->startSample,pcm,ch->thesfx->soundLength,dma.speed,0,origin,local,wet,gain,ch->cod1Alias?ch->cod1Pitch:1,0);if(pcm)Hunk_FreeTempMemory(pcm);
 }
 ALab_BeginLoops();
 for(i=0;i<snd.numLoopSounds;i++){loopSound_t *loop=&snd.loopSounds[i];int l,r;float scale,gain;short *pcm=NULL;slot=ALab_LoopSlot(loop->alEntity,loop->sfx);if(slot<0)continue;
  S_SpatializeOrigin(loop->origin,90,&l,&r,loop->range);scale=S_Cod1SlaveScale(loop->origin,loop->range,snd.volCurrent>0?(float)loop->vol/(255*snd.volCurrent):0,loop->cod1SlaveCap);
  gain=(float)(l+r)/256*(float)loop->vol/256*.70710678f*scale*(s_mute->integer?0:s_volume->value);
  if(ALab_NeedsVoice(slot,loop->sfx,0))pcm=S_AL_PCM(loop->sfx);
  ALab_Voice(slot,loop->sfx,0,pcm,loop->sfx->soundLength,dma.speed,1,loop->origin,0,1,gain,1,s_paintedtime%loop->sfx->soundLength);if(pcm)Hunk_FreeTempMemory(pcm);
 }ALab_EndLoops();
 for(i=0;i<MAX_STREAMING_SOUNDS;i++){streamingSound_t *ss=&streamingSounds[i];int spatial=(SCV_Reserved(i)||SCL_Reserved(i)||ss->attenuation)&&ss->entnum>=0&&ss->entnum<MAX_GENTITIES;int local=!spatial||ss->entnum==listener_number;int channels=alLabRawChannels[i]==2?2:1;float gain;const float *origin=spatial?snd.entityPositions[ss->entnum]:listener_origin;
  if(SCV_Reserved(i))origin=SCV_Origin(&scvTracks[i]);
  /* Spatial stereo stays unsupported: let the adapter retain the base mix,
   * rather than silently dropping its right channel. */
  gain=local?(float)s_rawVolume[i].left/256*(channels==1?1.41421356f:1):(float)(s_rawVolume[i].left+s_rawVolume[i].right)/256*.70710678f;
  ALab_Stream(i,s_paintedtime,s_rawend[i],(int*)s_rawsamples[i],MAX_RAW_SAMPLES-1,channels,origin,local,spatial,gain);
 }
 ALab_Render((int*)paint,frames);
}
