#ifndef COD1_CPU_WIN_H
#define COD1_CPU_WIN_H
#include <windows.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned logical,available;int cpuSets,restricted,conservative;} c1jCpuInfo;
static unsigned C1J_Popcount(ULONG_PTR x){unsigned n=0;while(x){x&=x-1;n++;}return n;}
static unsigned C1J_WorkerBudget(unsigned available,int enabled,int requested){
 unsigned n;if(!enabled||available<2||requested==0)return 0;
 n=available-1;if(requested>0&&(unsigned)requested<n)n=(unsigned)requested;
 return n>64?64:n; /* Bounded x86 address-space/stack budget; not an 8-CPU assumption. */
}
static unsigned C1J_AffinityBudget(unsigned cpus,ULONG_PTR mask,int restricted,int *conservative){
 unsigned guaranteed=C1J_Popcount(mask);
 /* WOW64 OR-folds upper/lower halves. Equal process/system masks therefore
  * do not prove unrestricted affinity: lower-32-only and all-64 can look
  * identical. Use the known lower bound whenever the mask is ambiguous.
  * CPU Sets can still report all logical CPUs; this only bounds our workers. */
 if(mask && (restricted || cpus>8*sizeof(ULONG_PTR)) && cpus>guaranteed){*conservative=1;return guaranteed;}
 return cpus;
}
/* Size/type walking supports future CPU_SET_INFORMATION extensions. CPU sets
 * identify logical CPUs across groups without folded-index aliasing.
 * A folded legacy affinity mask uses a conservative bound, even when it
 * happens to equal the system mask. No worker changes affinity settings. */
static unsigned C1J_CountCpuSets(const unsigned char *data,unsigned bytes,
 const ULONG *selected,unsigned selectedCount,const USHORT *groups,unsigned groupCount,
 ULONG_PTR processMask,int restricted){
 unsigned pos=0,n=0,i;while(pos<bytes){DWORD size,type,id;WORD group;unsigned char logical,flags;int allowed;
  if(bytes-pos<8)return 0;memcpy(&size,data+pos,4);memcpy(&type,data+pos+4,4);
  if(size<8||size>bytes-pos)return 0;
  if(type==0){
   if(size<24)return 0;memcpy(&id,data+pos+8,4);memcpy(&group,data+pos+12,2);
   logical=data[pos+14];flags=data[pos+19];allowed=!(flags&2)||(flags&4);
   if(groupCount){for(i=0;i<groupCount&&groups[i]!=group;i++);if(i==groupCount)allowed=0;}
   if(selectedCount){for(i=0;i<selectedCount&&selected[i]!=id;i++);if(i==selectedCount)allowed=0;}
   if(restricted && !(processMask&((ULONG_PTR)1<<(logical%(8*sizeof(ULONG_PTR))))))allowed=0;
   if(allowed)n++;
  }pos+=size;
 }return n;
}
static c1jCpuInfo C1J_QueryCpu(void){
 typedef BOOL(WINAPI *SetsFn)(void*,ULONG,PULONG,HANDLE,ULONG);
 typedef BOOL(WINAPI *DefaultFn)(HANDLE,PULONG,ULONG,PULONG);
 typedef BOOL(WINAPI *GroupsFn)(HANDLE,PUSHORT,PUSHORT);
 typedef DWORD(WINAPI *ActiveFn)(WORD);
 c1jCpuInfo out={0};HMODULE kernel=GetModuleHandleA("kernel32.dll");SYSTEM_INFO si;
 SetsFn sets=(SetsFn)GetProcAddress(kernel,"GetSystemCpuSetInformation");
 DefaultFn defaults=(DefaultFn)GetProcAddress(kernel,"GetProcessDefaultCpuSets");
 GroupsFn groupsFn=(GroupsFn)GetProcAddress(kernel,"GetProcessGroupAffinity");
 ActiveFn active=(ActiveFn)GetProcAddress(kernel,"GetActiveProcessorCount");
 ULONG_PTR processMask=0,systemMask=0;ULONG bytes=0,selectedCount=0;USHORT groupCount=64,groups[64];
 unsigned char *data=NULL;ULONG *selected=NULL;unsigned n=0,attempt;int selectionUnknown=0;
 GetNativeSystemInfo(&si);out.logical=active?active(0xffff):si.dwNumberOfProcessors;
 if(!out.logical)out.logical=1;
 if(!groupsFn||!groupsFn(GetCurrentProcess(),&groupCount,groups))groupCount=0;
 if(GetProcessAffinityMask(GetCurrentProcess(),&processMask,&systemMask)&&processMask&&processMask!=systemMask)out.restricted=1;
 if(defaults){
  if(!defaults(GetCurrentProcess(),NULL,0,&selectedCount) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)selectionUnknown=1;
  if(selectedCount&&selectedCount<65536){selected=(ULONG*)malloc(selectedCount*sizeof(ULONG));
   if(!selected||!defaults(GetCurrentProcess(),selected,selectedCount,&selectedCount)){free(selected);selected=NULL;selectedCount=0;selectionUnknown=1;}
  }else {if(selectedCount)selectionUnknown=1;selectedCount=0;}
 }
 if(sets){
  sets(NULL,0,&bytes,GetCurrentProcess(),0);
  for(attempt=0;attempt<3&&bytes>=24&&bytes<=16*1024*1024;attempt++){
   ULONG capacity=bytes;data=(unsigned char*)malloc(capacity);if(!data)break;
   if(sets(data,capacity,&bytes,GetCurrentProcess(),0) && bytes<=capacity){
    n=C1J_CountCpuSets(data,bytes,selected,selectedCount,groups,groupCount,processMask,out.restricted);
    free(data);data=NULL;out.cpuSets=1;if(!n){n=1;out.conservative=1;}break;
   }free(data);data=NULL;if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)break;
  }
 }
 free(selected);
 if(!n){n=si.dwNumberOfProcessors;if(processMask)n=C1J_Popcount(processMask);}
 n=C1J_AffinityBudget(n,processMask,out.restricted,&out.conservative);
 if(selectionUnknown){n=1;out.conservative=1;}
 out.available=n?n:1;if(out.available>out.logical)out.available=out.logical;
 return out;
}
#endif
