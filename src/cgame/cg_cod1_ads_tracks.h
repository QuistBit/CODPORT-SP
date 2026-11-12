/* PATCH47: one-bone absolute, nonlooping v14 ADS clips for the rigid adapter.
 * Runtime quaternion keys follow retail 0x47ecc0 and 0x478b71..0x478cfa:
 * reconstruct rounded int16 w at scale 32767, select signs during loading,
 * interpolate raw components, normalize only after mixing the two leaves.
 * This reader deliberately does not change the existing full-skeleton reader. */
#ifndef CG_COD1_ADS_TRACKS_H
#define CG_COD1_ADS_TRACKS_H
#include "../renderer/cod1_viewmodel14.h"
#define COD1_ADS_FRAMES 128
static int COD1_ADSReadRoots(const void *data,int size,cv14Transform *roots,int *outFrames) {
    xm14Reader r;char name[XM14_NAME];int frames,bones,flags,fps,sign,simple;
    int ids[COD1_ADS_FRAMES],count,i,j,k,track,lo;float keys[COD1_ADS_FRAMES][4],v,t;
    double square,dot;
    r.p=(const unsigned char*)data;r.n=size;r.at=0;r.ok=1;
    if(XM14_U16(&r)!=14)return 0;
    frames=XM14_U16(&r);bones=XM14_U16(&r);flags=XM14_U8(&r);fps=XM14_U16(&r);
    if(frames<1 || frames>COD1_ADS_FRAMES || bones!=1 || flags || !fps)return 0;
    sign=XM14_U8(&r);simple=XM14_U8(&r);
    if(sign>1 || simple>1 || !XM14_String(&r,name) || strcmp(name,"tag_torso"))return 0;
    for(i=0;i<frames;i++)CV14_Identity(&roots[i]);
    for(track=0;track<2;track++) {
        count=XM14_U16(&r);
        if(count>COD1_ADS_FRAMES || !CV14_Indices(&r,count,frames,ids))return 0;
        for(j=0;j<count;j++) {
            memset(keys[j],0,sizeof(keys[j]));
            if(!track) {
                square=32767.0*32767.0;
                for(k=simple?2:0;k<3;k++){v=(float)CV14_S16(&r);keys[j][k]=v;square-=(double)v*v;}
                keys[j][3]=square>0?(float)floor(sqrt(square)+.5):0;
                dot=0;
                if(j)for(k=0;k<4;k++)dot+=(double)keys[j-1][k]*keys[j][k];
                if((!j && sign) || (j && dot<0))for(k=0;k<4;k++)keys[j][k]=-keys[j][k];
            } else for(k=0;k<3;k++)keys[j][k]=XM14_F32(&r);
            if(!r.ok)return 0;
        }
        for(i=0;i<frames && count;i++) {
            lo=0;while(lo+1<count && ids[lo+1]<=i)lo++;
            t=lo+1<count?(float)(i-ids[lo])/(ids[lo+1]-ids[lo]):0;
            for(k=0;k<(track?3:4);k++) {
                v=keys[lo][k];if(t)v+=(keys[lo+1][k]-v)*t;
                if(track)roots[i].p[k]=v;else roots[i].q[k]=v*(1.0f/32767.0f);
            }
        }
    }
    if(XM14_U8(&r)!=0 || !r.ok || r.at!=r.n)return 0;
    *outFrames=frames;return 1;
}
static cv14Transform COD1_ADSSampleRoot(const cv14Transform *roots,int frames,float fraction) {
    cv14Transform t;float sample=fraction*(frames-1),f;int i,k,next;
    if(sample<0)sample=0;if(sample>frames-1)sample=(float)(frames-1);
    i=(int)sample;f=sample-i;next=i+1<frames?i+1:i;
    for(k=0;k<3;k++)t.p[k]=roots[i].p[k]+(roots[next].p[k]-roots[i].p[k])*f;
    for(k=0;k<4;k++)t.q[k]=roots[i].q[k]+(roots[next].q[k]-roots[i].q[k])*f;
    return t;
}
static cv14Transform COD1_ADSBlendRoots(const cv14Transform *up,const cv14Transform *down,
                                       float upWeight,float downWeight) {
    cv14Transform t;float total=upWeight+downWeight,length=0;int k;
    if(total<=0){upWeight=1;downWeight=0;total=1;}
    for(k=0;k<3;k++)t.p[k]=(up->p[k]*upWeight+down->p[k]*downWeight)/total;
    for(k=0;k<4;k++){t.q[k]=up->q[k]*upWeight+down->q[k]*downWeight;length+=t.q[k]*t.q[k];}
    if(length<=0){CV14_Identity(&t);return t;}
    length=(float)sqrt(length);for(k=0;k<4;k++)t.q[k]/=length;
    return t;
}
#endif
