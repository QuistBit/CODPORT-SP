#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COD1_ST_BODY (128u*1024u)
#define C1MAP_REPEAT 0
enum { COD1_MP_FALSE=0,COD1_MP_TRUE=1,COD1_MP_UNKNOWN=2 };
typedef struct {int units;} cod1MpCaps_t;
typedef struct {size_t start,end;unsigned char bundle,nested;} cod1MpOp_t;
typedef struct {int line,numBundles,numOps,numRequires,syntaxUnknown;int selected;cod1MpOp_t op[32];} cod1MpStage_t;
typedef struct {const char *source;unsigned sourceHash,selectedMask;int numStages,topOps;char error[96];cod1MpOp_t topOp[32];cod1MpStage_t stage[32];} cod1MaterialProbe_t;
typedef struct {char text[128];const char *start,*end;} cod1MpToken_t;
typedef struct {const char *cursor;} cod1MpLexer_t;
typedef struct {char path[128];unsigned hash,selected,excluded,nativeSurfaceFlags,bundleAuth[8];int ignoredSurfaceParms,ignoredRuntimeHints,stages,line;char reason[160];} cod1StResult_t;
static unsigned flags,parse_calls,allocs,frees,fail_at;
static const char source[]="surfaceparm sky\nradialNormals\ncull none\nmap $texturename\nrequires GL_test\nmap image\n";
static void *tmalloc(size_t n){if(++allocs==fail_at)return NULL;void *p=malloc(n);if(p)memset(p,0xa5,n);return p;}
static void *tcalloc(size_t n,size_t size){if(++allocs==fail_at)return NULL;return calloc(n,size);}
static void tfree(void *p){if(p)++frees;free(p);}
static int Cod1ST_Append(char *out,size_t cap,size_t *used,const char *text,size_t n){if(*used>=cap||n>=cap-*used)return 0;memcpy(out+*used,text,n);*used+=n;out[*used]=0;return 1;}
static int Cod1ST_Why(cod1StResult_t *r,int line,const char *why){r->line=line;snprintf(r->reason,sizeof(r->reason),"%s",why);return 0;}
static cod1MpOp_t op(const char *text){const char *p=strstr(source,text);assert(p);return (cod1MpOp_t){(size_t)(p-source),(size_t)(p-source)+strlen(text),0,0};}
/* Controlled parser: as the production parser does, initialize the entire
 * output on EVERY call. Model first/second parse results and stage failures. */
static int Cod1MP_Parse(const char *body,const cod1MpCaps_t *caps,cod1MaterialProbe_t *p){
 memset(p,0,sizeof(*p));++parse_calls;
 if(!caps||(parse_calls==1&&(flags&1))||(parse_calls==2&&(flags&2))){strcpy(p->error,"parse refused");return 0;}
 p->source=body;p->sourceHash=123+parse_calls;p->selectedMask=3;
 if(parse_calls==1){
  p->topOps=3;p->topOp[0]=op("surfaceparm sky");p->topOp[1]=op("radialNormals");p->topOp[2]=op("cull none");
  p->numStages=(flags&4)?0:((flags&8)?9:2);
  for(int i=0;i<p->numStages;i++){p->stage[i].line=i+1;p->stage[i].selected=(i==0&&(flags&16))?COD1_MP_FALSE:((i==0&&(flags&32))?COD1_MP_UNKNOWN:COD1_MP_TRUE);p->stage[i].numOps=2;p->stage[i].op[0]=op("requires GL_test");p->stage[i].op[1]=op(i%2?"map image":"map $texturename");}
 }else{p->numStages=(flags&16)?1:2;for(int i=0;i<p->numStages;i++){p->stage[i].line=80+i;p->stage[i].numBundles=1;p->stage[i].selected=COD1_MP_TRUE;}}
 return 1;
}
static int Cod1ST_Top(const cod1MaterialProbe_t *p,const cod1MpOp_t *o,char *why){(void)p;(void)o;if(flags&64){strcpy(why,"top refused");return 0;}return 1;}
static int Cod1ME_Line(const cod1MaterialProbe_t *p,const cod1MpOp_t *o,cod1MpLexer_t *l,char *line){size_t n=o->end-o->start;assert(n<512);memcpy(line,p->source+o->start,n);line[n]=0;l->cursor=line;return 1;}
static cod1MpToken_t Cod1MP_Next(cod1MpLexer_t *l){cod1MpToken_t t={0};const char *p=l->cursor;while(isspace((unsigned char)*p))++p;t.start=p;while(*p&&!isspace((unsigned char)*p))++p;size_t n=(size_t)(p-t.start);assert(n<sizeof(t.text));memcpy(t.text,t.start,n);t.end=p;l->cursor=p;return t;}
static int Cod1MP_Equal(const char *a,const char *b){return !strcmp(a,b);}
static int Cod1ME_OneOf(const char *a,const char *b){assert(!strcmp(b,"map clampmap"));return !strcmp(a,"map")||!strcmp(a,"clampmap");}
static unsigned Cod1ST_SurfaceFlag(const char *name,int *known){*known=!strcmp(name,"sky");return *known?4:0;}
static int Cod1ST_RequiresPlaced(const cod1MaterialProbe_t *p,const cod1MpStage_t *s,char *why){(void)p;(void)s;if(flags&128){strcpy(why,"requires refused");return 0;}return 1;}
static unsigned Cod1ST_BundleAuth(const cod1MaterialProbe_t *p,const cod1MpStage_t *s){(void)p;return (unsigned)s->line;}
static int C1Map_Option(const char *s,int *axes,unsigned *f){(void)s;(void)axes;(void)f;return 0;}
static int Cod1ME_End(cod1MpLexer_t *l){return !Cod1MP_Next(l).text[0];}
static int Cod1ST_CheckStageAuth(const cod1MaterialProbe_t *p,const cod1MpStage_t *s,int units,unsigned auth,int strict,char *why){(void)p;(void)s;(void)units;(void)auth;(void)strict;if(flags&256){strcpy(why,"stage refused");return 0;}return 1;}
#define malloc tmalloc
#define calloc tcalloc
#define free tfree
#define Cod1ST_PrepareAuth old_prepare
#include "stype.old"
#undef Cod1ST_PrepareAuth
#include "stype.new"
#undef malloc
#undef calloc
#undef free
int main(void){
 unsigned cases=0,passed=0;unsigned long long old_allocs=0,new_allocs=0;
 const cod1MpCaps_t caps={2};char a[1024],b[1024];cod1StResult_t x,y;
 for(flags=0;flags<512;flags++)for(int strict=0;strict<=1;strict++)for(int c=0;c<5;c++){
  size_t cap=c==0?0:c==1?4:c==2?32:sizeof(a);const char *body=c==4?NULL:source;
  memset(a,0x66,sizeof(a));memcpy(b,a,sizeof(a));memset(&x,0,sizeof(x));y=x;
  parse_calls=allocs=frees=fail_at=0;int old=old_prepare(body,"textures/metal@foo",&caps,a,cap,strict,&x);unsigned p=parse_calls;assert(allocs==2&&frees==2);old_allocs+=allocs;
  parse_calls=allocs=frees=0;int now=Cod1ST_PrepareAuth(body,"textures/metal@foo",&caps,b,cap,strict,&y);assert(allocs==1&&frees==1);new_allocs+=allocs;
  assert(old==now&&p==parse_calls&&!memcmp(a,b,sizeof(a))&&!memcmp(&x,&y,sizeof(x)));++cases;if(now)++passed;
 }
 flags=0;parse_calls=allocs=frees=0;fail_at=1;memset(&x,0,sizeof(x));assert(!Cod1ST_PrepareAuth(source,"textures/metal@foo",&caps,a,sizeof(a),0,&x));assert(allocs==1&&frees==0&&!parse_calls&&!strcmp(x.reason,"allocation failed"));
 printf("%u scratch-lifetime comparisons PASS (%u admitted), allocations %llu -> %llu; first allocation failure PASS\n",cases,passed,old_allocs,new_allocs);return 0;
}
