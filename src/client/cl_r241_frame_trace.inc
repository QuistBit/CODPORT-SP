/* WORK PROPOSAL: opt-in bounded native-Windows diagnostic; not a camera fix.
 * Included once by cl_main.c. Default off: no clock/cvar/IO/heap calls in hooks.
 * Non-SMP only; do not change r_smp or any presentation/simulation setting. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdint.h>
#include "../qcommon/cod1_client_trace_r241.h"
#define CT241_ROWS 2048
#define CT241_MS 20000
#define CT241_U64 "%I64u"
extern cvar_t *com_maxfps;
extern int com_frameNumber;
typedef struct {
 unsigned mask;int frame,partial,cgCount,cgOpen,ends,swaps;
 uint64_t begin,end,t[CT241_STAGE_COUNT],cgBegin[2],cgEnd[2];
 int stereo[2],cgTime[2],state,keyCatchers,realtime,frameMsec,rawMsec,serverTime,delta;
 int snapTime,extrapolated,cmdNumber,cmdTime,cmdYaw,cmdPitch,dx,dy;
 int playerFlags,heavy,viewlocked,viewlockedEntity,maxfps,interval,extension;
 int presentStereo,front,intervalCalls,intervalResult;
} CT241Row;
static struct {
 int initialized,active,current,count,reason,overflow,invalid,cgLost,cgAck,dumped,busy;
 unsigned id,map,pid;uint64_t frequency,start,deadline,last;
 CT241Row rows[CT241_ROWS];
} ct241;
static unsigned ct241Serial,ct241Map,ct241Mutation=1;
static int ct241CGDepth;
static cvar_t *ct241Control;
static void CT241_Stop(int reason){if(ct241.current>=0&&ct241.current<ct241.count&&reason!=1)ct241.rows[ct241.current].partial=1;ct241.active=0;if(!ct241.reason)ct241.reason=reason;}
static int CT241_Clock(uint64_t*out){LARGE_INTEGER value;
 if(!QueryPerformanceCounter(&value)||value.QuadPart<0||
    (uint64_t)value.QuadPart<ct241.start||(uint64_t)value.QuadPart<ct241.last){
  ct241.invalid=1;CT241_Stop(3);return 0;}
 ct241.last=(uint64_t)value.QuadPart;*out=ct241.last-ct241.start;return 1;
}
static CT241Row*CT241_Current(void){return ct241.active&&ct241.current>=0&&ct241.current<ct241.count?ct241.rows+ct241.current:NULL;}
static void CT241_State(CT241Row*r){usercmd_t*c=&cl.cmds[cl.cmdNumber&CMD_MASK];
 r->state=cls.state;r->keyCatchers=cls.keyCatchers;r->realtime=cls.realtime;
 r->frameMsec=cls.frametime;r->rawMsec=com_frameMsec;r->serverTime=cl.serverTime;
 r->delta=cl.serverTimeDelta;r->snapTime=cl.snap.serverTime;r->extrapolated=cl.extrapolatedSnapshot;
 r->cmdNumber=cl.cmdNumber;r->cmdTime=c->serverTime;r->cmdYaw=c->angles[YAW];r->cmdPitch=c->angles[PITCH];
 r->dx=cl.mouseDx[cl.mouseIndex];r->dy=cl.mouseDy[cl.mouseIndex];
 r->playerFlags=cl.snap.ps.eFlags;r->heavy=cl.snap.ps.persistant[PERS_HWEAPON_USE];
 r->viewlocked=cl.snap.ps.viewlocked;r->viewlockedEntity=cl.snap.ps.viewlocked_entNum;
 r->maxfps=com_maxfps?com_maxfps->integer:0;
}
static void CT241_NewRow(int frame,int partial){uint64_t now;CT241Row*r;
 if(!ct241.active||!CT241_Clock(&now))return;
 if(R241TraceRendererSmpActive()){ct241.invalid=1;CT241_Stop(8);return;}
 if(ct241.last>=ct241.deadline){CT241_Stop(1);return;}
 if(ct241.count==CT241_ROWS){ct241.overflow++;CT241_Stop(2);return;}
 if(ct241.current>=0){ct241.invalid=1;CT241_Stop(6);return;}
 ct241.current=ct241.count++;r=ct241.rows+ct241.current;memset(r,0,sizeof(*r));
 r->frame=frame;r->partial=partial;r->begin=now;CT241_State(r);
}
void CL_R241TraceFrameBegin(int frame){if(!ct241.active)return;CT241_NewRow(frame,0);}
void CL_R241TraceFrameEnd(void){CT241Row*r=CT241_Current();uint64_t now;
 if(!r||!CT241_Clock(&now))return;r->end=now;CT241_State(r);
 if(ct241.last>=ct241.deadline)r->partial=1;
 if(r->cgOpen){ct241.invalid=1;CT241_Stop(6);}ct241.current=-1;
 if(ct241.last>=ct241.deadline)CT241_Stop(1);
}
void CL_R241TraceAbort(void){ct241CGDepth=0;if(ct241.active){ct241.invalid=1;CT241_Stop(7);}}
void CL_R241TraceMark(int stage){CT241Row*r=CT241_Current();uint64_t now;
 if(!r||stage<0||stage>=CT241_STAGE_COUNT||!CT241_Clock(&now))return;
 if(stage==CT241_END_ENTER)r->ends++;if(stage==CT241_SWAP_ENTER){r->swaps++;r->front=0;}
 if(!(r->mask&(1u<<stage))||stage==CT241_END_EXIT||stage==CT241_SWAP_EXIT)r->t[stage]=now;
 r->mask|=1u<<stage;
}
void CL_R241TraceCGameBegin(int stereo){CT241Row*r=CT241_Current();uint64_t now;
 ct241CGDepth++;
 if(!r||!CT241_Clock(&now))return;
 if(r->cgOpen||r->cgCount==2){ct241.invalid=1;CT241_Stop(6);return;}
 r->cgOpen=1;r->stereo[r->cgCount]=stereo;r->cgTime[r->cgCount]=cl.serverTime;r->cgBegin[r->cgCount]=now;
}
void CL_R241TraceCGameEnd(void){CT241Row*r=CT241_Current();uint64_t now;
 if(ct241CGDepth>0)ct241CGDepth--;
 if(!r||!CT241_Clock(&now))return;
 if(!r->cgOpen){ct241.invalid=1;CT241_Stop(6);return;}
 r->cgEnd[r->cgCount++]=now;r->cgOpen=0;CT241_State(r);
}
void CL_R241TracePresent(int interval,int extension,int stereo,int front){CT241Row*r=CT241_Current();
 if(!r)return;r->interval=interval;r->extension=extension;r->presentStereo=stereo;r->front=front;
}
void CL_R241TraceSwapIntervalResult(int result){CT241Row*r=CT241_Current();if(r){r->intervalCalls++;r->intervalResult=result;}}
void CL_R241TraceMapReset(void){
 if(ct241Mutation!=UINT_MAX)ct241Mutation++;
 ct241CGDepth=0;
 if(ct241Map!=UINT_MAX)ct241Map++;
 if(ct241.id&&!ct241.dumped){ct241.cgLost=1;CT241_Stop(4);}
 if(ct241Control)Cvar_Set("r241_trace_control","0");
}
static int CT241_ExportCurrent(unsigned generation,unsigned id){return ct241Mutation==generation&&ct241.id==id&&!ct241.active&&ct241.busy;}
static int CT241_Write(fileHandle_t f,const char*s,unsigned generation,unsigned id){int n=(int)strlen(s);return FS_Write(s,n,f)==n&&CT241_ExportCurrent(generation,id);}
static int CT241_Dump(void){fileHandle_t f;char path[80],line[2048];int i,j,ok=1;unsigned generation=ct241Mutation,id=ct241.id;
 if(ct241.active||!ct241.id)return 0;if(ct241.dumped)return 1;
 Com_sprintf(path,sizeof(path),"r241/trace-%u-"CT241_U64"-%08u-engine.csv",ct241.pid,ct241.start,ct241.id);
 if(FS_FileExists(path)||!CT241_ExportCurrent(generation,id)){Com_Printf("R241 trace: existing/stale engine export refused, never overwritten: %s\n",path);return 0;}
 f=FS_FOpenFileWrite(path);if(!f)return 0;if(!CT241_ExportCurrent(generation,id)){FS_FCloseFile(f);return 0;}
 Com_sprintf(line,sizeof(line),"#id=%u,map=%u,pid=%u,frequency="CT241_U64",start="CT241_U64",deadline="CT241_U64",count=%d,reason=%d,overflow=%d,invalid=%d,cg_lost=%d,cg_ack=%d\n",ct241.id,ct241.map,ct241.pid,ct241.frequency,ct241.start,ct241.deadline,ct241.count,ct241.reason,ct241.overflow,ct241.invalid,ct241.cgLost,ct241.cgAck);ok=CT241_Write(f,line,generation,id);if(!ok){FS_FCloseFile(f);return 0;}
 ok&=CT241_Write(f,"frame,partial,begin,end,mask,cap_enter,cap_exit,sv_enter,sv_exit,end_enter,end_exit,swap_enter,swap_exit,cg_count,cg0_stereo,cg0_time,cg0_begin,cg0_end,cg1_stereo,cg1_time,cg1_begin,cg1_end,ends,swaps,state,keycatchers,realtime,frame_msec,raw_msec,server_time,delta,snap_time,extrapolated,cmd_number,cmd_time,cmd_yaw,cmd_pitch,unconsumed_dx,unconsumed_dy,player_flags,heavy,viewlocked,viewlocked_entity,maxfps,swap_interval,swap_extension,present_stereo,front,interval_calls,interval_result\n",generation,id);if(!ok){FS_FCloseFile(f);return 0;}
 for(i=0;i<ct241.count;i++){CT241Row*r=ct241.rows+i;int at;
  Com_sprintf(line,sizeof(line),"%d,%d,"CT241_U64","CT241_U64",%u",r->frame,r->partial,r->begin,r->end,r->mask);at=(int)strlen(line);
  for(j=0;j<CT241_STAGE_COUNT;j++){Com_sprintf(line+at,sizeof(line)-at,","CT241_U64,r->t[j]);at=(int)strlen(line);}
  Com_sprintf(line+at,sizeof(line)-at,",%d,%d,%d,"CT241_U64","CT241_U64",%d,%d,"CT241_U64","CT241_U64",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",r->cgCount,r->stereo[0],r->cgTime[0],r->cgBegin[0],r->cgEnd[0],r->stereo[1],r->cgTime[1],r->cgBegin[1],r->cgEnd[1],r->ends,r->swaps,r->state,r->keyCatchers,r->realtime,r->frameMsec,r->rawMsec,r->serverTime,r->delta,r->snapTime,r->extrapolated,r->cmdNumber,r->cmdTime,r->cmdYaw,r->cmdPitch,r->dx,r->dy,r->playerFlags,r->heavy,r->viewlocked,r->viewlockedEntity,r->maxfps,r->interval,r->extension,r->presentStereo,r->front,r->intervalCalls,r->intervalResult);
  ok&=CT241_Write(f,line,generation,id);if(!ok)break;
 }
 if(ok){Com_sprintf(line,sizeof(line),"#END,id=%u,rows=%d\n",ct241.id,ct241.count);ok=CT241_Write(f,line,generation,id);}FS_FCloseFile(f);return ok&&CT241_ExportCurrent(generation,id);
}
static void CL_R241TraceCommand(void){const char*op=Cmd_Argv(1);LARGE_INTEGER frequency,now;char control[192];int paired,engine;
 if(ct241.busy)return;ct241.busy=1;
 if(ct241.active&&CT241_Current())CT241_Current()->partial=1;
 if(ct241CGDepth){Com_Printf("R241 trace command refused inside CG draw; no nested CG command/IO\n");goto done;}
 if(Cmd_Argc()!=2){Com_Printf("r241_trace start|stop|status\n");goto done;}
 if(!Q_stricmp(op,"start")){
  if(ct241.active||(ct241.id&&!ct241.dumped)){Com_Printf("R241 trace: stop/export the previous capture first\n");goto done;}
  if(cls.state!=CA_ACTIVE||!cls.cgameStarted||!cgvm||R241TraceRendererSmpActive()){
   Com_Printf("R241 trace refused: requires active CG and actual non-SMP renderer; no mode was changed\n");goto done;}
  if(ct241Serial==UINT_MAX||ct241Map==UINT_MAX||ct241Mutation==UINT_MAX||!ct241Map||!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||frequency.QuadPart>INT64_MAX/CT241_MS||!QueryPerformanceCounter(&now)||now.QuadPart<0||now.QuadPart>INT64_MAX-frequency.QuadPart*CT241_MS/1000){Com_Printf("R241 trace: unsupported clock/generation\n");goto done;}
  memset(&ct241,0,sizeof(ct241));ct241.initialized=1;ct241.busy=1;ct241.current=-1;ct241.id=++ct241Serial;ct241.map=ct241Map;
  ct241.pid=(unsigned)GetCurrentProcessId();if(!ct241.pid){ct241.dumped=1;ct241.invalid=1;goto done;}
  ct241.frequency=(uint64_t)frequency.QuadPart;ct241.start=ct241.last=(uint64_t)now.QuadPart;ct241.deadline=ct241.start+ct241.frequency*CT241_MS/1000;
  Com_sprintf(control,sizeof(control),"%u %u %u "CT241_U64" "CT241_U64" "CT241_U64,ct241.id,ct241.map,ct241.pid,ct241.frequency,ct241.start,ct241.deadline);
  Cvar_Set("r241_trace_control",control);
  if(!CL_GameCommand()){Cvar_Set("r241_trace_control","0");ct241.dumped=1;ct241.invalid=1;Com_Printf("R241 trace refused by CG: no paired capture claimed\n");goto done;}
  ct241.active=1;CT241_NewRow(com_frameNumber,1);
  if(!ct241.active){ct241.invalid=1;Com_Printf("R241 trace start clock/state failed after CG ack; stop/export invalid capture, no paired claim\n");goto done;}
  Com_Printf("R241 trace %u: capturing <=20 seconds; auto-stop freezes only, use r241_trace stop to export. Console-open rows are marked.\n",ct241.id);
 }else if(!Q_stricmp(op,"stop")){
  if(!ct241.id){Com_Printf("R241 trace: no capture\n");goto done;}
  if(ct241.dumped){Com_Printf("R241 trace %u already exported (CG-ack=%d), not rewritten\n",ct241.id,ct241.cgAck);goto done;}
  if(ct241.active){CT241Row*r=CT241_Current();uint64_t t;if(r&&CT241_Clock(&t)){r->end=t;r->partial=1;CT241_State(r);}CT241_Stop(5);ct241.current=-1;}
  paired=cgvm&&!ct241.cgLost?CL_GameCommand():0;ct241.cgAck=paired;engine=CT241_Dump();ct241.dumped=engine;
  Com_Printf("R241 trace %u export: engine=%d CG-command-ack=%d cg-lost=%d. CSV footers and QPC joins require offline validation; this is not a paired-proof claim.\n",ct241.id,engine,paired,ct241.cgLost);
 }else if(!Q_stricmp(op,"status")){
  Com_Printf("R241 trace id=%u active=%d rows=%d reason=%d overflow=%d invalid=%d cg-lost=%d\n",ct241.id,ct241.active,ct241.count,ct241.reason,ct241.overflow,ct241.invalid,ct241.cgLost);if(cgvm)CL_GameCommand();
 }else Com_Printf("r241_trace start|stop|status\n");
 done:ct241.busy=0;
}
static void CL_R241TraceInit(void){
 if(!ct241.initialized){memset(&ct241,0,sizeof(ct241));ct241.current=-1;ct241.initialized=1;}
 ct241Control=Cvar_Get("r241_trace_control","0",0);Cvar_Set("r241_trace_control","0");Cmd_AddCommand("r241_trace",CL_R241TraceCommand);
}
static void CL_R241TraceShutdown(void){CL_R241TraceMapReset();Cmd_RemoveCommand("r241_trace");}
