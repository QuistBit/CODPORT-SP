/* Separate CoD transport: legacy RTCW mu_* keeps its previous semantics. */
#include "../game/bg_cod1_music.h"
static int CG_Cod1MusicList(void *user,char *list,int capacity){
    (void)user;return trap_Cod1FS_GetFileList("soundaliases",".csv",list,capacity);
}
static int CG_Cod1MusicRead(void *user,const char *path,char **out,int *size){
    fileHandle_t file=0;int length;char *data;(void)user;*out=NULL;*size=0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    if(!file||length<=0||length>COD1_MUSIC_TABLE_BYTES){if(file)trap_FS_FCloseFile(file);return 0;}
    data=(char *)calloc((size_t)length+1,1);if(!data){trap_FS_FCloseFile(file);return 0;}
    trap_FS_Read(data,length,file);trap_FS_FCloseFile(file);
    if(memchr(data,0,(size_t)length)){free(data);return 0;}
    *out=data;*size=length;return 1;
}
static void CG_Cod1MusicRelease(void *user,char *data){(void)user;free(data);}
static int CG_Cod1MusicExists(void *user,const char *path){
    fileHandle_t file=0;int length;(void)user;
    length=trap_FS_FOpenFile(path,&file,FS_READ);if(file)trap_FS_FCloseFile(file);
    return file&&length>0;
}
/* The datestamp cue belongs to the first visible intro frame. The
 * original GSC requests it on the same tick as the black HUD element. */
static cod1MusicPlayback cgIntroMusic;
static int cgIntroMusicPending;
static void CG_Cod1IntroMusicFrame(void){
    if(!cgIntroMusicPending || cg_norender.integer ||
       (trap_Key_GetCatcher()&KEYCATCH_UI))return;
    cgIntroMusicPending=0;
    if(!trap_Cod1ScriptMusic(&cgIntroMusic))CG_Printf("CoD1 intro music playback refused\n");
    else if(cg_debugAnim.integer)CG_Printf("[Training intro] datestamp on first visible black frame t=%d\n",cg.time);
}
static void CG_Cod1MusicCommand(int kind){
    cod1MusicPlayback playback;cod1MusicFiles files;char value[COD1_MUSIC_ALIAS_BYTES+1];
    if(trap_Argc()!=2)return;
    memset(value,0,sizeof(value));trap_Argv(1,value,sizeof(value));
    if(strlen(value)>=COD1_MUSIC_ALIAS_BYTES)return;
    memset(&playback,0,sizeof(playback));playback.kind=kind;
    if(kind==COD1_MUSIC_PLAY){
        if(!Cod1MusicAliasToken(value))return;
        memset(&files,0,sizeof(files));files.list=CG_Cod1MusicList;files.read=CG_Cod1MusicRead;
        files.release=CG_Cod1MusicRelease;files.exists=CG_Cod1MusicExists;
        if(!Cod1MusicResolve(&files,value,cgs.mapname,&playback.plan)){
            CG_Printf("CoD1 music: alias unavailable or unsupported: %s\n",value);return;}
    }else if(kind==COD1_MUSIC_STOP){
        if(!Cod1MusicMilliseconds(value,&playback.transitionMs))return;
    }else return;
    cgIntroMusicPending=0;
    if(kind==COD1_MUSIC_PLAY && !Q_stricmp(cgs.mapname,"maps/training.bsp") &&
       !Q_stricmp(value,"training_datestamp")){
        cgIntroMusic=playback;cgIntroMusicPending=1;return;
    }
    if(!trap_Cod1ScriptMusic(&playback))CG_Printf("CoD1 music: sound engine refused command\n");
}
