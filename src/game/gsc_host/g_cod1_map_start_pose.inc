#include "../../qcommon/cod1_animation_start_pose14.h"
#include "../../qcommon/cod1_actor_root14.h"
typedef struct {char tree[64];GscAnimationCatalogEntry declaration;c1ar14Sample_t sample;}GscMapPoseRow;
typedef struct {uint64_t epoch,serial;GscSession*session;GscMapPoseRow rows[5];}GscMapPoseBank;
static GscMapPoseBank gscMapPoseBank;
static uint64_t gscMapPoseSerial;static int gscMapPoseBusy,gscMapPoseCancelled;
static void GscMapPoseReset(void){if(gscMapPoseBusy)gscMapPoseCancelled=1;memset(&gscMapPoseBank,0,sizeof(gscMapPoseBank));}
static int GscMapPosePrivate(const void*p,size_t n){return C1V14_Overlap(p,n,&gscMapPoseBank,sizeof(gscMapPoseBank))||C1V14_Overlap(p,n,&gscMapPoseSerial,sizeof(gscMapPoseSerial))||C1V14_Overlap(p,n,&gscMapPoseBusy,sizeof(gscMapPoseBusy))||C1V14_Overlap(p,n,&gscMapPoseCancelled,sizeof(gscMapPoseCancelled))||GscVehiclePrivate(p,n);}
static int GscMapPoseState(uint64_t epoch,GscSession*session){return gscMapPoseBusy&&!gscMapPoseCancelled&&gscTestSession==session&&session&&epoch==gscTestMapEpoch&&GscCampaignProgramCurrent(epoch)&&GscTestModuleReadCurrent()&&gscMapPoseBusy&&!gscMapPoseCancelled&&gscTestSession==session;}
static int GscMapResourceRead(const char*path,size_t limit,void**out,size_t*outSize,char*error,size_t size){
 fileHandle_t file=0;int length,got;void*bytes;*out=NULL;*outSize=0;
 if(!GscTestModuleReadCurrent())return 0;
 length=trap_FS_FOpenFile(path,&file,FS_READ);
 if(length<=0||!file||(size_t)length>limit){if(file)trap_FS_FCloseFile(file);return 0;}
 bytes=COD1_GSC_VFS_CALLOC((size_t)length,1);if(!bytes){trap_FS_FCloseFile(file);return 0;}
 got=trap_FS_ReadCount(bytes,length,file);trap_FS_FCloseFile(file);
 if(got!=length||!GscTestModuleReadCurrent()){COD1_GSC_VFS_FREE(bytes);GscTestError(error,size,"map animation full VFS read refused");return 0;}
 *out=bytes;*outSize=length;return 1;
}
static int GscMapPoseResources(const GscSessionMapAnimationSelection*selection,char*error,size_t size){
 static const char*names[5]={"player_landing_roll","parachute_landing_roll","parachute_landing_firm","airborne_landing_roll","airborne_landing_firm"};
 GscMapPoseBank stage;GscAnimationCatalog*catalog=NULL;c1ar14Root_t*root=NULL;void*bytes=NULL;size_t length;char path[128];unsigned i,n,tree;int found,ok=0;
 if(gscMapPoseBusy){gscMapPoseCancelled=1;return 0;}if(gscMapPoseSerial==UINT64_MAX||!selection||selection->names.count<1||selection->names.count>GSC_MAP_ANIMATION_CATALOGS)return 0;
 memset(&stage,0,sizeof(stage));stage.epoch=G_Cod1EntityMapEpoch();stage.session=gscTestSession;gscMapPoseBusy=1;gscMapPoseCancelled=0;
 if(!GscMapPoseState(stage.epoch,stage.session))goto done;
 for(i=0;i<5;i++){
  for(tree=0;tree<selection->names.count;tree++)if(!strcmp(selection->names.names[tree],i<3?"animation_rig_parachute":"generic_human"))break;
  if(tree==selection->names.count)goto done;
  strcpy(stage.rows[i].tree,selection->names.names[tree]);
  if(strcmp(stage.rows[i].tree,i<3?"animation_rig_parachute":"generic_human"))goto done;
  catalog=gsc_animation_catalog_parse(stage.rows[i].tree,selection->catalogs[tree].atr_bytes,selection->catalogs[tree].atr_size,error,size);if(!catalog)goto done;
  found=0;for(n=0;n<gsc_animation_catalog_count(catalog);n++){GscAnimationCatalogEntry entry;if(!gsc_animation_catalog_entry(catalog,n,&entry))goto done;if(!strcmp(entry.name,names[i])){if(found||entry.flags)goto done;stage.rows[i].declaration=entry;found=1;}}
  gsc_animation_catalog_free(catalog);catalog=NULL;if(!found)goto done;
  Com_sprintf(path,sizeof(path),"xanim/%s",names[i]);if(!GscMapResourceRead(path,16777216,&bytes,&length,error,size)||!GscMapPoseState(stage.epoch,stage.session)||!C1AR14_Decode(bytes,length,&root)||!C1AR14_SampleRaw(root,0,&stage.rows[i].sample))goto done;
  COD1_GSC_VFS_FREE(bytes);bytes=NULL;C1AR14_Free(root);root=NULL;
 }
 if(!GscMapPoseState(stage.epoch,stage.session))goto done;
 stage.serial=++gscMapPoseSerial;gscMapPoseBank=stage;ok=1;
done:
 if(bytes)COD1_GSC_VFS_FREE(bytes);C1AR14_Free(root);gsc_animation_catalog_free(catalog);gscMapPoseBusy=0;
 if(!ok)GscTestError(error,size,"map start-pose resources unavailable");return ok;
}
static int GscMapPoseQuery(void*user,const GscStartOriginRequest*input,GscStartOriginReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscStartOriginRequest request;GscStartOriginReply result;GscMapPoseRow row;uint64_t epoch,serial;GscSession*session;unsigned i;int ok=0,found=0,time;
 (void)user;if(gscMapPoseBusy){gscMapPoseCancelled=1;return 0;}
 if(!input||!out||!current||(cap&&!error)||GscMapPosePrivate(input,sizeof(*input))||GscMapPosePrivate(out,sizeof(*out))||GscMapPosePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),error,cap)||C1V14_Overlap(out,sizeof(*out),error,cap))return 0;
 request=*input;if(!GscVehicleMapCurrent(G_Cod1EntityMapEpoch())||!request.runtime_identity||!request.catalog_identity||request.now_ms<0||(request.query_kind!=1&&request.query_kind!=2)||request.flags||!request.ordinal||!memchr(request.tree,0,64)||!memchr(request.name,0,64))return 0;
 epoch=gscMapPoseBank.epoch;serial=gscMapPoseBank.serial;session=gscMapPoseBank.session;time=level.time;if(!epoch||!serial)return 0;
 for(i=0;i<5;i++)if(!strcmp(request.tree,gscMapPoseBank.rows[i].tree)&&!strcmp(request.name,gscMapPoseBank.rows[i].declaration.name)&&request.ordinal==gscMapPoseBank.rows[i].declaration.ordinal){row=gscMapPoseBank.rows[i];found=1;break;}
 if(!found)return 0;gscMapPoseBusy=1;gscMapPoseCancelled=0;
#define MAP_POSE_CURRENT (GscMapPoseState(epoch,session)&&gscMapPoseBank.epoch==epoch&&gscMapPoseBank.serial==serial&&level.time==time&&!memcmp(&gscMapPoseBank.rows[i],&row,sizeof(row))&&!memcmp(input,&request,sizeof(request)))
 if(!MAP_POSE_CURRENT||current(context)!=1||!MAP_POSE_CURRENT)goto done;
 if(!C1ASP14_Query(request.query_kind,request.origin,request.angles,row.sample.q2,row.sample.translation,result.vector))goto done;
 if(!MAP_POSE_CURRENT||current(context)!=1||!MAP_POSE_CURRENT)goto done;
 *out=result;ok=1;
done:
#undef MAP_POSE_CURRENT
 gscMapPoseBusy=0;if(!ok)GscTestError(error,cap,"map start-pose resource or caller retired");return ok;
}
static const GscStartOriginHost gscMapPoseHost={NULL,GscMapPoseQuery};
