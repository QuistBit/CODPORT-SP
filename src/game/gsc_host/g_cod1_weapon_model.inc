#include "../g_cod1_weapon_model.h"
/* Static staging avoids two ~68KiB tables on the bounded VM stack. */
static cod1WeaponModelHost_t weaponModels,weaponModelsStage;
static uint64_t weaponModelsEpoch;
static int weaponModelsBusy;
static GscMapExecutionCertificate weaponModelsMap;
static void WeaponModelsReset(void){memset(&weaponModels,0,sizeof(weaponModels));weaponModelsEpoch=0;memset(&weaponModelsMap,0,sizeof(weaponModelsMap));}
/* Explicit early resource authority is independent of a constructed Actor.
 * Every lookup still retains the same map/program and CW registry revision. */
static int WeaponModelsReady(void){return ActorMapAvailable()&&!level.restarted&&
 (weaponModelsMap.current?(weaponModelsMap.receipt==cod1EntityEpoch&&(level.spawning||cod1EntityReady)):(cod1EntityReady&&!level.spawning));}
static int WeaponModelsProgramCurrent(const GscMapExecutionCertificate*input,uint64_t epoch){
 GscMapExecutionCertificate copy=*input;int now=level.time,count=level.num_entities,spawning=level.spawning,ready=cod1EntityReady,ok;
 if(!copy.current)return !copy.receipt;
 if(copy.receipt!=epoch||epoch!=cod1EntityEpoch||!ActorMapAvailable()||level.restarted)return 0;
 ok=copy.current(copy.receipt);
 return ok==1&&epoch==cod1EntityEpoch&&ActorMapAvailable()&&!level.restarted&&
  now==level.time&&count==level.num_entities&&spawning==level.spawning&&ready==cod1EntityReady&&!memcmp(input,&copy,sizeof(copy));
}
static int WeaponModelsCurrent(uint64_t epoch,uint64_t receipt,char *error,size_t size,int (*guard)(void *),void *context){
    int current,now=level.time,count=level.num_entities,spawning=level.spawning,ready=cod1EntityReady;
    if(guard&&guard(context)!=1)return 0;
    if(!WeaponModelsReady()||epoch!=cod1EntityEpoch||epoch!=weaponModelsEpoch||receipt!=weaponModels.receipt||!weaponModels.current){EntityError(error,size,"weapon model table is unavailable");return 0;}
    if(!WeaponModelsProgramCurrent(&weaponModelsMap,epoch))return 0;
    current=weaponModels.current(receipt);
    if(!WeaponModelsProgramCurrent(&weaponModelsMap,epoch))return 0;
    if(guard&&guard(context)!=1)return 0;
    if(current!=1||!WeaponModelsReady()||epoch!=cod1EntityEpoch||epoch!=weaponModelsEpoch||receipt!=weaponModels.receipt||now!=level.time||count!=level.num_entities||spawning!=level.spawning||ready!=cod1EntityReady){EntityError(error,size,"weapon model resource authority changed");return 0;}return 1;
}
static int WeaponModelsSelect(const cod1WeaponModelHost_t *input,const GscMapExecutionCertificate*program,char *error,size_t size){
    uint64_t epoch;unsigned i;int current,now=level.time,count=level.num_entities,spawning=level.spawning,ready=cod1EntityReady;GscMapExecutionCertificate map={0};
    if(program){if(!ActorArguments(program,sizeof(*program),NULL,0,error,size))return 0;map=*program;if(!map.current||map.receipt!=cod1EntityEpoch||!level.spawning||level.restarted||!ActorMapAvailable())return 0;}
    if(!input||!ActorArguments(input,sizeof(*input),NULL,0,error,size))return 0;
    if(weaponModelsBusy||weaponModelsEpoch||(!program&&!WeaponModelsReady())){EntityError(error,size,"weapon model selection requires an unbound ready map");return 0;}
    weaponModelsStage=*input;
    if(!weaponModelsStage.receipt||!weaponModelsStage.current||!weaponModelsStage.rowCount||weaponModelsStage.rowCount>COD1_WEAPON_MODELS_MAX){EntityError(error,size,"weapon model table is malformed");return 0;}
    for(i=0;i<weaponModelsStage.rowCount;++i){const cod1WeaponModelRow_t *row=&weaponModelsStage.rows[i];
        if((row->namePresent!=0&&row->namePresent!=1)||(row->modelPresent!=0&&row->modelPresent!=1)||
           (row->namePresent&&!memchr(row->name,0,sizeof(row->name)))||(row->modelPresent&&!memchr(row->model,0,sizeof(row->model)))){EntityError(error,size,"weapon model row is malformed");return 0;}}
    if(!weaponModelsStage.rows[0].namePresent||!ActorWeaponTextEqual(weaponModelsStage.rows[0].name,"none")){EntityError(error,size,"weapon model table requires semantic none at index zero");return 0;}
    epoch=cod1EntityEpoch;weaponModelsBusy=1;current=WeaponModelsProgramCurrent(&map,epoch)&&weaponModelsStage.current(weaponModelsStage.receipt)==1&&WeaponModelsProgramCurrent(&map,epoch);weaponModelsBusy=0;
    if(current!=1||epoch!=cod1EntityEpoch||(!program&&!WeaponModelsReady())||weaponModelsEpoch||now!=level.time||count!=level.num_entities||spawning!=level.spawning||ready!=cod1EntityReady||(program&&memcmp(program,&map,sizeof(map)))){EntityError(error,size,"weapon model authority changed during selection");return 0;}
    weaponModels=weaponModelsStage;weaponModelsEpoch=epoch;weaponModelsMap=map;return 1;
}
int G_Cod1SelectWeaponModelTable(const cod1WeaponModelHost_t*input,char*error,size_t size){return WeaponModelsSelect(input,NULL,error,size);}
int G_Cod1SelectWeaponModelTableForProgram(const cod1WeaponModelHost_t*input,const GscMapExecutionCertificate*program,char*error,size_t size){return program&&WeaponModelsSelect(input,program,error,size);}
static int WeaponModelsQuery(const char *input,char out[128],char *error,size_t size,int (*guard)(void *),void *context){
    char name[128],result[128];uint64_t epoch,receipt;unsigned i,index=0;size_t len;int found=0,ok=0;
    if(!input||!out)return 0;
    for(len=0;len<128&&input[len];++len){}
    if(!ActorArguments(input,len<128?len+1:128,out,128,error,size)||ActorOverlap(input,len<128?len+1:128,out,128))return 0;
    if(len==128){EntityError(error,size,"weapon model name exceeds capacity");return 0;}
    memset(name,0,sizeof(name));memcpy(name,input,len+1);memset(result,0,sizeof(result));
    if(weaponModelsBusy){EntityError(error,size,"weapon model query reentered");return 0;}
    epoch=weaponModelsEpoch;receipt=weaponModels.receipt;weaponModelsBusy=1;
    if(!WeaponModelsCurrent(epoch,receipt,error,size,guard,context))goto done;
    for(i=0;i<weaponModels.rowCount;++i)if(weaponModels.rows[i].namePresent&&ActorWeaponTextEqual(name,weaponModels.rows[i].name)){index=i;found=1;break;}
    if(!found){G_Printf("WARNING: weapon '%s' is not registered\n",name);if(!WeaponModelsCurrent(epoch,receipt,error,size,guard,context))goto done;}
    if(index){const cod1WeaponModelRow_t *row=&weaponModels.rows[index];if(!row->modelPresent){EntityError(error,size,"weapon worldModel pointer authority is absent");goto done;}memcpy(result,row->model,sizeof(result));}
    else if(name[0]&&!ActorWeaponTextEqual(name,"none")){G_Printf("WARNING: getWeaponModel: unknown weapon '%s'\n",name);if(!WeaponModelsCurrent(epoch,receipt,error,size,guard,context))goto done;}
    if(!WeaponModelsCurrent(epoch,receipt,error,size,guard,context))goto done;
    memcpy(out,result,sizeof(result));ok=1;
done:weaponModelsBusy=0;return ok;
}
int G_Cod1QueryWeaponModel(const char *name,char out[128],char *error,size_t size){return WeaponModelsQuery(name,out,error,size,NULL,NULL);}
static int WeaponModelQuery(void *user,const char *name,char model[128],char *error,size_t size,int (*guard)(void *),void *context){(void)user;return WeaponModelsQuery(name,model,error,size,guard,context);}
const GscWeaponModelHost *G_Cod1WeaponModelHost(void){static const GscWeaponModelHost host={NULL,WeaponModelQuery};return &host;}
