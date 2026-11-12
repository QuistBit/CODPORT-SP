extern vmCvar_t g_developer;
#include "../bg_cod1_weapondef.h"
/* Query owns its copied view only during this synchronous busy scope. Loaded
 * definitions, not item-registration/model/ATR certificates, are authority. */
typedef struct {const cwDef_t*source;cwDef_t copy;int radiantOffset,radiantBytes,altOffset,altBytes,linked;} WeaponClassRow;
static struct {int busy,cancelled,count;unsigned revision;uint64_t epoch;int now;WeaponClassRow rows[CW_MAX_DEFS];} weaponClassView;
static int WeaponClassnamePrivate(const void*p,size_t n){return ActorOverlap(p,n,&weaponClassView,sizeof(weaponClassView))||ActorOverlap(p,n,&g_developer,sizeof(g_developer))||ActorOverlap(p,n,G_Cod1WeaponClassnameHost(),sizeof(GscWeaponClassnameHost));}
/* CW definitions are readonly buffers of THIS query, not a new global policy
 * rejecting other existing GAME APIs' legitimate CW source pointers. */
static int WeaponClassnameDefinitionAlias(const void*p,size_t n){
 int i,count=CW_Count();
 if(count<0||count>CW_MAX_DEFS)return n!=0;
 for(i=0;i<count;i++)if(ActorOverlap(p,n,CW_Get(i),sizeof(cwDef_t)))return 1;return 0;
}
static int WeaponClassEqual(const char*a,const char*b){unsigned x,y;do{x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}while(x);return 1;}
static int WeaponClassToken(WeaponClassRow*r,int field,int*offset,int*bytes){
 const char*p=r->copy.value[field];uintptr_t base=(uintptr_t)r->source->text,value=(uintptr_t)p;const char*end;
 *offset=-1;*bytes=0;if(!p)return 1;
 if(value<base||value-base>=CW_TEXT_SIZE)return 0;*offset=(int)(value-base);
 end=memchr(r->copy.text+*offset,0,CW_TEXT_SIZE-*offset);if(!end)return 0;*bytes=(int)(end-(r->copy.text+*offset));return 1;
}
static const char*WeaponClassText(const WeaponClassRow*r,int offset){return offset<0?"":r->copy.text+offset;}
static int WeaponClassMap(void){return ActorMapAvailable()&&cod1EntityReady&&!level.spawning&&!level.restarted&&level.time>=0;}
static int WeaponClassStable(void){
 int i;if(!weaponClassView.busy||weaponClassView.cancelled||!WeaponClassMap()||weaponClassView.epoch!=cod1EntityEpoch||weaponClassView.now!=level.time||
    !weaponClassView.revision||weaponClassView.revision!=CW_RegistryRevision()||weaponClassView.count!=CW_Count())return 0;
 for(i=0;i<weaponClassView.count;i++)if(CW_Get(i)!=weaponClassView.rows[i].source||memcmp(CW_Get(i),&weaponClassView.rows[i].copy,sizeof(cwDef_t)))return 0;
 return 1;
}
static int WeaponClassCurrent(int(*current)(void*),void*ctx){return WeaponClassStable()&&current(ctx)==1&&WeaponClassStable();}
static int WeaponClassSnapshot(void){
 int i,j,field=CW_FieldIndex("radiantName"),alt=CW_FieldIndex("altWeapon");WeaponClassRow*r;
 if(field<0||alt<0)return 0;
 for(i=0;i<weaponClassView.count;i++){
  r=weaponClassView.rows+i;r->source=CW_Get(i);if(!r->source)return 0;r->copy=*r->source;
  if(!memchr(r->copy.name,0,sizeof(r->copy.name))||!r->copy.name[0]||WeaponClassEqual(r->copy.name,"none")||
     !WeaponClassToken(r,field,&r->radiantOffset,&r->radiantBytes)||!WeaponClassToken(r,alt,&r->altOffset,&r->altBytes))return 0;
 }
 for(i=0;i<weaponClassView.count;i++){
  r=weaponClassView.rows+i;r->linked=0;
  if(r->altBytes){for(j=0;j<weaponClassView.count;j++)if(WeaponClassEqual(WeaponClassText(r,r->altOffset),weaponClassView.rows[j].copy.name)){r->linked=j+1;break;}}
  /* Actual CW loader publishes a derived zero-based index. Check that it
   * agrees with original A9C3 ASCII matching; never use a WP inventory slot. */
  if(r->copy.altIndex!=(r->linked?r->linked-1:-1)||CI_Linked(r->source)!=r->linked)return 0;
 }
 return WeaponClassStable();
}
static int WeaponClassnameQuery(void*u,const char*input,int now,GscWeaponClassnameResult*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 char name[128];GscWeaponClassnameResult result;WeaponClassRow*r;size_t length;int index=0,found=0,i,start,steps=0,ok=0;(void)u;
 if(!input||!out||!current||ActorGoalPrivate(input,1)||WeaponClassnameDefinitionAlias(input,1))return 0;
 for(length=0;length<128&&input[length];++length){}if(length==128)return 0;
 if(!ActorGoalBuffers(input,length+1,out,sizeof(*out),error,cap)||WeaponClassnameDefinitionAlias(input,length+1)||WeaponClassnameDefinitionAlias(out,sizeof(*out))||WeaponClassnameDefinitionAlias(error,cap))return 0;
 if(weaponClassView.busy){weaponClassView.cancelled=1;return 0;}
 memset(name,0,sizeof(name));memcpy(name,input,length+1);memset(&result,0,sizeof(result));
 weaponClassView.busy=1;weaponClassView.cancelled=0;weaponClassView.epoch=cod1EntityEpoch;weaponClassView.now=level.time;weaponClassView.revision=CW_RegistryRevision();weaponClassView.count=CW_Count();
 if(!WeaponClassMap()||now!=level.time||!weaponClassView.revision||weaponClassView.count<0||weaponClassView.count>CW_MAX_DEFS)goto done;
 if(!WeaponClassSnapshot()||!WeaponClassCurrent(current,ctx)||memcmp(input,name,length+1))goto done;
 /* Original default definition0 has internal name "none". AF90 scans it
  * BEFORE query's empty/none exemption; empty still takes the miss log. */
 if(WeaponClassEqual(name,"none"))found=1;
 else for(i=0;i<weaponClassView.count;i++)if(WeaponClassEqual(name,weaponClassView.rows[i].copy.name)){found=1;index=i+1;break;}
 if(!found){G_DPrintf("Couldn't find weapon \"%s\"\n",name);if(!WeaponClassCurrent(current,ctx)||memcmp(input,name,length+1))goto done;}
 if(!index){
  if(name[0]&&!WeaponClassEqual(name,"none")){
   /* Original30DEB formats once then1E5E0 treats that result as a second
    * printf format. Do not claim arbitrary percent diagnostic parity or
    * reproduce an unsafe variadic read through this bounded adapter. */
   if(strchr(name,'%')){EntityError(error,cap,"weapon classname percent diagnostic requires original double-format contract");goto done;}
   G_Printf("unknown weapon '%s' in getWeaponClassname\n",name);if(!WeaponClassCurrent(current,ctx)||memcmp(input,name,length+1))goto done;}
 }else{
  start=index;
  for(;;){
   if(index<1||index>weaponClassView.count||++steps>weaponClassView.count){EntityError(error,cap,"weapon classname noninitial linked cycle outside native terminating registry domain");goto done;}
   r=weaponClassView.rows+index-1;
   if(r->radiantBytes){if(r->radiantBytes>=128){EntityError(error,cap,"weapon classname exceeds owned VM string capacity");goto done;}result.defined=1;memcpy(result.classname,WeaponClassText(r,r->radiantOffset),r->radiantBytes+1);break;}
   index=r->linked;if(!index||index==start){G_Printf("^3WARNING^7: no Radiant name found for weapon '%s' in getWeaponClassname\n",name);if(!WeaponClassCurrent(current,ctx)||memcmp(input,name,length+1))goto done;break;}
  }
 }
 if(!WeaponClassCurrent(current,ctx)||memcmp(input,name,length+1))goto done;*out=result;ok=1;
done:
 /* A callback can reactivate an initially inactive CW slot. Revalidate failure
  * diagnostics too; do not write an error through newly private storage. */
 if(!ok&&error&&cap&&ActorGoalBuffers(input,length+1,out,sizeof(*out),error,cap)&&
    !WeaponClassnameDefinitionAlias(input,length+1)&&!WeaponClassnameDefinitionAlias(out,sizeof(*out))&&!WeaponClassnameDefinitionAlias(error,cap)&&!error[0])
  EntityError(error,cap,"weapon classname requires stable loaded CW/current map/clock and owned strings");
 weaponClassView.busy=0;weaponClassView.cancelled=0;return ok;
}
const GscWeaponClassnameHost*G_Cod1WeaponClassnameHost(void){static const GscWeaponClassnameHost h={NULL,WeaponClassnameQuery};return &h;}
