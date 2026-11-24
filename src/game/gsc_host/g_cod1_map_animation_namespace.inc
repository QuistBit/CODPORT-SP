/* Copied declarations from the winning VFS. Playback and clip sampling remain
 * separate capabilities; no actor or fake animdata is constructed here. */
#include "../cod1_gsc/move-delta-profile.h"
static int GscMapAnimationNamespace(char*error,size_t size){
 static const char*names[]={"generic_human","animation_rig_chessgame","pathfinder_treeguy","animation_rig_parachute","c47","germantruck"};
 GscSessionMapAnimationSelection selection;void*bytes[6]={0};size_t length;char path[128];int i,ok=0;
 memset(&selection,0,sizeof(selection));selection.names.count=6;
 selection.map_certificate.receipt=G_Cod1EntityMapEpoch();selection.map_certificate.current=GscCampaignProgramCurrent;
 for(i=0;i<6;i++){
  strcpy(selection.names.names[i],names[i]);Com_sprintf(path,sizeof(path),"animtrees/%s.atr",names[i]);
  if(!GscMapResourceRead(path,131072,&bytes[i],&length,error,size))goto done;
  selection.catalogs[i].atr_bytes=bytes[i];selection.catalogs[i].atr_size=length;
 }
 if(!GscMapPoseResources(&selection,error,size)||!GscTestModuleReadCurrent()||!gsc_session_select_map_animation_namespace(gscTestSession,&selection,error,size)||!gsc_session_select_map_start_pose(gscTestSession,&gscMapPoseHost,error,size)||!gsc_session_select_map_animation_length(gscTestSession,&gscMapLengthHost,error,size)||!GscTestModuleReadCurrent())goto done;
 if(!GscMapDeveloperSelect(error,size)||!gsc_session_select_map_quoted_strings(gscTestSession,error,size)||!(gscTestPreparingEarly?R243GameMoveDeltaPrepare():R243GameMoveDeltaPrepareMap(&selection.map_certificate))||!GscTestModuleReadCurrent()||!gsc_session_select_r243_move_delta(gscTestSession,R243GameMoveDeltaHost(),error,size)||!GscTestModuleReadCurrent())goto done;
 G_Printf("[Campaign] map animation declarations selected: six original ATR catalogs; GetMoveDelta21 resources\n");ok=1;
done:
 for(i=0;i<6;i++)if(bytes[i])COD1_GSC_VFS_FREE(bytes[i]);return ok;
}
