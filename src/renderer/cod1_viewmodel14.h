/* PATCH31: bounded CoD1 v14 skeleton, sampled XAnim and posed mesh reader.
 * Independent implementation checked against retail Thompson assets.
 * Layout reference: mauserzjeh/cod-asset-importer/templates (v14).
 * Absolute non-delta zero-note clips; PATCH61 also reads looping idle assets.
 */
#ifndef COD1_VIEWMODEL14_H
#define COD1_VIEWMODEL14_H
#include "cod1_xmodel14.h"
#include <math.h>
#define CV14_BONES 128
/* Animated meshes fit the renderer tessellation buffers (4000/24000). */
#define CV14_VERTS 3999
#define CV14_TRIS 7999

typedef struct { float p[3], q[4]; } cv14Transform;
typedef struct {
    int n, parent[CV14_BONES]; char name[CV14_BONES][XM14_NAME];
    cv14Transform local[CV14_BONES], world[CV14_BONES];
    c1mBoneMetadata_t metadata[CV14_BONES];
    int hasClassification; /* legacy streams without the trailing bytes stay distinguishable */
} cv14Skeleton;
typedef struct {
    int n; char name[CV14_BONES][XM14_NAME]; cv14Transform t[CV14_BONES];
} cv14Pose;
static void CV14_Identity(cv14Transform *t) { memset(t,0,sizeof(*t));t->q[3]=1; }
static int CV14_S16(xm14Reader *r) { unsigned int x=XM14_U16(r);return x<32768?(int)x:(int)x-65536; }
static void CV14_Rotate(const float *q,const float *v,float *out) {
    float t[3];int i;
    t[0]=2*(q[1]*v[2]-q[2]*v[1]);t[1]=2*(q[2]*v[0]-q[0]*v[2]);t[2]=2*(q[0]*v[1]-q[1]*v[0]);
    out[0]=q[1]*t[2]-q[2]*t[1];out[1]=q[2]*t[0]-q[0]*t[2];out[2]=q[0]*t[1]-q[1]*t[0];
    for(i=0;i<3;i++)out[i]+=v[i]+q[3]*t[i];
}
static void CV14_Compose(const cv14Transform *a,const cv14Transform *b,cv14Transform *out) {
    cv14Transform t;int i;CV14_Rotate(a->q,b->p,t.p);
    for(i=0;i<3;i++)t.p[i]+=a->p[i];
    t.q[0]=a->q[3]*b->q[0]+a->q[0]*b->q[3]+a->q[1]*b->q[2]-a->q[2]*b->q[1];
    t.q[1]=a->q[3]*b->q[1]-a->q[0]*b->q[2]+a->q[1]*b->q[3]+a->q[2]*b->q[0];
    t.q[2]=a->q[3]*b->q[2]+a->q[0]*b->q[1]-a->q[1]*b->q[0]+a->q[2]*b->q[3];
    t.q[3]=a->q[3]*b->q[3]-a->q[0]*b->q[0]-a->q[1]*b->q[1]-a->q[2]*b->q[2];*out=t;
}
static void CV14_ReadQ(xm14Reader *r,float *q,int simple,int negative) {
    double sum=0,length;int k,value;q[0]=q[1]=q[2]=0;
    /* Retail 0x47ecc0/0x47ed50 reconstruct and round the fourth short
     * against 32767^2. The XAnim flag negates the WHOLE first quaternion
     * (0x4788e8, 0x478b85), not just w; q and -q describe the same pose. */
    for(k=simple?2:0;k<3;k++){value=CV14_S16(r);q[k]=(float)value;sum+=(double)value*value;}
    if(sum>32767.0*32767.0*1.001)r->ok=0;
    q[3]=(float)floor(sqrt(sum<32767.0*32767.0?32767.0*32767.0-sum:0)+.5);
    length=sqrt(sum+(double)q[3]*q[3]);
    if(length<1){r->ok=0;return;}
    for(k=0;k<4;k++)q[k]=(float)(q[k]/length)*(negative?-1:1);
}
static int CV14_Parts(const void *data,size_t size,cv14Skeleton *s) {
    xm14Reader r;int n,roots,i,k;memset(s,0,sizeof(*s));r.p=(const unsigned char*)data;r.n=size;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14)return 0;n=XM14_U16(&r);roots=XM14_U16(&r);
    if(!roots || n+roots>CV14_BONES)return 0;s->n=n+roots;
    for(i=0;i<s->n;i++){
        CV14_Identity(&s->local[i]);s->parent[i]=-1;
        if(i>=roots){s->parent[i]=XM14_U8(&r);if(s->parent[i]>=i)return 0;
            for(k=0;k<3;k++)s->local[i].p[k]=XM14_F32(&r);
            CV14_ReadQ(&r,s->local[i].q,0,0);
        }
    }
    for(i=0;i<s->n;i++){if(!XM14_String(&r,s->name[i]) ||
        !C1M_ReadBounds(r.p,r.n,&r.at,&s->metadata[i].bounds))return 0;}
    /* v14 has one trailing classification byte per bone. */
    if(r.n-r.at==(size_t)s->n){
        if(!C1M_ReadBoneClassifications(r.p,r.n,&r.at,s->metadata,
                                      (size_t)s->n,CV14_BONES))return 0;
        s->hasClassification=1;
    }
    return r.ok && r.at==r.n;
}
static int CV14_Find(const cv14Pose *p,const char *name) {
    int i;for(i=0;i<p->n;i++)if(!strcmp(name,p->name[i]))return i;return -1;
}
/* Sample the v14 absolute tracks used by first-person weapon clips. Sparse and
 * dense tracks share the same interpolation. Notetrack payload follows all pose
 * data and is presentation metadata, not a transform stream: this pose-only
 * reader deliberately ignores a bounded non-empty note tail instead of rejecting
 * otherwise valid melee/weapon animations. Notetracks are NOT executed here.
 * Retail CoDSP 0x478386..0x4783f5: bit 0 is looping, bit 1 is delta;
 * looping adds one endpoint to the serialized frame count for track decoding. */
static int CV14_Indices(xm14Reader *r,int count,int frames,int *ids) {
    int i;
    if(count>frames || frames>4096)return 0;
    for(i=0;i<count;i++){
        ids[i]=(count>1 && count<frames)?(frames>256?(int)XM14_U16(r):(int)XM14_U8(r)):i;
        if(ids[i]>=frames || (i && ids[i]<=ids[i-1]) || (!i && ids[i]))return 0;
    }
    return r->ok;
}
static int CV14_SamplePose(const void *data,size_t size,float at,cv14Pose *p) {
    xm14Reader r;unsigned char sign[16],simple[16];int frames,flags,fps,bytes,i,j,k,count,notes,ids[4096];
    float values[4],previous[4],blend,dot,length;int low;
    memset(p,0,sizeof(*p));r.p=(const unsigned char*)data;r.n=size;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14)return 0;frames=XM14_U16(&r);p->n=XM14_U16(&r);flags=XM14_U8(&r);fps=XM14_U16(&r);
    if(!frames || frames>4096 || !p->n || p->n>CV14_BONES || (flags&~1) || !fps || !(at>=0 && at<4096))return 0;
    if(flags&1){frames++;if(frames>4096)return 0;}
    bytes=(p->n+7)/8;for(i=0;i<bytes;i++)sign[i]=XM14_U8(&r);for(i=0;i<bytes;i++)simple[i]=XM14_U8(&r);
    for(i=0;i<p->n;i++)if(!XM14_String(&r,p->name[i]))return 0;
    for(i=0;i<p->n;i++){
        CV14_Identity(&p->t[i]);count=XM14_U16(&r);if(!CV14_Indices(&r,count,frames,ids))return 0;
        low=0;for(j=0;j<count;j++)if(ids[j]<=at)low=j;
        for(j=0;j<count;j++){
            CV14_ReadQ(&r,values,simple[i/8]&(1<<(i%8)),!j && (sign[i/8]&(1<<(i%8))));
            if(j==low){memcpy(previous,values,sizeof(values));memcpy(p->t[i].q,values,sizeof(values));}
            else if(j==low+1){
                blend=(at-ids[low])/(ids[j]-ids[low]);dot=length=0;
                for(k=0;k<4;k++)dot+=previous[k]*values[k];
                for(k=0;k<4;k++){p->t[i].q[k]=previous[k]*(1-blend)+(dot<0?-values[k]:values[k])*blend;length+=p->t[i].q[k]*p->t[i].q[k];}
                if(length<0.00001f)return 0;length=(float)sqrt(length);for(k=0;k<4;k++)p->t[i].q[k]/=length;
            }
        }
        count=XM14_U16(&r);if(!CV14_Indices(&r,count,frames,ids))return 0;
        low=0;for(j=0;j<count;j++)if(ids[j]<=at)low=j;
        for(j=0;j<count;j++){
            for(k=0;k<3;k++)values[k]=XM14_F32(&r);
            if(j==low){memcpy(previous,values,3*sizeof(float));memcpy(p->t[i].p,values,3*sizeof(float));}
            else if(j==low+1){blend=(at-ids[low])/(ids[j]-ids[low]);for(k=0;k<3;k++)p->t[i].p[k]=previous[k]*(1-blend)+values[k]*blend;}
        }
    }
    notes=XM14_U8(&r);
    if(!r.ok)return 0;
    if(!notes)return r.at==r.n;
    /* The binary note record layout is intentionally not interpreted by the
     * viewmodel baker. It only needs the completed bone pose above. Reject a
     * truncated count and absurd tails, then consume the opaque metadata. */
    if(r.at>=r.n || r.n-r.at>64*1024)return 0;
    r.at=r.n;
    return 1;
}
static int CV14_FirstPose(const void *data,size_t size,cv14Pose *p) {return CV14_SamplePose(data,size,0,p);}
static void CV14_ApplyPose(cv14Skeleton *s,const cv14Pose *pose,const cv14Transform *root) {
    int i,j;for(i=0;i<s->n;i++){
        j=CV14_Find(pose,s->name[i]);if(j>=0)s->local[i]=pose->t[j];
        CV14_Compose(s->parent[i]<0?root:&s->world[s->parent[i]],&s->local[i],&s->world[i]);
    }
}
/* A joined model may call its root tag_weapon instead of tag_origin.
 * That bone is the same attachment already evaluated in the hands skeleton:
 * copy its world transform once instead of applying its track a second time. */
static void CV14_ApplyAttachedPose(cv14Skeleton *s,const cv14Pose *pose,
                                 const cv14Transform *root,const char *tag) {
    int i,j;for(i=0;i<s->n;i++){
        if(s->parent[i]<0 && !strcmp(s->name[i],tag)){s->world[i]=*root;continue;}
        j=CV14_Find(pose,s->name[i]);if(j>=0)s->local[i]=pose->t[j];
        CV14_Compose(s->parent[i]<0?root:&s->world[s->parent[i]],&s->local[i],&s->world[i]);
    }
}
static void CV14_Point(const cv14Transform *t,const float *v,float *out) {
    int k;CV14_Rotate(t->q,v,out);for(k=0;k<3;k++)out[k]+=t->p[k];
}
static int CV14_Mesh(const void *data,size_t size,const cv14Skeleton *s,xm14Mesh *m) {
    xm14Reader r;int i,j,k,c,a,b,d,faces,bone,bi,totalWeights,expectedWeights,unweighted,expectedUnweighted;
    unsigned short counts[CV14_VERTS];float sums[CV14_VERTS],pos[3],out[3],w;
    xm14Surface *surf;memset(m,0,sizeof(*m));r.p=(const unsigned char*)data;r.n=size;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14)return 0;m->ns=XM14_U16(&r);if(!m->ns || m->ns>XM14_SURFS)goto bad;
    for(i=0;i<m->ns;i++){
        surf=&m->s[i];XM14_U8(&r);surf->nv=XM14_U16(&r);surf->nt=XM14_U16(&r);XM14_U16(&r);bone=XM14_U16(&r);
        if(!r.ok || !surf->nv || surf->nv>CV14_VERTS || !surf->nt || surf->nt>CV14_TRIS || (bone!=65535 && bone>=s->n))goto bad;
        expectedWeights=expectedUnweighted=-1;
        if(bone==65535){expectedWeights=XM14_U16(&r);expectedUnweighted=XM14_U16(&r);}
        surf->v=(xm14Vertex*)calloc(surf->nv,sizeof(*surf->v));surf->idx=(unsigned short*)calloc(surf->nt*3,sizeof(*surf->idx));
        if(!surf->v || !surf->idx)goto bad;faces=0;
        while(faces<surf->nt){
            c=XM14_U8(&r);if(c<3)goto bad;a=XM14_U16(&r);b=XM14_U16(&r);if(a>=surf->nv || b>=surf->nv)goto bad;
            for(j=2;j<c;j++){
                d=XM14_U16(&r);if(!r.ok || d>=surf->nv)goto bad;
                if(a!=b && a!=d && b!=d){
                    if(faces>=surf->nt)goto bad;
                    /* RTCW front winding: same conversion as PATCH19 statics. */
                    surf->idx[3*faces]=d;surf->idx[3*faces+1]=(unsigned short)((j&1)?b:a);surf->idx[3*faces+2]=(unsigned short)((j&1)?a:b);faces++;
                }a=b;b=d;
            }
        }
        totalWeights=unweighted=0;
        for(j=0;j<surf->nv;j++){
            for(k=0;k<3;k++)pos[k]=XM14_F32(&r);
            for(k=0;k<2;k++)surf->v[j].uv[k]=XM14_F32(&r);
            counts[j]=bone==65535?XM14_U16(&r):0;bi=bone==65535?XM14_U16(&r):bone;
            if(bi>=s->n || counts[j]>8)goto bad;
            CV14_Rotate(s->world[bi].q,pos,surf->v[j].normal);
            for(k=0;k<3;k++)pos[k]=XM14_F32(&r);
            w=counts[j]?XM14_F32(&r):1;if(w<0 || w>1)goto bad;sums[j]=w;
            CV14_Point(&s->world[bi],pos,out);for(k=0;k<3;k++)surf->v[j].xyz[k]=w*out[k];
            totalWeights+=counts[j];if(!counts[j])unweighted++;
        }
        if(bone==65535 && (totalWeights!=expectedWeights || unweighted!=expectedUnweighted))goto bad;
        for(j=0;j<surf->nv;j++){
            for(c=0;c<counts[j];c++){
                bi=XM14_U16(&r);if(bi>=s->n)goto bad;
                for(k=0;k<3;k++)pos[k]=XM14_F32(&r);w=XM14_F32(&r);if(w<0 || w>1)goto bad;
                sums[j]+=w;CV14_Point(&s->world[bi],pos,out);for(k=0;k<3;k++)surf->v[j].xyz[k]+=w*out[k];
            }
            if(fabs(sums[j]-1)>0.001)goto bad;
            for(k=0;k<3;k++)if(!(surf->v[j].xyz[k]>-100000000 && surf->v[j].xyz[k]<100000000))goto bad;
        }
        if(!r.ok)goto bad;
    }
    if(r.ok && r.at==r.n)return 1;
bad:XM14_Free(m);return 0;
}
#endif
