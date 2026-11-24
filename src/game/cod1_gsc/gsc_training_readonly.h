/* Private Training setup. The public binders retain both callback fences.
 * The native binding names the synchronous GAME allocation validator and
 * the exact map, developer, animation and actor callbacks it admits. */
#ifndef GSC_TRAINING_READONLY_H
#define GSC_TRAINING_READONLY_H
void gsc_session_bind_scan(GscSession *,uint64_t,const GscEntityHost *,const GscReadOnlyContext *,int);
void gsc_runtime_bind_scan(GscRuntime *,const GscReadOnlyContext *,void *,
                           int (*)(void *,const GscEntityRef *),int);
#endif
