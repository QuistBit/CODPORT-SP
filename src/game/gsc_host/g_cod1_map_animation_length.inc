/* Metadata for actual linked leaf references, loaded before script execution.
 * No query performs VFS I/O, samples a pose, creates a tree or starts playback. */
#define GSC_MAP_LENGTH_CAPACITY 4096u
typedef struct {GscAnimationLengthRequest request;GscAnimationLengthReply value;}GscMapLengthRow;
typedef struct {uint64_t epoch,serial,id,program;GscSession*session;unsigned count;GscMapLengthRow rows[GSC_MAP_LENGTH_CAPACITY];}GscMapLengthBank;
static GscMapLengthBank gscMapLengthBank;
static GscMapLengthBank*gscMapLengthStage;
static uint64_t gscMapLengthSerial;
static int gscMapLengthBusy,gscMapLengthCancelled;
static void GscMapLengthReset(void){if(gscMapLengthBusy)gscMapLengthCancelled=1;memset(&gscMapLengthBank,0,sizeof(gscMapLengthBank));}
static int GscMapLengthPrivate(const void*p,size_t n){return C1V14_Overlap(p,n,&gscMapLengthBank,sizeof(gscMapLengthBank))||C1V14_Overlap(p,n,&gscMapLengthStage,sizeof(gscMapLengthStage))||C1V14_Overlap(p,n,gscMapLengthStage,gscMapLengthStage?sizeof(*gscMapLengthStage):0)||C1V14_Overlap(p,n,&gscMapLengthSerial,sizeof(gscMapLengthSerial))||C1V14_Overlap(p,n,&gscMapLengthBusy,sizeof(gscMapLengthBusy))||C1V14_Overlap(p,n,&gscMapLengthCancelled,sizeof(gscMapLengthCancelled))||GscMapPosePrivate(p,n);}
static int GscMapLengthState(uint64_t epoch,GscSession*session,uint64_t id){return gscMapLengthBusy&&!gscMapLengthCancelled&&epoch&&epoch==gscTestMapEpoch&&session&&session==gscTestSession&&id&&id==gscTestId&&GscVehicleMapCurrent(epoch)&&GscTestModuleReadCurrent()&&gscMapLengthBusy&&!gscMapLengthCancelled&&epoch==gscTestMapEpoch&&session==gscTestSession&&id==gscTestId;}
static int GscMapLengthResources(char*error,size_t cap){
 GscMapLengthBank*stage=NULL;GscAnimationLengthRequest q,again;c1ar14Root_t*root=NULL;void*bytes=NULL;size_t length;char path[128];unsigned i,j,unique=0;uint64_t epoch,id,program;GscSession*session;int status,ok=0;
 if(gscMapLengthBusy){gscMapLengthCancelled=1;return 0;}if(gscMapLengthSerial==UINT64_MAX)return 0;
 epoch=G_Cod1EntityMapEpoch();session=gscTestSession;id=gscTestId;program=gsc_session_prepared_program_identity(session,id);if(!epoch||!program)return 0;
 stage=calloc(1,sizeof(*stage));if(!stage)return 0;gscMapLengthStage=stage;gscMapLengthBusy=1;gscMapLengthCancelled=0;
 stage->epoch=epoch;stage->session=session;stage->id=id;stage->program=program;
#define MAP_LENGTH_LOAD_CURRENT (GscMapLengthState(epoch,session,id)&&gsc_session_prepared_program_identity(session,id)==program)
 if(!MAP_LENGTH_LOAD_CURRENT)goto done;
 for(i=0;i<=GSC_MAP_LENGTH_CAPACITY;i++){
  memset(&q,0,sizeof(q));status=gsc_session_map_animation_metadata_request(session,id,i,&q);
  if(status==0)break;if(status!=1||i==GSC_MAP_LENGTH_CAPACITY||!q.runtime_identity||!q.ordinal||q.flags||!memchr(q.tree,0,64)||!memchr(q.name,0,64)||!q.tree[0]||!q.name[0])goto done;
  for(j=0;q.name[j];j++)if(!((q.name[j]>='a'&&q.name[j]<='z')||(q.name[j]>='A'&&q.name[j]<='Z')||(q.name[j]>='0'&&q.name[j]<='9')||q.name[j]=='_'))goto done;
  if(i&&q.runtime_identity!=stage->rows[0].request.runtime_identity)goto done;
  for(j=0;j<i;j++)if(!strcmp(q.name,stage->rows[j].request.name))break;
  stage->rows[i].request=q;
  if(j<i)stage->rows[i].value=stage->rows[j].value;
  else{
   Com_sprintf(path,sizeof(path),"xanim/%s",q.name);
   if(!MAP_LENGTH_LOAD_CURRENT||!GscMapResourceRead(path,C1AR14_CLIP_LIMIT,&bytes,&length,error,cap)||!MAP_LENGTH_LOAD_CURRENT||!C1AR14_Decode(bytes,length,&root))goto done;
   stage->rows[i].value.frames=root->intervals;stage->rows[i].value.fps=root->fps;unique++;
   COD1_GSC_VFS_FREE(bytes);bytes=NULL;C1AR14_Free(root);root=NULL;
  }
  if(!MAP_LENGTH_LOAD_CURRENT||gsc_session_map_animation_metadata_request(session,id,i,&again)!=1||memcmp(&q,&again,sizeof(q)))goto done;
  stage->count=i+1;
 }
 if(!MAP_LENGTH_LOAD_CURRENT)goto done;
 stage->serial=++gscMapLengthSerial;gscMapLengthBank=*stage;ok=1;
 G_Printf("[Campaign] map animation length metadata ready: %u bindings, %u unique XAnim resources\n",stage->count,unique);
done:
#undef MAP_LENGTH_LOAD_CURRENT
 if(bytes)COD1_GSC_VFS_FREE(bytes);C1AR14_Free(root);free(stage);gscMapLengthStage=NULL;gscMapLengthBusy=0;
 if(!ok)GscTestError(error,cap,"map animation length resources unavailable or owner retired");return ok;
}
static int GscMapLengthQuery(void*user,const GscAnimationLengthRequest*input,GscAnimationLengthReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscAnimationLengthRequest q;GscMapLengthRow row;GscSession*session;uint64_t epoch,serial,id;unsigned i,count;int now,ok=0;(void)user;
 if(gscMapLengthBusy){gscMapLengthCancelled=1;return 0;}
 if(!input||!out||!current||(cap&&!error)||GscMapLengthPrivate(input,sizeof(*input))||GscMapLengthPrivate(out,sizeof(*out))||GscMapLengthPrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),error,cap)||C1V14_Overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;if(!q.runtime_identity||!q.ordinal||q.flags||!memchr(q.tree,0,64)||!memchr(q.name,0,64))return 0;
 epoch=gscMapLengthBank.epoch;serial=gscMapLengthBank.serial;session=gscMapLengthBank.session;id=gscMapLengthBank.id;count=gscMapLengthBank.count;now=level.time;
 if(!epoch||!serial||count>GSC_MAP_LENGTH_CAPACITY)return 0;
 for(i=0;i<count;i++)if(!memcmp(&q,&gscMapLengthBank.rows[i].request,sizeof(q)))break;
 if(i==count)return 0;row=gscMapLengthBank.rows[i];gscMapLengthBusy=1;gscMapLengthCancelled=0;
#define MAP_LENGTH_QUERY_CURRENT (GscMapLengthState(epoch,session,id)&&gsc_session_is_running(session,id)&&gscMapLengthBank.epoch==epoch&&gscMapLengthBank.serial==serial&&gscMapLengthBank.session==session&&gscMapLengthBank.id==id&&gscMapLengthBank.count==count&&level.time==now&&!memcmp(&gscMapLengthBank.rows[i],&row,sizeof(row))&&!memcmp(input,&q,sizeof(q)))
 if(!MAP_LENGTH_QUERY_CURRENT||current(context)!=1||!MAP_LENGTH_QUERY_CURRENT)goto done;
 if(row.value.frames>4095||!row.value.fps||row.value.fps>120)goto done;
 if(current(context)!=1||!MAP_LENGTH_QUERY_CURRENT)goto done;
 *out=row.value;ok=1;
done:
#undef MAP_LENGTH_QUERY_CURRENT
 gscMapLengthBusy=0;if(!ok)GscTestError(error,cap,"map length resources or caller retired");return ok;
}
static const GscAnimationLengthHost gscMapLengthHost={NULL,GscMapLengthQuery};
