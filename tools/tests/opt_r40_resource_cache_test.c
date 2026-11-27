/* Include the production resource selector and loader with a controlled CG/FS
 * boundary. Real composition parser; deterministic stand-ins for XModel,
 * XAnim bank, renderer and snapshot callbacks. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CGTA_CACHE 16
#define COD1_SOURCE_POSE_WIRE_VERSION 29
#define COD1_SCRIPTED_ACTOR_WIRE_VERSION 25
#define CS_COD1_JUMPER 1766
#define COD1_TURRET_ACTOR_RESOURCE_VERSION 1
typedef struct {int version,size,partCount;char model[8][64],tag[8][64];} cod1TurretActorResourceState_t;
typedef struct {int key;} c1ad14Resource_t;
typedef struct {cod1TurretActorResourceState_t definition;int handle,version;c1ad14Resource_t *resource;unsigned char map[128];} cgfa4Resource_t;
typedef struct {struct {int cod1TurretActorVersion,cod1TurretActorConfig,cod1TurretEpochLo,cod1TurretEpochHi,number,cod1TurretActorGeneration;} actor;char config[1024];unsigned revision;} cgfa4Lease_t;
typedef struct {void *model,*parts;int modelBytes,partsBytes;const char *modelName,*partsName,*tag;} c1at14Input_t;
typedef struct {char lodName[1][64];} c1mrModelHeader_t;
static cgfa4Resource_t cgfa4Resources[CGTA_CACHE];
static unsigned revision=1,reads,creates,frees,regs,live,fail,reset_on_register;
static void C1AD14_Free(c1ad14Resource_t *r){if(r){assert(live);--live;++frees;free(r);}}
#include "cg_cod1_recyclable_resource40.inc"
static int C1TAW_Name(const char*s){size_t i,n;if(!s||!memchr(s,0,64)||!(n=strlen(s))||n>=64||strstr(s,".."))return 0;for(i=0;i<n;i++)if((unsigned char)s[i]<32||strchr("/\\:@|",s[i]))return 0;return 1;}
static int C1TAW_Composition(const char*text,cod1TurretActorResourceState_t*out){cod1TurretActorResourceState_t v;size_t at=6,n=0,i=0,j;if(!text||!out||strncmp(text,"c1ta1|",6))return 0;while(n<1024&&text[n])n++;if(n>=1024)return 0;memset(&v,0,sizeof(v));v.version=COD1_TURRET_ACTOR_RESOURCE_VERSION;v.size=sizeof(v);
 while(at<n){if(i>=8)return 0;j=0;while(at<n&&text[at]!='@'){if(text[at]=='|'||j>=63)return 0;v.model[i][j++]=text[at++];}if(at>=n||text[at++]!='@'||!C1TAW_Name(v.model[i]))return 0;j=0;while(at<n&&text[at]!='|'){if(j>=63)return 0;v.tag[i][j++]=text[at++];}if(v.tag[i][0]&&(strncmp(v.tag[i],"tag_",4)||!C1TAW_Name(v.tag[i])))return 0;i++;if(at<n){at++;if(at==n)return 0;}}
 if(!i||v.tag[0][0])return 0;v.partCount=i;*out=v;return 1;
}
static int CGFA4_Current(const cgfa4Lease_t *l){return l->revision==revision;}
static int CGFA4_LoadClips(const cgfa4Lease_t *l){return CGFA4_Current(l)&&fail!=1;}
static void *CGFA4_Read(const cgfa4Lease_t *l,const char *path,int *n){++reads;if(!CGFA4_Current(l)||fail==2)return NULL;*n=(int)strlen(path)+1;void *p=malloc((size_t)*n);assert(p);memcpy(p,path,(size_t)*n);return p;}
static int C1MR_DecodeModel(void *data,int n,c1mrModelHeader_t *m){assert(n>0&&data);if(fail==3)return 0;memset(m,0,sizeof(*m));strcpy(m->lodName[0],"lod");return 1;}
static int C1AD14_Create(const c1at14Input_t *in,unsigned count,c1ad14Resource_t **out){assert(count&&in[0].model&&in[0].parts&&!*out);if(fail==4)return 0;*out=malloc(sizeof(**out));assert(*out);(*out)->key=in[0].modelName[0];++creates;++live;return 1;}
static int CGFA4_BuildBank(const cgfa4Lease_t *l,cgfa4Resource_t *r){r->version=l->actor.cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION?COD1_SCRIPTED_ACTOR_WIRE_VERSION:l->actor.cod1TurretActorVersion;return fail!=5;}
static int trap_R_RegisterCod1TurretActor(const cod1TurretActorResourceState_t *d){++regs;assert(d->partCount>0);if(reset_on_register){++revision;CGFA4_RecyclableReset();}return fail==6?0:100+(unsigned char)d->model[0][0];}
#define CG_Printf(...) ((void)0)
#include "cg_cod1_face_resource40.inc"
static cgfa4Lease_t lease(char model,int owner,int generation,int version,int recycled){cgfa4Lease_t l={0};l.revision=revision;l.actor.cod1TurretActorVersion=version;l.actor.cod1TurretActorConfig=recycled?CS_COD1_JUMPER:1750;l.actor.cod1TurretEpochLo=1;l.actor.number=owner;l.actor.cod1TurretActorGeneration=generation;if(recycled)snprintf(l.config,sizeof(l.config),"j:1:0:%u:%u|c1ta1|%c@",(unsigned)owner,(unsigned)generation,model);else snprintf(l.config,sizeof(l.config),"c1ta1|%c@",model);return l;}
static cgfa4Resource_t *get(char model,int owner,int generation,int version,int recycled){cgfa4Lease_t l=lease(model,owner,generation,version,recycled);return CGFA4_Resource(&l);}
int main(void){cgfa4Resource_t *a,*b,*r;unsigned i,oldReads,oldCreates,oldFrees,oldRegs,oldCursor;
 a=get('a',1,1,29,1);assert(a&&a->resource->key=='a');a->map[5]=77;
 b=get('b',2,2,25,1);assert(b&&b!=a);oldReads=reads;oldCreates=creates;oldRegs=regs;
 for(i=0;i<1000;i++){r=get(i%2?'a':'b',(int)i+3,(int)i+5,i%2?25:29,1);assert(r==(i%2?a:b));if(i%2)assert(r->map[5]==77);}
 assert(reads==oldReads&&creates==oldCreates&&regs==oldRegs&&live==2);
 {cgfa4Lease_t l=lease('a',1,2,29,1);l.actor.cod1TurretActorGeneration=3;assert(!CGFA4_Resource(&l));l=lease('a',1,2,29,1);--l.revision;assert(!CGFA4_Resource(&l));}
 assert(reads==oldReads);assert(get('a',3,4,4,1)!=a);assert(get('c',4,5,29,1));assert(live==4);
 oldCursor=cgfa4RecyclableNext;oldFrees=frees;oldCreates=creates;
 for(i=1;i<=6;i++){fail=i;assert(!get('d',5,6,29,1));assert(cgfa4RecyclableNext==oldCursor&&live==4);fail=0;assert(get('a',6,7,29,1)==a);}
 assert(creates-oldCreates==frees-oldFrees);fail=0;
 r=get('d',5,6,29,1);assert(r==a&&r->resource->key=='d'&&r->map[5]==0&&live==4);
 assert(get('b',6,7,29,1)==b);assert(get('a',7,8,29,1)==b&&live==4); /* full cache replaces, never refuses */
 r=get('z',1,1,25,0);assert(r==cgfa4Resources);oldReads=reads;assert(get('z',2,2,29,0)==r&&reads==oldReads);
 reset_on_register=1;assert(!get('y',3,3,29,1));assert(live==1&&cgfa4RecyclableNext==0);reset_on_register=0;
 assert(get('y',3,3,29,1));CGFA4_RecyclableReset();assert(live==1);C1AD14_Free(r->resource);memset(r,0,sizeof(*r));assert(live==0&&creates==frees);
 printf("R40 PASS: 1000 recurring hits, exact version, stale prefix/lease, six failure boundaries, full-cache replacement, callback reset, ordinary namespace; creates=%u frees=%u\n",creates,frees);return 0;}
