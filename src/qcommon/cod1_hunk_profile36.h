#ifndef COD1_HUNK_PROFILE36_H
#define COD1_HUNK_PROFILE36_H
#include <stdint.h>
#include <string.h>
/* Observe allocator offsets, never change the allocation policy. All calls
 * follow the original Hunk owner's serialization. Bytes include padding and
 * temporary headers, not just file payloads. */
typedef struct {
 uint64_t epoch,permCalls,tempCalls,freeCalls,nonFinal,clearCalls;
 uint64_t permBytes,tempBytes,nonFinalBytes,clearedBytes,clearedLiveBytes;
 uint64_t peakUsed,peakPermanent,peakTemp,retained,peakRetained,largestTemp;
} c1hp36Stats;
static void C1HP36_Observe(c1hp36Stats *s,uint64_t permanent,uint64_t used){
 uint64_t temp=used>=permanent?used-permanent:0;
 if(used>s->peakUsed)s->peakUsed=used;
 if(permanent>s->peakPermanent)s->peakPermanent=permanent;
 if(temp>s->peakTemp)s->peakTemp=temp;
 if(s->retained>s->peakRetained)s->peakRetained=s->retained;
}
static void C1HP36_Reset(c1hp36Stats *s,uint64_t permanent,uint64_t used){
 uint64_t epoch=s->epoch+1;
 memset(s,0,sizeof(*s));s->epoch=epoch;C1HP36_Observe(s,permanent,used);
}
static void C1HP36_Alloc(c1hp36Stats *s,uint64_t bytes,int temporary,uint64_t permanent,uint64_t used){
 if(temporary){s->tempCalls++;s->tempBytes+=bytes;if(bytes>s->largestTemp)s->largestTemp=bytes;}
 else{s->permCalls++;s->permBytes+=bytes;}
 C1HP36_Observe(s,permanent,used);
}
static void C1HP36_Free(c1hp36Stats *s,uint64_t bytes,int final,uint64_t permanent,uint64_t used){
 s->freeCalls++;
 if(!final){s->nonFinal++;s->nonFinalBytes+=bytes;s->retained+=bytes;}
 C1HP36_Observe(s,permanent,used);
}
static void C1HP36_ClearTemp(c1hp36Stats *s,uint64_t bytes){
 s->clearCalls++;s->clearedBytes+=bytes;
 if(bytes>s->retained)s->clearedLiveBytes+=bytes-s->retained;
 s->retained=0;
}
#endif
