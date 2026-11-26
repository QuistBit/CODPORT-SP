#ifndef COD1_ANIM_PROFILE31_H
#define COD1_ANIM_PROFILE31_H
#include <stdint.h>
#include <string.h>
/* Diagnostic only: one independent accumulator per measured call site.
 * Game-time windows, raw monotonic counter ticks; no heap or state mutation. */
#define C1AP31_OWNERS 1024u
#define C1AP31_WINDOW 250

typedef struct {
 uint64_t epoch, sum, tickSum, maxTick, maxCall, iterations;
 int first, last;
 unsigned calls, failures, actors, ladderActors, maxIterations, maxNodes, maxLeaves, ladderMask;
 unsigned char seen[C1AP31_OWNERS], ladderSeen[C1AP31_OWNERS];
} c1ap31Window;

static void C1AP31_Reset(c1ap31Window *w){memset(w,0,sizeof(*w));}
static unsigned C1AP31_Ladder(unsigned ordinal){
 return ordinal>=318u&&ordinal<=321u?1u<<(ordinal-318u):0u;
}
/* Return a completed window before adding the new observation. Epoch changes
 * and backwards ticks discard a partial window rather than mixing maps. */
static int C1AP31_Record(c1ap31Window *w,c1ap31Window *completed,uint64_t epoch,int now,
 unsigned owner,uint64_t elapsed,int ok,unsigned iterations,unsigned nodes,unsigned leaves,unsigned ladder){
 int ready=0;
 if(!epoch||now<0)return 0;
 if(w->calls&&(w->epoch!=epoch||now<w->last))C1AP31_Reset(w);
 if(w->calls&&(int64_t)now-w->first>=C1AP31_WINDOW){*completed=*w;C1AP31_Reset(w);ready=1;}
 if(!w->calls){w->epoch=epoch;w->first=w->last=now;}
 if(now!=w->last){w->tickSum=0;w->last=now;}
 w->calls++;if(!ok)w->failures++;
 if(owner<C1AP31_OWNERS&&!w->seen[owner]){w->seen[owner]=1;w->actors++;}
 if(ladder&&owner<C1AP31_OWNERS&&!w->ladderSeen[owner]){w->ladderSeen[owner]=1;w->ladderActors++;}
 w->sum+=elapsed;w->tickSum+=elapsed;
 if(elapsed>w->maxCall)w->maxCall=elapsed;
 if(w->tickSum>w->maxTick)w->maxTick=w->tickSum;
 w->iterations+=iterations;if(iterations>w->maxIterations)w->maxIterations=iterations;
 if(nodes>w->maxNodes)w->maxNodes=nodes;
 if(leaves>w->maxLeaves)w->maxLeaves=leaves;
 w->ladderMask|=ladder;return ready;
}
#endif
