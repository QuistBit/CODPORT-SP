/* Original GAME0AB30 cold resource-table prefix, before09BB0 WeaponDef loading.
 * Reuses retained native SP list/order evidence. No live registry selection,
 * cache publication, WeaponDef parse or inventory remapping is implied here.
 * Accepted domain: 1..63 unique case-insensitive ASCII VFS basenames, each
 * 1..63 bytes, from a complete <=4096-byte NUL-separated list. Duplicate or
 * non-ASCII names need their own VFS/locale evidence and are rejected here.
 * Array row0 describes original weapon ID1; original none slot0 is separate.
 */
#ifndef BG_COD1_WEAPON_NAMES14_H
#define BG_COD1_WEAPON_NAMES14_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {unsigned count;char names[63][64];char configstring[4096];} C1WN14Names;
static unsigned C1WN14_Upper(unsigned c){return c>='a'&&c<='z'?c-32:c;}
/* Native4AB20 normalizes to uppercase: punctuation order matters. */
static int C1WN14_Compare(const char*a,const char*b){unsigned x,y;do{x=C1WN14_Upper((unsigned char)*a++);y=C1WN14_Upper((unsigned char)*b++);if(x!=y)return x<y?-1:1;}while(x);return 0;}
static int C1WN14_Build(const char*list,size_t bytes,int count,C1WN14Names*out){
 C1WN14Names v;size_t cursor=0,n,used=0;unsigned i,j,k;char name[64];uintptr_t a=(uintptr_t)list,b=(uintptr_t)out;
 if(!list||!out||!bytes||bytes>4096||count<1||count>63||(a<b?b-a<bytes:a-b<sizeof(*out)))return 0;
 memset(&v,0,sizeof(v));
 for(i=0;i<(unsigned)count;i++){
  n=0;while(cursor+n<bytes&&list[cursor+n]){unsigned c=(unsigned char)list[cursor+n];if(n>=63||!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;n++;}
  if(!n||cursor+n>=bytes)return 0;memset(name,0,sizeof(name));memcpy(name,list+cursor,n);cursor+=n+1;
  for(j=0;j<i;j++){int cmp=C1WN14_Compare(name,v.names[j]);if(!cmp)return 0;if(cmp<0)break;}
  for(k=i;k>j;k--)memcpy(v.names[k],v.names[k-1],64);memcpy(v.names[j],name,64);
 }
 for(i=0;i<(unsigned)count;i++){n=strlen(v.names[i]);if(used+(i?1:0)+n>=sizeof(v.configstring))return 0;if(i)v.configstring[used++]=' ';memcpy(v.configstring+used,v.names[i],n);used+=n;}
 v.count=(unsigned)count;*out=v;return 1;
}
#endif
