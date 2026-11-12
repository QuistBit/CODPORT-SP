/* R114 paired game/cgame transport for script earthquakes. This is a private
 * port command, not the original entity snapshot event or an FX namespace. */
#ifndef BG_COD1_SCRIPT_QUAKE_H
#define BG_COD1_SCRIPT_QUAKE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <float.h>

#define COD1_SCRIPT_QUAKE_SCHEMA 1
#define COD1_SCRIPT_QUAKE_COMMAND "c1q"
#define COD1_SCRIPT_QUAKE_METADATA "c1e"
#define COD1_SCRIPT_QUAKE_ARGC 10
#define COD1_SCRIPT_QUAKE_COMMAND_LENGTH 93
#define COD1_SCRIPT_QUAKE_COMMAND_CAPACITY 128
#define COD1_SCRIPT_QUAKE_METADATA_LENGTH 22
#define COD1_SCRIPT_QUAKE_METADATA_CAPACITY 32

typedef char cod1ScriptQuakeFloat32Check[(sizeof(float) == 4 &&
    FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128) ? 1 : -1];
typedef char cod1ScriptQuakeInt32Check[(sizeof(int) == 4) ? 1 : -1];

typedef struct {
    uint64_t epoch;
    uint64_t sequence;
    int duration_ms;
    float scale;
    float radius;
    float origin[3];
} cod1ScriptQuake_t;

#if defined(_MSC_VER)
#define CQ_INLINE __inline
#elif defined(__GNUC__)
#define CQ_INLINE __inline__
#else
#define CQ_INLINE inline
#endif

/* memcpy preserves all IEEE binary32 bits, including a signed zero. */
static CQ_INLINE uint32_t CQ_FloatBits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static CQ_INLINE float CQ_FloatFromBits(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static CQ_INLINE int CQ_Finite(float value) {
    return (CQ_FloatBits(value) & UINT32_C(0x7f800000)) != UINT32_C(0x7f800000);
}

static CQ_INLINE int CQ_Validate(const cod1ScriptQuake_t *quake) {
    return quake && quake->epoch && quake->sequence && quake->duration_ms >= 0 &&
        CQ_Finite(quake->scale) && CQ_Finite(quake->radius) && quake->radius > 0.f &&
        CQ_Finite(quake->origin[0]) && CQ_Finite(quake->origin[1]) &&
        CQ_Finite(quake->origin[2]);
}

static CQ_INLINE int CQ_IsActive(const cod1ScriptQuake_t *quake) {
    return CQ_Validate(quake) && quake->duration_ms > 0 && quake->scale > 0.f;
}

/* Caller owns the high-water mark and commits it only after admission. */
static CQ_INLINE int CQ_CommandFollows(const cod1ScriptQuake_t *quake,
    uint64_t epoch, uint64_t last_sequence) {
    return CQ_Validate(quake) && quake->epoch == epoch &&
        quake->sequence > last_sequence;
}

static CQ_INLINE int CQ_HexDigit(char digit) {
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    return -1;
}

static CQ_INLINE int CQ_ParseHex(const char *text, size_t digits, uint64_t *out) {
    uint64_t value = 0;
    size_t i;
    int digit;
    if (!text || !out || !digits || digits > 16) return 0;
    for (i = 0; i < digits; ++i) {
        digit = CQ_HexDigit(text[i]);
        if (digit < 0) return 0;
        value = (value << 4) | (uint64_t)digit;
    }
    if (text[digits]) return 0;
    *out = value;
    return 1;
}

static CQ_INLINE void CQ_WriteHex(char *out, size_t digits, uint64_t value) {
    static const char hex[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < digits; ++i) {
        out[digits - 1 - i] = hex[value & UINT64_C(15)];
        value >>= 4;
    }
}

static CQ_INLINE int CQ_EncodeCommand(char *out, size_t capacity,
    const cod1ScriptQuake_t *quake) {
    char encoded[COD1_SCRIPT_QUAKE_COMMAND_LENGTH + 1];
    uint32_t fields[6];
    size_t offset;
    int i;
    if (!out || capacity < sizeof(encoded) || !CQ_Validate(quake)) return 0;
    fields[0] = (uint32_t)quake->duration_ms;
    fields[1] = CQ_FloatBits(quake->scale);
    fields[2] = CQ_FloatBits(quake->radius);
    for (i = 0; i < 3; ++i) fields[3 + i] = CQ_FloatBits(quake->origin[i]);
    memcpy(encoded, "c1q 1 ", 6);
    CQ_WriteHex(encoded + 6, 16, quake->epoch);
    encoded[22] = ' ';
    CQ_WriteHex(encoded + 23, 16, quake->sequence);
    offset = 39;
    for (i = 0; i < 6; ++i) {
        encoded[offset++] = ' ';
        CQ_WriteHex(encoded + offset, 8, fields[i]);
        offset += 8;
    }
    encoded[offset] = '\0';
    memcpy(out, encoded, sizeof(encoded));
    return 1;
}

static CQ_INLINE int CQ_DecodeCommand(int argc, const char *const *argv,
    cod1ScriptQuake_t *out) {
    cod1ScriptQuake_t decoded;
    uint64_t fields[6];
    int i;
    if (!argv || !out || argc != COD1_SCRIPT_QUAKE_ARGC || !argv[0] ||
        !argv[1] || strcmp(argv[0], COD1_SCRIPT_QUAKE_COMMAND) ||
        strcmp(argv[1], "1")) return 0;
    if (!CQ_ParseHex(argv[2], 16, &decoded.epoch) ||
        !CQ_ParseHex(argv[3], 16, &decoded.sequence)) return 0;
    for (i = 0; i < 6; ++i)
        if (!CQ_ParseHex(argv[i + 4], 8, &fields[i])) return 0;
    if (fields[0] > UINT32_C(0x7fffffff)) return 0;
    decoded.duration_ms = (int)fields[0];
    decoded.scale = CQ_FloatFromBits((uint32_t)fields[1]);
    decoded.radius = CQ_FloatFromBits((uint32_t)fields[2]);
    for (i = 0; i < 3; ++i)
        decoded.origin[i] = CQ_FloatFromBits((uint32_t)fields[3 + i]);
    if (!CQ_Validate(&decoded)) return 0;
    *out = decoded;
    return 1;
}

/* Empty metadata is an inactive scene. The integration handles that reset;
 * this parser accepts only a complete active marker, never a fabricated epoch. */
static CQ_INLINE int CQ_EncodeMetadata(char *out, size_t capacity, uint64_t epoch) {
    char encoded[COD1_SCRIPT_QUAKE_METADATA_LENGTH + 1];
    if (!out || capacity < sizeof(encoded) || !epoch) return 0;
    memcpy(encoded, "c1e 1 ", 6);
    CQ_WriteHex(encoded + 6, 16, epoch);
    encoded[22] = '\0';
    memcpy(out, encoded, sizeof(encoded));
    return 1;
}

static CQ_INLINE int CQ_DecodeMetadata(const char *text, uint64_t *out) {
    uint64_t epoch;
    char hex[17];
    size_t i;
    if (!text || !out) return 0;
    for (i = 0; i <= COD1_SCRIPT_QUAKE_METADATA_LENGTH; ++i) {
        if (!text[i]) break;
    }
    if (i != COD1_SCRIPT_QUAKE_METADATA_LENGTH || text[i] ||
        memcmp(text, "c1e 1 ", 6)) return 0;
    memcpy(hex, text + 6, 16);
    hex[16] = '\0';
    if (!CQ_ParseHex(hex, 16, &epoch) || !epoch) return 0;
    *out = epoch;
    return 1;
}

#undef CQ_INLINE
#endif
