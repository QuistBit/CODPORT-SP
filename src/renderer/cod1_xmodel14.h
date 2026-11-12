/* CoD1 v14 rigid, single-root XModel reader. PATCH14.
 * Independent bounded implementation; format reference:
 * https://github.com/mauserzjeh/cod-asset-importer/tree/main/templates
 * No animated/skinned models or bone transforms are accepted here.
 */
#ifndef COD1_XMODEL14_H
#define COD1_XMODEL14_H
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "../qcommon/cod1_model_metadata.h"
/* Retail Flakvierling has 40 surfaces; damaged Tiger has 34.
 * This is adapter capacity, not a claimed format limit. */
#define XM14_SURFS 64
#define XM14_NAME 128
#define XM14_VERTS 999
#define XM14_TRIS 1999

typedef struct { const unsigned char *p; size_t n, at; int ok; } xm14Reader;
typedef struct { float normal[3], uv[2], xyz[3]; } xm14Vertex;
typedef struct { int nv, nt; xm14Vertex *v; unsigned short *idx; } xm14Surface;
typedef struct { char lod[3][XM14_NAME]; char skin[3][XM14_SURFS][XM14_NAME]; int ns[3]; c1mBounds_t bounds; } xm14Model;
typedef struct { int ns; xm14Surface s[XM14_SURFS]; } xm14Mesh;
static int XM14_Take(xm14Reader *r, size_t n) {
    if (!r->ok || n > r->n-r->at) { r->ok=0; return 0; }
    r->at+=n; return 1;
}
static unsigned int XM14_U8(xm14Reader *r) { return XM14_Take(r,1)?r->p[r->at-1]:0; }
static unsigned int XM14_U16(xm14Reader *r) {
    unsigned int a=XM14_U8(r), b=XM14_U8(r); return a|(b<<8);
}
static unsigned int XM14_U32(xm14Reader *r) {
    unsigned int a=XM14_U16(r), b=XM14_U16(r); return a|(b<<16);
}
static float XM14_F32(xm14Reader *r) {
    unsigned int bits=XM14_U32(r); float f;
    memcpy(&f,&bits,4);
    if (!(f>=-FLT_MAX && f<=FLT_MAX)) { r->ok=0; return 0; }
    return f;
}
static int XM14_String(xm14Reader *r, char *s) {
    int i=0; unsigned int c;
    do { c=XM14_U8(r); if(i>=XM14_NAME-1 && c) {r->ok=0;break;} s[i++]=(char)c; } while(c && r->ok);
    s[XM14_NAME-1]=0; return r->ok;
}
static int XM14_Model(const void *data, size_t n, xm14Model *m) {
    xm14Reader r; unsigned int count, sub; int i,j;
    memset(m,0,sizeof(*m)); r.p=(const unsigned char*)data;r.n=n;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14 || !C1M_ReadBounds(r.p,r.n,&r.at,&m->bounds))return 0;
    for(i=0;i<3;i++){ XM14_F32(&r); if(!XM14_String(&r,m->lod[i]))return 0; }
    if(!XM14_Take(&r,4))return 0;
    count=XM14_U32(&r);
    if(count>r.n/40)return 0;
    while(count--){sub=XM14_U32(&r);if(!r.ok || r.n-r.at<36 || sub>(r.n-r.at-36)/48)return 0;XM14_Take(&r,36+sub*48);}
    for(i=0;i<3;i++)if(m->lod[i][0]){
        m->ns[i]=(int)XM14_U16(&r);if(m->ns[i]>XM14_SURFS)return 0;
        for(j=0;j<m->ns[i];j++)if(!XM14_String(&r,m->skin[i][j]))return 0;
    }
    return r.ok && r.at==r.n && m->lod[0][0] && m->ns[0]>0;
}
static int XM14_Parts(const void *data,size_t n) {
    xm14Reader r;char name[XM14_NAME];
    r.p=(const unsigned char*)data;r.n=n;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14 || XM14_U16(&r)!=0 || XM14_U16(&r)!=1)return 0;
    XM14_String(&r,name);XM14_Take(&r,24);
    /* Shipped v14 parts may carry a final flags byte. */
    if(r.ok && r.n-r.at==1)XM14_Take(&r,1);
    return r.ok && r.at==r.n;
}
static void XM14_Free(xm14Mesh *m){int i;for(i=0;i<XM14_SURFS;i++){free(m->s[i].v);free(m->s[i].idx);}memset(m,0,sizeof(*m));}
static int XM14_Mesh(const void *data,size_t n,xm14Mesh *m) {
    xm14Reader r;int i,j,k,faces,c,a,b,d;unsigned int bone;xm14Surface *s;
    memset(m,0,sizeof(*m));r.p=(const unsigned char*)data;r.n=n;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14)return 0;
    m->ns=(int)XM14_U16(&r);if(!m->ns || m->ns>XM14_SURFS)goto bad;
    for(i=0;i<m->ns;i++){
        s=&m->s[i];XM14_U8(&r);s->nv=(int)XM14_U16(&r);s->nt=(int)XM14_U16(&r);
        XM14_U16(&r);bone=XM14_U16(&r);
        if(!r.ok || bone!=0 || !s->nv || s->nv>XM14_VERTS || !s->nt || s->nt>XM14_TRIS)goto bad;
        s->v=(xm14Vertex*)calloc(s->nv,sizeof(*s->v));s->idx=(unsigned short*)calloc(s->nt*3,sizeof(*s->idx));
        if(!s->v || !s->idx)goto bad;
        faces=0;
        while(faces<s->nt){
            c=(int)XM14_U8(&r);if(c<3)goto bad;
            a=(int)XM14_U16(&r);b=(int)XM14_U16(&r);
            if(a>=s->nv || b>=s->nv)goto bad;
            for(j=2;j<c;j++){
                d=(int)XM14_U16(&r);if(!r.ok || d>=s->nv)goto bad;
                if(a!=b && a!=d && b!=d){
                    if(faces>=s->nt)goto bad;
                    /* Alternating triangle strips, reversed front winding. */
                    s->idx[3*faces]=d;s->idx[3*faces+1]=(unsigned short)((j&1)?a:b);
                    s->idx[3*faces+2]=(unsigned short)((j&1)?b:a);faces++;
                }
                a=b;b=d;
            }
        }
        for(j=0;j<s->nv;j++){
            for(k=0;k<3;k++)s->v[j].normal[k]=XM14_F32(&r);
            for(k=0;k<2;k++)s->v[j].uv[k]=XM14_F32(&r);
            for(k=0;k<3;k++){s->v[j].xyz[k]=XM14_F32(&r);if(s->v[j].xyz[k]<-100000 || s->v[j].xyz[k]>100000)goto bad;}
            if(!r.ok)goto bad;
        }
    }
    if(r.ok && r.at==r.n)return 1;
bad: XM14_Free(m);return 0;
}
#endif
