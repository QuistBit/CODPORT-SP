#ifndef COD1_ANIM_PROFILE32_H
#define COD1_ANIM_PROFILE32_H
#include <stdint.h>
#include <string.h>
#define C1AP32_OWNERS 1024u
#define C1AP32_WINDOW 250
/* Independent inclusive intervals on the CGame owner thread. */
enum { C1AP32_ACTOR, C1AP32_RESOURCE, C1AP32_HIT, C1AP32_MISS,
 C1AP32_SOURCE, C1AP32_CLIP, C1AP32_MAP, C1AP32_TREE, C1AP32_SKELETON,
 C1AP32_BLEND, C1AP32_SUBMIT, C1AP32_STAGES };
typedef struct {
 uint64_t sum, frameSum, maxFrame, maxCall;
 unsigned calls, failed;
} c1ap32Stage;
typedef struct {
 uint64_t revision;
 int first, last, frame;
 unsigned frames, actors, decodedLadderActors, maxNodes, maxLeaves, ladderMask;
 unsigned char seen[C1AP32_OWNERS], ladderSeen[C1AP32_OWNERS];
 c1ap32Stage stage[C1AP32_STAGES];
} c1ap32Window;
static void C1AP32_Reset(c1ap32Window *w){memset(w,0,sizeof(*w));}
static int C1AP32_Frame(c1ap32Window *w,c1ap32Window *completed,uint64_t revision,int now,int frame){
 unsigned i;int ready=0;
 if(!revision||now<0)return 0;
 if(w->frames&&(w->revision!=revision||now<w->last||frame<w->frame))C1AP32_Reset(w);
 if(w->frames&&frame==w->frame)return 0;
 if(w->frames&&(int64_t)now-w->first>=C1AP32_WINDOW){*completed=*w;C1AP32_Reset(w);ready=1;}
 if(!w->frames){w->revision=revision;w->first=now;}
 w->last=now;w->frame=frame;w->frames++;
 for(i=0;i<C1AP32_STAGES;i++)w->stage[i].frameSum=0;
 return ready;
}
static void C1AP32_Record(c1ap32Window *w,unsigned stage,uint64_t elapsed,int ok,unsigned owner){
 c1ap32Stage *s;
 if(!w->frames||stage>=C1AP32_STAGES)return;
 s=w->stage+stage;s->calls++;if(!ok)s->failed++;
 s->sum+=elapsed;s->frameSum+=elapsed;
 if(s->frameSum>s->maxFrame)s->maxFrame=s->frameSum;
 if(elapsed>s->maxCall)s->maxCall=elapsed;
 if(stage==C1AP32_ACTOR&&owner<C1AP32_OWNERS&&!w->seen[owner]){w->seen[owner]=1;w->actors++;}
}
static unsigned C1AP32_Ladder(unsigned ordinal){return ordinal>=318u&&ordinal<=321u?1u<<(ordinal-318u):0u;}
static void C1AP32_Decoded(c1ap32Window *w,unsigned owner,unsigned nodes,unsigned leaves,unsigned ladder){
 if(!w->frames)return;
 if(nodes>w->maxNodes)w->maxNodes=nodes;
 if(leaves>w->maxLeaves)w->maxLeaves=leaves;
 w->ladderMask|=ladder;
 if(ladder&&owner<C1AP32_OWNERS&&!w->ladderSeen[owner]){w->ladderSeen[owner]=1;w->decodedLadderActors++;}
}
#endif
