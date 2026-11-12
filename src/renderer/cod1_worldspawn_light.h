/* R54: the worldspawn lighting-input contract, not a new lighting solver.
 * CoDSP SHA256 af73842d..., VA 4d23d4..4d27ef and 429b50..429bdc.
 * Missing keys keep native defaults. minlight is NOT an alias for ambient.
 * Coefficients are normalized-unit inputs for the existing static-cache path;
 * native GL/global state, local lights and overbright integration are separate.
 */
#ifndef COD1_WORLDSPAWN_LIGHT_H
#define COD1_WORLDSPAWN_LIGHT_H
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum {
    C1WL_AMBIENT=1, C1WL_SUNLIGHT=2, C1WL_FRACTION=4,
    C1WL_SUNCOLOR=8, C1WL_SKYCOLOR=16, C1WL_AMBIENTCOLOR=32,
    C1WL_ALL=63
};
typedef struct {
    float ambient, sunlight, fraction;
    float ambientColor[3], sunColor[3], skyColor[3];
    unsigned int present, invalid;
    int legacyAmbient;
} c1WorldLightParams;

static int C1WL_Finite(float v) { return v >= -FLT_MAX && v <= FLT_MAX; }
static int C1WL_Equal(const char *a,const char *b) {
    if(!a||!b)return 0;
    while(*a&&*b){if(tolower((unsigned char)*a)!=tolower((unsigned char)*b))return 0;a++;b++;}
    return *a==*b;
}
static unsigned int C1WL_Field(const char *key) {
    if(C1WL_Equal(key,"ambient"))return C1WL_AMBIENT;
    if(C1WL_Equal(key,"sunlight"))return C1WL_SUNLIGHT;
    if(C1WL_Equal(key,"diffusefraction"))return C1WL_FRACTION;
    if(C1WL_Equal(key,"suncolor"))return C1WL_SUNCOLOR;
    if(C1WL_Equal(key,"sundiffusecolor"))return C1WL_SKYCOLOR;
    if(C1WL_Equal(key,"_color"))return C1WL_AMBIENTCOLOR;
    return 0;
}
static void C1WL_Defaults(c1WorldLightParams *p) {
    memset(p,0,sizeof(*p));p->sunlight=1.0f;p->fraction=0.5f;
}
/* Explicit validation boundary: unlike unchecked native atof/sscanf, malformed
 * or nonfinite values are rejected, not replaced with guessed light. Valid
 * signed finite values are not subject to the old arbitrary 0..100 gate. */
static int C1WL_Numbers(const char *s,float *v,int count,double *first) {
    int i;char *end;
    if(!s)return 0;
    for(i=0;i<count;i++){
        double d=strtod(s,&end);
        if(end==s || !(d>=-FLT_MAX&&d<=FLT_MAX))return 0;
        if(i==0 && first)*first=d;
        v[i]=(float)d;if(!C1WL_Finite(v[i]))return 0;s=end;
    }
    while(*s && isspace((unsigned char)*s))s++;
    return !*s;
}
static int C1WL_Normalize(float v[3],float *peak) {
    int k;float m=v[0];volatile float reciprocal;
    for(k=1;k<3;k++)if(v[k]>m)m=v[k];
    if(peak)*peak=m;
    if(m==0.0f){v[0]=v[1]=v[2]=1.0f;return 1;}
    /* Native stores the reciprocal as float before three multiplies. This is
     * deliberately not three independently rounded divisions. */
    reciprocal=(float)(1.0/(double)m);
    if(!C1WL_Finite(reciprocal))return 0;
    for(k=0;k<3;k++){
        volatile float n=(float)((double)v[k]*(double)reciprocal);
        if(!C1WL_Finite(n))return 0;v[k]=n;
    }
    return 1;
}
/* Returns 0 for an unrelated key, +1 on success, -1 for an invalid light input. */
static int C1WL_Set(c1WorldLightParams *p,const char *key,const char *value) {
    unsigned int field=C1WL_Field(key);float v[3];double first=0.0;int count;
    if(!field)return 0;p->present|=field;
    count=(field&(C1WL_SUNCOLOR|C1WL_SKYCOLOR|C1WL_AMBIENTCOLOR))?3:1;
    if(!C1WL_Numbers(value,v,count,&first))goto invalid;
    if((field&(C1WL_SUNCOLOR|C1WL_SKYCOLOR)) && !C1WL_Normalize(v,NULL))goto invalid;
    switch(field){
    case C1WL_AMBIENT:
        /* fst stores ambient as float, but fcomp still tests atof's double.
         * A decimal just above 2 can round to float 2 and still use old scale. */
        p->legacyAmbient=first>2.0;
        if(p->legacyAmbient){
            /* Native float constant 0x3c808081: 4/255, NOT 1/255. */
            const float oldScale=0.01568627543747425079345703125f;
            volatile float scaled=(float)((double)v[0]*(double)oldScale);v[0]=scaled;
        }
        p->ambient=v[0];break;
    case C1WL_SUNLIGHT:p->sunlight=v[0];break;
    case C1WL_FRACTION:p->fraction=v[0];break;
    case C1WL_SUNCOLOR:memcpy(p->sunColor,v,sizeof(v));break;
    case C1WL_SKYCOLOR:memcpy(p->skyColor,v,sizeof(v));break;
    case C1WL_AMBIENTCOLOR:memcpy(p->ambientColor,v,sizeof(v));break;
    }
    p->invalid&=~field;return 1;
invalid:
    p->invalid|=field;return -1;
}
static int C1WL_Coefficients(const c1WorldLightParams *p,float sun[3],float sky[3],float ambient[3]) {
    int k;float ac[3],peak=0;const float *sc;
    double delta=(double)p->sunlight-(double)p->ambient;
    if(p->invalid)return 0;
    /* Omitted color stays zero, while an explicitly supplied zero color is
     * normalized by the native helper to white. Preserve this distinction. */
    sc=(p->present&C1WL_SKYCOLOR)?p->skyColor:p->sunColor;
    memcpy(ac,p->ambientColor,sizeof(ac));
    if(p->ambient!=0.0f && !C1WL_Normalize(ac,&peak))return 0;
    for(k=0;k<3;k++){
        double d=delta*(1.0-(double)p->fraction)*(double)p->sunColor[k];
        double s=delta*(double)p->fraction*(double)sc[k];
        double a=(p->ambient!=0.0f && peak!=0.0f)?(double)p->ambient*(double)ac[k]:0.0;
        if(!(d>=-FLT_MAX&&d<=FLT_MAX && s>=-FLT_MAX&&s<=FLT_MAX && a>=-FLT_MAX&&a<=FLT_MAX))return 0;
        sun[k]=(float)d;sky[k]=(float)s;ambient[k]=(float)a;
    }
    return 1;
}
#endif
