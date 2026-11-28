#include <assert.h>
#include <stdio.h>
#include "gsc_r237_queue_index54.h"
#define CAP 1024
#define CASES 300000
static R54QEntry heap[CAP],ref[CAP];
static size_t pos[CAP];
static unsigned char live[CAP];
static uint64_t rng=UINT64_C(0x739bf0218a5c2d01);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
/* Independent linear selection, earliest deadline then newest insertion. */
static int reference(R54QEntry *out){
    size_t i;int found=0;
    for(i=0;i<CAP;i++)if(live[i]&&(!found||ref[i].deadline<out->deadline||(ref[i].deadline==out->deadline&&ref[i].insertion>out->insertion))){*out=ref[i];found=1;}
    return found;
}
static void check(const R54Queue*q){
    R54QEntry a={0},b={0};size_t i,count=0;int x=reference(&a),y=R54Q_Peek(q,q->stamp,&b);
    assert(x==y);if(x)assert(R54Q_Equal(a,b));
    for(i=0;i<CAP;i++){if(live[i]){count++;assert(pos[i]&&pos[i]<=q->count&&R54Q_Equal(heap[pos[i]-1],ref[i]));}else assert(!pos[i]);}
    assert(count==q->count);
    for(i=1;i<q->count;i++)assert(!R54Q_Before(heap[i],heap[(i-1)/2]));
}
int main(void){
    R54Queue q;R54QEntry a={0},b={0};uint64_t stamp=7,serial=0,insertion=0;unsigned i,puts=0,removes=0,pops=0,requeues=0,refusals=0,resets=0;size_t slot;
    assert(!R54Q_Init(&q,heap,pos,0,stamp));assert(R54Q_Init(&q,heap,pos,CAP,stamp));
    for(i=0;i<CASES;i++){
        unsigned op=rnd()%12;slot=rnd()%CAP;
        if(i%9973==0){++stamp;assert(R54Q_Init(&q,heap,pos,CAP,stamp));memset(live,0,sizeof(live));resets++;}
        if(op<5){
            if(!live[slot]){a=(R54QEntry){slot,++serial,++insertion,(int)(rnd()%64)*50};assert(R54Q_Put(&q,stamp,a));ref[slot]=a;live[slot]=1;puts++;}
            else {a=ref[slot];assert(R54Q_Put(&q,stamp,a));a.insertion++;assert(!R54Q_Put(&q,stamp,a));refusals++;}
        }else if(op<8){
            if(live[slot]){a=ref[slot];b=a;b.serial++;assert(!R54Q_Remove(&q,stamp,b));assert(!R54Q_Remove(&q,stamp+1,a));refusals+=2;assert(R54Q_Remove(&q,stamp,a));live[slot]=0;removes++;}
            else {a=(R54QEntry){slot,1,1,0};assert(!R54Q_Remove(&q,stamp,a));refusals++;}
        }else if(op<11){
            int found=reference(&a);assert(R54Q_Peek(&q,stamp,&b)==found);
            if(found){assert(R54Q_Equal(a,b));assert(R54Q_Remove(&q,stamp,b));live[b.slot]=0;pops++;
                if(op==10){b.insertion=++insertion;assert(R54Q_Put(&q,stamp,b));ref[b.slot]=b;live[b.slot]=1;requeues++;}}
        }else{
            a=(R54QEntry){CAP,1,1,0};assert(!R54Q_Put(&q,stamp,a));a.slot=slot;a.deadline=-1;assert(!R54Q_Put(&q,stamp,a));a.deadline=0x1000000;assert(!R54Q_Put(&q,stamp,a));a.deadline=0;a.insertion=0;assert(!R54Q_Put(&q,stamp,a));a.insertion=1;a.serial=0;assert(!R54Q_Put(&q,stamp,a));a.serial=1;assert(!R54Q_Put(&q,stamp+1,a));refusals+=6;
        }
        check(&q);
    }
    /* Dense equal-deadline batch: newest insertion first, even after reuse. */
    assert(R54Q_Init(&q,heap,pos,CAP,0));memset(live,0,sizeof(live));
    for(slot=0;slot<CAP;slot++){a=(R54QEntry){slot,++serial,++insertion,0xffffff};assert(R54Q_Put(&q,0,a));ref[slot]=a;live[slot]=1;}
    check(&q);for(slot=CAP;slot;slot--){assert(R54Q_Peek(&q,0,&a)&&a.slot==slot-1);assert(R54Q_Remove(&q,0,a));live[a.slot]=0;check(&q);}
    assert(!R54Q_Peek(&q,0,&a));printf("R54A PASS: %u operations, puts=%u removes=%u pops=%u requeues=%u refusals=%u resets=%u, dense=%u\n",CASES,puts,removes,pops,requeues,refusals,resets,CAP);return 0;
}
