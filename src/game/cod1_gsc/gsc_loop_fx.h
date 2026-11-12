/* Opt-in native playLoopedFx transaction. GAME owns allocation/link and exact
 * rollback. A VM operation is only a transaction key, never entity authority.
 * finish(0) must cancel that operation even after current() has become false;
 * runtime is the non-reused VM array identity; key receipts by (runtime,
 * operation) and the real host Session/map owner. It must never free an entity merely supplied in a borrowed reply. The host
 * must keep a private allocation receipt and make cancel idempotent. Published
 * entities remain host-owned until delete or map/Session retirement. */
#ifndef GSC_LOOP_FX_H
#define GSC_LOOP_FX_H
#include "../bg_cod1_loop_fx.h"
typedef struct {uint64_t runtime,operation;int now_ms;cod1LoopFxValue_t value;} GscLoopFxRequest;
typedef struct {uint64_t operation;GscEntityRef entity;} GscLoopFxTicket;
typedef struct {void *context;int (*current)(void *);} GscLoopFxServices;
typedef struct {
 void *user;
 int (*prepare)(void *,const GscLoopFxRequest *,GscLoopFxTicket *,const GscLoopFxServices *,char *,size_t);
 int (*finish)(void *,uint64_t runtime,uint64_t operation,int publish,const GscLoopFxServices *,char *,size_t);
} GscLoopFxHost;
int gsc_program_select_loop_fx(GscProgram *);
int gsc_runtime_select_loop_fx(GscRuntime *,const GscLoopFxHost *);
#endif
