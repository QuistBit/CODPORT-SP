/* Paired loop-FX consumer. A record is not a renderer handle. Exact resource
 * owner and allocation key survive callbacks; replacement never adopts stale
 * work. Native centity+174 clock survives slot reuse; full map reset clears it.
 * GAME producer/VM creation and deletion gates must close before activation. */
#include "../game/bg_cod1_loop_fx.h"
static struct {
 uint64_t incarnation,sequence,entity_epoch;
 int busy,cancelled,fault;
 unsigned reset_depth;
 struct {uint32_t generation;int live,last;cod1LoopFxValue_t value;} slots[1024];
} cgCod1LoopFx;
typedef struct {cod1ScriptFxClientLease_t resource;uint64_t incarnation,sequence,entity_epoch;uint32_t slot,generation;} cod1LoopFxLease_t;
static void CFL_Reset(qboolean full){
 int n;if(cgCod1LoopFx.reset_depth<UINT_MAX)cgCod1LoopFx.reset_depth++;
 CFS_Advance(&cgCod1LoopFx.incarnation);if(cgCod1LoopFx.busy)cgCod1LoopFx.cancelled=1;
 if(full){cgCod1LoopFx.fault=0;cgCod1LoopFx.sequence=0;cgCod1LoopFx.entity_epoch=0;memset(cgCod1LoopFx.slots,0,sizeof(cgCod1LoopFx.slots));}
 else for(n=0;n<1024;n++)cgCod1LoopFx.slots[n].live=0;
}
static void CFL_ResetEnd(void){if(cgCod1LoopFx.reset_depth)cgCod1LoopFx.reset_depth--;}
static int CFL_Current(void *p){const cod1LoopFxLease_t *l=p;
 return l&&l->incarnation&&!cgCod1LoopFx.fault&&!cgCod1LoopFx.reset_depth&&!cgCod1LoopFx.cancelled&&l->incarnation==cgCod1LoopFx.incarnation&&l->sequence==cgCod1LoopFx.sequence&&
 l->entity_epoch==cgCod1LoopFx.entity_epoch&&l->slot<1024&&cgCod1LoopFx.slots[l->slot].live==COD1_LOOP_FX_VISIBLE&&
 l->generation==cgCod1LoopFx.slots[l->slot].generation&&CFS_Current(&l->resource);
}
void CG_Cod1LoopFxCommand(void){
 char copied[COD1_LOOP_FX_ARGC][24];const char *args[COD1_LOOP_FX_ARGC];cod1LoopFxSnapshot_t s;
 cod1ScriptFxClientLease_t resource;cod1ScriptFxResource_t name;uint64_t incarnation;int i,count;
 if(cgCod1LoopFx.reset_depth)return;
 if(cgCod1LoopFx.busy){cgCod1LoopFx.cancelled=cgCod1LoopFx.fault=1;return;}
 if(cgCod1LoopFx.fault)return;
 cgCod1LoopFx.busy=1;cgCod1LoopFx.cancelled=0;CFS_SynchronizeGeneration();resource=CFS_Lease(0);incarnation=cgCod1LoopFx.incarnation;
 count=trap_Argc();if(!CFS_Current(&resource)||count!=COD1_LOOP_FX_ARGC)goto done;
 for(i=0;i<count;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));args[i]=copied[i];if(!CFS_Current(&resource)||cgCod1LoopFx.cancelled||incarnation!=cgCod1LoopFx.incarnation)goto done;}
 if(!CLF_Decode(count,args,&s)||s.epoch!=resource.epoch||s.sequence<=cgCod1LoopFx.sequence||
  (cgCod1LoopFx.entity_epoch&&s.entity_epoch!=cgCod1LoopFx.entity_epoch)||s.generation<cgCod1LoopFx.slots[s.slot].generation)goto done;
 /* A retired generation cannot be resurrected by a newer reliable sequence. */
 if(s.live==COD1_LOOP_FX_VISIBLE&&s.generation==cgCod1LoopFx.slots[s.slot].generation&&!cgCod1LoopFx.slots[s.slot].live)goto done;
 if(s.live==COD1_LOOP_FX_HIDDEN&&(s.generation!=cgCod1LoopFx.slots[s.slot].generation||!cgCod1LoopFx.slots[s.slot].live))goto done;
 /* Unknown newer tombstones also retire a generation whose create was refused
  * (for example missing resource); a delayed create cannot resurrect it. */
 if(s.live==COD1_LOOP_FX_VISIBLE&&(!CFS_ReadResource(s.value.index,&name,&resource)||
   (cgCod1ScriptFx.resource[s.value.index].name[0]&&strcmp(name.name,cgCod1ScriptFx.resource[s.value.index].name))))goto done;
 if(cgCod1LoopFx.fault||cgCod1LoopFx.cancelled||incarnation!=cgCod1LoopFx.incarnation||!CFS_Current(&resource))goto done;
 cgCod1LoopFx.entity_epoch=s.entity_epoch;cgCod1LoopFx.sequence=s.sequence;
 cgCod1LoopFx.slots[s.slot].generation=s.generation;cgCod1LoopFx.slots[s.slot].live=s.live;
 if(s.live!=COD1_LOOP_FX_HIDDEN)cgCod1LoopFx.slots[s.slot].value=s.value;
done:cgCod1LoopFx.busy=0;
}
void CG_Cod1LoopFxFrame(void){
 cod1LoopFxLease_t lease;cod1ScriptFxResource_t resource;cod1LoopFxValue_t value;int i,last,emit,id;
 if(cgCod1LoopFx.reset_depth)return;
 /* Dropping a nested retire/update could leave an unowned effect running.
  * Refuse the channel until a full scene boundary, never clear it next frame. */
 if(cgCod1LoopFx.busy){cgCod1LoopFx.cancelled=cgCod1LoopFx.fault=1;return;}
 if(cgCod1LoopFx.fault)return;
 cgCod1LoopFx.busy=1;cgCod1LoopFx.cancelled=0;CFS_SynchronizeGeneration();
 for(i=0;i<1024;i++)if(cgCod1LoopFx.slots[i].live==COD1_LOOP_FX_VISIBLE){
  memset(&lease,0,sizeof(lease));lease.resource=CFS_Lease(0);lease.incarnation=cgCod1LoopFx.incarnation;lease.sequence=cgCod1LoopFx.sequence;lease.entity_epoch=cgCod1LoopFx.entity_epoch;lease.slot=i;lease.generation=cgCod1LoopFx.slots[i].generation;
  if(!CFL_Current(&lease))break;
  value=cgCod1LoopFx.slots[i].value;last=cgCod1LoopFx.slots[i].last;
  emit=CLF_Step(&value,cg.time,&last,cg.refdef.vieworg);if(emit<0)continue;
  cgCod1LoopFx.slots[i].last=last;if(!emit)continue;
  /* Cadence is committed even if the resource/renderer is unavailable, just
   * as a native distance-cull or disabled visual does not backlog emissions. */
  if(!CFS_ReadResource(value.index,&resource,&lease.resource)||!CFL_Current(&lease))continue;
  id=CFS_Acquire(&resource);if(!CFL_Current(&lease))break;
  lease.resource=CFS_Lease(value.index);if(!id||!CFL_Current(&lease))continue;
  (void)CG_Cod1EfxScriptPlay(id,value.origin,value.direction[0]!=0||value.direction[1]!=0||value.direction[2]!=0,value.direction,CFL_Current,&lease);
  if(!CFL_Current(&lease))break;
 }
 cgCod1LoopFx.busy=0;
}
