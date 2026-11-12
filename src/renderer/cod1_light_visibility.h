/* R56: lazy visibility records for absent IBSP59 cache entries.
 * Observed native producer: 4a9c30, sun trace 4a99c0, sky grid 4a9a90.
 * Immutable disk records take precedence. R72 finite source visibility covers
 * types 2/3/4/5/7 independently of the bounded R57 type 2/3/4 RGB consumer;
 * native ranking, cone shading and directional sky remain separate work. */
#ifndef COD1_LIGHT_VISIBILITY_H
#define COD1_LIGHT_VISIBILITY_H
#include "cod1_lightcache.h"
#include "cod1_local_light.h"
#include <stdlib.h>

typedef struct {
    float fraction;
    unsigned int surfaceFlags, contents;
    int startsolid;
} c1VisibilityHit;
typedef int (*c1VisibilityTrace)(void *,const float *,const float *,c1VisibilityHit *);
typedef int (*c1VisibilityContents)(void *,const float *,unsigned int *);
typedef struct {
    unsigned int key;
    unsigned char state, sky;
    unsigned short mask;
} c1VisibilityRecord;
typedef struct {
    c1VisibilityRecord records[262144];
    int quality;
    const unsigned char *lightRecords;
    unsigned int numLights;
    unsigned int generated, rejected, unavailable, memoHits, rays, contentsQueries;
} c1VisibilityRuntime;

static int C1V_Trace(c1VisibilityRuntime *rt,c1VisibilityTrace trace,void *context,
                    const float *start,const float *end,c1VisibilityHit *hit) {
    memset(hit,0,sizeof(*hit));hit->fraction=1;
    rt->rays++;
    if(!trace(context,start,end,hit))return 0;
    return hit->fraction>=0 && hit->fraction<=1;
}
/* Native sky test: startsolid uses contents bit 11; otherwise a clear trace
 * or a SURF_SKY contact is visible. Original trace byte +2f is startsolid. */
static int C1V_Sky(const c1VisibilityHit *hit) {
    if(hit->startsolid)return (hit->contents&0x800u)!=0;
    return hit->fraction==1 || (hit->surfaceFlags&4u)!=0;
}
static float C1V_Store(double value) { volatile float out=(float)value;return out; }

/* The native decoder sets runtime +0x50 to one for finite types 2/3/4/5/7
 * and copies authored +0x10..+0x18 to runtime +0x44..+0x4c. The grid producer
 * 4a9c30 branches on that position/direction role without reading cone fields.
 * Decode geometry separately from C1L_Point so an unimplemented RGB consumer
 * cannot suppress a valid finite-source visibility bit. */
static int C1V_PointPosition(const unsigned char *record,float *position) {
    unsigned int type;int k;
    if(!record || !position)return 0;
    type=C1_U32(record);
    if(type!=2 && type!=3 && type!=4 && type!=5 && type!=7)return 0;
    for(k=0;k<3;k++){
        position[k]=C1_F32(record+16+k*4);
        if(!(position[k]>=-131072 && position[k]<131072))return 0;
    }
    return 1;
}

/* Return 1 for a valid record, 0 for rejected geometry, -1 when the native
 * collision service is unavailable. Never turn an unavailable trace into light. */
static int C1V_Produce(c1VisibilityRuntime *rt,const c1LightCache *cache,
                      const float *grid,const float *query,const float *sunDir,
                      int sunBit,c1VisibilityTrace trace,
                      c1VisibilityContents contents,void *context,
                      c1VisibilityRecord *out) {
    unsigned int flags;float delta[3],start[3],end[3],length;
    c1VisibilityHit hit;int k,l,x,y,first,nlights,hasSky,q=rt->quality;double sum=0;
    out->state=2;out->sky=0;out->mask=0;
    l=C1_Leaf(cache,grid);
    if(l<0 || C1_I32(cache->leaves+l*36)<0)return 0;
    /* 4a9e35/4a9e3e pass the GRID leaf to 4a9a90, not the caller leaf. */
    first=C1_I32(cache->leaves+l*36+28);nlights=C1_I32(cache->leaves+l*36+32);
    hasSky=nlights && (cache->refs[first*2+1]&128);
    rt->contentsQueries++;
    if(!contents(context,grid,&flags))return -1;
    if(flags&1u)return 0;
    for(k=0;k<3;k++){delta[k]=C1V_Store((double)query[k]-grid[k]);sum+=(double)delta[k]*delta[k];}
    length=C1V_Store(sqrt(sum));
    for(k=0;k<3;k++){
        float unit=length>0?C1V_Store((double)delta[k]/length):0;
        start[k]=C1V_Store((double)grid[k]+(double)unit*0.009999999776482582);
    }
    if(!C1V_Trace(rt,trace,context,start,query,&hit))return -1;
    if(hit.startsolid || hit.fraction<1)return 0;
    if(sunBit>=0){
        for(k=0;k<3;k++){
            start[k]=C1V_Store((double)grid[k]+(double)sunDir[k]*0.10000000149011612);
            end[k]=C1V_Store((double)grid[k]+(double)sunDir[k]*32768.0);
        }
        if(!C1V_Trace(rt,trace,context,start,end,&hit))return -1;
        if(C1V_Sky(&hit))out->mask|=(unsigned short)(1u<<sunBit);
    }
    /* 4a9dad..4a9e20: point source ray starts grid + unit*0.1 and
     * ends at the authored source. Bit order is the CALLER leaf list. */
    if(rt->lightRecords){
        int queryLeaf=C1_Leaf(cache,query),refFirst,refCount,bit;
        if(queryLeaf>=0){
            refFirst=C1_I32(cache->leaves+queryLeaf*36+28);
            refCount=C1_I32(cache->leaves+queryLeaf*36+32);
            if(refCount && (cache->refs[refFirst*2+1]&128)){refFirst++;refCount--;}
            for(bit=0;bit<refCount && bit<15;bit++){
                unsigned int index=cache->refs[(refFirst+bit)*2]|(cache->refs[(refFirst+bit)*2+1]<<8);
                float position[3];double distance=0;
                if(index>=rt->numLights || !C1V_PointPosition(rt->lightRecords+index*72,position))continue;
                for(k=0;k<3;k++){delta[k]=C1V_Store((double)position[k]-grid[k]);distance+=(double)delta[k]*delta[k];}
                length=C1V_Store(sqrt(distance));
                for(k=0;k<3;k++){
                    float unit=length>0?C1V_Store((double)delta[k]/length):0;
                    start[k]=C1V_Store((double)grid[k]+(double)unit*0.10000000149011612);
                }
                if(!C1V_Trace(rt,trace,context,start,position,&hit))return -1;
                if(!hit.startsolid && hit.fraction==1)out->mask|=(unsigned short)(1u<<bit);
            }
        }
    }
    if(hasSky){
        /* Avoid the legacy Win32 "far" macro from windows.h. */
        float skyGridStride=q>3?16384.0f:32768.0f;
        float spread=(q-1)*0.5f*skyGridStride;
        for(y=0;y<q;y++)for(x=0;x<q;x++){
            float dx=-spread+x*skyGridStride,dy=-spread+y*skyGridStride;
            start[0]=C1V_Store((double)grid[0]+(double)dx*6.103515488575795e-7);
            start[1]=C1V_Store((double)grid[1]+(double)dy*6.103515488575795e-7);
            start[2]=C1V_Store((double)grid[2]+0.10000000149011612);
            end[0]=C1V_Store((double)grid[0]+dx);end[1]=C1V_Store((double)grid[1]+dy);
            /* 4a9ade..4a9ae7: vertical extent remains 32768 at every quality. */
            end[2]=C1V_Store((double)grid[2]+32768.0);
            if(!C1V_Trace(rt,trace,context,start,end,&hit))return -1;
            if(C1V_Sky(&hit)){out->sky+=2;out->mask|=0x8000u;}
        }
    }
    out->state=1;return 1;
}

static const c1VisibilityRecord *C1V_Record(c1VisibilityRuntime *rt,
                        const c1LightCache *cache,const unsigned int *gridIndex,
                        unsigned int cluster,const float *query,const float *sunDir,
                        int sunBit,c1VisibilityTrace trace,
                        c1VisibilityContents contents,void *context) {
    unsigned int key=C1_Key(gridIndex[0],gridIndex[1],cluster);
    unsigned int bucket=C1_Bucket(gridIndex[0],gridIndex[1],gridIndex[2]);
    c1VisibilityRecord record,*slots=rt->records+bucket*32;float grid[3];int i,result;
    for(i=0;i<32;i++){
        if(!slots[i].state)break;
        if(slots[i].key==key){rt->memoHits++;return &slots[i];}
    }
    grid[0]=(float)((int)gridIndex[0]-4096)*32;
    grid[1]=(float)((int)gridIndex[1]-4096)*32;
    grid[2]=(float)((int)gridIndex[2]-2048)*64;
    memset(&record,0,sizeof(record));record.key=key;
    result=C1V_Produce(rt,cache,grid,query,sunDir,sunBit,trace,contents,context,&record);
    if(result<0){rt->unavailable++;return NULL;}
    if(result)rt->generated++;else rt->rejected++;
    if(i==32){memmove(slots,slots+1,31*sizeof(*slots));i=31;}
    slots[i]=record;return &slots[i];
}

/* Keep the R55 consumer formula. Only fill absent records; explicit state 2
 * records remain excluded. Existing sky/local-light shading gaps stay explicit. */
static int C1V_Sample(const c1LightCache *cache,const float *point,int sunIndex,int quality,
                      const float *sunDir,c1VisibilityRuntime *rt,
                      c1VisibilityTrace trace,c1VisibilityContents contents,void *context,
                      float *sun,float *sky,float *coverage) {
    int l,lo[3],i,j,bit=-1,nlights,first;float t[3],total=0,sv=0,sk=0;
    const unsigned char *leaf;
    if(!rt || !trace || !contents)return C1_Sample(cache,point,sunIndex,quality,sun,sky,coverage);
    *sun=*sky=*coverage=0;
    if(quality<1 || quality>5 || quality!=rt->quality)return 0;
    l=C1_Leaf(cache,point);if(l<0)return 0;
    leaf=cache->leaves+l*36;if(C1_I32(leaf)<0)return 0;
    first=C1_I32(leaf+28);nlights=C1_I32(leaf+32);
    if(nlights && (cache->refs[first*2+1]&128)){first++;nlights--;}
    for(i=0;i<nlights && i<15;i++){
        unsigned int ref=cache->refs[(first+i)*2]|(cache->refs[(first+i)*2+1]<<8);
        if(ref==(unsigned int)sunIndex)bit=i;
    }
    for(i=0;i<3;i++){float v=(point[i]+131072.0f)/(i==2?64.0f:32.0f);lo[i]=(int)floor(v);t[i]=v-lo[i];}
    for(i=0;i<8;i++){
        float w=1;unsigned int q[3],mask,count;const unsigned char *disk;
        const c1VisibilityRecord *runtime;
        for(j=0;j<3;j++){int upper=(i>>j)&1;q[j]=lo[j]+upper;w*=upper?t[j]:1-t[j];}
        if(w<=0)continue;
        disk=C1_Lookup(cache,q[0],q[1],q[2],C1_U32(leaf));
        if(disk){
            if(disk[4]!=1)continue;
            mask=disk[10]|(disk[11]<<8);count=disk[4+quality];
        }else{
            runtime=C1V_Record(rt,cache,q,C1_U32(leaf),point,sunDir,bit,trace,contents,context);
            if(!runtime || runtime->state!=1)continue;
            mask=runtime->mask;count=runtime->sky;
        }
        total+=w;if(bit>=0 && (mask&(1u<<bit)))sv+=w;
        sk+=w*(float)count/(2*quality*quality);
    }
    if(total<0.000001f)return 0;
    *sun=sv/total;*sky=sk/total;if(*sky>1)*sky=1;*coverage=total;return 1;
}

/* R57 retains R56 interpolation for this bounded point-light consumer. Full
 * native ranking/aggregation and partial-sun behavior remain separate work. */
static int C1V_PointRGB(const c1LightCache *cache,const float *point,const float *normal,
                       const float *sunDir,int sunIndex,c1VisibilityRuntime *rt,
                       c1VisibilityTrace trace,c1VisibilityContents contents,void *context,float *rgb) {
    float weights[15]={0},t[3],total=0;int lo[3],l,first,count,bit=-1,i,j,used=0;
    if(!rt || !rt->lightRecords || !trace || !contents)return 0;
    l=C1_Leaf(cache,point);if(l<0 || C1_I32(cache->leaves+l*36)<0)return 0;
    first=C1_I32(cache->leaves+l*36+28);count=C1_I32(cache->leaves+l*36+32);
    if(count && (cache->refs[first*2+1]&128)){first++;count--;}
    if(count>15)count=15;
    for(i=0;i<count;i++){
        unsigned int ref=cache->refs[(first+i)*2]|(cache->refs[(first+i)*2+1]<<8);
        if(ref==(unsigned int)sunIndex)bit=i;
    }
    for(i=0;i<3;i++){float v=(point[i]+131072.0f)/(i==2?64.0f:32.0f);lo[i]=(int)floor(v);t[i]=v-lo[i];}
    for(i=0;i<8;i++){
        float w=1;unsigned int q[3],mask;const unsigned char *disk;const c1VisibilityRecord *runtime;
        for(j=0;j<3;j++){int upper=(i>>j)&1;q[j]=lo[j]+upper;w*=upper?t[j]:1-t[j];}
        if(w<=0)continue;
        disk=C1_Lookup(cache,q[0],q[1],q[2],C1_U32(cache->leaves+l*36));
        if(disk){if(disk[4]!=1)continue;mask=disk[10]|(disk[11]<<8);}
        else{
            runtime=C1V_Record(rt,cache,q,C1_U32(cache->leaves+l*36),point,sunDir,bit,trace,contents,context);
            if(!runtime || runtime->state!=1)continue;mask=runtime->mask;
        }
        total+=w;for(j=0;j<count;j++)if(mask&(1u<<j))weights[j]+=w;
    }
    if(total<0.000001f)return 0;
    for(i=0;i<count;i++)if(weights[i]>0){
        unsigned int ref=cache->refs[(first+i)*2]|(cache->refs[(first+i)*2+1]<<8);
        c1PointLight light;float value[3];
        if(ref>=rt->numLights || !C1L_Point(rt->lightRecords+ref*72,&light))continue;
        if(!C1L_RGB(&light,point,normal,weights[i]/total,value))continue;
        for(j=0;j<3;j++)rgb[j]+=value[j];used++;
    }
    return used;
}
#endif
