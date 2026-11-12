/* R115 private paired script FX transport. The original event snapshots and
 * renderer are separate boundaries; resource indices never mean shader/model
 * handles. Float fields retain exact IEEE binary32 bits. */
#ifndef BG_COD1_SCRIPT_FX_H
#define BG_COD1_SCRIPT_FX_H
#include "bg_cod1_script_quake.h"

#define COD1_SCRIPT_FX_SCHEMA 1
#define COD1_SCRIPT_FX_COMMAND "c1f"
#define COD1_SCRIPT_FX_ARGC 11
#define COD1_SCRIPT_FX_COMMAND_LENGTH 84
#define COD1_SCRIPT_FX_COMMAND_CAPACITY 128
#define COD1_SCRIPT_FX_METADATA_LENGTH 22
#define COD1_SCRIPT_FX_METADATA_CAPACITY 32
#define COD1_SCRIPT_FX_RESOURCE_CAPACITY 128
#define COD1_SCRIPT_FX_NAME_CAPACITY 96
#define COD1_SCRIPT_FX_NAME_LIMIT 90

typedef struct {
    uint64_t epoch, sequence;
    int index, directed;
    float origin[3];
    unsigned int direction_code;
    float direction_length;
} cod1ScriptFx_t;
typedef struct {
    uint64_t epoch;
    int index;
    char name[COD1_SCRIPT_FX_NAME_CAPACITY];
} cod1ScriptFxResource_t;

#if defined(_MSC_VER)
#define CF_INLINE __inline
#elif defined(__GNUC__)
#define CF_INLINE __inline__
#else
#define CF_INLINE inline
#endif

static CF_INLINE int CF_Validate(const cod1ScriptFx_t *event) {
    int i;
    if (!event || !event->epoch || !event->sequence ||
        event->index < 1 || event->index > 63 ||
        (event->directed != 0 && event->directed != 1)) return 0;
    for (i = 0; i < 3; ++i) if (!CQ_Finite(event->origin[i])) return 0;
    if (!event->directed)
        return event->direction_code == 0 &&
            CQ_FloatBits(event->direction_length) == UINT32_C(0);
    return event->direction_code < 162 &&
        CQ_Finite(event->direction_length) && event->direction_length > 0.f;
}
static CF_INLINE int CF_CommandFollows(const cod1ScriptFx_t *event,
    uint64_t epoch, uint64_t last_sequence) {
    return CF_Validate(event) && event->epoch == epoch &&
        event->sequence > last_sequence;
}
static CF_INLINE int CF_EncodeCommand(char *out, size_t capacity,
    const cod1ScriptFx_t *event) {
    char encoded[COD1_SCRIPT_FX_COMMAND_LENGTH + 1];
    const size_t widths[7] = {2,2,8,8,8,2,8};
    uint64_t fields[7];
    size_t offset;
    int i;
    if (!out || capacity < sizeof(encoded) || !CF_Validate(event)) return 0;
    fields[0] = (unsigned)event->index;
    fields[1] = (unsigned)event->directed;
    for (i = 0; i < 3; ++i) fields[2+i] = CQ_FloatBits(event->origin[i]);
    fields[5] = event->direction_code;
    fields[6] = CQ_FloatBits(event->direction_length);
    memcpy(encoded, "c1f 1 ", 6);
    CQ_WriteHex(encoded+6, 16, event->epoch);
    encoded[22] = ' ';
    CQ_WriteHex(encoded+23, 16, event->sequence);
    offset = 39;
    for (i = 0; i < 7; ++i) {
        encoded[offset++] = ' ';
        CQ_WriteHex(encoded+offset, widths[i], fields[i]);
        offset += widths[i];
    }
    encoded[offset] = 0;
    memcpy(out, encoded, sizeof(encoded));
    return 1;
}
static CF_INLINE int CF_DecodeCommand(int argc, const char *const *argv,
    cod1ScriptFx_t *out) {
    cod1ScriptFx_t event;
    const size_t widths[7] = {2,2,8,8,8,2,8};
    uint64_t fields[7];
    int i;
    if (!out || !argv || argc != COD1_SCRIPT_FX_ARGC || !argv[0] ||
        !argv[1] || strcmp(argv[0], "c1f") || strcmp(argv[1], "1")) return 0;
    memset(&event, 0, sizeof(event));
    if (!CQ_ParseHex(argv[2], 16, &event.epoch) ||
        !CQ_ParseHex(argv[3], 16, &event.sequence)) return 0;
    for (i = 0; i < 7; ++i)
        if (!CQ_ParseHex(argv[4+i], widths[i], &fields[i])) return 0;
    event.index = (int)fields[0]; event.directed = (int)fields[1];
    for (i = 0; i < 3; ++i)
        event.origin[i] = CQ_FloatFromBits((uint32_t)fields[2+i]);
    event.direction_code = (unsigned)fields[5];
    event.direction_length = CQ_FloatFromBits((uint32_t)fields[6]);
    if (!CF_Validate(&event)) return 0;
    *out = event;
    return 1;
}
static CF_INLINE int CF_EncodeMetadata(char *out, size_t capacity,
    uint64_t epoch) {
    char encoded[COD1_SCRIPT_FX_METADATA_LENGTH + 1];
    if (!out || capacity < sizeof(encoded) || !epoch) return 0;
    memcpy(encoded, "c1x 1 ", 6);
    CQ_WriteHex(encoded+6, 16, epoch);
    encoded[22] = 0;
    memcpy(out, encoded, sizeof(encoded));
    return 1;
}
static CF_INLINE int CF_DecodeMetadata(const char *text, uint64_t *out) {
    char hex[17];
    uint64_t epoch;
    size_t i;
    if (!text || !out) return 0;
    for (i = 0; i <= COD1_SCRIPT_FX_METADATA_LENGTH; ++i)
        if (!text[i]) break;
    if (i != COD1_SCRIPT_FX_METADATA_LENGTH || text[i] ||
        memcmp(text, "c1x 1 ", 6)) return 0;
    memcpy(hex, text+6, 16); hex[16] = 0;
    if (!CQ_ParseHex(hex, 16, &epoch) || !epoch) return 0;
    *out = epoch;
    return 1;
}
/* Explicit virtual-path subset. Raw accepted spelling remains the identity.
 * The client may canonicalize it to an fx/ VFS path ending in .efx for registration. */
static CF_INLINE int CF_ValidateName(const char *name) {
    char canonical[COD1_SCRIPT_FX_NAME_CAPACITY];
    size_t n, i, start, component, extension = 0, expanded;
    unsigned char c;
    if (!name) return 0;
    for (n = 0; n <= COD1_SCRIPT_FX_NAME_LIMIT; ++n)
        if (!name[n]) break;
    if (!n || n > COD1_SCRIPT_FX_NAME_LIMIT) return 0;
    for (i = 0; i < n; ++i) {
        c = (unsigned char)name[i];
        if (!((c>='a' && c<='z') || (c>='A' && c<='Z') ||
            (c>='0' && c<='9') || c=='_' || c=='-' || c=='.' ||
            c=='/' || c=='\\')) return 0;
        if (c>='A' && c<='Z') c = (unsigned char)(c-'A'+'a');
        canonical[i] = c=='\\' ? '/' : (char)c;
    }
    canonical[n] = 0;
    if (canonical[0]=='/' || canonical[n-1]=='/' ||
        strstr(canonical, "..") || strstr(canonical, "//") ||
        strstr(canonical, "/./")) return 0;
    start = 0;
    for (i = 0; i < n; ++i) if (canonical[i]=='/') start = i+1;
    component = n-start;
    if (!component) return 0;
    for (i = 0; i < n; ++i) if (canonical[i]=='.') {
        if (i < start || extension || n-i != 4 ||
            memcmp(canonical+i, ".efx", 4)) return 0;
        extension = 1;
    }
    if (extension && component == 4) return 0;
    expanded = n + (n>=3 && !memcmp(canonical,"fx/",3) ? 0 : 3) +
        (extension ? 0 : 4);
    return expanded < COD1_SCRIPT_FX_NAME_CAPACITY;
}
static CF_INLINE int CF_EncodeResource(char *out, size_t capacity,
    uint64_t epoch, int index, const char *name) {
    char encoded[COD1_SCRIPT_FX_RESOURCE_CAPACITY];
    size_t n;
    if (!out || !epoch || index<1 || index>63 || !CF_ValidateName(name)) return 0;
    n = strlen(name);
    if (capacity < 27+n) return 0;
    memcpy(encoded, "c1r 1 ", 6);
    CQ_WriteHex(encoded+6, 16, epoch);
    encoded[22] = ' ';
    CQ_WriteHex(encoded+23, 2, (unsigned)index);
    encoded[25] = ' ';
    memcpy(encoded+26, name, n+1);
    memcpy(out, encoded, n+27);
    return 1;
}
static CF_INLINE int CF_DecodeResource(const char *text,
    cod1ScriptFxResource_t *out) {
    cod1ScriptFxResource_t resource;
    char hex[17], index_hex[3];
    uint64_t index;
    size_t n;
    if (!text || !out) return 0;
    for (n = 0; n < COD1_SCRIPT_FX_RESOURCE_CAPACITY; ++n)
        if (!text[n]) break;
    if (n < 27 || n >= COD1_SCRIPT_FX_RESOURCE_CAPACITY ||
        memcmp(text, "c1r 1 ", 6) || text[22]!=' ' || text[25]!=' ' ||
        !CF_ValidateName(text+26)) return 0;
    memset(&resource,0,sizeof(resource));
    memcpy(hex,text+6,16); hex[16]=0;
    memcpy(index_hex,text+23,2); index_hex[2]=0;
    if (!CQ_ParseHex(hex,16,&resource.epoch) || !resource.epoch ||
        !CQ_ParseHex(index_hex,2,&index) || index<1 || index>63) return 0;
    resource.index = (int)index;
    memcpy(resource.name,text+26,n-26+1);
    *out = resource;
    return 1;
}
#undef CF_INLINE
#endif

