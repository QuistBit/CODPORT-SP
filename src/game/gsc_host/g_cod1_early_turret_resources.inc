#include "../../qcommon/cod1_exact_bytes_r242.h"
/* Independent immutable DATA intake before BSP entity allocation. These bytes
 * do not certify any loaded clip, native tree, live gun or Actor. */
static int Cod1EarlyTurretScope(uint64_t epoch,uint64_t prepass){
 return Cod1ActorProgramCurrent(NULL,epoch,prepass)&&!level.restarted;
}
static int Cod1EarlyTurretCurrent(void*user,uint64_t epoch,uint64_t receipt){
 uint64_t prepass;int feature,developer,ok;unsigned i;(void)user;
 if(!receipt||epoch!=cod1EarlyTurretResources.epoch||receipt!=cod1EarlyTurretResources.receipt||
    !cod1EarlyTurretResources.ready||cod1EarlyTurretResources.cancelled)return 0;
 if(cod1EarlyTurretResources.busy){cod1EarlyTurretResources.cancelled=1;return 0;}
 prepass=cod1EarlyTurretResources.prepass;if(!Cod1EarlyTurretScope(epoch,prepass))return 0;
 cod1EarlyTurretResources.busy=1;
 feature=trap_Cvar_VariableIntegerValue("g_cod1GscR232Turret");
 developer=trap_Cvar_VariableIntegerValue("developer_script");
 ok=feature==1&&developer==cod1EarlyTurretResources.developer&&
  epoch==cod1EarlyTurretResources.epoch&&receipt==cod1EarlyTurretResources.receipt&&
  prepass==cod1EarlyTurretResources.prepass&&Cod1EarlyTurretScope(epoch,prepass)&&
  cod1EarlyTurretResources.ready&&!cod1EarlyTurretResources.cancelled;
 for(i=0;i<2&&ok;i++)ok=cod1EarlyTurretResources.sizes[i]&&cod1EarlyTurretResources.sizes[i]<=COD1_EARLY_ATR_MAX&&
  (cod1EarlyTurretSeals[i].base ?
   Cod1ATRSealCurrent(cod1EarlyTurretSeals+i,cod1EarlyTurretSeals[i].base,
     cod1EarlyTurretSeals[i].base+cod1EarlyTurretResources.sizes[i],cod1EarlyTurretResources.sizes[i]) :
   R242ExactATRBytesEqual(cod1EarlyTurretResources.bytes[i],cod1EarlyTurretResources.reference[i],cod1EarlyTurretResources.sizes[i]));
 if(!ok)cod1EarlyTurretResources.cancelled=1;cod1EarlyTurretResources.busy=0;return ok;
}
static int Cod1EarlyTurretLoading(uint64_t epoch,uint64_t prepass){
 return Cod1EarlyTurretScope(epoch,prepass)&&level.spawning&&!cod1ActorPrefix.placedRows&&
  cod1EarlyTurretResources.busy&&!cod1EarlyTurretResources.cancelled&&!cod1EarlyTurretResources.ready;
}
static int Cod1EarlyTurretLoad(void){
 static const char*const names[2]={"generic_human","mg42"};
 unsigned char*bytes[2]={NULL,NULL};size_t sizes[2]={0,0};GscAnimationCatalog*c=NULL;
 uint64_t epoch=cod1EarlyProgram.epoch,prepass=cod1EarlyProgram.receipt;
 fileHandle_t file=0;char path[128],why[256];int n,length,feature,developer,ok=0;
 if(!Cod1EarlyTurretScope(epoch,prepass)||!level.spawning||cod1ActorPrefix.placedRows||
    cod1EarlyTurretResources.ready||cod1EarlyTurretResources.busy||cod1EarlyTurretResources.cancelled||cod1EarlyTurretSerial==UINT64_MAX)return 0;
 cod1EarlyTurretResources.busy=1;
 feature=trap_Cvar_VariableIntegerValue("g_cod1GscR232Turret");
 if(!Cod1EarlyTurretLoading(epoch,prepass))goto done;
 if(feature==0){ok=1;goto done;}if(feature!=1)goto done;
 developer=trap_Cvar_VariableIntegerValue("developer_script");
 if(!Cod1EarlyTurretLoading(epoch,prepass)||(developer!=0&&developer!=1))goto done;
 for(n=0;n<2;n++){
  snprintf(path,sizeof(path),"animtrees/%s.atr",names[n]);length=trap_FS_FOpenFile(path,&file,FS_READ);
  if(!Cod1EarlyTurretLoading(epoch,prepass)||length<=0||length>COD1_EARLY_ATR_MAX||!file)goto done;
  sizes[n]=(size_t)length;bytes[n]=malloc(sizes[n]);if(!bytes[n])goto done;memset(bytes[n],0,sizes[n]);
  trap_FS_Read(bytes[n],length,file);trap_FS_FCloseFile(file);file=0;
  if(!Cod1EarlyTurretLoading(epoch,prepass))goto done;
  c=gsc_animation_catalog_parse(names[n],bytes[n],sizes[n],why,sizeof(why));if(!c||!gsc_animation_catalog_count(c))goto done;
  gsc_animation_catalog_free(c);c=NULL;
 }
 if(trap_Cvar_VariableIntegerValue("g_cod1GscR232Turret")!=1||
    trap_Cvar_VariableIntegerValue("developer_script")!=developer||!Cod1EarlyTurretLoading(epoch,prepass))goto done;
 for(n=0;n<2;n++){
  memcpy(cod1EarlyTurretResources.bytes[n],bytes[n],sizes[n]);memcpy(cod1EarlyTurretResources.reference[n],bytes[n],sizes[n]);cod1EarlyTurretResources.sizes[n]=sizes[n];
 }
 /* The sealed pair becomes authoritative before its address or receipt is
  * published. Failed protection retains the original exact byte checks.
  * Dynamic map, feature, developer and cancellation checks stay per call. */
 for(n=0;n<2;n++)Cod1ATRSealCreate(cod1EarlyTurretSeals+n,bytes[n],sizes[n]);
 cod1EarlyTurretResources.epoch=epoch;cod1EarlyTurretResources.prepass=prepass;
 cod1EarlyTurretResources.receipt=++cod1EarlyTurretSerial;cod1EarlyTurretResources.developer=developer;cod1EarlyTurretResources.ready=1;ok=1;
done:
 if(file)trap_FS_FCloseFile(file);gsc_animation_catalog_free(c);for(n=0;n<2;n++)free(bytes[n]);
 if(!ok){cod1EarlyTurretResources.ready=0;cod1EarlyTurretResources.cancelled=1;}
 cod1EarlyTurretResources.busy=0;return ok;
}
int G_Cod1EarlyTurretProgramPlan(GscSessionTurretProgramPlan*out){
 GscSessionTurretProgramPlan plan;int i;
 if(!out||Cod1ActorPrefixPrivate(out,sizeof(*out)))return 0;
 if(!Cod1EarlyTurretCurrent(NULL,cod1EarlyTurretResources.epoch,cod1EarlyTurretResources.receipt))return 0;
 memset(&plan,0,sizeof(plan));plan.epoch=cod1EarlyTurretResources.epoch;plan.receipt=cod1EarlyTurretResources.receipt;
 plan.current=Cod1EarlyTurretCurrent;plan.developer_enabled=cod1EarlyTurretResources.developer;
 for(i=0;i<2;i++){plan.catalogs[i].atr_bytes=cod1EarlyTurretSeals[i].base?cod1EarlyTurretSeals[i].base:cod1EarlyTurretResources.bytes[i];plan.catalogs[i].atr_size=cod1EarlyTurretResources.sizes[i];}
 *out=plan;return 1;
}
