#include <assert.h>
#include <stdio.h>
#include "../../src/qcommon/cod1_hunk_profile36.h"
/* Mimic the existing stack offsets for either bank. The counter header is
 * the production implementation; the allocator itself is not linked here. */
static void order(int high,int reverse,unsigned count){
 c1hp36Stats s={0};unsigned i,j,offset=0;unsigned starts[21],sizes[21];
 C1HP36_Reset(&s,64,64);
 for(i=0;i<count;i++){
  sizes[i]=12+4*i;starts[i]=high?4096-offset-sizes[i]:offset;
  offset+=sizes[i];C1HP36_Alloc(&s,sizes[i],1,64,64+offset);
 }
 assert(s.tempCalls==count&&s.peakUsed==64+offset&&s.peakTemp==offset);
 for(j=0;j<count;j++){
  int final;i=reverse?count-1-j:j;
  final=high?starts[i]==4096-offset:starts[i]+sizes[i]==offset;
  if(final)offset-=sizes[i];
  C1HP36_Free(&s,sizes[i],final,64,64+offset);
 }
 assert(s.freeCalls==count);
 if(reverse||count==0){assert(!offset&&!s.nonFinal&&!s.retained);}
 else{assert(s.nonFinal==count-1&&s.retained==offset&&s.peakRetained==offset);}
 C1HP36_ClearTemp(&s,offset);assert(!s.retained&&!s.clearedLiveBytes&&s.clearedBytes==offset);
}
int main(void){
 c1hp36Stats s={0};unsigned i;
 for(i=0;i<=21;i++){order(0,0,i);order(0,1,i);order(1,0,i);order(1,1,i);}
 C1HP36_Reset(&s,0,0);assert(s.epoch==1);
 C1HP36_Alloc(&s,32,0,32,32);C1HP36_Alloc(&s,16,1,32,48);C1HP36_Alloc(&s,40,1,32,88);
 C1HP36_Free(&s,16,0,32,88);assert(s.retained==16&&s.nonFinalBytes==16);
 C1HP36_ClearTemp(&s,56);assert(s.clearedLiveBytes==40&&s.clearedBytes==56&&!s.retained);
 assert(s.peakUsed==88&&s.peakPermanent==32&&s.peakTemp==56&&s.largestTemp==40);
 C1HP36_Reset(&s,16,16);assert(s.epoch==2&&s.peakUsed==16&&!s.tempCalls&&!s.peakRetained);
 puts("R36 counters and LIFO order: PASS");return 0;
}
