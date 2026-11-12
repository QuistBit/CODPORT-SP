#ifndef G_COD1_NATIVE_THREAD_FRAME_POLICY_H
#define G_COD1_NATIVE_THREAD_FRAME_POLICY_H
#include <stdint.h>
/* Pure clock admission, never owner/readiness authority. R237 uses a bounded
 * nonwrapping24-bit cursor; duplicate engine calls do not reopen a frame. */
enum { C1_NATIVE_FRAME_REFUSE=0,C1_NATIVE_FRAME_SAME=1,C1_NATIVE_FRAME_ADVANCE=2 };
static int C1NativeThreadFrameDecision(int64_t origin,int64_t last,int logical,
 int physical,int open,int now){
 int64_t elapsed;
 if(open||origin<0||last<origin||now<last)return C1_NATIVE_FRAME_REFUSE;
 elapsed=(int64_t)now-origin;
 if(physical<0||elapsed<physical||elapsed>0xffffff||logical!=physical||last-origin!=physical)
  return C1_NATIVE_FRAME_REFUSE;
 return elapsed==physical?C1_NATIVE_FRAME_SAME:C1_NATIVE_FRAME_ADVANCE;
}
#endif
