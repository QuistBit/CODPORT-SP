#ifndef COD1_CLIENT_TRACE_R241_H
#define COD1_CLIENT_TRACE_R241_H
/* Diagnostic only. No gameplay state, message field or VM syscall is added. */
enum {
 CT241_CAP_ENTER, CT241_CAP_EXIT, CT241_SV_ENTER, CT241_SV_EXIT,
 CT241_END_ENTER, CT241_END_EXIT, CT241_SWAP_ENTER, CT241_SWAP_EXIT,
 CT241_STAGE_COUNT
};
void CL_R241TraceFrameBegin(int frame);
void CL_R241TraceFrameEnd(void);
void CL_R241TraceAbort(void);
void CL_R241TraceMark(int stage);
void CL_R241TraceServerBudget(int budget, int residual, int tickMsec);
void CL_R241TraceCGameBegin(int stereo);
void CL_R241TraceCGameEnd(void);
void CL_R241TraceMapReset(void);
void CL_R241TracePresent(int interval,int extension,int stereo,int front);
void CL_R241TraceSwapIntervalResult(int result);
int R241TraceRendererSmpActive(void);
#endif
