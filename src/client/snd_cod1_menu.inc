/* Existing CoD sound-menu CVARs. The menu owns staged ui_mss_* values;
 * only its Apply command copies them to mss_* and calls snd_restart.
 * Initialization must never read staged values: Cancel has no side effects. */
static cvar_t *s_cod1Provider;

static void S_Cod1SoundMenuInit(void) {
    cvar_t *quality = Cvar_Get("mss_khz", s_khz->string, CVAR_ARCHIVE);
    s_cod1Provider = Cvar_Get("mss_3d_provider",
        "Miles Fast 2D Positional Audio", CVAR_ARCHIVE);
    if (!Q_stricmp(quality->string, "11") ||
        !Q_stricmp(quality->string, "22") ||
        !Q_stricmp(quality->string, "44")) {
        Cvar_Set("s_khz", quality->string);
    } else {
        Com_Printf("Sound: unsupported mss_khz '%s'; retaining s_khz=%s\n",
            quality->string, s_khz->string);
    }
}

static int S_Cod1SoundMenuUseOpenAL(void) {
    if (!Q_stricmp(s_cod1Provider->string, "Creative Labs EAX 3 (TM)"))
        return 1;
    if (Q_stricmp(s_cod1Provider->string, "Miles Fast 2D Positional Audio"))
        Com_Printf("Sound: provider '%s' is not implemented; using base mixer. "
            "Select EAX 3 for OpenAL/EAL audio.\n", s_cod1Provider->string);
    return 0;
}
