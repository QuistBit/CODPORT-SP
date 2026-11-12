/* R184 integer arithmetic for an explicit conditional profile, not ambient
 * host floating-point state. All admitted results are finite normal binary32
 * or signed zero. A normal product has <=39 significant bits; PC53/64 are
 * exact and PC24 rounds directly to the final binary32 significand. Scaling
 * by 2^-15 is exact in the original extended exponent range. */
static int float_arithmetic_profile_valid(const GscModuleFloatArithmeticProfile *p) {
    return p&&(p->precision_bits==24||p->precision_bits==53||p->precision_bits==64)&&p->rounding<=GSC_FLOAT_ROUND_ZERO;
}
static int float_arithmetic_round(unsigned sign,uint64_t magnitude,int exponent,unsigned rounding,uint32_t *out) {
    uint64_t scan,significand,remainder,half;int top=0,shift,biased;unsigned increment=0;
    if(!out||rounding>GSC_FLOAT_ROUND_ZERO)return 0;
    if(!magnitude){*out=sign?UINT32_C(0x80000000):0;return 1;}
    scan=magnitude;while(scan>>1){scan>>=1;++top;}
    biased=top+exponent+127;if(biased<1||biased>254)return 0;
    shift=top-23;
    if(shift>0) {
        significand=magnitude>>shift;remainder=magnitude&((UINT64_C(1)<<shift)-1);
        half=UINT64_C(1)<<(shift-1);
        if(rounding==GSC_FLOAT_ROUND_NEAREST)increment=remainder>half||(remainder==half&&(significand&1));
        else if(rounding==GSC_FLOAT_ROUND_DOWN)increment=sign&&remainder;
        else if(rounding==GSC_FLOAT_ROUND_UP)increment=!sign&&remainder;
        significand+=increment;
        if(significand==UINT64_C(0x1000000)){significand>>=1;if(++biased>254)return 0;}
    } else significand=magnitude<<(23-top);
    *out=(sign?UINT32_C(0x80000000):0)|((uint32_t)biased<<23)|((uint32_t)significand&UINT32_C(0x7fffff));return 1;
}
/* Exact bounded rational conversion. Native CRT witnesses admit canonical
 * source tokens 0.4/0.8 only; no generic original decimal-parser parity is
 * inferred from this reusable integer calculation. */
static int float_arithmetic_decimal_nearest(const char *text,uint32_t *out) {
    uint64_t numerator=0,denominator=1,remainder,significand;int digits=0,fraction=0,exponent=0,n;
    const unsigned char *p=(const unsigned char *)text;
    if(!p||!out||!*p)return 0;
    for(;*p;++p) {
        if(*p=='.'&&!fraction){fraction=1;continue;}
        if(*p<'0'||*p>'9'||++digits>18)return 0;
        numerator=numerator*10+(*p-'0');
        if(fraction){if(denominator>UINT64_C(100000000))return 0;denominator*=10;}
    }
    if(!digits)return 0;
    if(!numerator){*out=0;return 1;}
    while(numerator<denominator){numerator*=2;--exponent;}
    while(numerator>=denominator*2){denominator*=2;++exponent;}
    if(exponent < -126||exponent>127)return 0;
    remainder=numerator-denominator;significand=1;
    for(n=0;n<23;++n){remainder*=2;significand<<=1;if(remainder>=denominator){++significand;remainder-=denominator;}}
    if(remainder*2>denominator||(remainder*2==denominator&&(significand&1)))++significand;
    if(significand==UINT64_C(0x1000000)){significand>>=1;if(++exponent>127)return 0;}
    *out=((uint32_t)(exponent+127)<<23)|((uint32_t)significand&UINT32_C(0x7fffff));return 1;
}
static int float_arithmetic_bound_valid(uint32_t bits) {
    uint32_t magnitude=bits&UINT32_C(0x7fffffff),biased=magnitude>>23;
    return !magnitude||(biased>=16&&biased<255);
}
static int float_arithmetic_project(uint32_t bound,uint32_t draw,unsigned rounding,uint32_t *out) {
    uint32_t magnitude=bound&UINT32_C(0x7fffffff),mantissa;int exponent;
    if(!float_arithmetic_bound_valid(bound)||draw>32767)return 0;
    if(!magnitude||!draw){*out=bound&UINT32_C(0x80000000);return 1;}
    mantissa=UINT32_C(0x800000)|(magnitude&UINT32_C(0x7fffff));exponent=(int)(magnitude>>23)-150-15;
    return float_arithmetic_round(bound>>31,(uint64_t)mantissa*draw,exponent,rounding,out);
}
static int float_arithmetic_add(uint32_t a,uint32_t b,unsigned rounding,uint32_t *out) {
    uint32_t am=a&UINT32_C(0x7fffffff),bm=b&UINT32_C(0x7fffffff);int ae,be,gap;int64_t av,bv,sum;unsigned sign;
    if((am>>23)==255||(bm>>23)==255||(am&&!(am>>23))||(bm&&!(bm>>23)))return 0;
    if(!am&&!bm){*out=((a^b)&UINT32_C(0x80000000))?(rounding==GSC_FLOAT_ROUND_DOWN?UINT32_C(0x80000000):0):(a&UINT32_C(0x80000000));return 1;}
    if(!am){*out=b;return 1;}if(!bm){*out=a;return 1;}
    ae=(int)(am>>23)-150;be=(int)(bm>>23)-150;gap=ae>be?ae-be:be-ae;if(gap>31)return 0;
    av=(int64_t)(UINT32_C(0x800000)|(am&UINT32_C(0x7fffff)));bv=(int64_t)(UINT32_C(0x800000)|(bm&UINT32_C(0x7fffff)));
    if(ae>be)av<<=gap;else bv<<=gap;
    if(a>>31){av=-av;}
    if(b>>31){bv=-bv;}
    sum=av+bv;
    if(!sum){*out=rounding==GSC_FLOAT_ROUND_DOWN?UINT32_C(0x80000000):0;return 1;}
    sign=sum<0;return float_arithmetic_round(sign,sign?(uint64_t)(-sum):(uint64_t)sum,ae<be?ae:be,rounding,out);
}
/* R187 exact multiplication/division for timed animation arithmetic. Input
 * and output are finite normals or signed zero. The product has <=48 bits;
 * after removing factors of two it must fit 24 bits. A ratio is admitted only
 * when the denominator's odd factor divides the numerator significand. Every
 * admitted value is exact at PC24/53/64 under every declared rounding mode.
 * Inexact products/ratios, subnormals and overflow remain outside this lane. */
static int float_arithmetic_normal_or_zero(uint32_t bits) {
    uint32_t magnitude=bits&UINT32_C(0x7fffffff),biased=magnitude>>23;
    return !magnitude||(biased>0&&biased<255);
}
static int float_arithmetic_exact_product_ratio(uint32_t a,uint32_t b,int op,uint32_t *out) {
    uint32_t am=a&UINT32_C(0x7fffffff),bm=b&UINT32_C(0x7fffffff),asig,bsig;
    uint64_t magnitude;int exponent,ae,be;unsigned sign=(a^b)>>31;
    if(!out||(op!='*'&&op!='/')||!float_arithmetic_normal_or_zero(a)||!float_arithmetic_normal_or_zero(b))return 0;
    if(op=='/'&&!bm)return 0;
    if(!am||(op=='*'&&!bm)){*out=sign?UINT32_C(0x80000000):0;return 1;}
    asig=UINT32_C(0x800000)|(am&UINT32_C(0x7fffff));bsig=UINT32_C(0x800000)|(bm&UINT32_C(0x7fffff));
    ae=(int)(am>>23)-150;be=(int)(bm>>23)-150;
    if(op=='*'){magnitude=(uint64_t)asig*bsig;exponent=ae+be;}
    else {
        exponent=ae-be;
        while(!(bsig&1)){bsig>>=1;--exponent;}
        if(asig%bsig)return 0;
        magnitude=asig/bsig;
    }
    while(!(magnitude&1)){magnitude>>=1;++exponent;}
    if(magnitude>UINT32_C(0xffffff))return 0;
    return float_arithmetic_round(sign,magnitude,exponent,GSC_FLOAT_ROUND_NEAREST,out);
}
