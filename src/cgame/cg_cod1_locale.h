#ifndef CG_COD1_LOCALE_H
#define CG_COD1_LOCALE_H

/* Included once by cgame.  The original StringEd resources live in PK3s;
 * loading them through trap_FS_* preserves the game's normal PK3 precedence.
 * GAME_*, CGAME_*, WEAPON_* and KEY_* are needed by the native in-game HUD.
 * Other menu packages are already handled by the UI module. */
#define COD1_LOCALE_MAX_ENTRIES 2048
#define COD1_LOCALE_MAX_FILE    131072
#define COD1_LOCALE_MAX_KEY     96
#define COD1_LOCALE_MAX_VALUE   1024
#define COD1_LOCALE_EXTRA_PACKAGES 16

typedef struct {
    char key[COD1_LOCALE_MAX_KEY];
    char value[COD1_LOCALE_MAX_VALUE];
} cod1LocaleEntry_t;

static cod1LocaleEntry_t cod1LocaleEntries[COD1_LOCALE_MAX_ENTRIES];
static int cod1LocaleCount;
static qboolean cod1LocaleLoaded;
static qboolean cod1LocaleKeyLoaded;
static qboolean cod1LocaleSubtitleLoaded;
static char cod1LocaleLanguage[32];
static char cod1LocaleExtraPackages[COD1_LOCALE_EXTRA_PACKAGES][32];
static int cod1LocaleExtraCount;

/* Also call at cgame/map initialization if the game is restarted in place. */
void CG_Cod1LocaleReset(void) {
    cod1LocaleCount = 0;
    cod1LocaleLoaded = qfalse;
    cod1LocaleKeyLoaded = qfalse;
    cod1LocaleSubtitleLoaded = qfalse;
    cod1LocaleLanguage[0] = 0;
    cod1LocaleExtraCount = 0;
}

static qboolean CG_Cod1LocaleIdentifier(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    if (!*p) return qfalse;
    while (*p) {
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
              (*p >= '0' && *p <= '9') || *p == '_')) return qfalse;
        ++p;
    }
    return qtrue;
}

static void CG_Cod1LocaleStore(const char *package, const char *reference,
                                const char *value) {
    char key[COD1_LOCALE_MAX_KEY];
    char decoded[COD1_LOCALE_MAX_VALUE];
    size_t length = strlen(package) + strlen(reference) + 2;
    const unsigned char *src = (const unsigned char *)value;
    size_t used = 0;
    int i;
    if (!CG_Cod1LocaleIdentifier(reference) || length > sizeof(key) ||
        !*value || !Q_stricmp(value, "#same")) return;
    /* COM_ParseExt intentionally returns literal backslash escapes. */
    while (*src && used + 1 < sizeof(decoded)) {
        unsigned char ch = *src++;
        if (ch == '\\' && (*src == 'n' || *src == 't' || *src == '\\')) {
            ch = *src == 'n' ? '\n' : (*src == 't' ? '\t' : '\\');
            ++src;
        }
        decoded[used++] = (char)ch;
    }
    if (*src) return; /* Never show a silently truncated translation. */
    decoded[used] = 0;
    snprintf(key, sizeof(key), "%s_%s", package, reference);
    for (i = 0; i < cod1LocaleCount; ++i)
        if (!Q_stricmp(key, cod1LocaleEntries[i].key)) break;
    if (i == cod1LocaleCount) {
        if (cod1LocaleCount == COD1_LOCALE_MAX_ENTRIES) return;
        ++cod1LocaleCount;
    }
    strcpy(cod1LocaleEntries[i].key, key);
    strcpy(cod1LocaleEntries[i].value, decoded);
}

static void CG_Cod1LocaleLoadFile(const char *package, const char *language) {
    char path[128], tag[48], reference[COD1_LOCALE_MAX_KEY], filename[32];
    char *file, *cursor, *token;
    fileHandle_t handle = 0;
    int len, i;
    if (strlen(package) >= sizeof(filename) || !CG_Cod1LocaleIdentifier(package)) return;
    for (i=0; package[i]; ++i) {
        char c=package[i];
        filename[i]=(c>='A' && c<='Z') ? c-'A'+'a' : c;
    }
    filename[i]=0;
    snprintf(path, sizeof(path), "localizedstrings/%s/%s.str", language, filename);
    len = trap_FS_FOpenFile(path, &handle, FS_READ);
    if (!handle) return;
    if (len <= 0 || len > COD1_LOCALE_MAX_FILE) {
        trap_FS_FCloseFile(handle);
        return;
    }
    file = malloc((size_t)len + 1);
    if (!file) {
        trap_FS_FCloseFile(handle);
        return;
    }
    trap_FS_Read(file, len, handle);
    trap_FS_FCloseFile(handle);
    file[len] = 0;
    snprintf(tag, sizeof(tag), "LANG_%s", language);
    reference[0] = 0;
    cursor = file;
    while (cursor && cursor < file + len) {
        token = COM_ParseExt(&cursor, qtrue);
        if (!token[0]) {
            /* FILENOTES "" is a valid token before mission references.
             * Empty quoted tokens do not mean EOF. COM_ParseExt can also
             * leave cursor just past the terminating NUL on a bad quote. */
            if (!cursor || cursor >= file + len) break;
            continue;
        }
        if (!Q_stricmp(token, "REFERENCE")) {
            token = COM_ParseExt(&cursor, qfalse);
            if (strlen(token) < sizeof(reference)) strcpy(reference, token);
            else reference[0] = 0;
        } else if (!Q_stricmp(token, tag)) {
            token = COM_ParseExt(&cursor, qfalse);
            if (reference[0]) CG_Cod1LocaleStore(package, reference, token);
        }
    }
    free(file);
}

static void CG_Cod1LocaleSelectedLanguage(char selected[32]) {
    size_t i;
    trap_Cvar_VariableStringBuffer("ui_cod1Language", selected,
                                    sizeof(cod1LocaleLanguage));
    for (i = 0; selected[i]; ++i) {
        if (!((selected[i] >= 'a' && selected[i] <= 'z') ||
              (selected[i] >= 'A' && selected[i] <= 'Z'))) {
            selected[0] = 0;
            break;
        }
        if (selected[i] >= 'A' && selected[i] <= 'Z') selected[i] += 'a' - 'A';
    }
}

static void CG_Cod1LocaleLoad(void) {
    char selected[sizeof(cod1LocaleLanguage)];
    CG_Cod1LocaleSelectedLanguage(selected);
    if (cod1LocaleLoaded && !strcmp(cod1LocaleLanguage, selected)) return;
    /* Changing languages in the options menu during a level rebuilds the
     * fixed cache on the next HUD lookup; no map restart is required. */
    cod1LocaleCount = 0;
    cod1LocaleLoaded = qtrue;
    cod1LocaleKeyLoaded = qfalse;
    cod1LocaleSubtitleLoaded = qfalse;
    cod1LocaleExtraCount = 0;
    strcpy(cod1LocaleLanguage, selected);
    CG_Cod1LocaleLoadFile("GAME", "english");
    CG_Cod1LocaleLoadFile("CGAME", "english");
    CG_Cod1LocaleLoadFile("WEAPON", "english");
    if (*selected && Q_stricmp(selected, "english")) {
        CG_Cod1LocaleLoadFile("GAME", selected);
        CG_Cod1LocaleLoadFile("CGAME", selected);
        CG_Cod1LocaleLoadFile("WEAPON", selected);
    }
}

/* Returns NULL for unknown/absent IDs so the caller can use its existing
 * display fallback.  The pointer remains valid until a language change or
 * CG_Cod1LocaleReset. */
const char *CG_Cod1LocaleLookup(const char *key) {
    int i;
    if (!key) return NULL;
    if (*key == '@') ++key;
    if (strlen(key) >= COD1_LOCALE_MAX_KEY ||
        (Q_stricmpn(key, "GAME_", 5) &&
         Q_stricmpn(key, "CGAME_", 6) &&
         Q_stricmpn(key, "WEAPON_", 7) &&
         Q_stricmpn(key, "KEY_", 4))) return NULL;
    CG_Cod1LocaleLoad();
    /* The original key.str contains UI key labels. Load its 110 entries
     * only when a HUD binding actually needs a translated key label. */
    if (!Q_stricmpn(key, "KEY_", 4) && !cod1LocaleKeyLoaded) {
        cod1LocaleKeyLoaded = qtrue;
        CG_Cod1LocaleLoadFile("KEY", "english");
        if (*cod1LocaleLanguage && Q_stricmp(cod1LocaleLanguage, "english"))
            CG_Cod1LocaleLoadFile("KEY", cod1LocaleLanguage);
    }
    for (i = 0; i < cod1LocaleCount; ++i)
        if (!Q_stricmp(key, cod1LocaleEntries[i].key))
            return cod1LocaleEntries[i].value;
    return NULL;
}

/* Objective scripts supply references such as AIRFIELD_OBJ_... and
 * RE_OBJ_... . Their StringEd package is the name of the .str resource,
 * which may contain an underscore. Probe prefix packages from longest to
 * shortest; remember missing packages so the VFS is not scanned per frame. */
const char *CG_Cod1LocaleLookupObjective(const char *key) {
    const char *translated;
    char package[32];
    size_t split, len;
    int i;
    if (!key) return NULL;
    if (*key=='@') ++key;
    len=strlen(key);
    if (len >= COD1_LOCALE_MAX_KEY || !CG_Cod1LocaleIdentifier(key)) return NULL;
    translated=CG_Cod1LocaleLookup(key);
    if (translated) return translated;
    CG_Cod1LocaleLoad();
    for (split=len; split>0; --split) {
        if (key[split]!='_' || split>=sizeof(package)) continue;
        memcpy(package,key,split);
        package[split]=0;
        for (i=0; i<cod1LocaleExtraCount; ++i)
            if (!Q_stricmp(package,cod1LocaleExtraPackages[i])) break;
        if (i==cod1LocaleExtraCount &&
            cod1LocaleExtraCount<COD1_LOCALE_EXTRA_PACKAGES) {
            strcpy(cod1LocaleExtraPackages[cod1LocaleExtraCount++],package);
            CG_Cod1LocaleLoadFile(package,"english");
            if (*cod1LocaleLanguage && Q_stricmp(cod1LocaleLanguage,"english"))
                CG_Cod1LocaleLoadFile(package,cod1LocaleLanguage);
        }
        for (i=0; i<cod1LocaleCount; ++i)
            if (!Q_stricmp(key,cod1LocaleEntries[i].key))
                return cod1LocaleEntries[i].value;
    }
    return NULL;
}

/* R230: original subtitle.str is 74997 bytes, 567 references, with
 * English values up to 469 bytes. Keep a dedicated lazy package lease so
 * subtitle lookup cannot consume the objective prefix-package budget.
 * Missing references return NULL, never the untranslated reference. */
const char *CG_Cod1LocaleLookupSubtitle(const char *key) {
    int i;
    if (!key) return NULL;
    if (*key == '@') ++key;
    if (strlen(key) >= COD1_LOCALE_MAX_KEY ||
        Q_stricmpn(key, "SUBTITLE_", 9) || !key[9] ||
        !CG_Cod1LocaleIdentifier(key)) return NULL;
    CG_Cod1LocaleLoad();
    if (!cod1LocaleSubtitleLoaded) {
        cod1LocaleSubtitleLoaded = qtrue;
        CG_Cod1LocaleLoadFile("SUBTITLE", "english");
        if (*cod1LocaleLanguage && Q_stricmp(cod1LocaleLanguage,"english"))
            CG_Cod1LocaleLoadFile("SUBTITLE",cod1LocaleLanguage);
    }
    for (i=0; i<cod1LocaleCount; ++i)
        if (!Q_stricmp(key,cod1LocaleEntries[i].key))
            return cod1LocaleEntries[i].value;
    return NULL;
}

#endif /* CG_COD1_LOCALE_H */
