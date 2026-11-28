#include <assert.h>
#include <stdio.h>
#include "gsc_r237_queue_sync54.h"
#define CAP 256
#define CASES 50000
/* Model of committed R237 rows. Status is separate from stale queue bits. */
typedef struct {int status,enqueued,deadline;uint64_t stamp,serial,insertion;} Row;
static Row rows[CAP];static R54QEntry heap[CAP];static size_t pos[CAP];
static uint64_t rng=UINT64_C(0xb81c593f209d7a61);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static int pending(Row r){return r.status==1||r.status==2;}
static int feed(R54Sync*s,size_t n){Row r=rows[n];return R54S_Row(s,n,pending(r),r.enqueued,r.stamp,r.serial,r.insertion,r.deadline);}
/* Reference selection mirrors R237's scan for valid rows and unique IDs. */
static void check(R54Sync*s,size_t slots,uint64_t stamp,int cursor){
    size_t i,best=CAP,live=0;int next=INT32_MAX,actual=-1,has=-1;R54QEntry e={0},keep={0};
    for(i=0;i<slots;i++)if(pending(rows[i])){++live;if(rows[i].deadline==cursor){if(best==CAP||rows[i].insertion>rows[best].insertion)best=i;}else if(rows[i].deadline<next)next=rows[i].deadline;}
    assert(R54S_Choice(s,stamp,cursor,&has,&e,&actual));assert(has==(best!=CAP));
    if(has){assert(e.slot==best&&e.serial==rows[best].serial&&e.insertion==rows[best].insertion&&e.deadline==cursor);assert(actual==-1);}else{assert(actual==next);assert(R54Q_Equal(e,keep));}
    assert(s->queue.count==live);
    for(i=0;i<slots;i++)if(pending(rows[i])){assert(pos[i]&&pos[i]<=live);e=heap[pos[i]-1];assert(e.slot==i&&e.serial==rows[i].serial&&e.insertion==rows[i].insertion&&e.deadline==rows[i].deadline);}else assert(!pos[i]);
    for(i=slots;i<CAP;i++)assert(!pos[i]);
    for(i=1;i<live;i++)assert(!R54Q_Before(heap[i],heap[(i-1)/2]));
    assert(!R54S_Choice(s,stamp+1,cursor,&has,&e,&actual));assert(!R54S_Choice(s,stamp,cursor+1,&has,&e,&actual));
}
int main(void){
    R54Sync s;size_t slots=0,i,j;uint64_t stamp=0,serial=0,insertion=0,changes=0,feeds=0;unsigned resets=0,partial=0,invalid=0;int cursor=0;
    assert(R54S_Init(&s,heap,pos,CAP));assert(!R54S_Begin(&s,0,0,CAP+1));assert(!R54S_Begin(&s,0,-1,0));
    for(i=0;i<CASES;i++){
        int has=-1,next=-1;R54QEntry e={0};
        if(i%997==0){++stamp;slots=0;memset(rows,0,sizeof(rows));cursor=0;++resets;}
        if(i%173==0){slots=rnd()%(CAP+1);for(j=slots;j<CAP;j++)memset(&rows[j],0,sizeof(rows[j]));}
        if(slots<CAP&&rnd()%4==0)++slots;
        if(i%13==0){++cursor;for(j=0;j<slots;j++)if(pending(rows[j])&&rows[j].deadline<cursor)rows[j].status=0;}
        if(slots)for(j=0;j<4;j++){
            size_t k=rnd()%slots;Row *r=&rows[k];unsigned op=rnd()%5;
            if(op<=1){r->status=1+(int)(rnd()%2);r->enqueued=1;r->stamp=stamp;r->serial=++serial;r->insertion=++insertion;r->deadline=cursor+(int)(rnd()%5);}
            else if(op==2)r->status=0;else if(op==3)r->status=3;
            else if(pending(*r)){r->deadline=cursor;r->insertion=++insertion;}
        }
        assert(R54S_Begin(&s,stamp,cursor,slots));assert(!R54S_Choice(&s,stamp,cursor,&has,&e,&next));
        if(slots&&i%191==0){assert(feed(&s,0));assert(!R54S_End(&s)||slots==1);R54S_Invalidate(&s);++partial;assert(R54S_Begin(&s,stamp,cursor,slots));}
        if(slots&&i%223==0){assert(!R54S_Row(&s,0,1,1,stamp+1,1,1,cursor));assert(!R54S_End(&s));++invalid;assert(R54S_Begin(&s,stamp,cursor,slots));}
        for(j=0;j<slots;j++){assert(feed(&s,j));++feeds;}
        assert(R54S_End(&s));check(&s,slots,stamp,cursor);changes+=s.changes;
        /* Unchanged snapshot must make no index changes. */
        if(i%67==0){assert(R54S_Begin(&s,stamp,cursor,slots));for(j=0;j<slots;j++)assert(feed(&s,j));assert(R54S_End(&s)&&s.changes==0);check(&s,slots,stamp,cursor);}
    }
    assert(R54S_Begin(&s,0,0,1));assert(!R54S_Row(&s,0,1,0,0,1,1,0));assert(!R54S_End(&s));
    assert(R54S_Begin(&s,0,0,1));assert(!R54S_Row(&s,0,1,1,0,0,1,0));
    assert(R54S_Begin(&s,0,1,1));assert(!R54S_Row(&s,0,1,1,0,1,1,0));
    assert(R54S_Begin(&s,0,0,1));assert(!R54S_Row(&s,1,0,0,0,0,0,0));
    assert(R54S_Begin(&s,0,0,1));assert(R54S_Row(&s,0,0,1,99,0,0,-1));assert(R54S_End(&s));assert(s.queue.count==0);
    printf("R54B PASS: %u snapshots, %llu rows, %llu changes, %u resets, %u partial, %u invalid\n",CASES,(unsigned long long)feeds,(unsigned long long)changes,resets,partial,invalid);return 0;
}
