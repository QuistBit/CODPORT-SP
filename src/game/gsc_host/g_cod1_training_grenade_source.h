/* R216 opt-in original-function subset. No original GSC bodies are embedded.
 * Full winning VFS bytes are authenticated before the first write. Selected
 * whole functions retain exact bytes, offsets, columns and CR/LF positions.
 * This does not admit original training main or suppress selected statements. */
#ifndef G_COD1_TRAINING_GRENADE_SOURCE_H
#define G_COD1_TRAINING_GRENADE_SOURCE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct { size_t first,end; int restock; } cod1TrainingGrenadeSpan_t;
static uint32_t C1TGS_Ror(uint32_t v,unsigned n){return (v>>n)|(v<<(32-n));}
static void C1TGS_Block(uint32_t h[8],const unsigned char block[64]) {
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[64],a,b,c,d,e,f,g,x,t1,t2,s0,s1;unsigned i;
    for(i=0;i<16;i++)w[i]=((uint32_t)block[i*4]<<24)|((uint32_t)block[i*4+1]<<16)|((uint32_t)block[i*4+2]<<8)|block[i*4+3];
    for(i=16;i<64;i++){s0=C1TGS_Ror(w[i-15],7)^C1TGS_Ror(w[i-15],18)^(w[i-15]>>3);s1=C1TGS_Ror(w[i-2],17)^C1TGS_Ror(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
    a=h[0];b=h[1];c=h[2];d=h[3];e=h[4];f=h[5];g=h[6];x=h[7];
    for(i=0;i<64;i++){s1=C1TGS_Ror(e,6)^C1TGS_Ror(e,11)^C1TGS_Ror(e,25);t1=x+s1+((e&f)^((~e)&g))+k[i]+w[i];s0=C1TGS_Ror(a,2)^C1TGS_Ror(a,13)^C1TGS_Ror(a,22);t2=s0+((a&b)^(a&c)^(b&c));x=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=x;
}
static int C1TGS_Matches(const unsigned char *data,size_t size,const unsigned char expected[32]) {
    uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    unsigned char tail[128],digest[32];size_t full=size/64,left=size%64,j;uint64_t bits=(uint64_t)size*8;unsigned i;
    for(j=0;j<full;j++)C1TGS_Block(h,data+j*64);
    memset(tail,0,sizeof(tail));memcpy(tail,data+full*64,left);tail[left]=0x80;
    j=left<56?64:128;for(i=0;i<8;i++)tail[j-1-i]=(unsigned char)(bits>>(i*8));
    C1TGS_Block(h,tail);if(j==128)C1TGS_Block(h,tail+64);
    for(i=0;i<8;i++){digest[i*4]=(unsigned char)(h[i]>>24);digest[i*4+1]=(unsigned char)(h[i]>>16);digest[i*4+2]=(unsigned char)(h[i]>>8);digest[i*4+3]=(unsigned char)h[i];}
    return !memcmp(digest,expected,32);
}
static const unsigned char c1tgsHash0[32]={0xbb,0x86,0xa2,0xa2,0x1f,0x3f,0xa3,0x9c,0x8e,0x53,0xec,0xc5,0x9d,0x47,0xc0,0x99,0x09,0x5a,0x71,0x9a,0xa1,0x1e,0x59,0xc5,0x6d,0x7f,0x87,0xeb,0x24,0x17,0x03,0x15};
static const cod1TrainingGrenadeSpan_t c1tgsSpans0[]={{19360u,20744u,0},{50561u,50848u,0},{50850u,51007u,0},{58969u,60527u,1}};
static const unsigned char c1tgsHash1[32]={0xce,0x48,0x16,0xbf,0xbc,0xfe,0x2b,0xf9,0xd3,0x4e,0x49,0x94,0xb2,0x08,0x97,0x16,0x24,0x22,0x1e,0x9c,0xe7,0xbd,0x69,0x6e,0x8a,0x58,0x2e,0x8f,0x3a,0xe3,0x7e,0x27};
static const cod1TrainingGrenadeSpan_t c1tgsSpans1[]={{3259u,3423u,0},{3425u,3514u,0},{24740u,24892u,0}};
/* Return 0 for an unrelated canonical path (untouched), -1 for a known
 * original module with wrong bytes/profile (untouched), 1 after selection.
 * restock0 retains six whole functions; restock1 adds whole give_ammo().
 * Caller owns a mutable size-byte source lease, not live GAME storage. */
static int G_Cod1TrainingGrenadeSourceSelect(const char *path,char *data,size_t size,int restock) {
    const unsigned char *expected;const cod1TrainingGrenadeSpan_t *spans;size_t count,wanted,i,j;int keep;
    if(!path)return -1;
    if(!strcmp(path,"maps/training.gsc")){expected=c1tgsHash0;spans=c1tgsSpans0;count=sizeof(c1tgsSpans0)/sizeof(c1tgsSpans0[0]);wanted=61690;}
    else if(!strcmp(path,"maps/_utility.gsc")){expected=c1tgsHash1;spans=c1tgsSpans1;count=sizeof(c1tgsSpans1)/sizeof(c1tgsSpans1[0]);wanted=25279;}
    else return 0;
    if(!data||size!=wanted||(restock!=0&&restock!=1)||!C1TGS_Matches((const unsigned char*)data,size,expected))return -1;
    for(i=0;i<size;i++){
        keep=0;for(j=0;j<count;j++)if(i>=spans[j].first&&i<spans[j].end&&(!spans[j].restock||restock)){keep=1;break;}
        if(!keep&&data[i]!='\r'&&data[i]!='\n')data[i]=' ';
    }
    return 1;
}
#endif
