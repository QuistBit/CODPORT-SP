/* CoD1 v59 light visibility cache. Independently implemented from observed
 * CoDSP.exe behavior: 4ac6a0 load, 4a9e70 key/hash, 4a9ec0 lookup.
 * These are visibility samples, NOT CoD2 RGB irradiance coefficients. */
#ifndef COD1_LIGHTCACHE_H
#define COD1_LIGHTCACHE_H
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
typedef struct {const unsigned char *planes,*nodes,*leaves,*refs,*slots;int np,nn,nl,nr;} c1LightCache;
static unsigned int C1_U32(const unsigned char*p){return (unsigned int)p[0]|((unsigned int)p[1]<<8)|((unsigned int)p[2]<<16)|((unsigned int)p[3]<<24);}
static int C1_I32(const unsigned char*p){unsigned int u=C1_U32(p);int v;memcpy(&v,&u,4);return v;}
static float C1_F32(const unsigned char*p){unsigned int u=C1_U32(p);float v;memcpy(&v,&u,4);return v;}
static const unsigned char *C1_Lump(const unsigned char*b,size_t n,int i,unsigned int *len){unsigned int off;if(n<272)return NULL;*len=C1_U32(b+8+i*8);off=C1_U32(b+12+i*8);if(off>n || *len>n-off)return NULL;return b+off;}
static int C1_Init(c1LightCache*c,const void*data,size_t n){const unsigned char*b=(const unsigned char*)data;unsigned int z;int i,j,k;memset(c,0,sizeof(*c));
 if(n<272 || memcmp(b,"IBSP",4) || C1_U32(b+4)!=59)return 0;
 c->planes=C1_Lump(b,n,2,&z);if(!c->planes || !z || z%16)return 0;c->np=z/16;
 c->nodes=C1_Lump(b,n,20,&z);if(!c->nodes || !z || z%36)return 0;c->nn=z/36;
 c->leaves=C1_Lump(b,n,21,&z);if(!c->leaves || !z || z%36)return 0;c->nl=z/36;
 c->refs=C1_Lump(b,n,19,&z);if(!c->refs || z%2)return 0;c->nr=z/2;
 c->slots=C1_Lump(b,n,32,&z);if(!c->slots || z!=3145728)return 0;
 for(i=0;i<c->np;i++)for(j=0;j<4;j++){float v=C1_F32(c->planes+i*16+j*4);if(!(v>=-FLT_MAX && v<=FLT_MAX))return 0;}
 for(i=0;i<c->nn;i++){const unsigned char*p=c->nodes+i*36;k=C1_I32(p);if(k<0||k>=c->np)return 0;for(j=1;j<3;j++){k=C1_I32(p+j*4);if(k>=c->nn || k < -c->nl)return 0;}}
 for(i=0;i<c->nl;i++){const unsigned char*p=c->leaves+i*36;int a=C1_I32(p+28),m=C1_I32(p+32);if(a<0||m<0||a>c->nr||m>c->nr-a)return 0;}
 for(i=0;i<262144;i++)if(c->slots[i*12+4]>2)return 0;
 return 1;
}
static int C1_Leaf(const c1LightCache*c,const float*p){int i=0,steps,k;for(k=0;k<3;k++)if(!(p[k]>=-131072 && p[k]<131072))return -1;
 for(steps=0;steps<=c->nn;steps++){const unsigned char*nd,*pl;double d;if(i<0)return -1-i;nd=c->nodes+i*36;pl=c->planes+C1_I32(nd)*16;d=(double)p[0]*C1_F32(pl)+(double)p[1]*C1_F32(pl+4)+(double)p[2]*C1_F32(pl+8)-C1_F32(pl+12);i=C1_I32(nd+(d>0?4:8));}return -1;}
static unsigned int C1_ReverseBytes(unsigned int v){v=((v&0x55555555u)<<1)|((v>>1)&0x55555555u);v=((v&0x33333333u)<<2)|((v>>2)&0x33333333u);return ((v&0x0f0f0f0fu)<<4)|((v>>4)&0x0f0f0f0fu);}
static unsigned int C1_Key(unsigned int x,unsigned int y,unsigned int cluster){return (((y<<10)|(x&1023u))<<12)|(cluster&4095u);}
static unsigned int C1_Bucket(unsigned int x,unsigned int y,unsigned int z){return (C1_ReverseBytes(z)+y*0xc41u-x*0xc3du)&8191u;}
static const unsigned char*C1_Lookup(const c1LightCache*c,unsigned int x,unsigned int y,unsigned int z,unsigned int cluster){unsigned int key=C1_Key(x,y,cluster);unsigned int bucket=C1_Bucket(x,y,z);int i;const unsigned char*p=c->slots+bucket*384;
 for(i=0;i<32;i++,p+=12){if(!p[4])return NULL;if(C1_U32(p)==key)return p;}return NULL;}
/* Trilinear visibility interpolation of valid stored samples. Missing records
 * are not synthesized; no original runtime collision-trace fallback yet. */
static int C1_Sample(const c1LightCache*c,const float*p,int sunIndex,int quality,float*sun,float*sky,float*coverage){int l=C1_Leaf(c,p),lo[3],i,j,bit=-1,nlights,first;float t[3],total=0,sv=0,sk=0;const unsigned char*leaf;
 *sun=*sky=*coverage=0;if(l<0||quality<1||quality>5)return 0;leaf=c->leaves+l*36;if(C1_I32(leaf)<0)return 0;
 first=C1_I32(leaf+28);nlights=C1_I32(leaf+32);if(nlights && (c->refs[first*2+1]&128)){first++;nlights--;}
 for(i=0;i<nlights && i<15;i++){unsigned int ref=c->refs[(first+i)*2]|(c->refs[(first+i)*2+1]<<8);if(ref==(unsigned int)sunIndex)bit=i;}
 for(i=0;i<3;i++){float v=(p[i]+131072.0f)/(i==2?64.0f:32.0f);lo[i]=(int)floor(v);t[i]=v-lo[i];}
 for(i=0;i<8;i++){float w=1;unsigned int q[3];const unsigned char*r;unsigned int mask;for(j=0;j<3;j++){int upper=(i>>j)&1;q[j]=lo[j]+upper;w*=upper?t[j]:1-t[j];}if(w<=0)continue;r=C1_Lookup(c,q[0],q[1],q[2],C1_U32(leaf));if(!r||r[4]!=1)continue;mask=r[10]|(r[11]<<8);total+=w;if(bit>=0 && (mask&(1u<<bit)))sv+=w;sk+=w*(float)r[4+quality]/(2*quality*quality);}
 if(total<0.000001f)return 0;*sun=sv/total;*sky=sk/total;if(*sky>1)*sky=1;*coverage=total;return 1;
}
#endif
