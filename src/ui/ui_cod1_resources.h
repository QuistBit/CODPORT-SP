/* CoD menu resources through the existing PK3 filesystem. Included by ui_main.c. */
#define COD_UI_STRINGS 4096
static struct { char key[128]; const char *value; } codStrings[COD_UI_STRINGS];
static int codStringCount, codPoolUsed;
static char codPool[524288];
static char codLoaded[64][64];
static int codLoadedCount;
static char codLanguage[32];
static struct { char name[96], path[256]; } codSounds[2048];
static int codSoundCount;

static char *CoD_UIRead(const char *path) {
    fileHandle_t f;
    int len = trap_FS_FOpenFile(path, &f, FS_READ);
    char *data;
    if (!f) return NULL;
    if (len <= 0 || len > 1048576) { trap_FS_FCloseFile(f); return NULL; }
    data = malloc(len + 1);
    if (data) { trap_FS_Read(data, len, f); data[len] = 0; }
    trap_FS_FCloseFile(f);
    return data;
}

static void CoD_UIStore(const char *key, const char *value) {
    int i, n = strlen(value) + 1;
    if (codPoolUsed + n > sizeof(codPool)) return;
    for (i=0; i<codStringCount; i++) if (!Q_stricmp(key,codStrings[i].key)) break;
    if (i==COD_UI_STRINGS) return;
    if (i==codStringCount) codStringCount++;
    Q_strncpyz(codStrings[i].key,key,sizeof(codStrings[i].key));
    memcpy(codPool+codPoolUsed,value,n);
    codStrings[i].value=codPool+codPoolUsed; codPoolUsed+=n;
}

static void CoD_UILoadStrings(const char *package, const char *language) {
    char path[256], reference[96], key[128], lang[64], tokenCopy[MAX_TOKEN_CHARS];
    char *data, *cursor, *token;
    Com_sprintf(path,sizeof(path),"localizedstrings/%s/%s.str",language,package);
    data=CoD_UIRead(path); if (!data) return;
    cursor=data; reference[0]=0;
    Com_sprintf(lang,sizeof(lang),"LANG_%s",language);
    while (cursor) {
        token=COM_ParseExt(&cursor,qtrue);
        /* Empty quoted metadata is valid. Only a null cursor denotes EOF. */
        if (!cursor) break;
        if (!token[0]) continue;
        if (!Q_stricmp(token,"REFERENCE")) {
            Q_strncpyz(reference,COM_ParseExt(&cursor,qfalse),sizeof(reference));
        } else if (!Q_stricmp(token,lang)) {
            Q_strncpyz(tokenCopy,COM_ParseExt(&cursor,qfalse),sizeof(tokenCopy));
            if (reference[0] && tokenCopy[0] && Q_stricmp(tokenCopy,"#same")) {
                Com_sprintf(key,sizeof(key),"%s_%s",package,reference);
                CoD_UIStore(key,tokenCopy);
            }
        }
    }
    free(data);
}

static const char *CoD_UITranslate(const char *text) {
    char package[64]; const char *sep; int i,n;
    if (!text || text[0]!='@') return text;
    sep=strchr(text+1,'_'); if (!sep) return text;
    n=sep-text-1; if (n<=0 || n>=sizeof(package)) return text;
    memcpy(package,text+1,n); package[n]=0; Q_strlwr(package);
    for (i=0;package[i];i++) if (!((package[i]>='a' && package[i]<='z') ||
        (package[i]>='0' && package[i]<='9'))) return text;
    for (i=0;i<codLoadedCount;i++) if (!strcmp(package,codLoaded[i])) break;
    if (i==codLoadedCount && codLoadedCount<64) {
        Q_strncpyz(codLoaded[codLoadedCount++],package,64);
        CoD_UILoadStrings(package,"english");
        if (codLanguage[0] && Q_stricmp(codLanguage,"english")) CoD_UILoadStrings(package,codLanguage);
    }
    for (i=0;i<codStringCount;i++) if (!Q_stricmp(text+1,codStrings[i].key)) return codStrings[i].value;
    return text;
}

/* CSV fields may be quoted, contain commas, or escape quotes as doubled quotes. */
static char *CoD_UICell(char *p,char *out,int size) {
    int quoted=0,n=0;
    if (*p=='"') { quoted=1; p++; }
    while (*p && *p!='\r' && *p!='\n') {
        if (quoted && *p=='"') {
            if (p[1]=='"') p++; else { quoted=0; p++; continue; }
        } else if (!quoted && *p==',') { p++; break; }
        if (n<size-1) out[n++]=*p;
        p++;
    }
    out[n]=0; return p;
}

static void CoD_UILoadAliases(const char *path) {
    char *data=CoD_UIRead(path), *line,*next,*p;
    char cell[256],name[96],file[256]; int nc=-1,fc=-1,col;
    if (!data) return;
    for (line=data; *line; line=next) {
        next=strchr(line,'\n'); if(next) *next++=0; else next=line+strlen(line);
        p=line; col=0; name[0]=file[0]=0;
        if (*p=='#' || *p==',' || *p=='\r') continue;
        if (nc<0) {
            while (*p && *p!='\r') {
                p=CoD_UICell(p,cell,sizeof(cell));
                if (!Q_stricmp(cell,"name")) nc=col;
                if (!Q_stricmp(cell,"file")) fc=col;
                col++;
            }
            if(fc<0) nc=-1;
            continue;
        }
        while (*p && *p!='\r') {
            p=CoD_UICell(p,cell,sizeof(cell));
            if(col==nc) Q_strncpyz(name,cell,sizeof(name));
            if(col==fc) Q_strncpyz(file,cell,sizeof(file));
            col++;
        }
        if(name[0] && name[0]!='#' && file[0] && codSoundCount<2048) {
            Q_strncpyz(codSounds[codSoundCount].name,name,96);
            Com_sprintf(codSounds[codSoundCount].path,256,"%s%s",
                Q_strncmp(file,"sound/",6)?"sound/":"",file);
            codSoundCount++;
        }
    }
    free(data);
}

static const char *CoD_UISoundPath(const char *name) {
    int i;
    if (!name) return "";
    for(i=0;i<codSoundCount;i++) if(!Q_stricmp(name,codSounds[i].name)) return codSounds[i].path;
    return name;
}
static sfxHandle_t CoD_UIRegisterSound(const char *name) {
    return trap_S_RegisterSound(CoD_UISoundPath(name));
}
static void CoD_UIBackgroundTrack(const char *intro,const char *loop,int fade) {
    uiClientState_t state;
    char current[MAX_QPATH];
    trap_GetClientState(&state);
    /* Menu scripts also run when opening pause/settings during a level. */
    if (state.connState > CA_DISCONNECTED || trap_Cvar_VariableValue("sv_running")) return;
    trap_Cvar_VariableStringBuffer("s_currentMusic",current,sizeof(current));
    if (current[0]) return; /* returning from Options must not restart the song */
    trap_S_StartBackgroundTrack(CoD_UISoundPath(intro),CoD_UISoundPath(loop),fade);
}
static void CoD_UIResourcesInit(void) {
    int i;
    codStringCount=codPoolUsed=codLoadedCount=codSoundCount=0;
    trap_Cvar_VariableStringBuffer("ui_cod1Language",codLanguage,sizeof(codLanguage));
    /* Explicit folder name avoids guessing language indices across editions. */
    for(i=0;codLanguage[i];i++) if(!((codLanguage[i]>='a' && codLanguage[i]<='z') ||
        (codLanguage[i]>='A' && codLanguage[i]<='Z'))) { codLanguage[0]=0; break; }
    Q_strlwr(codLanguage);
    CoD_UILoadAliases("soundaliases/iw_sound.csv");
    CoD_UILoadAliases("soundaliases/iw_music.csv");
}
