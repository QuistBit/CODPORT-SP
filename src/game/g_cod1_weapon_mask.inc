/* Sole owner g_spawn.c. Initial authored weapon mask binding only.
 * Original A0C0 fills dynamic rows from WeaponDef; 4151F exact classname
 * lookup selects1C5A0. Native item row = CW definition index+1, NOT WPslot.
 * This receipt grants no weapon, registration completion or item geometry. */
#include "bg_cod1_weapondef.h"
#include "../qcommon/cod1_exact_bytes_r242.h"
typedef struct {unsigned revision;int count;const cwDef_t*owner;cwDef_t bytes;} C1WeaponDefinition;
typedef struct {unsigned revision;int cwIndex,nativeItemIndex;} C1WeaponMaskBinding;
static C1WeaponDefinition cod1WeaponMaskDefinitions[CW_MAX_DEFS];
static C1WeaponMaskBinding cod1WeaponMaskBindings[MAX_GENTITIES];
static int Cod1WeaponRegistrySnapshot(unsigned revision,int count){
 int i,j;const cwDef_t*d;uintptr_t begin,end,p;C1WeaponDefinition*c;
 for(i=0;i<count;i++){
  d=CW_Get(i);if(!d||!memchr(d->name,0,sizeof(d->name)))return 0;
  begin=(uintptr_t)d->text;end=begin+sizeof(d->text);
  for(j=0;j<CW_FIELD_COUNT;j++)if(d->value[j]){
   p=(uintptr_t)d->value[j];if(p<begin||p>=end||!memchr(d->value[j],0,(size_t)(end-p)))return 0;
  }
  c=cod1WeaponMaskDefinitions+i;
  if(c->revision==revision&&(c->owner!=d||c->count!=count||memcmp(&c->bytes,d,sizeof(*d))))return 0;
 }
 if(CW_RegistryRevision()!=revision||CW_Count()!=count)return 0;
 for(i=0;i<count;i++){c=cod1WeaponMaskDefinitions+i;d=CW_Get(i);c->owner=d;c->bytes=*d;c->count=count;c->revision=revision;}
 return 1;
}
static const char*Cod1WeaponMaskName(const char*classname,int*slot){
 if(!classname)return NULL;
 if(!strcmp(classname,"weapon_m1carbine")){*slot=WP_M1CARBINE;return "m1carbine";}
 if(!strcmp(classname,"weapon_thompson")){*slot=WP_THOMPSON;return "thompson";}
 if(!strcmp(classname,"weapon_springfield")){*slot=WP_SPRINGFIELD;return "springfield";}
 if(!strcmp(classname,"weapon_fraggrenade")){*slot=WP_GRENADE_PINEAPPLE;return "fraggrenade";}
 return NULL;
}
static int Cod1WeaponMaskCurrent(unsigned slot){
 const C1WeaponMaskBinding*b=cod1WeaponMaskBindings+slot;const C1WeaponDefinition*c;const cwDef_t*d;int i,count=CW_Count();
 if(!b->revision||CW_RegistryRevision()!=b->revision||b->cwIndex<0||b->cwIndex>=CW_MAX_DEFS||b->nativeItemIndex!=b->cwIndex+1)return 0;
 if(count<1||count>CW_MAX_DEFS||b->cwIndex>=count)return 0;
 /* Earlier dynamic radiantName rows affect original first-match dispatch. */
 for(i=0;i<count;i++){c=cod1WeaponMaskDefinitions+i;d=CW_Get(i);
  if(c->revision!=b->revision||c->count!=count||!d||d!=c->owner||!R242ExactATRBytesEqual(d,&c->bytes,sizeof(*d)))return 0;
 }
 return CW_RegistryRevision()==b->revision;
}
static int Cod1WeaponMaskCapture(const GscEntityRef*ref,const gentity_t*e,const gitem_t*item){
 const char*name;const cwDef_t*d=NULL;unsigned revision=CW_RegistryRevision();
 int slot,index,i,count=CW_Count(),radiant,model;char path[MAX_QPATH];const char*world;
 name=Cod1WeaponMaskName(e->classname,&slot);
 if(!name||!revision||count<1||count>CW_MAX_DEFS||item->giType!=IT_WEAPON||item->giTag!=slot)return 0;
 if(!Cod1WeaponRegistrySnapshot(revision,count))return 0;
 for(index=0;index<count;index++){
  d=CW_Get(index);if(!d||!memchr(d->name,0,sizeof(d->name)))return 0;
  if(!strcmp(d->name,name))break;
 }
 if(index==count)return 0;
 radiant=CW_FieldIndex("radiantName");model=CW_FieldIndex("worldModel");
 if(radiant<0||model<0||!d->value[radiant]||strcmp(d->value[radiant],e->classname)||!d->value[model])return 0;
 for(i=0;i<index;i++)if(CW_Get(i)->value[radiant]&&!strcmp(CW_Get(i)->value[radiant],e->classname))return 0;
 world=d->value[model];
 if(!strncmp(world,"xmodel/",7)){if(strlen(world)>=sizeof(path))return 0;strcpy(path,world);}
 else{if(strlen(world)+7>=sizeof(path))return 0;strcpy(path,"xmodel/");strcat(path,world);}
 /* Engine XModel/VFS paths are case insensitive; BSP's exact namespace
  * spelling is still checked by the shared resource stage/current reads. */
 if(!e->model||Q_stricmp(path,e->model)||CW_RegistryRevision()!=revision)return 0;
 cod1WeaponMaskBindings[ref->slot].revision=revision;
 cod1WeaponMaskBindings[ref->slot].cwIndex=index;cod1WeaponMaskBindings[ref->slot].nativeItemIndex=index+1;
 return Cod1WeaponMaskCurrent(ref->slot);
}
