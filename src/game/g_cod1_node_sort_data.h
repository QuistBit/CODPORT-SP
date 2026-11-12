#ifndef C1NAV14_NODE_SORT_DATA_H
#define C1NAV14_NODE_SORT_DATA_H
#include <stddef.h>
#include <stdint.h>
/* DATA-only exact sort of finalized 0x74-byte native node records. This
 * neither produces their keys/poses/script handles nor owns a native pool.
 * GAME 2006FF10 comparator, 200748B0 shortsort and 20074920 qsort. */
#define C1NAV14_RECORD_BYTES 0x74u
static uint32_t C1NAV14_SortU16(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8);}
static uint32_t C1NAV14_SortU32(const unsigned char*p){return C1NAV14_SortU16(p)|(C1NAV14_SortU16(p+2)<<16);}
static int32_t C1NAV14_SortS16(uint32_t u){u&=65535u;return u>=32768u?(int32_t)u-65536:(int32_t)u;}
static int32_t C1NAV14_NodeCompare(const unsigned char*a,const unsigned char*b){
 int32_t r;uint32_t u;
 r=C1NAV14_SortS16(C1NAV14_SortU16(b+0x50)-C1NAV14_SortU16(a+0x50));if(r)return r;
 r=C1NAV14_SortS16(C1NAV14_SortU16(a+0x52)-C1NAV14_SortU16(b+0x52));if(r)return r;
 r=C1NAV14_SortS16(C1NAV14_SortU16(a+0x54)-C1NAV14_SortU16(b+0x54));if(r)return r;
 u=C1NAV14_SortU32(b+0x24)-C1NAV14_SortU32(a+0x24);
 return u>=0x80000000u?(int32_t)((int64_t)u-INT64_C(4294967296)):(int32_t)u;
}
static unsigned char*C1NAV14_SortAt(unsigned char*p,int i){return p+(size_t)i*C1NAV14_RECORD_BYTES;}
static int32_t C1NAV14_SortCompare(unsigned char*p,int a,int b){return C1NAV14_NodeCompare(C1NAV14_SortAt(p,a),C1NAV14_SortAt(p,b));}
static void C1NAV14_SortSwap(unsigned char*p,int a,int b){
 unsigned i;unsigned char t,*x,*y;if(a==b)return;x=C1NAV14_SortAt(p,a);y=C1NAV14_SortAt(p,b);
 for(i=0;i<C1NAV14_RECORD_BYTES;i++){t=x[i];x[i]=y[i];y[i]=t;}
}
static int C1NAV14_NodeSort(unsigned char*p,size_t bytes,unsigned count){
 int lo=0,hi,mid,l,r,max,i,n=0,los[32],his[32];size_t need;
 if(count>2048u)return 0;need=(size_t)count*C1NAV14_RECORD_BYTES;
 if(bytes<need||(need&&(!p||(uintptr_t)p>UINTPTR_MAX-need)))return 0;
 if(count<2)return 1;hi=(int)count-1;
 for(;;){
  if(hi-lo+1<=8){
   while(hi>lo){max=lo;for(i=lo+1;i<=hi;i++)if(C1NAV14_SortCompare(p,i,max)>0)max=i;C1NAV14_SortSwap(p,max,hi);hi--;}
  }else{
   mid=lo+(hi-lo+1)/2;
   if(C1NAV14_SortCompare(p,lo,mid)>0)C1NAV14_SortSwap(p,lo,mid);
   if(C1NAV14_SortCompare(p,lo,hi)>0)C1NAV14_SortSwap(p,lo,hi);
   if(C1NAV14_SortCompare(p,mid,hi)>0)C1NAV14_SortSwap(p,mid,hi);
   l=lo;r=hi;
   for(;;){
    if(mid>l){do{l++;}while(l<mid&&C1NAV14_SortCompare(p,l,mid)<=0);}
    if(mid<=l){do{l++;}while(l<=hi&&C1NAV14_SortCompare(p,l,mid)<=0);}
    do{r--;}while(r>mid&&C1NAV14_SortCompare(p,r,mid)>0);
    if(l>r)break;
    C1NAV14_SortSwap(p,l,r);if(mid==r)mid=l;
   }
   r++;
   if(mid<r){do{r--;}while(r>mid&&C1NAV14_SortCompare(p,r,mid)==0);}
   if(mid>=r){do{r--;}while(r>lo&&C1NAV14_SortCompare(p,r,mid)==0);}
   /* Native uses signed byte-span sizes; fixed positive width preserves
    * this comparison. On ties, defer left and process right immediately.
    * Processing the smaller partition first bounds stack depth by log2 N. */
   if(r-lo>=hi-l){
    if(lo<r){los[n]=lo;his[n++]=r;}
    if(l<hi){lo=l;continue;}
   }else{
    if(l<hi){los[n]=l;his[n++]=hi;}
    if(lo<r){hi=r;continue;}
   }
  }
  if(!n)break;--n;lo=los[n];hi=his[n];
 }
 return 1;
}
#endif
