/* Original0AB30 cold table order and A910 alt cycles. One shared CW table;
 * no per-turret cache. Called before gameplay consumers acquire receipts.
 * Existing strict CW numeric/file grammar is retained as the admitted domain.
 * Full native arbitrary-text coercion and engine-owned warm storage are not
 * claimed by this cold-file entry point. */
#include "bg_cod1_weapon_names14.h"
#include "bg_cod1_weapon_enums14.inc"
/* Loading is single-threaded and rejects reentry. Retired registry pointers
 * must not let a callback silently alter definitions already staged. */
static cwDef_t cwTableBefore[CW_MAX_DEFS];
static int cwTableRootsBefore[CW_MAX_DEFS];
static void CW_TableHold(void){memcpy(cwTableBefore,cwDefs,sizeof(cwDefs));memcpy(cwTableRootsBefore,cwFamilyRoot,sizeof(cwFamilyRoot));}
static int CW_TableHeld(void){return !memcmp(cwTableBefore,cwDefs,sizeof(cwDefs))&&!memcmp(cwTableRootsBefore,cwFamilyRoot,sizeof(cwFamilyRoot));}
static int CW_TableOverlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||!n||!m)return 0;if(n>UINTPTR_MAX-x||m>UINTPTR_MAX-y)return 1;return x<y?y-x<n:x-y<m;}
static int CW_TablePrivate(const void*p,size_t n){return CW_TableOverlap(p,n,cwDefs,sizeof(cwDefs))||CW_TableOverlap(p,n,cwInput,sizeof(cwInput))||CW_TableOverlap(p,n,cwFields,sizeof(cwFields))||CW_TableOverlap(p,n,cwFamilyRoot,sizeof(cwFamilyRoot))||CW_TableOverlap(p,n,cwError,sizeof(cwError))||CW_TableOverlap(p,n,&cwCount,sizeof(cwCount))||CW_TableOverlap(p,n,&cwRegistryRevision,sizeof(cwRegistryRevision))||CW_TableOverlap(p,n,&cwRegistryMutationDepth,sizeof(cwRegistryMutationDepth))||CW_TableOverlap(p,n,&cwTableBusy,sizeof(cwTableBusy))||CW_TableOverlap(p,n,&cwTableCancelled,sizeof(cwTableCancelled));}
typedef struct {cwTableHost_t host;const cwTableHost_t*borrowed;unsigned revision;} cwTableLease_t;
static int CW_TableDirect(const cwTableLease_t*l){return cwTableBusy&&!cwTableCancelled&&!cwCount&&cwRegistryMutationDepth==1&&l->revision&&cwRegistryRevision==l->revision&&!memcmp(&l->host,l->borrowed,sizeof(l->host));}
static int CW_TableCurrent(const cwTableLease_t*l){char input[CW_TEXT_SIZE];int result;if(!CW_TableDirect(l))return 0;CW_TableHold();memcpy(input,cwInput,sizeof(input));result=l->host.current(l->host.user);return result==1&&CW_TableDirect(l)&&CW_TableHeld()&&!memcmp(input,cwInput,sizeof(input));}
static int CW_TableParse(cwDef_t*d,const char*name,const char*bytes,int n){
 unsigned char env[108];unsigned mx,nearMx=0x1f80;const unsigned short cw=0x027f;unsigned i,j;int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=CW_Parse(d,name,bytes,n)>0;
 if(ok)for(i=0;i<CW_FIELD_COUNT;i++){
  unsigned type=cwFields[i].type;
  if(type==0&&!d->value[i])d->value[i]=d->text+sizeof(d->text)-1;
  else if(type==6)d->number[i]=(double)(float)d->number[i];
  else if(type>=8&&type<=13&&d->value[i]&&*d->value[i]){
   for(j=0;j<cwNativeEnumCounts[type-8];j++)if(!C1WN14_Compare(d->value[i],cwNativeEnums[type-8][j]))break;
   if(j==cwNativeEnumCounts[type-8]){CW_Fail(cwFields[i].name);ok=0;break;}d->number[i]=j;
  }
 }
 __asm ldmxcsr mx
 __asm frstor env
 return ok;
}
/* Original ammo/clip/shared-cap postprocessors normalize the owned name
 * tokens. Only those tokens are changed; original file hash remains raw. */
static void CW_TableLowerToken(cwDef_t*d,const char*field){int i=CW_FieldIndex(field);char*p=i>=0?(char*)d->value[i]:NULL;if(p)while(*p){if(*p>='A'&&*p<='Z')*p+=32;p++;}}
static int CW_TableLinks(int n){int i,j,cursor,seen[CW_MAX_DEFS];
 for(i=0;i<n;i++){cwDefs[i].altIndex=-1;cwFamilyRoot[i]=i;}
 for(i=0;i<n;i++){
  const char*alt=CW_String(cwDefs+i,"altWeapon");if(!*alt)continue;
  for(j=0;j<n;j++)if(!C1WN14_Compare(alt,cwDefs[j].name))break;
  if(j==n)return CW_Fail("missing alternate weapon");
  if(CW_Number(cwDefs+i,"weaponSlot")!=CW_Number(cwDefs+j,"weaponSlot")||CW_Number(cwDefs+i,"slotStackable")!=CW_Number(cwDefs+j,"slotStackable"))return CW_Fail("alternate weapon slot mismatch");
  cwDefs[i].altIndex=j;
 }
 for(i=0;i<n;i++){
  if(cwDefs[i].altIndex<0)continue;memset(seen,0,sizeof(seen));cursor=i;
  while(cursor>=0&&!seen[cursor]){seen[cursor]=1;cursor=cwDefs[cursor].altIndex;}
  if(cursor!=i)return CW_Fail("alternate weapons must form a closed cycle");
  for(j=0;j<n;j++)if(seen[j])break;cwFamilyRoot[i]=j;
 }
 return 1;
}
int CW_LoadOriginalTable(const cwTableHost_t*host){
 cwTableLease_t lease;C1WN14Names names;char list[4096],listCopy[4096],path[80],pathCopy[80],inputCopy[CW_TEXT_SIZE];int n,i,size,ok=0;
 if(cwTableBusy||cwRegistryMutationDepth){cwTableCancelled=1;return CW_Fail("reentrant weapon table load");}
 if(!host||CW_TablePrivate(host,sizeof(*host))||CW_TableOverlap(host,sizeof(*host),cwTableBefore,sizeof(cwTableBefore))||CW_TableOverlap(host,sizeof(*host),cwTableRootsBefore,sizeof(cwTableRootsBefore))||!host->current||!host->list||!host->read)return CW_Fail("missing weapon table host");
 memset(&lease,0,sizeof(lease));lease.host=*host;lease.borrowed=host;CW_ResetRegistry();lease.revision=cwRegistryRevision;cwRegistryMutationDepth=1;cwTableBusy=1;cwTableCancelled=0;
 if(!CW_TableCurrent(&lease))goto done;memset(list,0,sizeof(list));CW_TableHold();n=lease.host.list(lease.host.user,list,sizeof(list));if(!CW_TableDirect(&lease)||!CW_TableHeld())goto done;memcpy(listCopy,list,sizeof(list));
 if(!CW_TableCurrent(&lease)||memcmp(listCopy,list,sizeof(list))||!C1WN14_Build(list,sizeof(list),n,&names)){CW_Fail("invalid original weapon filename table");goto done;}
 for(i=0;i<n;i++){
  if(!CW_NameValid(names.names[i])){CW_Fail("invalid weapon resource name");goto done;}
  memset(path,0,sizeof(path));strcpy(path,"weapons/sp/");strcat(path,names.names[i]);memcpy(pathCopy,path,sizeof(path));memset(cwInput,0,sizeof(cwInput));
  if(!CW_TableCurrent(&lease)||memcmp(pathCopy,path,sizeof(path)))goto done;CW_TableHold();size=lease.host.read(lease.host.user,path,cwInput,sizeof(cwInput)-1);if(!CW_TableDirect(&lease)||!CW_TableHeld())goto done;
  memcpy(inputCopy,cwInput,sizeof(cwInput));
  if(!CW_TableCurrent(&lease)||memcmp(pathCopy,path,sizeof(path))||memcmp(inputCopy,cwInput,sizeof(cwInput)))goto done;
  /* Original header10 then signed remainder<8192. Existing parser also
   * checks complete header/backslash and finite decimal grammar. */
  if(size<11||size>=8202)goto done;
  {int k;for(k=10;k<size;k++)if(cwInput[k]=='"'||cwInput[k]==';'){CW_Fail("quoted or semicolon weapon payload");goto done;}}
  if(!CW_TableParse(cwDefs+i,names.names[i],cwInput,size))goto done;
  CW_TableLowerToken(cwDefs+i,"ammoName");CW_TableLowerToken(cwDefs+i,"clipName");CW_TableLowerToken(cwDefs+i,"sharedAmmoCapName");
 }
 if(CW_TableLinks(n)<0||!CW_TableCurrent(&lease))goto done;
 cwCount=n;ok=1;
done:cwTableBusy=0;cwRegistryMutationDepth=0;if(!ok){cwCount=0;if(!cwError[0])CW_Fail("weapon table owner/read/parse refused");return -1;}return n;
}
