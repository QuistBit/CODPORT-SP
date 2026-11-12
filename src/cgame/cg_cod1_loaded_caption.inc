/* The sound event completes on the admitted sample's duration. Retail CGAME
 * 30019DB0 separately sets the caption lifetime from cg_subtitleMinTime. */
static int CGV_LoadedCurrent(const C1NPacket *p,uint64_t incarnation){
 return CGV_Current(p->sound.epoch,incarnation) && cgvSequence==p->sound.sequence;
}

static void CG_Cod1LoadedCaptionCommand(void){
 char copied[C1V_ARGC][COD1_SS_PATH_BYTES],reply[C1D_CAPACITY];
 const char *args[C1V_ARGC],*translation;
 C1VPacket packet;
 C1NPacket *p=&packet.sound;
 CGVTrack caption={0};
 C1DReport report={0};
 vmCvar_t minimum={0};
 uint64_t epoch=cgCod1ScriptQuakeEpoch,inc=cgCod1ScriptQuakeIncarnation;
 sfxHandle_t sound;
 int i,duration,displayMs;
 float minimumMs;

 if(!epoch || trap_Argc()!=C1V_ARGC)return;
 for(i=0;i<C1V_ARGC;i++){
  trap_Argv(i,copied[i],sizeof(copied[i]));
  if(!CGV_Current(epoch,inc))return;
  args[i]=copied[i];
 }
 if(!C1V_DecodeLoadedCaption(C1V_ARGC,args,&packet) || p->sound.epoch!=epoch ||
    !CGV_Scope() || !CGV_Current(epoch,inc) || p->sound.sequence<=cgvSequence)return;
 cgvSequence=p->sound.sequence;
 translation=CG_Cod1LocaleLookupSubtitle(packet.subtitle);
 if(!CGV_LoadedCurrent(p,inc))return;
 if(!translation || !*translation || cg.time<0)goto refuse;
 trap_Cvar_Register(&minimum,"cg_subtitleMinTime","3",CVAR_ARCHIVE);
 if(!CGV_LoadedCurrent(p,inc))return;
 minimumMs=1000.0f*minimum.value;
 if(!isfinite(minimumMs) || minimumMs>=(float)INT_MAX)goto refuse;
 sound=trap_S_RegisterSound(p->sound.path);
 if(!CGV_LoadedCurrent(p,inc))return;
 if(!sound)goto refuse;
 duration=trap_Cod1SoundDuration(sound,p->sound.pitch);
 if(!CGV_LoadedCurrent(p,inc))return;
 if(duration<=0 || duration>C1N_MAX_DURATION_MS)goto refuse;
 displayMs=minimumMs>duration?(int)minimumMs:duration;
 if(!trap_Cod1TryStartAliasSound(p->sound.origin,p->sound.entity,CHAN_VOICE,sound,
    p->sound.gain,p->sound.pitch,p->sound.dist_min,p->sound.dist_max))goto refuse;
 if(!CGV_LoadedCurrent(p,inc))return;

 caption.loadedCaption=1;
 caption.start=cg.time;caption.duration=displayMs;
 caption.request.packet=*p;caption.request.incarnation=inc;
 strcpy(caption.subtitle,packet.subtitle);
 cgvTracks[p->sound.entity]=caption;cgvCaption=p->sound.entity;
 report.identity=C1N_PacketIdentity(p);report.duration=duration;
 if(C1D_Encode(reply,sizeof(reply),&report))trap_SendClientCommand(reply);
 return;
refuse:
 if(CGV_LoadedCurrent(p,inc))CGV_Report(p,0,'f');
}
