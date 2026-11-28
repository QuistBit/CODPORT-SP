#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef struct { unsigned char pad[32]; } Task;
typedef struct { int task_count; Task *tasks[1025]; void *r54_dirty_scope; } GscRuntime;
static Task *task_at(const GscRuntime*r,int i){return r->tasks[i];}
#include "gsc_vm_r237_dirty_scope54.inc"
static Task bank[8192];
static void seed(R54DirtyScope*s,int count){
 size_t i;
 assert(R54D_Begin(&s->journal,7,0,(size_t)count));
 while(R54D_Next(&s->journal,&i))assert(R54D_Row(&s->journal,i,0,0,0,0,0,0));
 assert(R54D_End(&s->journal));
}
int main(void){
 R54DirtyScope s={0};GscRuntime r={0};Task foreign;size_t i,j;unsigned collisions=0;
 assert(R54D_Init(&s.journal,s.heap,s.positions,s.dirty,s.marked,R54_DIRTY_CAPACITY));
 r.r54_dirty_scope=&s;
 /* Find two genuine allocated objects with colliding hash values. */
 for(i=0;i<8192&&!collisions;i++)for(j=i+1;j<8192;j++)if(r54_dirty_hash(bank+i)==r54_dirty_hash(bank+j)){r.tasks[0]=bank+i;r.tasks[1]=bank+j;collisions=1;break;}
 assert(collisions);r.task_count=2;seed(&s,2);
 r54_dirty_mark_task(&r,r.tasks[1]);assert(s.journal.count==1&&s.journal.dirty[0]==1&&s.probes>=2);
 r54_dirty_mark_task(&r,r.tasks[1]);assert(s.journal.count==1);
 r54_dirty_mark_task(&r,r.tasks[0]);assert(s.journal.count==2);seed(&s,2);
 for(i=2,j=0;i<1024;i++){while(bank+j==r.tasks[0]||bank+j==r.tasks[1])++j;r.tasks[i]=bank+j++;}
 r.task_count=1024;r54_dirty_mark_task(&r,r.tasks[1023]);assert(s.mapped==1024&&s.journal.count==1&&s.journal.dirty[0]==1023);seed(&s,1024);
 for(i=0;i<1024;i++){r54_dirty_mark_task(&r,r.tasks[i]);assert(s.journal.dirty[i]==i);}
 assert(s.journal.count==1024);seed(&s,1024);
 r54_dirty_mark_task(&r,&foreign);assert(s.journal.state==R54D_INVALID&&s.misses==1&&!s.mapped);seed(&s,1024);
 r54_dirty_mark_task(&r,r.tasks[17]);assert(s.journal.count==1&&s.journal.dirty[0]==17);seed(&s,1024);
 r.task_count=1025;r54_dirty_mark_task(&r,r.tasks[0]);assert(s.journal.state==R54D_INVALID&&s.misses==2);r.task_count=2;seed(&s,2);
 r.tasks[1]=r.tasks[0];r54_dirty_mark_task(&r,r.tasks[0]);assert(s.journal.state==R54D_INVALID&&s.misses==3);
 r.tasks[1]=&foreign;seed(&s,2);r54_dirty_mark_task(&r,r.tasks[1]);assert(s.journal.count==1&&s.journal.dirty[0]==1);seed(&s,2);
 r.task_count=1;r54_dirty_mark_task(&r,r.tasks[0]);assert(s.journal.state==R54D_INVALID&&s.misses==4);seed(&s,1);r54_dirty_mark_task(&r,r.tasks[0]);assert(s.journal.count==1&&s.journal.dirty[0]==0);
 puts("R54D3 slot map PASS: collisions, 1024 slots, growth, duplicate marks, foreign/duplicate pointers, overflow, shrink and reseed");return 0;
}
