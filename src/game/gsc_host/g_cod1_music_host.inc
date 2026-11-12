/* R107 diagnostic-only music producer. Callback success means the validated
 * alias command was broadcast; it does not acknowledge client playback. */
#include "../bg_cod1_music.h"
static int GscTestMusicCurrent(uint64_t id) {
    return id&&id==gscTestId&&(gscTestEntityMode==14||gscTestEntityMode==15||gscTestEntityMode==16||gscTestEntityMode==17||gscTestEntityMode==18||(gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))))&&
        G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&
        !level.restarted&&GscTestModuleReadCurrent();
}
static int GscTestMusicList(void *user,char *list,int capacity) {
    uint64_t id=*(const uint64_t *)user;int count;
    if(!GscTestMusicCurrent(id))return -1;
    count=trap_FS_GetFileList("soundaliases",".csv",list,capacity);
    return GscTestMusicCurrent(id)?count:-1;
}
static int GscTestMusicRead(void *user,const char *path,char **out,int *size) {
    uint64_t id=*(const uint64_t *)user;fileHandle_t file=0;int length;char *data;
    *out=NULL;*size=0;if(!GscTestMusicCurrent(id))return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    if(!file||length<=0||length>COD1_MUSIC_TABLE_BYTES||!GscTestMusicCurrent(id)){
        if(file)trap_FS_FCloseFile(file);return 0;}
    data=(char *)COD1_GSC_VFS_CALLOC((size_t)length+1,1);
    if(!data){trap_FS_FCloseFile(file);return 0;}
    trap_FS_Read(data,length,file);trap_FS_FCloseFile(file);
    if(!GscTestMusicCurrent(id)||memchr(data,0,(size_t)length)){
        COD1_GSC_VFS_FREE(data);return 0;}
    *out=data;*size=length;return 1;
}
static void GscTestMusicRelease(void *user,char *data){(void)user;COD1_GSC_VFS_FREE(data);}
static int GscTestMusicExists(void *user,const char *path) {
    uint64_t id=*(const uint64_t *)user;fileHandle_t file=0;int length;
    if(!GscTestMusicCurrent(id))return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);if(file)trap_FS_FCloseFile(file);
    return file&&length>0&&GscTestMusicCurrent(id);
}
static int GscTestMusicPlay(void *user,uint64_t id,int logicalMs,
                           const GscMusicEvent *event,char *error,size_t capacity) {
    cod1MusicFiles files;cod1MusicPlan plan;char map[128],command[256];int count;
    (void)user;
    if(!event||event->kind!=GSC_MUSIC_PLAY||event->transition_ms||logicalMs<0||
       !GscTestMusicCurrent(id)||!Cod1MusicAliasToken(event->alias))goto refuse;
    memset(&files,0,sizeof(files));files.user=&id;
    files.list=GscTestMusicList;files.read=GscTestMusicRead;
    files.release=GscTestMusicRelease;files.exists=GscTestMusicExists;
    memset(map,0,sizeof(map));trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
    if(!GscTestMusicCurrent(id)||!Cod1MusicResolve(&files,event->alias,map,&plan)||
       !GscTestMusicCurrent(id))goto refuse;
    count=snprintf(command,sizeof(command),"cod1_mu_play %s",event->alias);
    if(count<=0||count>=(int)sizeof(command)||!GscTestMusicCurrent(id))goto refuse;
    trap_SendServerCommand(-1,command);
    return GscTestMusicCurrent(id);
refuse:
    GscTestError(error,capacity,"music alias unsupported, unavailable or session changed");return 0;
}
static int GscTestMusicStop(void *user,uint64_t id,int logicalMs,
                           const GscMusicEvent *event,char *error,size_t capacity) {
    char command[64];int count;(void)user;
    if(!event||event->kind!=GSC_MUSIC_STOP||event->alias[0]||event->transition_ms<0||
       logicalMs<0||!GscTestMusicCurrent(id))goto refuse;
    count=snprintf(command,sizeof(command),"cod1_mu_stop %d",event->transition_ms);
    if(count<=0||count>=(int)sizeof(command)||!GscTestMusicCurrent(id))goto refuse;
    trap_SendServerCommand(-1,command);return GscTestMusicCurrent(id);
refuse:
    GscTestError(error,capacity,"music stop invalid or session changed");return 0;
}
