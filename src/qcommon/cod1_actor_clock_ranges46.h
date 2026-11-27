#ifndef COD1_ACTOR_CLOCK_RANGES46_H
#define COD1_ACTOR_CLOCK_RANGES46_H
/* Derived read-only metadata address index. The caller owns the table and
 * proof for the same synchronous resource lifetime. No metadata content is
 * cached or validated here. Exact table mismatch uses the reference scan. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
 unsigned count,used;
 const c1anm14Metadata_t *table[C1AGF14_MAX];
 uintptr_t start[C1AGF14_MAX];
} c1agc46Ranges_t;
static int C1AGC46_AddressCompare(const void*a,const void*b){
 uintptr_t x=*(const uintptr_t*)a,y=*(const uintptr_t*)b;
 return (x>y)-(x<y);
}
static int C1AGC46_Prepare(const c1anm14Metadata_t*const*m,unsigned count,c1agc46Ranges_t*out){
 unsigned i;
 if(!m||!out||!count||count>C1AGF14_MAX||C1ANC14_Overlap(m,count*sizeof(*m),out,sizeof(*out)))return 0;
 out->count=0;out->used=0;
 memcpy(out->table,m,count*sizeof(*m));
 for(i=0;i<count;i++)if(m[i])out->start[out->used++]=(uintptr_t)m[i];
 qsort(out->start,out->used,sizeof(*out->start),C1AGC46_AddressCompare);
 out->count=count;return 1;
}
/* All indexed objects have exactly sizeof(c1anm14Metadata_t) bytes.
 * The greatest start < x and the first start >= x are sufficient.
 * Keep the original subtractive (non-wrapping) overlap semantics. */
static int C1AGC46_Overlap(const c1agc46Ranges_t*r,const void*p,size_t n){
 uintptr_t x=(uintptr_t)p;unsigned lo=0,hi=r->used;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(r->start[mid]<x)lo=mid+1;else hi=mid;}
 return (lo&&x-r->start[lo-1]<sizeof(c1anm14Metadata_t))||
        (lo<r->used&&r->start[lo]-x<n);
}
/* -1: use old scan; 0: overlapping; 1: no overlap. Only the internal
 * protected map owner may supply a Build-produced proof. Recheck every
 * pointer in the table; the indexed addresses are not metadata identities. */
static int C1AGC46_Admit(const c1agc46Ranges_t*r,const c1anm14Metadata_t*const*m,unsigned count,
 const void*a,size_t an,const void*b,size_t bn,const void*c,size_t cn){
 if(!r||!m||!count||count>C1AGF14_MAX||r->count!=count||r->used>count||
    C1ANC14_Overlap(a,an,r,sizeof(*r))||C1ANC14_Overlap(b,bn,r,sizeof(*r))||C1ANC14_Overlap(c,cn,r,sizeof(*r))||
    memcmp(r->table,m,count*sizeof(*m)))return -1;
 return !C1AGC46_Overlap(r,a,an)&&!C1AGC46_Overlap(r,b,bn)&&!C1AGC46_Overlap(r,c,cn);
}
#endif
