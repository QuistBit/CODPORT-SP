/* Included once by cg_view.c. Native Windows diagnostic, no game behavior fix.
 * Clock/ring access is entirely local during capture; no syscall, FS or heap.
 * The manual control command alone uses existing cvar/FS/console syscalls. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdint.h>
#define CG241_ROWS 4096
#define CG241_U64 "%I64u"
enum {CG241_WORLD=1,CG241_VIEW=2,CG241_HUD_BEGIN=4,CG241_HUD_END=8,CG241_END=16};
enum {CG241_INPUT_TIME,CG241_TIME,CG241_OLD_ENTRY,CG241_DT_ENTRY,CG241_DT_EXIT,
 CG241_SNAP,CG241_NEXT,CG241_DRAW2D,CG241_CAMERA,CG241_PM,CG241_PFLAGS,CG241_SFLAGS,
 CG241_PHEAVY,CG241_SHEAVY,CG241_LOCK,CG241_LOCKENT,CG241_NEXT_TELE,CG241_THIS_TELE,
 CG241_ZOOM_BEFORE,CG241_ZOOM_AFTER,CG241_BINOC,CG241_ZOOMVAL,CG241_COMMAND_TIME,
 CG241_ERROR_TIME,CG241_STEP_TIME,CG241_STEREO,CG241_FRAME,CG241_BOB,CG241_PREDICT_BRANCH,CG241_INTS};
enum {CG241_LERP,CG241_ORIGIN,CG241_ANGLES=CG241_ORIGIN+3,
 CG241_VELOCITY=CG241_ANGLES+3,CG241_VIEWORG=CG241_VELOCITY+3,
 CG241_AXIS=CG241_VIEWORG+3,CG241_FOV=CG241_AXIS+9,
 CG241_ERROR=CG241_FOV+2,CG241_SENS=CG241_ERROR+3,CG241_SPEED,CG241_STEP_CHANGE,CG241_ZOOM_VALUE,CG241_FLOATS};
typedef struct {unsigned ordinal,mask;uint64_t t[6];int i[CG241_INTS];float f[CG241_FLOATS];} CG241Row;
static struct {
 int active,current,count,reason,overflow,invalid,busy,dumped;unsigned id,map,pid;
 uint64_t frequency,start,deadline,last;CG241Row rows[CG241_ROWS];
} cg241;
static unsigned cg241LastId,cg241Mutation=1;
static void CG241_Stop(int reason){cg241.active=0;if(!cg241.reason)cg241.reason=reason;}
static int CG241_Clock(uint64_t*out){LARGE_INTEGER q;
 if(!QueryPerformanceCounter(&q)||q.QuadPart<0||(uint64_t)q.QuadPart<cg241.start||(uint64_t)q.QuadPart<cg241.last){cg241.invalid=1;CG241_Stop(3);return 0;}
 cg241.last=(uint64_t)q.QuadPart;*out=cg241.last-cg241.start;return 1;
}
static CG241Row*CG241_Current(void){return cg241.active&&cg241.current>=0&&cg241.current<cg241.count?cg241.rows+cg241.current:NULL;}
void CG_R241TraceReset(void){if(cg241Mutation!=UINT_MAX)cg241Mutation++;memset(&cg241,0,sizeof(cg241));cg241.current=-1;}
void CG_R241TraceBegin(int time,int stereo){CG241Row*r;uint64_t now;
 if(!cg241.active)return;
 if(cg241.current>=0){cg241.invalid=1;CG241_Stop(6);return;}
 if(!CG241_Clock(&now))return;
 if(cg241.last>=cg241.deadline){CG241_Stop(1);return;}
 if(cg241.count==CG241_ROWS){cg241.overflow++;CG241_Stop(2);return;}
 cg241.current=cg241.count++;r=cg241.rows+cg241.current;memset(r,0,sizeof(*r));
 r->ordinal=(unsigned)cg241.current;r->t[0]=now;r->i[CG241_INPUT_TIME]=time;r->i[CG241_STEREO]=stereo;
 r->i[CG241_OLD_ENTRY]=cg.oldTime;r->i[CG241_DT_ENTRY]=cg.frametime;
}
void CG_R241TraceView(void){CG241Row*r=CG241_Current();uint64_t now;if(!r||!CG241_Clock(&now))return;r->t[1]=now;r->mask|=CG241_VIEW;}
static void CG241_State(CG241Row*r){int k,j;
 r->i[CG241_TIME]=cg.time;r->i[CG241_DT_EXIT]=cg.frametime;
 r->i[CG241_SNAP]=cg.snap?cg.snap->serverTime:-1;r->i[CG241_NEXT]=cg.nextSnap?cg.nextSnap->serverTime:-1;
 r->i[CG241_DRAW2D]=cg_draw2D.integer;r->i[CG241_CAMERA]=cg.cameraMode;r->i[CG241_PM]=cg.predictedPlayerState.pm_type;
 r->i[CG241_PFLAGS]=cg.predictedPlayerState.eFlags;r->i[CG241_SFLAGS]=cg.snap?cg.snap->ps.eFlags:0;
 r->i[CG241_PHEAVY]=cg.predictedPlayerState.persistant[PERS_HWEAPON_USE];r->i[CG241_SHEAVY]=cg.snap?cg.snap->ps.persistant[PERS_HWEAPON_USE]:0;
 r->i[CG241_LOCK]=cg.predictedPlayerState.viewlocked;r->i[CG241_LOCKENT]=cg.predictedPlayerState.viewlocked_entNum;
 r->i[CG241_NEXT_TELE]=cg.nextFrameTeleport;r->i[CG241_THIS_TELE]=cg.thisFrameTeleport;
 r->i[CG241_BINOC]=cg.zoomedBinoc;r->i[CG241_ZOOMVAL]=(int)cg.zoomval;
 r->i[CG241_COMMAND_TIME]=cg.predictedPlayerState.commandTime;r->i[CG241_ERROR_TIME]=cg.predictedErrorTime;
 r->i[CG241_STEP_TIME]=cg.cod1StepTime;r->i[CG241_FRAME]=cg.clientFrame;r->i[CG241_BOB]=cg.predictedPlayerState.bobCycle;
 r->i[CG241_PREDICT_BRANCH]=(cg.demoPlayback||(cg.snap&&(cg.snap->ps.pm_flags&PMF_FOLLOW)))?1:(cg_nopredict.integer||cg_synchronousClients.integer||(cg.snap&&(cg.snap->ps.eFlags&EF_MG42_ACTIVE)))?2:0;
 r->f[CG241_LERP]=cg.frameInterpolation;
 for(k=0;k<3;k++){r->f[CG241_ORIGIN+k]=cg.predictedPlayerState.origin[k];r->f[CG241_ANGLES+k]=cg.predictedPlayerState.viewangles[k];
  r->f[CG241_VELOCITY+k]=cg.predictedPlayerState.velocity[k];r->f[CG241_VIEWORG+k]=cg.refdef.vieworg[k];r->f[CG241_ERROR+k]=cg.predictedError[k];
  for(j=0;j<3;j++)r->f[CG241_AXIS+k*3+j]=cg.refdef.viewaxis[k][j];}
 r->f[CG241_FOV]=cg.refdef.fov_x;r->f[CG241_FOV+1]=cg.refdef.fov_y;r->f[CG241_SENS]=cg.zoomSensitivity;r->f[CG241_SPEED]=cg.xyspeed;r->f[CG241_STEP_CHANGE]=cg.cod1StepChange;r->f[CG241_ZOOM_VALUE]=cg.zoomval;
}
void CG_R241TraceWorld(void){CG241Row*r=CG241_Current();uint64_t now;if(!r||!CG241_Clock(&now))return;r->t[2]=now;r->mask|=CG241_WORLD;CG241_State(r);}
void CG_R241TraceHudBegin(void){CG241Row*r=CG241_Current();uint64_t now;if(!r||!CG241_Clock(&now))return;r->t[3]=now;r->mask|=CG241_HUD_BEGIN;r->i[CG241_ZOOM_BEFORE]=cg.zoomTime;}
void CG_R241TraceHudEnd(void){CG241Row*r=CG241_Current();uint64_t now;if(!r||!CG241_Clock(&now))return;r->t[4]=now;r->mask|=CG241_HUD_END;r->i[CG241_ZOOM_AFTER]=cg.zoomTime;}
void CG_R241TraceEnd(void){CG241Row*r=CG241_Current();uint64_t now;
 if(!r||!CG241_Clock(&now))return;r->t[5]=now;r->mask|=CG241_END;if(!(r->mask&CG241_WORLD))CG241_State(r);
 cg241.current=-1;if(cg241.last>=cg241.deadline)CG241_Stop(1);
}
static int CG241_Control(int starting,unsigned*id,unsigned*map,unsigned*pid,uint64_t*frequency,uint64_t*start,uint64_t*deadline){char text[192],tail;LARGE_INTEGER f,q;
 memset(text,0,sizeof(text));trap_Cvar_VariableStringBuffer("r241_trace_control",text,sizeof(text));
 if(sscanf(text,"%u %u %u "CG241_U64" "CG241_U64" "CG241_U64" %c",id,map,pid,frequency,start,deadline,&tail)!=6||!*id||!*map||!*pid||*pid!=(unsigned)GetCurrentProcessId()||!*frequency||*frequency>INT64_MAX/20000||*deadline<=*start||*deadline-*start>*frequency*20)return 0;
 if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0||(uint64_t)f.QuadPart!=*frequency||!QueryPerformanceCounter(&q)||q.QuadPart<0||(uint64_t)q.QuadPart<*start||(starting&&(uint64_t)q.QuadPart>=*deadline))return 0;return 1;
}
static int CG241_ExportCurrent(unsigned generation,unsigned id){return cg241Mutation==generation&&cg241.id==id&&!cg241.active&&cg241.busy;}
static int CG241_Write(fileHandle_t file,const char*line,unsigned generation,unsigned id){trap_FS_Write(line,(int)strlen(line),file);return CG241_ExportCurrent(generation,id);}
static int CG241_Dump(void){char path[80],line[2048];fileHandle_t file=0;int n,k,at,exists,ok=1;unsigned generation=cg241Mutation,id=cg241.id;
 if(cg241.active||!cg241.id)return 0;if(cg241.dumped)return 1;
 Com_sprintf(path,sizeof(path),"r241/trace-%u-"CG241_U64"-%08u-cgame.csv",cg241.pid,cg241.start,cg241.id);
 exists=trap_FS_FOpenFile(path,&file,FS_READ);if(file)trap_FS_FCloseFile(file);file=0;
 if(exists>=0||!CG241_ExportCurrent(generation,id)){CG_Printf("R241 CG existing/stale export refused, never overwritten: %s\n",path);return 0;}
 trap_FS_FOpenFile(path,&file,FS_WRITE);if(!file)return 0;if(!CG241_ExportCurrent(generation,id)){trap_FS_FCloseFile(file);return 0;}
 Com_sprintf(line,sizeof(line),"#id=%u,map=%u,pid=%u,frequency="CG241_U64",start="CG241_U64",deadline="CG241_U64",count=%d,reason=%d,overflow=%d,invalid=%d,write_status=issued_not_verified\n",cg241.id,cg241.map,cg241.pid,cg241.frequency,cg241.start,cg241.deadline,cg241.count,cg241.reason,cg241.overflow,cg241.invalid);ok=CG241_Write(file,line,generation,id);if(!ok){trap_FS_FCloseFile(file);return 0;}
 strcpy(line,"ordinal,mask,begin,view,world,hud_begin,hud_end,end,input_time,cg_time,old_entry,dt_entry,dt_exit,snap_time,next_time,draw2d,camera,pm_type,pred_flags,snap_flags,pred_heavy,snap_heavy,viewlocked,viewlocked_entity,next_tele,this_tele,zoom_before,zoom_after,binoc,zoomval_integer,command_time,error_time,step_time,stereo,cg_frame,bob_cycle,prediction_branch,interpolation,origin_x,origin_y,origin_z,angle_x,angle_y,angle_z,velocity_x,velocity_y,velocity_z,view_x,view_y,view_z,axis00,axis01,axis02,axis10,axis11,axis12,axis20,axis21,axis22,fov_x,fov_y,error_x,error_y,error_z,sensitivity,bob_speed,step_change,zoomval\n");ok=CG241_Write(file,line,generation,id);if(!ok){trap_FS_FCloseFile(file);return 0;}
 for(n=0;n<cg241.count;n++){CG241Row*r=cg241.rows+n;Com_sprintf(line,sizeof(line),"%u,%u",r->ordinal,r->mask);at=(int)strlen(line);
  for(k=0;k<6;k++){Com_sprintf(line+at,sizeof(line)-at,","CG241_U64,r->t[k]);at=(int)strlen(line);}
  for(k=0;k<CG241_INTS;k++){Com_sprintf(line+at,sizeof(line)-at,",%d",r->i[k]);at=(int)strlen(line);}
  for(k=0;k<CG241_FLOATS;k++){Com_sprintf(line+at,sizeof(line)-at,",%.9g",(double)r->f[k]);at=(int)strlen(line);}
  Com_sprintf(line+at,sizeof(line)-at,"\n");ok=CG241_Write(file,line,generation,id);if(!ok)break;
 }
 if(ok){Com_sprintf(line,sizeof(line),"#END,id=%u,rows=%d\n",cg241.id,cg241.count);ok=CG241_Write(file,line,generation,id);}trap_FS_FCloseFile(file);ok=ok&&CG241_ExportCurrent(generation,id);if(ok)cg241.dumped=1;return ok;
}
qboolean CG_R241TraceCommand(void){const char*op=CG_Argv(1);unsigned id,map,pid;uint64_t frequency,start,deadline;int ok=0;
 if(cg241.busy)return qfalse;cg241.busy=1;
 if(trap_Argc()!=2)goto done;
 if(!Q_stricmp(op,"start")){
  if(cg241Mutation==UINT_MAX||cg241.active||cg241.current>=0||!CG241_Control(1,&id,&map,&pid,&frequency,&start,&deadline)||id<=cg241LastId)goto done;
  memset(&cg241,0,sizeof(cg241));cg241.busy=1;cg241.current=-1;cg241.id=cg241LastId=id;cg241.map=map;cg241.pid=pid;
  cg241.frequency=frequency;cg241.start=cg241.last=start;cg241.deadline=deadline;cg241.active=1;ok=1;
 }else if(!Q_stricmp(op,"stop")){
  /* Freeze before any provider/IO callback, including failure cases. */
  CG241_Stop(5);cg241.current=-1;
  if(!CG241_Control(0,&id,&map,&pid,&frequency,&start,&deadline)||id!=cg241.id||map!=cg241.map||pid!=cg241.pid||frequency!=cg241.frequency||start!=cg241.start||deadline!=cg241.deadline){cg241.invalid=1;goto done;}
  ok=CG241_Dump();
 }else if(!Q_stricmp(op,"status")){
  CG_Printf("R241 CG id=%u active=%d rows=%d reason=%d overflow=%d invalid=%d\n",cg241.id,cg241.active,cg241.count,cg241.reason,cg241.overflow,cg241.invalid);ok=1;
 }
 done:cg241.busy=0;return ok?qtrue:qfalse;
}
