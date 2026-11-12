/* Runtime MP3 -> PCM WAV bridge. Decodes bounded data from the virtual FS.
 * minimp3, CC0, revision ea99364f61c14656440e8d77e9c233ccf3124633.
 * No extra Windows DLL or build target is required. */
#ifndef COD1_MP3_H
#define COD1_MP3_H
#include <stdlib.h>
#include <string.h>
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"
#define COD_MP3_MAX_INPUT (16 * 1024 * 1024)
#define COD_MP3_MAX_PCM (64 * 1024 * 1024)
static void CodMP3_U32(unsigned char *p, unsigned int n) {
    p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8);
    p[2]=(unsigned char)(n>>16); p[3]=(unsigned char)(n>>24);
}
/* Returned allocation contains a complete little-endian PCM WAV. */
static unsigned char *CodMP3_Decode(const unsigned char *data, int length, int *wavBytes) {
    mp3dec_t dec;
    mp3dec_frame_info_t info;
    mp3d_sample_t frame[MINIMP3_MAX_SAMPLES_PER_FRAME];
    unsigned char *wav=NULL, *grown;
    int pos=0, used=44, capacity=65536, rate=0, channels=0, samples, bytes, i;
    *wavBytes=0;
    if (!data || length<=0 || length>COD_MP3_MAX_INPUT) return NULL;
    /* ID3v1 trailing metadata must not conceal the final MPEG frame. */
    if(length>=128 && !memcmp(data+length-128,"TAG",3)) length-=128;
    wav=(unsigned char *)malloc(capacity);
    if (!wav) return NULL;
    mp3dec_init(&dec);
    while(pos<length) {
        samples=mp3dec_decode_frame(&dec,data+pos,length-pos,frame,&info);
        if(info.frame_bytes<=0) break;
        if(info.frame_bytes>length-pos) goto fail;
        pos+=info.frame_bytes;
        if(!samples) continue;
        if(info.channels<1 || info.channels>2 || info.hz<8000 || info.hz>48000) goto fail;
        if(!rate) {rate=info.hz; channels=info.channels;}
        if(rate!=info.hz || channels!=info.channels) goto fail;
        bytes=samples*channels*2;
        if(used>COD_MP3_MAX_PCM-bytes) goto fail;
        if(used+bytes>capacity) {
            int next=capacity*2;
            if(next>COD_MP3_MAX_PCM) next=COD_MP3_MAX_PCM;
            grown=(unsigned char *)realloc(wav,next);
            if(!grown) goto fail;
            wav=grown; capacity=next;
        }
        for(i=0;i<samples*channels;i++) {
            unsigned int n=(unsigned short)frame[i];
            wav[used++]=(unsigned char)n; wav[used++]=(unsigned char)(n>>8);
        }
    }
    if(used==44) goto fail;
    memset(wav,0,44);
    memcpy(wav,"RIFF",4); CodMP3_U32(wav+4,used-8);
    memcpy(wav+8,"WAVEfmt ",8); CodMP3_U32(wav+16,16);
    wav[20]=1; wav[22]=(unsigned char)channels;
    CodMP3_U32(wav+24,rate); CodMP3_U32(wav+28,rate*channels*2);
    wav[32]=(unsigned char)(channels*2); wav[34]=16;
    memcpy(wav+36,"data",4); CodMP3_U32(wav+40,used-44);
    *wavBytes=used;
    return wav;
fail:
    free(wav); return NULL;
}
#endif
