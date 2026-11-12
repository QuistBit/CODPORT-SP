#include "cod1_mp3.h"
/* Reuse a generated PCM cache only when it has a complete expected WAV header.
 * The key includes source bytes, so a localized/replaced PK3 invalidates it. */
static qboolean S_Cod1MusicPath(const char *source, char *result, int resultSize) {
    void *input=NULL;
    unsigned char *wav, header[44];
    int length, size, existing;
    unsigned int checksum;
    fileHandle_t file=0;
    Q_strncpyz(result,source,resultSize);
    length=(int)strlen(source);
    if(length<4 || Q_stricmp(source+length-4,".mp3")) return qtrue;
    length=FS_ReadFile(source,NULL);
    if(length<=0 || length>COD_MP3_MAX_INPUT) return qfalse;
    if(FS_ReadFile(source,&input)!=length || !input) {
        if(input) FS_FreeFile(input);
        return qfalse;
    }
    checksum=Com_BlockChecksum(input,length);
    Com_sprintf(result,resultSize,"cod1cache/mp3_v1_%08x_%x.wav",checksum,length);
    existing=FS_FOpenFileRead(result,&file,qtrue);
    if(file) {
        int got=FS_Read(header,44,file);
        FS_FCloseFile(file);
        if(got==44 && existing>44 && existing<=COD_MP3_MAX_PCM &&
            !memcmp(header,"RIFF",4) && !memcmp(header+8,"WAVEfmt ",8) &&
            !memcmp(header+36,"data",4) && header[20]==1 && header[34]==16 &&
            (unsigned int)(existing-44)==((unsigned int)header[40] |
              ((unsigned int)header[41]<<8) | ((unsigned int)header[42]<<16) |
              ((unsigned int)header[43]<<24))) {
            FS_FreeFile(input); return qtrue;
        }
    }
    wav=CodMP3_Decode((const unsigned char *)input,length,&size);
    FS_FreeFile(input);
    if(!wav) {Com_Printf("CoD1 MP3: cannot decode %s\n",source); return qfalse;}
    file=FS_FOpenFileWrite(result);
    if(!file) {free(wav); return qfalse;}
    existing=FS_Write(wav,size,file);
    FS_FCloseFile(file); free(wav);
    if(existing!=size) return qfalse;
    Com_Printf("CoD1 MP3: %s decoded to PCM (%d bytes)\n",source,size);
    return qtrue;
}
