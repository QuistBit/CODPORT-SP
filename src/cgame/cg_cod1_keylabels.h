/* ASCII key labels for CoD1 HUD hints. Keep these names in step with the
 * English keynames table in src/client/cl_keys.c: cgame can query the bound
 * key number, but this port has no cgame syscall to ask the client for its
 * translated key name. A bound, unnamed key is shown as 0xNN (the client's
 * own Key_KeynumToString fallback), never as the unrelated word "USE". */
#ifndef CG_COD1_KEYLABELS_H
#define CG_COD1_KEYLABELS_H

#include <string.h>

/* Requires keycodes.h and Com_sprintf via the including cgame translation
 * unit. `buffer` is always NUL terminated on success; NULL means unbound,
 * invalid, or insufficient buffer space. Callers choose their own localized
 * KEY_UNBOUND fallback. */
static const char *CG_Cod1KeyLabel(int key, char *buffer, int bufferSize) {
    const char *name=NULL;
    static const char hex[]="0123456789abcdef";
    char temporary[24];
    size_t length;

    if (!buffer || bufferSize<=0) return NULL;
    buffer[0]=0;
    if (key<0 || key>255) return NULL;

    if (key>32 && key<127 && key!='"') {
        temporary[0]=(char)(key>='a' && key<='z' ? key-'a'+'A' : key);
        temporary[1]=0;
        name=temporary;
    } else if (key>=K_F1 && key<=K_F12) {
        Com_sprintf(temporary,sizeof(temporary),"F%d",key-K_F1+1);
        name=temporary;
    } else if (key>=K_MOUSE1 && key<=K_MOUSE5) {
        Com_sprintf(temporary,sizeof(temporary),"MOUSE%d",key-K_MOUSE1+1);
        name=temporary;
    } else if (key>=K_JOY1 && key<=K_JOY32) {
        Com_sprintf(temporary,sizeof(temporary),"JOY%d",key-K_JOY1+1);
        name=temporary;
    } else if (key>=K_AUX1 && key<=K_AUX16) {
        Com_sprintf(temporary,sizeof(temporary),"AUX%d",key-K_AUX1+1);
        name=temporary;
    } else {
        switch (key) {
        case K_TAB: name="TAB"; break;
        case K_ENTER: name="ENTER"; break;
        case K_ESCAPE: name="ESCAPE"; break;
        case K_SPACE: name="SPACE"; break;
        case K_BACKSPACE: name="BACKSPACE"; break;
        case K_COMMAND: name="COMMAND"; break;
        case K_CAPSLOCK: name="CAPSLOCK"; break;
        case K_PAUSE: name="PAUSE"; break;
        case K_UPARROW: name="UPARROW"; break;
        case K_DOWNARROW: name="DOWNARROW"; break;
        case K_LEFTARROW: name="LEFTARROW"; break;
        case K_RIGHTARROW: name="RIGHTARROW"; break;
        case K_ALT: name="ALT"; break;
        case K_CTRL: name="CTRL"; break;
        case K_SHIFT: name="SHIFT"; break;
        case K_INS: name="INS"; break;
        case K_DEL: name="DEL"; break;
        case K_PGDN: name="PGDN"; break;
        case K_PGUP: name="PGUP"; break;
        case K_HOME: name="HOME"; break;
        case K_END: name="END"; break;
        case K_KP_HOME: name="KP_HOME"; break;
        case K_KP_UPARROW: name="KP_UPARROW"; break;
        case K_KP_PGUP: name="KP_PGUP"; break;
        case K_KP_LEFTARROW: name="KP_LEFTARROW"; break;
        case K_KP_5: name="KP_5"; break;
        case K_KP_RIGHTARROW: name="KP_RIGHTARROW"; break;
        case K_KP_END: name="KP_END"; break;
        case K_KP_DOWNARROW: name="KP_DOWNARROW"; break;
        case K_KP_PGDN: name="KP_PGDN"; break;
        case K_KP_ENTER: name="KP_ENTER"; break;
        case K_KP_INS: name="KP_INS"; break;
        case K_KP_DEL: name="KP_DEL"; break;
        case K_KP_SLASH: name="KP_SLASH"; break;
        case K_KP_MINUS: name="KP_MINUS"; break;
        case K_KP_PLUS: name="KP_PLUS"; break;
        case K_KP_NUMLOCK: name="KP_NUMLOCK"; break;
        case K_KP_STAR: name="KP_STAR"; break;
        case K_KP_EQUALS: name="KP_EQUALS"; break;
        case K_MWHEELUP: name="MWHEELUP"; break;
        case K_MWHEELDOWN: name="MWHEELDOWN"; break;
        default:
            temporary[0]='0'; temporary[1]='x';
            temporary[2]=hex[(key>>4)&15];
            temporary[3]=hex[key&15];
            temporary[4]=0;
            name=temporary;
            break;
        }
    }
    length=strlen(name);
    if (length>=(size_t)bufferSize) return NULL;
    memcpy(buffer,name,length+1);
    return buffer;
}

#endif /* CG_COD1_KEYLABELS_H */
