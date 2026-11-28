#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/server/sv_cod1_model_configstring_r50.h"
#define MAX_MODELS 256
#define MAX_STRING_CHARS 1024
#define CS_MODELS 32
typedef struct {int unused;} WeaponRegistrationLease;
static char storage[MAX_MODELS][1500];static char *models[MAX_MODELS];
static int reads,scopes,queries,retired,retire_on_query,unsupported,override_slot;
static int WeaponRegistrationScope(const WeaponRegistrationLease *lease){(void)lease;scopes++;return !retired;}
static int WeaponRegistrationGet(const WeaponRegistrationLease *lease,int index,char*text,size_t size){
 if(!WeaponRegistrationScope(lease))return 0;
 ++reads;const char *v=models[index-CS_MODELS];if(!v)v="";
 size_t n=strlen(v);if(n>=size)n=size-1;memcpy(text,v,n);text[n]=0;
 return WeaponRegistrationScope(lease)&&memchr(text,0,size)!=NULL;
}
int trap_Cod1FindModelConfigstringR50(const char*name){++queries;int result=unsupported?-1:override_slot?override_slot:C1R50_FindModelConfigstring(models,MAX_MODELS,name);if(retire_on_query)retired=1;return result;}
#include "lookup.inc"
static int original(const WeaponRegistrationLease*lease,const char*name){
 char text[MAX_STRING_CHARS];int slot;
 for(slot=1;slot<MAX_MODELS;++slot){if(!WeaponRegistrationGet(lease,CS_MODELS+slot,text,sizeof(text)))return 0;if(!text[0])return 0;if(!strcmp(text,name))break;}
 return slot==MAX_MODELS?0:slot;
}
static unsigned seed=0x5a50c3u;static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static unsigned cases;
static void check(const char*name){WeaponRegistrationLease lease={0};int x,y;retired=retire_on_query=unsupported=override_slot=0;reads=scopes=queries=0;x=original(&lease,name);reads=scopes=queries=0;y=WeaponRegistrationFindModelSlot(&lease,name);assert(x==y);assert(reads==0&&scopes==2&&queries==1);unsupported=1;reads=scopes=queries=0;assert(WeaponRegistrationFindModelSlot(&lease,name)==x);assert(queries==1);++cases;}
int main(void){
 for(int i=0;i<MAX_MODELS;i++){models[i]=storage[i];snprintf(storage[i],sizeof(storage[i]),"model_%d",i);}
 check("model_255");check("model_0");check("missing");
 strcpy(storage[3],"duplicate");strcpy(storage[7],"duplicate");check("duplicate");
 models[2]=NULL;check("duplicate");models[2]=storage[2];storage[2][0]=0;check("duplicate");strcpy(storage[2],"model_2");
 memset(storage[1],'a',sizeof(storage[1])-1);storage[1][sizeof(storage[1])-1]=0;char name[64];memset(name,'a',63);name[63]=0;check(name);
 assert(C1R50_FindModelConfigstring(NULL,MAX_MODELS,"x")==0);assert(C1R50_FindModelConfigstring(models,1,"x")==0);assert(C1R50_FindModelConfigstring(models,MAX_MODELS,NULL)==0);assert(C1R50_FindModelConfigstring(models,MAX_MODELS,"")==0);char invalid[64];memset(invalid,'x',sizeof(invalid));assert(C1R50_FindModelConfigstring(models,MAX_MODELS,invalid)==0);
 for(int k=0;k<2000;k++){
  int prefix=(int)(rnd()%MAX_MODELS);for(int i=0;i<MAX_MODELS;i++){models[i]=storage[i];snprintf(storage[i],sizeof(storage[i]),"v%u",rnd()%300);}
  if(prefix){if(k%2)models[prefix]=NULL;else storage[prefix][0]=0;}
  for(int j=0;j<5;j++){char query[64];snprintf(query,sizeof(query),"v%u",rnd()%400);check(query);}
 }
 for(int i=0;i<MAX_MODELS;i++){models[i]=storage[i];snprintf(storage[i],sizeof(storage[i]),"model_%d",i);}
 WeaponRegistrationLease lease={0};unsupported=retired=retire_on_query=override_slot=0;reads=scopes=queries=0;assert(original(&lease,"model_255")==255);printf("last-slot old reads=%d scopes=%d\n",reads,scopes);reads=scopes=queries=0;assert(WeaponRegistrationFindModelSlot(&lease,"model_255")==255);printf("last-slot new reads=%d queries=%d scopes=%d\n",reads,queries,scopes);
 retire_on_query=1;assert(!WeaponRegistrationFindModelSlot(&lease,"model_255"));retired=retire_on_query=0;override_slot=MAX_MODELS;assert(!WeaponRegistrationFindModelSlot(&lease,"model_255"));retired=1;queries=0;assert(!WeaponRegistrationFindModelSlot(&lease,"model_255")&&!queries);
 printf("%u differential lookup cases PASS; invalid input, retirement and range checks PASS\n",cases);return 0;
}
