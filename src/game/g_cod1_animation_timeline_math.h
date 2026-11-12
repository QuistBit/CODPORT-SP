#ifndef G_COD1_ANIMATION_TIMELINE_MATH_H
#define G_COD1_ANIMATION_TIMELINE_MATH_H
/* R187 private GAME timeline math. No host floating-point arithmetic.
 * Bounded native branch: goal 0/1, current 0 or [2^-24,1], dyadic power-of-two
 * blend/remaining/dt in [2^-24,1] (zero permitted). For these inputs every
 * extended expression below has at most 49 significant bits. PC53/64 are
 * therefore exact between the original FST/FSTP32 boundaries; only those
 * stores round, under the explicitly declared RC. General fade retargeting,
 * non-power-of-two time steps and PC24 are held. */
#include <stdint.h>
#if defined(_MSC_VER)
#define COD1_TIMELINE_FN static __inline
#else
#define COD1_TIMELINE_FN static __inline__
#endif
typedef struct { uint32_t precision_bits,rounding; } cod1AnimationTimelinePolicy_t;
typedef struct { uint64_t magnitude; int exponent; unsigned sign; } cod1TimelineDyadic_t;
COD1_TIMELINE_FN int Cod1TimelinePolicy(const cod1AnimationTimelinePolicy_t *p) {
    return p&&(p->precision_bits==53||p->precision_bits==64)&&p->rounding<=3;
}
COD1_TIMELINE_FN cod1TimelineDyadic_t Cod1TimelineNormalize(cod1TimelineDyadic_t a) {
    if(!a.magnitude){a.exponent=0;return a;}
    while(!(a.magnitude&1)){a.magnitude>>=1;++a.exponent;}return a;
}
COD1_TIMELINE_FN cod1TimelineDyadic_t Cod1TimelineBits(uint32_t bits) {
    cod1TimelineDyadic_t a;a.sign=bits>>31;a.magnitude=bits&UINT32_C(0x7fffffff);a.exponent=0;
    if(a.magnitude){a.exponent=(int)(a.magnitude>>23)-150;a.magnitude=UINT32_C(0x800000)|(a.magnitude&UINT32_C(0x7fffff));}return Cod1TimelineNormalize(a);
}
COD1_TIMELINE_FN int Cod1TimelineAdd(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,unsigned rc,cod1TimelineDyadic_t *out) {
    uint64_t av,bv;int exponent,gap;cod1TimelineDyadic_t v;
    if(!a.magnitude&&!b.magnitude){v=a;v.sign=a.sign==b.sign?a.sign:rc==1;*out=v;return 1;}
    if(!a.magnitude){*out=b;return 1;}if(!b.magnitude){*out=a;return 1;}
    exponent=a.exponent<b.exponent?a.exponent:b.exponent;gap=a.exponent-exponent;
    if(gap>=64||a.magnitude>(UINT64_MAX>>gap))return 0;
    av=a.magnitude<<gap;gap=b.exponent-exponent;
    if(gap>=64||b.magnitude>(UINT64_MAX>>gap))return 0;
    bv=b.magnitude<<gap;v.exponent=exponent;
    if(a.sign==b.sign){if(UINT64_MAX-av<bv)return 0;v.magnitude=av+bv;v.sign=a.sign;}
    else if(av>=bv){v.magnitude=av-bv;v.sign=v.magnitude?a.sign:rc==1;}
    else {v.magnitude=bv-av;v.sign=b.sign;}
    *out=Cod1TimelineNormalize(v);return 1;
}
COD1_TIMELINE_FN int Cod1TimelineStore(cod1TimelineDyadic_t a,unsigned rc,uint32_t *out) {
    uint64_t scan,q,remainder,half;int top=0,biased,shift;unsigned increment=0;
    if(!out||rc>3)return 0;
    if(!a.magnitude){*out=a.sign?UINT32_C(0x80000000):0;return 1;}
    scan=a.magnitude;while(scan>>1){scan>>=1;++top;}biased=top+a.exponent+127;if(biased<1||biased>254)return 0;shift=top-23;
    if(shift>0){q=a.magnitude>>shift;remainder=a.magnitude&((UINT64_C(1)<<shift)-1);half=UINT64_C(1)<<(shift-1);
        if(rc==0)increment=remainder>half||(remainder==half&&(q&1));else if(rc==1)increment=a.sign&&remainder;else if(rc==2)increment=!a.sign&&remainder;
        q+=increment;if(q==UINT64_C(0x1000000)){q>>=1;if(++biased>254)return 0;}}
    else q=a.magnitude<<(23-top);
    *out=(a.sign?UINT32_C(0x80000000):0)|((uint32_t)biased<<23)|((uint32_t)q&UINT32_C(0x7fffff));return 1;
}
COD1_TIMELINE_FN int Cod1TimelineCompare(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,int *order) {
    cod1TimelineDyadic_t difference;b.sign^=1;if(!Cod1TimelineAdd(a,b,0,&difference))return 0;
    *order=!difference.magnitude?0:difference.sign?-1:1;return 1;
}
COD1_TIMELINE_FN int Cod1TimelineWeightInput(uint32_t bits) {
    return !bits||(bits>=UINT32_C(0x33800000)&&bits<=UINT32_C(0x3f800000));
}
COD1_TIMELINE_FN int Cod1TimelineTimeInput(uint32_t bits,int zero) {
    return (!bits&&zero)||(!(bits&UINT32_C(0x807fffff))&&bits>=UINT32_C(0x33800000)&&bits<=UINT32_C(0x3f800000));
}
COD1_TIMELINE_FN int Cod1AnimTimelineGoal(uint32_t current,uint32_t goal,uint32_t blend,const cod1AnimationTimelinePolicy_t *policy,uint32_t *newcurrent,uint32_t *newremaining) {
    cod1TimelineDyadic_t w,g,b,delta,product,epsilon;uint32_t db,weight,remaining;int order;
    if(!newcurrent||!newremaining||!Cod1TimelinePolicy(policy)||!Cod1TimelineWeightInput(current)||(goal&&goal!=UINT32_C(0x3f800000))||!Cod1TimelineTimeInput(blend,1))return 0;
    w=Cod1TimelineBits(current);g=Cod1TimelineBits(goal);b=Cod1TimelineBits(blend);epsilon=Cod1TimelineBits(UINT32_C(0x3a83126f));
    w.sign^=1;if(!Cod1TimelineAdd(g,w,policy->rounding,&delta)||!Cod1TimelineStore(delta,policy->rounding,&db))return 0;
    delta=Cod1TimelineBits(db&UINT32_C(0x7fffffff));product=delta;product.magnitude*=b.magnitude;product.exponent+=b.exponent;product.sign=0;product=Cod1TimelineNormalize(product);
    if(!Cod1TimelineStore(product,policy->rounding,&remaining)||!Cod1TimelineCompare(product,epsilon,&order))return 0;
    weight=current;
    if(order<0){remaining=0;weight=goal;}
    else if(!current){product=g;product.magnitude*=epsilon.magnitude;product.exponent+=epsilon.exponent;if(!Cod1TimelineStore(Cod1TimelineNormalize(product),policy->rounding,&weight))return 0;}
    *newcurrent=weight;*newremaining=remaining;return 1;
}
COD1_TIMELINE_FN int Cod1AnimTimelineWeight(uint32_t current,uint32_t goal,uint32_t remaining,uint32_t dt,const cod1AnimationTimelinePolicy_t *policy,uint32_t *newcurrent,uint32_t *newremaining) {
    cod1TimelineDyadic_t w,g,t,d,epsilon,threshold,sum,delta,result;uint32_t weight,timeleft;int order;
    if(!newcurrent||!newremaining||!Cod1TimelinePolicy(policy)||!Cod1TimelineWeightInput(current)||(goal&&goal!=UINT32_C(0x3f800000))||!Cod1TimelineTimeInput(remaining,1)||!Cod1TimelineTimeInput(dt,1))return 0;
    if(!remaining){*newcurrent=goal;*newremaining=0;return 1;}
    w=Cod1TimelineBits(current);g=Cod1TimelineBits(goal);t=Cod1TimelineBits(remaining);d=Cod1TimelineBits(dt);epsilon=Cod1TimelineBits(UINT32_C(0x3a83126f));threshold=Cod1TimelineBits(UINT32_C(0x358637be));
    if(!Cod1TimelineAdd(d,epsilon,policy->rounding,&sum)||!Cod1TimelineCompare(sum,t,&order))return 0;
    if(order>0){*newcurrent=goal;*newremaining=0;return 1;}
    sum=w;sum.sign^=1;if(!Cod1TimelineAdd(g,sum,policy->rounding,&delta))return 0;
    result=delta;result.exponent-=t.exponent;result.magnitude*=d.magnitude;result.exponent+=d.exponent;result.sign^=d.sign;
    if(!Cod1TimelineAdd(Cod1TimelineNormalize(result),w,policy->rounding,&result)||!Cod1TimelineStore(result,policy->rounding,&weight)||!Cod1TimelineCompare(result,threshold,&order))return 0;
    if(order<0){result=g;result.magnitude*=epsilon.magnitude;result.exponent+=epsilon.exponent;if(!Cod1TimelineStore(Cod1TimelineNormalize(result),policy->rounding,&weight))return 0;}
    d.sign^=1;if(!Cod1TimelineAdd(t,d,policy->rounding,&sum)||!Cod1TimelineStore(sum,policy->rounding,&timeleft))return 0;
    *newcurrent=weight;*newremaining=timeleft;return 1;
}
/* R188 separate cadence path. The old power-of-two API above is unchanged.
 * Every new x87 arithmetic boundary explicitly rounds to PC53/64, and each
 * native FST32 performs its own rounding. A portable two-word accumulator
 * preserves up to 128 exact integer bits without host FP or __int128. */
typedef struct { uint64_t hi,lo; } cod1TimelineWide_t;
COD1_TIMELINE_FN int Cod1TimelineTop64(uint64_t value) {
    int top=-1;while(value){value>>=1;++top;}return top;
}
COD1_TIMELINE_FN cod1TimelineWide_t Cod1TimelineWideShift(uint64_t value,int shift) {
    cod1TimelineWide_t result;result.hi=result.lo=0;
    if(shift<0||shift>=128)return result;
    if(shift>=64)result.hi=value<<(shift-64);
    else if(shift){result.hi=value>>(64-shift);result.lo=value<<shift;}
    else result.lo=value;
    return result;
}
COD1_TIMELINE_FN int Cod1TimelineWideCompare(cod1TimelineWide_t a,cod1TimelineWide_t b) {
    if(a.hi!=b.hi)return a.hi>b.hi?1:-1;
    return a.lo==b.lo?0:a.lo>b.lo?1:-1;
}
COD1_TIMELINE_FN cod1TimelineWide_t Cod1TimelineWideAdd(cod1TimelineWide_t a,cod1TimelineWide_t b) {
    cod1TimelineWide_t result;result.lo=a.lo+b.lo;result.hi=a.hi+b.hi+(result.lo<a.lo);return result;
}
COD1_TIMELINE_FN cod1TimelineWide_t Cod1TimelineWideSubtract(cod1TimelineWide_t a,cod1TimelineWide_t b) {
    cod1TimelineWide_t result;result.lo=a.lo-b.lo;result.hi=a.hi-b.hi-(a.lo<b.lo);return result;
}
COD1_TIMELINE_FN cod1TimelineWide_t Cod1TimelineWideMultiply(uint64_t a,uint64_t b) {
    uint64_t p0=(a&UINT32_MAX)*(b&UINT32_MAX),p1=(a&UINT32_MAX)*(b>>32),p2=(a>>32)*(b&UINT32_MAX),part;
    cod1TimelineWide_t result;result.lo=p0;result.hi=(a>>32)*(b>>32)+(p1>>32)+(p2>>32);
    part=p1<<32;result.lo+=part;result.hi+=result.lo<part;
    part=p2<<32;result.lo+=part;result.hi+=result.lo<part;return result;
}
COD1_TIMELINE_FN uint64_t Cod1TimelineWideRight(cod1TimelineWide_t value,int shift) {
    if(shift>=128)return 0;
    if(shift>=64)return value.hi>>(shift-64);
    if(shift)return (value.lo>>shift)|(value.hi<<(64-shift));
    return value.lo;
}
COD1_TIMELINE_FN int Cod1TimelineWideBit(cod1TimelineWide_t value,int bit) {
    if(bit<0||bit>=128)return 0;
    return (int)((bit>=64?value.hi>>(bit-64):value.lo>>bit)&1);
}
COD1_TIMELINE_FN int Cod1TimelineWideLow(cod1TimelineWide_t value,int count) {
    if(count<=0)return 0;
    if(count>=128)return value.hi||value.lo;
    if(count>=64)return value.lo||(count>64&&(value.hi&((UINT64_C(1)<<(count-64))-1)));
    return (value.lo&((UINT64_C(1)<<count)-1))!=0;
}
COD1_TIMELINE_FN int Cod1TimelineRoundWide(cod1TimelineWide_t magnitude,int exponent,unsigned sign,const cod1AnimationTimelinePolicy_t *policy,cod1TimelineDyadic_t *out) {
    int top,shift,discarded,guard,sticky;uint64_t q;unsigned increment=0;cod1TimelineDyadic_t value;
    if(!out||!Cod1TimelinePolicy(policy)||exponent < -512||exponent>512)return 0;
    value.sign=sign;value.exponent=exponent;
    if(!magnitude.hi&&!magnitude.lo){value.magnitude=0;*out=value;return 1;}
    top=magnitude.hi?64+Cod1TimelineTop64(magnitude.hi):Cod1TimelineTop64(magnitude.lo);shift=top+1-(int)policy->precision_bits;
    if(shift<=0){value.magnitude=magnitude.lo;*out=Cod1TimelineNormalize(value);return 1;}
    q=Cod1TimelineWideRight(magnitude,shift);guard=Cod1TimelineWideBit(magnitude,shift-1);sticky=Cod1TimelineWideLow(magnitude,shift-1);discarded=guard||sticky;
    if(policy->rounding==0)increment=guard&&(sticky||(q&1));
    else if(policy->rounding==1)increment=sign&&discarded;
    else if(policy->rounding==2)increment=!sign&&discarded;
    value.exponent=exponent+shift;
    if(increment&&q==UINT64_MAX){q=UINT64_C(1)<<63;++value.exponent;}
    else {q+=increment;if(policy->precision_bits<64&&q==(UINT64_C(1)<<policy->precision_bits)){q>>=1;++value.exponent;}}
    value.magnitude=q;*out=Cod1TimelineNormalize(value);return 1;
}
COD1_TIMELINE_FN int Cod1TimelineAddExtended(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,const cod1AnimationTimelinePolicy_t *policy,cod1TimelineDyadic_t *out) {
    cod1TimelineWide_t av,bv,result;int exponent,agap,bgap,order;unsigned sign;
    if(!out||!Cod1TimelinePolicy(policy))return 0;
    if(!a.magnitude&&!b.magnitude){a.sign=a.sign==b.sign?a.sign:policy->rounding==1;*out=a;return 1;}
    if(!a.magnitude){*out=b;return 1;}if(!b.magnitude){*out=a;return 1;}
    exponent=a.exponent<b.exponent?a.exponent:b.exponent;agap=a.exponent-exponent;bgap=b.exponent-exponent;
    if(agap+Cod1TimelineTop64(a.magnitude)>=127||bgap+Cod1TimelineTop64(b.magnitude)>=127)return 0;
    av=Cod1TimelineWideShift(a.magnitude,agap);bv=Cod1TimelineWideShift(b.magnitude,bgap);
    if(a.sign==b.sign){result=Cod1TimelineWideAdd(av,bv);sign=a.sign;}
    else {order=Cod1TimelineWideCompare(av,bv);result=order>=0?Cod1TimelineWideSubtract(av,bv):Cod1TimelineWideSubtract(bv,av);sign=order?order>0?a.sign:b.sign:policy->rounding==1;}
    return Cod1TimelineRoundWide(result,exponent,sign,policy,out);
}
COD1_TIMELINE_FN int Cod1TimelineMultiplyExtended(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,const cod1AnimationTimelinePolicy_t *policy,cod1TimelineDyadic_t *out) {
    return Cod1TimelineRoundWide(Cod1TimelineWideMultiply(a.magnitude,b.magnitude),a.exponent+b.exponent,a.sign^b.sign,policy,out);
}
COD1_TIMELINE_FN int Cod1TimelineDivideExtended(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,const cod1AnimationTimelinePolicy_t *policy,cod1TimelineDyadic_t *out) {
    uint64_t numerator,denominator,remainder,q;int e,j;unsigned sign,increment=0;cod1TimelineDyadic_t value;
    if(!out||!Cod1TimelinePolicy(policy)||!b.magnitude||Cod1TimelineTop64(a.magnitude)>47||Cod1TimelineTop64(b.magnitude)>23)return 0;
    sign=a.sign^b.sign;if(!a.magnitude){value.magnitude=0;value.exponent=0;value.sign=sign;*out=value;return 1;}
    e=Cod1TimelineTop64(a.magnitude)-Cod1TimelineTop64(b.magnitude);
    if(e>=0){if(a.magnitude<(b.magnitude<<e))--e;}
    else if((a.magnitude<<-e)<b.magnitude)--e;
    numerator=a.magnitude;denominator=b.magnitude;
    if(e>=0)denominator<<=e;else numerator<<=-e;
    if(numerator<denominator||numerator-denominator>=denominator)return 0;
    remainder=numerator-denominator;q=1;
    for(j=1;j<(int)policy->precision_bits;++j){remainder<<=1;q<<=1;if(remainder>=denominator){remainder-=denominator;++q;}}
    if(policy->rounding==0)increment=remainder*2>denominator||(remainder*2==denominator&&(q&1));
    else if(policy->rounding==1)increment=sign&&remainder;
    else if(policy->rounding==2)increment=!sign&&remainder;
    value.sign=sign;value.exponent=a.exponent-b.exponent+e-((int)policy->precision_bits-1);
    if(increment&&q==UINT64_MAX){q=UINT64_C(1)<<63;++value.exponent;}
    else {q+=increment;if(policy->precision_bits<64&&q==(UINT64_C(1)<<policy->precision_bits)){q>>=1;++value.exponent;}}
    value.magnitude=q;*out=Cod1TimelineNormalize(value);return 1;
}
COD1_TIMELINE_FN int Cod1TimelineCompareExtended(cod1TimelineDyadic_t a,cod1TimelineDyadic_t b,int *order) {
    int at,bt,exponent,agap,bgap;cod1TimelineWide_t av,bv;
    if(!order)return 0;
    if(!a.magnitude&&!b.magnitude){*order=0;return 1;}
    if(a.sign!=b.sign){*order=a.sign?-1:1;return 1;}
    if(!a.magnitude){*order=b.sign?1:-1;return 1;}if(!b.magnitude){*order=a.sign?-1:1;return 1;}
    at=Cod1TimelineTop64(a.magnitude)+a.exponent;bt=Cod1TimelineTop64(b.magnitude)+b.exponent;
    if(at!=bt){*order=(at>bt?1:-1)*(a.sign?-1:1);return 1;}
    exponent=a.exponent<b.exponent?a.exponent:b.exponent;agap=a.exponent-exponent;bgap=b.exponent-exponent;
    av=Cod1TimelineWideShift(a.magnitude,agap);bv=Cod1TimelineWideShift(b.magnitude,bgap);*order=Cod1TimelineWideCompare(av,bv)*(a.sign?-1:1);return 1;
}
COD1_TIMELINE_FN int Cod1TimelineFiniteBits(uint32_t bits) {
    uint32_t magnitude=bits&UINT32_C(0x7fffffff),biased=magnitude>>23;
    return !magnitude||(biased>0&&biased<255);
}
COD1_TIMELINE_FN int Cod1TimelineDivideFinite(uint32_t numerator,uint32_t denominator,const cod1AnimationTimelinePolicy_t *policy,uint32_t *out) {
    cod1TimelineDyadic_t result;uint32_t bits;
    if(!out||!Cod1TimelineFiniteBits(numerator)||!Cod1TimelineFiniteBits(denominator)||!(denominator&UINT32_C(0x7fffffff)))return 0;
    if(!Cod1TimelineDivideExtended(Cod1TimelineBits(numerator),Cod1TimelineBits(denominator),policy,&result)||!Cod1TimelineStore(result,policy->rounding,&bits))return 0;
    *out=bits;return 1;
}
COD1_TIMELINE_FN int Cod1TimelineCadenceTimeInput(uint32_t bits,int zero) {
    return (!bits&&zero)||(bits>=UINT32_C(0x33800000)&&bits<=UINT32_C(0x3f800000));
}
COD1_TIMELINE_FN int Cod1AnimTimelineWeightCadence(uint32_t current,uint32_t goal,uint32_t remaining,uint32_t dt,const cod1AnimationTimelinePolicy_t *policy,uint32_t *newcurrent,uint32_t *newremaining) {
    cod1TimelineDyadic_t w,g,t,d,epsilon,threshold,sum,negative,result;uint32_t weight,timeleft;int order;
    if(!newcurrent||!newremaining||!Cod1TimelinePolicy(policy)||!Cod1TimelineWeightInput(current)||(goal&&goal!=UINT32_C(0x3f800000))||!Cod1TimelineCadenceTimeInput(remaining,1)||dt!=UINT32_C(0x3d4ccccd))return 0;
    if(!remaining){*newcurrent=goal;*newremaining=0;return 1;}
    w=Cod1TimelineBits(current);g=Cod1TimelineBits(goal);t=Cod1TimelineBits(remaining);d=Cod1TimelineBits(dt);epsilon=Cod1TimelineBits(UINT32_C(0x3a83126f));threshold=Cod1TimelineBits(UINT32_C(0x358637be));
    if(!Cod1TimelineAddExtended(d,epsilon,policy,&sum)||!Cod1TimelineCompareExtended(sum,t,&order))return 0;
    if(order>0){*newcurrent=goal;*newremaining=0;return 1;}
    negative=w;negative.sign^=1;if(!Cod1TimelineAddExtended(g,negative,policy,&result)||!Cod1TimelineDivideExtended(result,t,policy,&result)||!Cod1TimelineMultiplyExtended(result,d,policy,&result)||!Cod1TimelineAddExtended(result,w,policy,&result)||!Cod1TimelineStore(result,policy->rounding,&weight)||!Cod1TimelineCompareExtended(result,threshold,&order))return 0;
    if(order<0){if(!Cod1TimelineMultiplyExtended(g,epsilon,policy,&result)||!Cod1TimelineStore(result,policy->rounding,&weight))return 0;}
    d.sign^=1;if(!Cod1TimelineAddExtended(t,d,policy,&sum)||!Cod1TimelineStore(sum,policy->rounding,&timeleft))return 0;
    *newcurrent=weight;*newremaining=timeleft;return 1;
}
COD1_TIMELINE_FN int Cod1AnimTimelineTimeCadence(uint32_t current,int active,uint32_t dt,const cod1AnimationTimelinePolicy_t *policy,uint32_t *out) {
    cod1TimelineDyadic_t result,frequency,rate,one;uint32_t time;int order;
    if(!out||!Cod1TimelinePolicy(policy)||!Cod1TimelineWeightInput(current)||(active!=0&&active!=1)||dt!=UINT32_C(0x3d4ccccd))return 0;
    if(!active){*out=current;return 1;}
    frequency=Cod1TimelineBits(UINT32_C(0x40000000));rate=one=Cod1TimelineBits(UINT32_C(0x3f800000));
    if(!Cod1TimelineMultiplyExtended(frequency,rate,policy,&result)||!Cod1TimelineMultiplyExtended(result,Cod1TimelineBits(dt),policy,&result)||!Cod1TimelineAddExtended(result,Cod1TimelineBits(current),policy,&result)||!Cod1TimelineStore(result,policy->rounding,&time)||!Cod1TimelineCompareExtended(result,one,&order))return 0;
    if(order>=0)time=UINT32_C(0x3f800000);
    *out=time;return 1;
}
COD1_TIMELINE_FN int Cod1TimelineWaitMilliseconds(uint32_t seconds,const cod1AnimationTimelinePolicy_t *policy,uint32_t *out) {
    cod1TimelineDyadic_t result,epsilon;uint32_t ms;uint64_t integer,remainder,half;int shift;unsigned increment=0;
    if(!out||!Cod1TimelinePolicy(policy)||!Cod1TimelineCadenceTimeInput(seconds,1))return 0;
    if(!Cod1TimelineMultiplyExtended(Cod1TimelineBits(seconds),Cod1TimelineBits(UINT32_C(0x447a0000)),policy,&result)||!Cod1TimelineStore(result,policy->rounding,&ms))return 0;
    epsilon.magnitude=1;epsilon.exponent=-30;epsilon.sign=0;
    if(!Cod1TimelineAddExtended(Cod1TimelineBits(ms),epsilon,policy,&result)||result.sign)return 0;
    if(result.exponent>=0){if(result.exponent>=31||result.magnitude>(UINT32_MAX>>result.exponent))return 0;integer=result.magnitude<<result.exponent;}
    else {shift=-result.exponent;if(shift>=64)return 0;integer=result.magnitude>>shift;remainder=result.magnitude&((UINT64_C(1)<<shift)-1);half=UINT64_C(1)<<(shift-1);
        if(policy->rounding==0)increment=remainder>half||(remainder==half&&(integer&1));else if(policy->rounding==2)increment=remainder!=0;integer+=increment;}
    if(integer>INT32_MAX)return 0;
    *out=(uint32_t)integer;return 1;
}

#undef COD1_TIMELINE_FN
#endif
