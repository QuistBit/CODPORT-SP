#ifndef CG_R241_FRAME_TRACE_H
#define CG_R241_FRAME_TRACE_H
void CG_R241TraceReset(void);
qboolean CG_R241TraceCommand(void);
void CG_R241TraceBegin(int time,int stereo);
void CG_R241TraceView(void);
void CG_R241TracePredictBegin(void);
void CG_R241TracePredictEnd(void);
void CG_R241TraceWorld(void);
void CG_R241TraceHudBegin(void);
void CG_R241TraceHudEnd(void);
void CG_R241TraceEnd(void);
#endif
