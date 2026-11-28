#include <assert.h>
#include <stdio.h>
#include "gsc_r237_queue_dirty54.h"
#define CAP 512
#define CASES 100000u
typedef struct {int status,enqueued,deadline;uint64_t stamp,serial,insertion;} Row;
static Row rows[CAP];static R54QEntry heap[CAP];static size_t pos[CAP],dirty[CAP];static unsigned char marked[CAP];
static uint64_t rng=UINT64_C(0xb213962eed3a6af1);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static int pending(Row r){return r.status==1||r.status==2;}
static int feed(R54Dirty*d,size_t i){Row r=rows[i];return R54D_Row(d,i,pending(r),r.enqueued,r.stamp,r.serial,r.insertion,r.deadline);}
static int verify(const R54Dirty*d,size_t i){Row r=rows[i];return R54D_VerifyRow(d,i,pending(r),r.enqueued,r.stamp,r.serial,r.insertion,r.deadline);}
static size_t flush(R54Dirty*d,uint64_t stamp,int cursor,size_t slots){size_t i,n=0;assert(R54D_Begin(d,stamp,cursor,slots));while(R54D_Next(d,&i)){assert(feed(d,i));++n;}assert(R54D_End(d));return n;}
static void check(const R54Dirty*d,uint64_t stamp,int cursor,size_t slots){
    size_t i,best=CAP,live=0;int has=-1,next=-1,want=INT32_MAX;R54QEntry e={0};
    for(i=0;i<slots;i++){assert(verify(d,i));if(pending(rows[i])){++live;if(rows[i].deadline==cursor){if(best==CAP||rows[i].insertion>rows[best].insertion)best=i;}else if(rows[i].deadline<want)want=rows[i].deadline;}}
    assert(R54D_Choice(d,stamp,cursor,&has,&e,&next));assert(has==(best!=CAP));
    if(has){assert(e.slot==best&&e.serial==rows[best].serial&&e.insertion==rows[best].insertion&&next==-1);}else assert(next==want);
    assert(d->queue.count==live);for(i=slots;i<CAP;i++)assert(!pos[i]);
    for(i=1;i<live;i++)assert(!R54Q_Before(heap[i],heap[(i-1)/2]));
}
int main(void){
    R54Dirty d;uint64_t stamp=0,serial=0,insertion=0,feeds=0,reference=0;unsigned step,reseeds=0,misses=0;size_t slots=CAP,i,j;int cursor=0;
    assert(R54D_Init(&d,heap,pos,dirty,marked,CAP));assert(flush(&d,stamp,cursor,slots)==CAP);
    for(step=0;step<CASES;step++){
        if(step%997==0){++stamp;cursor=0;memset(rows,0,sizeof(rows));}
        if(step%173==0){size_t count=rnd()%(CAP+1);for(i=count;i<CAP;i++)memset(&rows[i],0,sizeof(rows[i]));slots=count;}
        if(step%13==0){++cursor;for(i=0;i<slots;i++)if(pending(rows[i])&&rows[i].deadline<cursor){assert(R54D_Mark(&d,i));rows[i].status=0;}}
        for(j=0;j<4&&slots;j++){
            i=rnd()%slots;Row*r=rows+i;unsigned op=rnd()%5;
            assert(R54D_Mark(&d,i));assert(R54D_Mark(&d,i));
            if(op<=1){r->status=1+(int)(rnd()%2);r->enqueued=1;r->stamp=stamp;r->serial=++serial;r->insertion=++insertion;r->deadline=cursor+(int)(rnd()%8);}
            else if(op==2)r->status=0;else if(op==3)r->status=3;
            else if(pending(*r)){r->deadline=cursor;r->insertion=++insertion;}
        }
        if(step%211==0)R54D_Invalidate(&d);
        feeds+=flush(&d,stamp,cursor,slots);reference+=slots;reseeds+=(unsigned)d.full;check(&d,stamp,cursor,slots);
        if(step%101==0&&slots){/* Deliberately omit a notification; full audit must detect it even off-head. */
            i=rnd()%slots;rows[i].status=1;rows[i].enqueued=1;rows[i].stamp=stamp;rows[i].serial=++serial;rows[i].insertion=++insertion;rows[i].deadline=cursor+8;
            assert(!verify(&d,i));++misses;R54D_Invalidate(&d);feeds+=flush(&d,stamp,cursor,slots);reference+=slots;++reseeds;check(&d,stamp,cursor,slots);
        }
    }
    /* Incomplete, invalid, stale and over-capacity transactions fail closed. */
    R54D_Invalidate(&d);memset(rows,0,sizeof(rows));assert(flush(&d,0,0,2)==2);
    assert(R54D_Mark(&d,0));assert(R54D_Begin(&d,0,0,2));assert(!R54D_End(&d));assert(d.state==R54D_INVALID);
    assert(flush(&d,0,0,2)==2);assert(R54D_Mark(&d,2));assert(!R54D_Begin(&d,0,0,2));assert(flush(&d,0,0,2)==2);
    assert(!R54D_Mark(&d,CAP));assert(!R54D_Begin(&d,0,0,CAP+1));assert(flush(&d,0,0,2)==2);
    rows[0]=(Row){1,1,1,0,1,1};assert(R54D_Mark(&d,0));assert(flush(&d,0,0,2)==1);
    assert(R54D_Begin(&d,0,2,2));assert(!R54D_End(&d));rows[0].status=0;assert(flush(&d,0,2,2)==2);
    assert(R54D_Mark(&d,1));assert(R54D_Begin(&d,0,2,2));assert(!R54D_Mark(&d,0));assert(!R54D_End(&d));
    assert(feeds<reference);printf("R54D PASS: %u boundaries, %llu dirty feeds / %llu full-scan rows, %u reseeds, %u omitted mutations detected\n",CASES,(unsigned long long)feeds,(unsigned long long)reference,reseeds,misses);return 0;
}
