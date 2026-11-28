#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reference.inc"
#include "fs_pack_list_match56.inc"
static int reference(const char *name,const char *path,int pathLength,int pathDepth,const char *extension,int extensionLength){
 char zpath[256];int zpathLen,depth,length;
 zpathLen=FS_ReturnPath(name,zpath,&depth);
 if(depth!=pathDepth||pathLength>zpathLen||(pathLength&&name[pathLength]!='/')||Q_stricmpn(name,path,pathLength))return 0;
 length=(int)strlen(name);
 if(length<extensionLength)return 0;
 if(Q_stricmp(name+length-extensionLength,extension))return 0;
 return 1;
}
static uint64_t rng=UINT64_C(0xb19397a1f2c5bd6e);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static void random_name(char *s,unsigned cap){static const char alphabet[]="aAbB0/\\._-\x80\xff";unsigned i,n=rnd()%cap;for(i=0;i<n;++i)s[i]=alphabet[rnd()%(sizeof(alphabet)-1)];s[n]=0;}
static uint64_t old_walks,new_walks;static unsigned accepted,rejected;
static void check(const char *name,const char *path,const char *ext){
 char zpath[256];int pathLength,pathDepth,extensionLength,a,b,n;
 if(path[0]=='/'||path[0]=='\\')++path;
 pathLength=(int)strlen(path);
 if(pathLength&&(path[pathLength-1]=='/'||path[pathLength-1]=='\\'))--pathLength;
 extensionLength=(int)strlen(ext);FS_ReturnPath(path,zpath,&pathDepth);if(path[0])++pathDepth;
 a=reference(name,path,pathLength,pathDepth,ext,extensionLength);b=FS_PackListMatch56(name,path,pathLength,pathDepth,ext,extensionLength);
 assert(a==b);accepted+=(unsigned)a;rejected+=(unsigned)!a;
 ++old_walks;
 if(!pathLength||(!Q_stricmpn(name,path,pathLength)&&name[pathLength]=='/')){n=(int)strlen(name);if(n>=extensionLength&&!Q_stricmp(name+n-extensionLength,ext))++new_walks;}
}
int main(void){
 unsigned i,j;char name[256],path[256],ext[256];const char *dirs[]={"soundaliases","maps","xanim","textures/world","skins","","/","//","\\","a/","a\\","A//","./"};const char *exts[]={".csv",".CSV","","/","\\",".gsc",".wav","a",".bsp"};
 for(i=0;i<200000;i++){
  random_name(name,256);random_name(path,256);random_name(ext,16);
  if(i%4==0){const char *p=dirs[rnd()%(sizeof(dirs)/sizeof(dirs[0]))];snprintf(name,sizeof(name),"%s/%s%s",p,"asset",exts[rnd()%9]);strcpy(path,p);}
  if(i%5==0){size_t n=strlen(name);size_t pos=n?rnd()%n:0;memcpy(path,name,pos);path[pos]=0;}
  if(i%3==0)strcpy(ext,exts[rnd()%9]);
  if(i%7==0){size_t n=strlen(name);strcpy(ext,name+(n?rnd()%n:0));}
  /* Separate exact-size allocations catch reads past NUL, not just array bounds. */
  {char *a=malloc(strlen(name)+1),*b=malloc(strlen(path)+1),*c=malloc(strlen(ext)+1);assert(a&&b&&c);strcpy(a,name);strcpy(b,path);strcpy(c,ext);check(a,b,c);free(a);free(b);free(c);}
 }
 for(i=0;i<sizeof(dirs)/sizeof(dirs[0]);i++)for(j=0;j<sizeof(exts)/sizeof(exts[0]);j++){check("",dirs[i],exts[j]);check("a",dirs[i],exts[j]);check("a/",dirs[i],exts[j]);check("a\\",dirs[i],exts[j]);check("a//b",dirs[i],exts[j]);}
 printf("PASS 200000 random cases + edge matrix; accepted=%u rejected=%u depth_walks=%llu -> %llu\n",accepted,rejected,(unsigned long long)old_walks,(unsigned long long)new_walks);
 old_walks=new_walks=0;accepted=rejected=0;
 for(i=0;i<60000;i++){if(i%1000==0)snprintf(name,sizeof(name),"soundaliases/table%u.csv",i);else if(i%250==0)snprintf(name,sizeof(name),"soundaliases/other%u.wav",i);else snprintf(name,sizeof(name),"textures/world/material%u.dds",i);check(name,"soundaliases",".csv");}
 assert(accepted==60&&new_walks==60&&old_walks==60000);
 printf("PACK MODEL: 60000 entries, 60 matching CSV, depth_walks=%llu -> %llu\n",(unsigned long long)old_walks,(unsigned long long)new_walks);
 return 0;
}
