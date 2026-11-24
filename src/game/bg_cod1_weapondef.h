#ifndef BG_COD1_WEAPONDEF_H
#define BG_COD1_WEAPONDEF_H
/* Portable metadata registry. Implementation lives in bg_pmove.c so the old
 * game and cgame projects compile the same parser without project-file edits.
 * This registry is separate from RTCW inventory slots. The current engine
 * loads the Thompson family; CW_AddFamily prepares independent families for
 * subsequent inventory/viewmodel adapters without enabling them implicitly. */
#define CW_FIELD_COUNT 248
#define CW_TEXT_SIZE 16384
#define CW_MAX_DEFS 63
typedef struct { const char *name; int type; } cwField_t;
typedef struct {
    char name[64], text[CW_TEXT_SIZE];
    const char *value[CW_FIELD_COUNT];
    double number[CW_FIELD_COUNT];
    int count, unknown, altIndex;
    unsigned int hash;
} cwDef_t;
/* Return byte count, -1 if missing, -2 if too large. No terminator required. */
typedef int (*cwRead_t)(const char *path, char *data, int capacity);
/* Loading invalidates previous resource receipts. The table stays unpublished
 * until all ordered definitions and alt links have been validated. */
typedef struct {void*user;int(*current)(void*);int(*list)(void*,char*,int);int(*read)(void*,const char*,char*,int);} cwTableHost_t;
int CW_LoadOriginalTable(const cwTableHost_t*host);
int CW_FieldIndex(const char *key);
const char *CW_String(const cwDef_t *def, const char *key);
double CW_Number(const cwDef_t *def, const char *key);
int CW_Parse(cwDef_t *def, const char *name, const char *data, int size);
int CW_LoadRegistry(cwRead_t readFile, const char *root);
/* Add one root and its alternate modes without replacing previous families.
 * Returns the root index, -2 for an absent root, or -1 for invalid data.
 * Failure leaves all previously loaded definitions and IDs intact. */
int CW_AddFamily(cwRead_t readFile, const char *root);
int CW_Find(const char *name);
int CW_FamilyRoot(int index);
void CW_ResetRegistry(void);
const cwDef_t *CW_Get(int index);
int CW_Count(void);
/* Nonzero monotonic resource-table revision, independent of inventory IDs.
 * Reset and attempted new-family mutation invalidate old receipts even when
 * loading fails or identical bytes are reloaded. It is unavailable during an
 * in-flight AddFamily, including nested reader callbacks. Exhaustion latches0
 * without changing parser/loading outcomes. A reader must return normally;
 * nonlocal unwinding conservatively leaves receipt availability disabled. */
unsigned int CW_RegistryRevision(void);
const char *CW_Error(void);
int BG_Cod1WeaponShotMsec(void);
int BG_Cod1WeaponDelayMsec(void);

#ifdef COD1_WEAPONDEF_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
static const cwField_t cwFields[CW_FIELD_COUNT] = {
#include "bg_cod1_weapon_fields.h"
};
static cwDef_t cwDefs[CW_MAX_DEFS];
static char cwInput[CW_TEXT_SIZE];
static int cwCount;
static unsigned int cwRegistryRevision=1;
static unsigned int cwRegistryMutationDepth;
static int cwTableBusy,cwTableCancelled;
static int cwFamilyRoot[CW_MAX_DEFS];
static char cwError[160];
static void CW_RegistryChanged(void) {
    if(cwRegistryRevision) {
        if(cwRegistryRevision==UINT_MAX)cwRegistryRevision=0;
        else ++cwRegistryRevision;
    }
}
unsigned int CW_RegistryRevision(void) { return cwRegistryMutationDepth?0:cwRegistryRevision; }
static int CW_Fail(const char *message) {
    size_t n = strlen(message);
    if (n >= sizeof(cwError)) n = sizeof(cwError)-1;
    memcpy(cwError, message, n); cwError[n] = 0; return -1;
}
const char *CW_Error(void) { return cwError; }
static int CW_NameValid(const char *s) {
    int n = 0;
    if (!s || !*s) return 0;
    while (*s) {
        char c = *s++;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
        if (++n >= 64) return 0;
    }
    return 1;
}
int CW_FieldIndex(const char *key) {
    int i;
    for (i=0;i<CW_FIELD_COUNT;i++) if (!strcmp(cwFields[i].name,key)) return i;
    return -1;
}
const char *CW_String(const cwDef_t *d,const char *key) {
    int i = CW_FieldIndex(key);
    return d && i>=0 && d->value[i] ? d->value[i] : "";
}
double CW_Number(const cwDef_t *d,const char *key) {
    int i = CW_FieldIndex(key);
    return d && i>=0 ? d->number[i] : 0;
}
/* Decimal grammar is checked separately: strtod must not accept NaN, Inf,
 * hexadecimal values, junk suffixes, or overflow from untrusted PK3 data. */
static int CW_Decimal(const char *s) {
    int digits=0;
    if (*s=='-' || *s=='+') s++;
    while (*s>='0' && *s<='9') { digits++; s++; }
    if (*s=='.') { s++; while (*s>='0' && *s<='9') { digits++; s++; } }
    if (!digits) return 0;
    if (*s=='e' || *s=='E') {
        s++; if (*s=='-' || *s=='+') s++;
        digits=0; while (*s>='0' && *s<='9') { digits++; s++; }
        if (!digits) return 0;
    }
    return !*s;
}
int CW_Parse(cwDef_t *d,const char *name,const char *data,int size) {
    char *key,*value,*next,*end;
    int i,type;
    double n;
    unsigned int hash=2166136261u;
    cwError[0]=0;
    if (!CW_NameValid(name)) return CW_Fail("invalid weapon resource name");
    if (!data || size<11 || size>=CW_TEXT_SIZE || memcmp(data,"WEAPONFILE\\",11))
        return CW_Fail("bad WEAPONFILE header or file size");
    for(i=0;i<size;i++) {
        if (!data[i]) return CW_Fail("embedded NUL in weapon file");
        hash=(hash^(unsigned char)data[i])*16777619u;
    }
    memset(d,0,sizeof(*d)); strcpy(d->name,name); d->altIndex=-1; d->hash=hash;
    memcpy(d->text,data,size); d->text[size]=0;
    /* Retail files may have a final CR/LF; it is not part of the last value. */
    while(size>11 && (d->text[size-1]=='\r' || d->text[size-1]=='\n')) d->text[--size]=0;
    key=d->text+11;
    while (*key) {
        value=strchr(key,'\\');
        if (!value || value==key) return CW_Fail("missing key/value pair");
        *value++=0; next=strchr(value,'\\'); if(next) *next++=0;
        i=CW_FieldIndex(key);
        if (i<0) d->unknown++;
        else {
            if(d->value[i]) return CW_Fail("duplicate weapon field");
            d->value[i]=value; d->count++; type=cwFields[i].type;
            if (*value && type>=4 && type<=7) {
                if(!CW_Decimal(value)) return CW_Fail(key);
                errno=0; n=strtod(value,&end);
                if(errno || *end || !(n>=-3.402823466e38 && n<=3.402823466e38)) return CW_Fail(key);
                if(type==7) {
                    /* Original CoD1 SP parser: seconds * 1000, truncation. */
                    n*=1000;
                    if(!(n>=INT_MIN && n<=INT_MAX)) return CW_Fail(key);
                    n=(int)n;
                } else if(type==4 || type==5) {
                    if(!(n>=INT_MIN && n<=INT_MAX) || n!=(int)n) return CW_Fail(key);
                    if(type==5 && n!=0 && n!=1) return CW_Fail(key);
                }
                d->number[i]=n;
            }
            /* Types 0 and 8..13 retain resource/enum strings. Enum semantic
             * mapping belongs to a future simulation implementation. */
        }
        if(!next) break;
        if(!*next) return CW_Fail("trailing separator without a key");
        key=next;
    }
    if(!d->count) return CW_Fail("empty weapon definition");
    return 1;
}
static void CW_LowerName(char *out,const char *in) {
    while(*in) { char c=*in++;*out++=(c>='A' && c<='Z')?c+32:c; } *out=0;
}
int CW_Find(const char *name) {
    int i; char lower[64],candidate[64];
    if(!CW_NameValid(name))return -1;CW_LowerName(lower,name);
    for(i=0;i<cwCount;i++){CW_LowerName(candidate,cwDefs[i].name);if(!strcmp(candidate,lower))return i;}
    return -1;
}
int CW_FamilyRoot(int index) {
    return index>=0 && index<cwCount ? cwFamilyRoot[index] : -1;
}
void CW_ResetRegistry(void) { if(cwTableBusy)cwTableCancelled=1;CW_RegistryChanged();cwCount=0; cwError[0]=0; }
static int CW_AddFamilyBody(cwRead_t readFile,const char *root) {
    char path[80];
    const char *name;
    int cursor,size,index,start=cwCount;
    cwError[0]=0;
    if(!readFile)return CW_Fail("missing weapon reader");
    if(!CW_NameValid(root)) return CW_Fail("invalid root weapon name");
    index=CW_Find(root);
    if(index>=0) {
        if(cwFamilyRoot[index]!=index)return CW_Fail("root already belongs to another family");
        return index;
    }
    if(cwCount==CW_MAX_DEFS)return CW_Fail("weapon metadata registry full");
    CW_RegistryChanged();
    CW_LowerName(cwDefs[start].name,root);cwFamilyRoot[start]=start;cwCount++;
    /* Insert names before following altWeapon: A -> B -> A loads twice,
     * without recursion or duplicate IDs. No mesh/animation preloading. */
    for(cursor=start;cursor<cwCount;cursor++) {
        char savedName[64];
        strcpy(savedName,cwDefs[cursor].name);
        strcpy(path,"weapons/sp/"); strcat(path,savedName);
        size=readFile(path,cwInput,sizeof(cwInput)-1);
        if(size==-1 && cursor==start) { cwCount=start; return -2; }
        if(size<0) { cwCount=start; return CW_Fail(path); }
        if(CW_Parse(&cwDefs[cursor],savedName,cwInput,size)<0) { cwCount=start; return -1; }
        name=CW_String(&cwDefs[cursor],"altWeapon");
        if(!*name) continue;
        if(!CW_NameValid(name)) { cwCount=start; return CW_Fail("unsafe altWeapon name"); }
        index=CW_Find(name);
        if(index<0) {
            if(cwCount==CW_MAX_DEFS) { cwCount=start; return CW_Fail("weapon metadata registry full"); }
            index=cwCount++; CW_LowerName(cwDefs[index].name,name);cwFamilyRoot[index]=start;
        } else if(cwFamilyRoot[index]!=start) {
            cwCount=start;return CW_Fail("altWeapon crosses weapon families");
        }
        cwDefs[cursor].altIndex=index;
    }
    return start;
}
int CW_AddFamily(cwRead_t readFile,const char *root) {
    int result,counted=cwRegistryMutationDepth<UINT_MAX;
    if(cwTableBusy){cwTableCancelled=1;return CW_Fail("reentrant weapon table mutation");}
    if(counted)++cwRegistryMutationDepth;
    else cwRegistryRevision=0;
    result=CW_AddFamilyBody(readFile,root);
    if(counted)--cwRegistryMutationDepth;
    return result;
}
int CW_LoadRegistry(cwRead_t readFile,const char *root) {
    int result;
    if(cwTableBusy){cwTableCancelled=1;return CW_Fail("reentrant weapon table mutation");}
    CW_ResetRegistry();result=CW_AddFamily(readFile,root);
    return result>=0 ? 1 : (result==-2 ? 0 : -1);
}
#include "bg_cod1_weapon_table14.inc"
const cwDef_t *CW_Get(int index) { return index>=0 && index<cwCount ? &cwDefs[index] : NULL; }
int CW_Count(void) { return cwCount; }
int BG_Cod1WeaponShotMsec(void) {
    const cwDef_t*d=CW_Get(CW_Find("thompson"));return d ? (int)CW_Number(d,"fireTime") : 86;
}
int BG_Cod1WeaponDelayMsec(void) {
    const cwDef_t*d=CW_Get(CW_Find("thompson"));return d ? (int)CW_Number(d,"fireDelay") : 0;
}
#endif
#endif
