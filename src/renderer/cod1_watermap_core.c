#include "cod1_watermap_core.h"
#include <float.h>
#include <math.h>
#include <fenv.h>
#include <limits.h>

static int positive(float x) { return isfinite(x) && x>0; }
static int dimension(int n) { return n>=4 && n<=256 && !(n&(n-1)); }
c1w_status C1W_Count(const c1w_desc *d, size_t *count) {
    size_t n;
    if (!d || !count) return C1W_ARGUMENT;
    if (!dimension(d->width) || !dimension(d->height)) return C1W_RANGE;
    n=(size_t)d->width*(size_t)d->height;
    if (n>32768) return C1W_RANGE;
    *count=n; return C1W_OK;
}
static c1w_status check_desc(const c1w_desc *d,size_t *n) {
    long double wind2;
    c1w_status s=C1W_Count(d,n);
    if(s) return s;
    if (!positive(d->world_width) || !positive(d->world_height) ||
        !positive(d->gravity) || !positive(d->wind_velocity) ||
        !positive(d->amplitude) || !isfinite(d->wind_x) || !isfinite(d->wind_y)) return C1W_RANGE;
    wind2=(long double)d->wind_x*d->wind_x+(long double)d->wind_y*d->wind_y;
    /* Parser normalizes this vector. Do not silently normalize at a different
       floating-point boundary or pretend zero direction is meaningful. */
    if (fabsl(wind2-1)>0.00001L) return C1W_RANGE;
    return C1W_OK;
}
void C1W_Tables(c1w_tables *t) {
    int i;
    if (!t) return;
    for(i=0;i<256;++i) {
        float angle=(float)((long double)i*0x1.921fb6p-6f); /* VA 55b77c */
        t->twiddle[i].re=(float)cosl((long double)angle);
        t->twiddle[i].im=(float)sinl((long double)angle);
    }
    for(i=0;i<1024;++i) {
        /* VA 4a8d55..71: three binary32 constants, NO intermediate float store. */
        long double angle=(long double)i*0.3515625f;
        angle*=0x1.921fb6p+1f;
        angle*=0x1.6c16c2p-8f;
        t->phase_sine[i]=(float)sinl(angle);
    }
}
static c1w_status gaussian(c1w_rand15 rng,void *user,float *a,float *b) {
    int attempt;
    for(attempt=0;attempt<4096;++attempt) {
        int r0=rng(user),r1=rng(user);
        float x;
        long double y,s,k;
        if(r0<0 || r0>32767 || r1<0 || r1>32767) return C1W_RANDOM;
        x=(float)((long double)r0*0x1p-15f*2-1);
        y=(long double)r1*0x1p-15f*2-1;
        s=y*y+(long double)x*x;
        if(s>1) continue;
        /* Native lacks a zero guard. Report the degenerate injected stream,
           never publish NaNs or secretly use an invented replacement wave. */
        if(s==0) return C1W_RANDOM;
        k=sqrtl(-2*logl(s)/s);
        *a=(float)(x*k); *b=(float)(y*k); return C1W_OK;
    }
    return C1W_RANDOM;
}
c1w_status C1W_Spectrum(const c1w_desc *d,c1w_rand15 rng,void *user,
                       c1w_complex *initial,float *omega,size_t capacity) {
    size_t n,at=0; int row,col;
    float wind_factor,k0step,k1step;
    c1w_status s=check_desc(d,&n);
    if(s) return s;
    if(!rng || !initial || !omega) return C1W_ARGUMENT;
    if(capacity<n) return C1W_CAPACITY;
    if(fegetround()!=FE_TONEAREST) return C1W_ROUNDING;
    {
        long double v2=(long double)d->wind_velocity*d->wind_velocity;
        wind_factor=(float)((v2/d->gravity)*v2); /* V^4/g, not guessed V^4/g^2 */
    }
    k0step=(float)((long double)0x1.921fb6p+2f/((long double)d->height*d->world_width));
    k1step=(float)((long double)0x1.921fb6p+2f/((long double)d->width*d->world_height));
    if(!positive(wind_factor) || !positive(k0step) || !positive(k1step)) return C1W_RANGE;
    for(row=-d->height/2;row<d->height/2;++row) {
        float k0=(float)((long double)row*k0step);
        float k0square=(float)((long double)k0*k0);
        for(col=-d->width/2;col<d->width/2;++col,++at) {
            float k1=(float)((long double)col*k1step),ga,gb,gk;
            long double k2,dot,power,factor;
            s=gaussian(rng,user,&ga,&gb); /* even for discarded directions */
            if(s) return s;
            k2=(long double)k1*k1+k0square;
            gk=(float)(sqrtl(k2)*d->gravity);
            dot=(long double)k1*d->wind_y+(long double)k0*d->wind_x;
            if(dot<=0) { initial[at].re=initial[at].im=omega[at]=0;continue; }
            power=expl(-1/(k2*wind_factor))*d->amplitude;
            power/=k2*k2*k2;
            power*=dot*dot;
            factor=sqrtl(power*0.5f)*d->amplitude;
            initial[at].re=(float)(ga*factor);initial[at].im=(float)(gb*factor);
            omega[at]=(float)sqrtl(gk);
            if(!isfinite(initial[at].re)||!isfinite(initial[at].im)||!isfinite(omega[at])) return C1W_RANGE;
        }
    }
    return C1W_OK;
}
c1w_status C1W_AtTime(const c1w_desc *d,const c1w_tables *t,float seconds,
                     const c1w_complex *initial,const float *omega,
                     c1w_complex *field,size_t capacity) {
    size_t i,n;c1w_status s=C1W_Count(d,&n);
    if(s) return s;
    if(!t || !initial || !omega || !field) return C1W_ARGUMENT;
    if(capacity<n) return C1W_CAPACITY;
    if(!isfinite(seconds)) return C1W_RANGE;
    if(fegetround()!=FE_TONEAREST) return C1W_ROUNDING;
    /* Preflight before writing. Native FISTP range is signed32, not modulo-fmod
       at unlimited time; reject overflow instead of relying on invalid cast. */
    for(i=0;i<n;++i) {
        float phase=(float)((long double)seconds*omega[i]*0x1.45f306p+7f);
        if(!isfinite(omega[i]) || omega[i]<0 || !isfinite(initial[i].re) ||
           !isfinite(initial[i].im) || !isfinite(phase) ||
           nearbyintl(phase)<INT32_MIN || nearbyintl(phase)>INT32_MAX) return C1W_RANGE;
    }
    for(i=0;i<n;++i) {
        float phase;unsigned index;
        if(omega[i]==0) {field[i].re=field[i].im=0;continue;}
        phase=(float)((long double)seconds*omega[i]*0x1.45f306p+7f);
        index=(uint32_t)(int32_t)nearbyintl(phase)&1023u;
        /* Literal native +255, not a 'corrected' +256; no complex rotation. */
        field[i].re=(float)((long double)t->phase_sine[(index+255u)&1023u]*initial[i].re);
        field[i].im=(float)((long double)t->phase_sine[index]*initial[i].im);
    }
    return C1W_OK;
}
static void fft1(c1w_complex *p,int n,int stride,const c1w_tables *t) {
    int i,j,half,base;
    for(i=0,j=0;i<n;++i) {
        int bit=n>>1;
        if(j<i) {c1w_complex x=p[i*stride];p[i*stride]=p[j*stride];p[j*stride]=x;}
        while(j&bit) {j^=bit;bit>>=1;} j^=bit;
    }
    for(base=0;base<n;base+=4) {
        c1w_complex *a=p+base*stride,*b=a+stride,*c=b+stride,*d=c+stride;
        long double er=(long double)a->re+b->re,ei=(long double)a->im+b->im;
        long double fr=(long double)c->re+d->re,fi=(long double)c->im+d->im;
        long double ur=(long double)b->re-a->re,ui=(long double)b->im-a->im;
        long double vr=(long double)d->re-c->re,vi=(long double)d->im-c->im;
        a->re=(float)(er+fr);a->im=(float)(ei+fi);
        c->re=(float)(er-fr);c->im=(float)(ei-fi);
        b->re=(float)(ur-vi);b->im=(float)(ui+vr);
        d->re=(float)(ur+vi);d->im=(float)(ui-vr);
    }
    for(half=4;half<n;half*=2) for(j=0;j<half;++j) {
        c1w_complex w=t->twiddle[j*(128/half)];
        for(i=j;i<n;i+=2*half) {
            c1w_complex *a=p+i*stride,*b=p+(i+half)*stride;
            long double re=(long double)w.re*b->re-(long double)w.im*b->im;
            long double im=(long double)w.re*b->im+(long double)w.im*b->re;
            b->re=(float)((long double)a->re-re);b->im=(float)((long double)a->im-im);
            a->re=(float)(re+a->re);a->im=(float)(im+a->im);
        }
    }
}
c1w_status C1W_FFT2(const c1w_desc *d,const c1w_tables *t,c1w_complex *field,size_t capacity) {
    size_t n,i;int j;c1w_status s=C1W_Count(d,&n);
    if(s) return s;
    if(!t||!field) return C1W_ARGUMENT;
    if(capacity<n) return C1W_CAPACITY;
    if(fegetround()!=FE_TONEAREST) return C1W_ROUNDING;
    for(i=0;i<n;++i) if(!isfinite(field[i].re)||!isfinite(field[i].im)) return C1W_RANGE;
    for(j=0;j<d->height;++j) fft1(field+j*d->width,d->width,1,t);
    for(j=0;j<d->width;++j) fft1(field+j,d->height,d->width,t);
    for(i=0;i<n;++i) if(!isfinite(field[i].re)||!isfinite(field[i].im)) return C1W_RANGE;
    return C1W_OK;
}
c1w_status C1W_Normals(const c1w_desc *d,const c1w_complex *field,
                      float *mag,size_t capacity,uint8_t *rgba,size_t rgba_capacity) {
    size_t n,i;int row,col;c1w_status s=C1W_Count(d,&n);
    if(s) return s;
    if(!field||!mag||!rgba) return C1W_ARGUMENT;
    if(capacity<n||rgba_capacity<n*4) return C1W_CAPACITY;
    if(fegetround()!=FE_TONEAREST) return C1W_ROUNDING;
    for(i=0;i<n;++i) {
        mag[i]=(float)sqrtl((long double)field[i].re*field[i].re+(long double)field[i].im*field[i].im);
        if(!isfinite(mag[i])) return C1W_RANGE;
    }
    for(row=0;row<d->height;++row) for(col=0;col<d->width;++col) {
        size_t at=(size_t)row*d->width+col;
        size_t left=(size_t)row*d->width+((col-1)&(d->width-1));
        size_t above=(size_t)((row-1)&(d->height-1))*d->width+col;
        long double x=(long double)mag[left]-mag[at],y=(long double)mag[above]-mag[at];
        long double scale=127/sqrtl(x*x+(float)(n*n)+y*y);
        rgba[at*4]=(uint8_t)(127+(int)nearbyintl(x*scale));
        rgba[at*4+1]=(uint8_t)(127+(int)nearbyintl(y*scale));
        rgba[at*4+2]=(uint8_t)(127+(int)nearbyintl((long double)n*scale));
        rgba[at*4+3]=255;
    }
    return C1W_OK;
}
