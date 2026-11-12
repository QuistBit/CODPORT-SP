#include "gsc_string_cast.h"
/* R164 copied Value formatter. Original CRT: %i, %g (six significant
 * digits, three exponent digits), and (%.2f, %.2f, %.2f), C locale.
 * Decimal arithmetic starts from IEEE float32 bits. No host printf, locale,
 * floating multiply/cast, or rounding-mode dependency is used for text.
 * This is not a native string pool or a VM stack/refcount implementation. */
typedef struct {
    unsigned char digit[160]; /* least significant decimal digit first */
    int count, scale, negative;
} cod1ModelDecimal_t;
static int Cod1ModelStringOverlap(const void *a,size_t as,const void *b,size_t bs) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!a || !b || !as || !bs)return 0;
    return x>UINTPTR_MAX-as || y>UINTPTR_MAX-bs || (x<y+bs && y<x+as);
}
static void Cod1ModelDecimalMultiply(cod1ModelDecimal_t *d,unsigned factor) {
    unsigned carry=0;int i;
    for(i=0;i<d->count;++i){unsigned n=d->digit[i]*factor+carry;d->digit[i]=(unsigned char)(n%10);carry=n/10;}
    while(carry){d->digit[d->count++]=(unsigned char)(carry%10);carry/=10;}
}
static int Cod1ModelDecimal(float value,cod1ModelDecimal_t *d) {
    uint32_t bits,mantissa,exponent;int power,i;
    memcpy(&bits,&value,sizeof(bits));exponent=(bits>>23)&255;
    if(exponent==255)return 0; /* original nonfinite text is a separate lane */
    memset(d,0,sizeof(*d));mantissa=bits&0x7fffff;
    if(exponent)mantissa|=0x800000;
    d->negative=!!(bits>>31) && mantissa!=0;
    do{d->digit[d->count++]=(unsigned char)(mantissa%10);mantissa/=10;}while(mantissa);
    if(!(bits&0x7fffffff))return 1;
    power=exponent?(int)exponent-150:-149;
    if(power>=0)for(i=0;i<power;++i)Cod1ModelDecimalMultiply(d,2);
    else{d->scale=-power;for(i=0;i<-power;++i)Cod1ModelDecimalMultiply(d,5);}
    return 1;
}
static void Cod1ModelDecimalRound(cod1ModelDecimal_t *d,int drop) {
    int carry,i,remaining;
    if(drop<=0)return;
    carry=drop<=d->count && d->digit[drop-1]>=5;
    remaining=d->count-drop;
    if(remaining<=0){d->count=1;d->digit[0]=(unsigned char)carry;}
    else {
        memmove(d->digit,d->digit+drop,(size_t)remaining);d->count=remaining;
        for(i=0;carry && i<d->count;++i){int n=d->digit[i]+1;d->digit[i]=(unsigned char)(n%10);carry=n/10;}
        if(carry)d->digit[d->count++]=1;
    }
    d->scale-=drop;
}
static char Cod1ModelDecimalDigit(const cod1ModelDecimal_t *d,int position) {
    int i=d->scale+position;
    return (char)('0'+(i>=0 && i<d->count?d->digit[i]:0));
}
static void Cod1ModelFloatText(const cod1ModelDecimal_t *source,int fixed,char *text) {
    cod1ModelDecimal_t d=*source;char *p=text;int exponent,i,last;
    /* Original 52b36c -> 52c0f1 first creates a 17-digit decimal buffer;
     * the %g/%.2f precision is applied to that buffer afterwards. */
    Cod1ModelDecimalRound(&d,d.count-17);
    Cod1ModelDecimalRound(&d,fixed?d.scale-2:d.count-6);
    if(d.negative)*p++='-';
    exponent=d.count-1-d.scale;
    if(!fixed && (exponent>=6 || exponent<-4) && !(d.count==1 && !d.digit[0])) {
        *p++=(char)('0'+d.digit[d.count-1]);last=0;
        while(last<d.count-1 && !d.digit[last])++last;
        if(last<d.count-1){*p++='.';for(i=d.count-2;i>=last;--i)*p++=(char)('0'+d.digit[i]);}
        *p++='e';*p++=exponent<0?'-':'+';if(exponent<0)exponent=-exponent;
        *p++=(char)('0'+exponent/100);*p++=(char)('0'+exponent/10%10);*p++=(char)('0'+exponent%10);
    } else {
        if(exponent<0)*p++='0';else for(i=exponent;i>=0;--i)*p++=Cod1ModelDecimalDigit(&d,i);
        last=fixed?-2:0;
        if(!fixed){i=0;while(i<d.count-1 && !d.digit[i])++i;last=i-d.scale;if(last>0)last=0;}
        if(last<0){*p++='.';for(i=-1;i>=last;--i)*p++=Cod1ModelDecimalDigit(&d,i);}
    }
    *p=0;
}
static void Cod1ModelIntText(int32_t value,char *text) {
    char reversed[10],*p=text;uint32_t magnitude;int count=0;
    magnitude=value<0?0u-(uint32_t)value:(uint32_t)value;
    if(value<0)*p++='-';
    do{reversed[count++]=(char)('0'+magnitude%10);magnitude/=10;}while(magnitude);
    while(count)*p++=reversed[--count];
    *p=0;
}
/* A legacy integer carrier must represent an exact integral int32. It cannot
 * transport INT_MAX; the typed record below carries that value without float
 * rounding. Never silently reinterpret a rounded or out-of-range carrier. */
static int Cod1ModelLegacyInt(float value,int32_t *out) {
    uint32_t bits,exponent,magnitude;int shift;
    memcpy(&bits,&value,4);exponent=(bits>>23)&255;magnitude=bits&0x7fffff;
    if(!exponent && !magnitude){*out=0;return 1;}
    if(exponent<127 || exponent>158)return 0;
    magnitude|=0x800000;shift=(int)exponent-150;
    if(shift<0){if(magnitude&((1u<<-shift)-1))return 0;magnitude>>=-shift;}
    else magnitude<<=shift;
    if(magnitude>(bits>>31?0x80000000u:0x7fffffffu))return 0;
    *out=bits>>31?-(int32_t)(magnitude-1)-1:(int32_t)magnitude;return 1;
}
static int GscStringCast(const GscStringCastValue *input,
    GscStringCastResult *out,char *error,size_t size) {
    static const char *const names[]={"undefined","string","localized string","vector","float","int",
        "codepos","object","key/value","function","stack","animation","thread","entity","struct","array",
        "dead thread","dead entity","dead object"};
    GscStringCastValue value;GscStringCastResult result;cod1ModelDecimal_t d;
    char text[160],component[48];size_t used;unsigned i;
    if(!input || !out || (size && !error) ||
        Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out)) ||
        Cod1ModelStringOverlap(input,sizeof(*input),error,size) ||
        Cod1ModelStringOverlap(out,sizeof(*out),error,size))return 0;
    value=*input;memset(&result,0,sizeof(result));text[0]=0;
    if(value.type>18)return 0;
    if(value.type==1){if(!memchr(value.string,0,sizeof(value.string)))return 0;strcpy(text,value.string);}
    else if(value.type==5){Cod1ModelIntText(value.integer,text);result.converted=1;}
    else if(value.type==4){if(!Cod1ModelDecimal(value.number,&d))return 0;Cod1ModelFloatText(&d,0,text);result.converted=1;}
    else if(value.type==3){
        strcpy(text,"(");used=1;
        for(i=0;i<3;++i){if(!Cod1ModelDecimal(value.vector[i],&d))return 0;Cod1ModelFloatText(&d,1,component);
            if(i){strcpy(text+used,", ");used+=2;}strcpy(text+used,component);used+=strlen(component);}
        strcpy(text+used,")");result.converted=1;
    } else {
        strcpy(text,"cannot cast ");strcat(text,names[value.type]);strcat(text," to string");
        if(error && size){size_t n=strlen(text);if(n>=size)n=size-1;memcpy(error,text,n);error[n]=0;}
        return -1;
    }
    if(strlen(text)>=sizeof(result.string))return 0;
    strcpy(result.string,text);*out=result;return 1;
}

