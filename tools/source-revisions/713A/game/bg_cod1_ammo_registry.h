/* R207 genuine CW metadata -> explicit bounded native-shaped ammo domains.
 * Original gamex86.dll2000a0c0/a220/a4b0/a680 defines name/pool ordering.
 * CW IDs remain zero-based. CAP ordinals below are selected i+1, never WP
 * enum values or a claim of retail registry order. Native2000aba1..abfc
 * reserves ammo/clip0="none",limit0; those rows are retained explicitly.
 * Shared-cap0 is an ordinary first-seen group. No player inventory,
 * grant, item publication, handler, VM admission or gameplay caller. */
#ifndef BG_COD1_AMMO_REGISTRY_H
#define BG_COD1_AMMO_REGISTRY_H
#include "bg_cod1_item_precache.h"
#include "bg_cod1_ammo_pickup.h"
#include <stdint.h>
#include <string.h>
#include <limits.h>

#define COD1_AMMO_REGISTRY_VERSION 1
#define COD1_AMMO_REGISTRY_BINDINGS 2
typedef struct { char name[64]; int32_t limit; } cod1AmmoRegistryPool_t;
typedef struct {
    int cwIndex, familyRoot, altIndex;
    cod1AmmoPickupWeapon_t pickup;
    uint32_t definitionHash;
    char name[64];
} cod1AmmoRegistryWeapon_t;
typedef struct {
    int itemIndex, weaponIndex, cwIndex, ammoIndex, clipIndex;
    int sharedCapIndex, clipOnly, quantity;
} cod1AmmoRegistryBinding_t;
typedef struct {
    uint32_t version, bytes, registryRevision, descriptorFingerprint, metadataFingerprint;
    int weaponCount, ammoCount, clipCount, sharedCapCount;
    cod1AmmoRegistryWeapon_t weapons[CW_MAX_DEFS+1];
    cod1AmmoRegistryPool_t ammo[COD1_AMMO_PICKUP_CELLS];
    cod1AmmoRegistryPool_t clip[COD1_AMMO_PICKUP_CELLS];
    cod1AmmoRegistryPool_t sharedCap[COD1_AMMO_PICKUP_CELLS];
    cod1AmmoRegistryBinding_t bindings[COD1_AMMO_REGISTRY_BINDINGS];
} cod1AmmoRegistryReceipt_t;

typedef struct {
    int valid,count;
    uint32_t revision;
    const cwDef_t *owners[CW_MAX_DEFS];
    int roots[CW_MAX_DEFS];
    cwDef_t definitions[CW_MAX_DEFS];
    cod1AmmoRegistryReceipt_t receipt;
} cod1AmmoRegistryCache_t;
static cod1AmmoRegistryCache_t carCurrentCache;
static int CAR_Overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!a||!b||!na||!nb)return 0;
    if(na>UINTPTR_MAX-x||nb>UINTPTR_MAX-y)return 1;
    return x<y+nb&&y<x+na;
}
static int CAR_StorageProtected(const void *p,size_t n) {
    return CAR_Overlap(p,n,&carCurrentCache,sizeof(carCurrentCache));
}
/* MSVC x86's variable-size memcmp uses a byte loop here. The native build
 * already targets SSE2. Compare the same exact bytes in bounded 64-byte
 * blocks, with a portable fallback; never read beyond either object. */
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP>=2)
#include <emmintrin.h>
#endif
static int CAR_BytesEqual(const void *left,const void *right,size_t size) {
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP>=2)
    const unsigned char *a=(const unsigned char *)left,*b=(const unsigned char *)right;size_t i;
    for(i=0;i+64<=size;i+=64) {
        __m128i x=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+i)),_mm_loadu_si128((const __m128i *)(b+i)));
        __m128i y=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+i+16)),_mm_loadu_si128((const __m128i *)(b+i+16)));
        __m128i z=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+i+32)),_mm_loadu_si128((const __m128i *)(b+i+32)));
        __m128i v=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+i+48)),_mm_loadu_si128((const __m128i *)(b+i+48)));
        if(_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_or_si128(_mm_or_si128(x,y),_mm_or_si128(z,v)),_mm_setzero_si128()))!=65535)return 0;
    }
    return !memcmp(a+i,b+i,size-i);
#else
    return !memcmp(left,right,size);
#endif
}
static int CAR_Output(const void *out,size_t bytes,int count) {
    int i;const cwDef_t *d;
    if(!out||count<0||count>CW_MAX_DEFS||CAR_StorageProtected(out,bytes))return 0;
    for(i=0;i<count;++i) {
        d=CW_Get(i);if(!d||CAR_Overlap(out,bytes,d,sizeof(*d)))return 0;
    }
    return 1;
}
static unsigned char CAR_Lower(unsigned char c) {
    return c>='A'&&c<='Z'?(unsigned char)(c+('a'-'A')):c;
}
/* Native binding compares strlen(internal token) bytes, case-insensitively;
 * it does not require a word separator or use displayName/radiantName. */
static int CAR_Prefix(const char *name,const char *pickup) {
    size_t i;if(!name||!pickup||!*name)return 0;
    for(i=0;name[i];++i)if(CAR_Lower((unsigned char)name[i])!=CAR_Lower((unsigned char)pickup[i]))return 0;
    return 1;
}
static int CAR_Name(char out[64],const char *in) {
    size_t i;if(!in)return 0;
    for(i=0;i<64;++i) {
        unsigned char c=(unsigned char)in[i];
        if(!c){out[i]=0;return 1;}
        if(c<32||c>126)return 0;
        out[i]=(char)CAR_Lower(c);
    }
    return 0;
}
static int CAR_Integer(const cwDef_t *d,const char *field,int32_t *out) {
    double v=CW_Number(d,field);int32_t value;
    if(v!=v||v<0||v>(double)INT32_MAX)return 0;
    value=(int32_t)v;if((double)value!=v)return 0;
    *out=value;return 1;
}
/* Successful duplicate names retain the first pool index. The bounded lane
 * requires consistent limits; native conflicting-index0 exemptions are
 * refused. Empty ammo/clip names form distinct real pools after the sentinel. */
static int CAR_Pool(cod1AmmoRegistryPool_t pools[COD1_AMMO_PICKUP_CELLS],
    int *count,const char *name,int32_t limit,int *index) {
    char normalized[64]={0};int i;
    if(!CAR_Name(normalized,name))return 0;
    for(i=0;i<*count;++i)if(!strcmp(pools[i].name,normalized)) {
        if(pools[i].limit!=limit)return 0;
        *index=i;return 1;
    }
    if(*count>=COD1_AMMO_PICKUP_CELLS)return 0;
    i=(*count)++;memcpy(pools[i].name,normalized,sizeof(normalized));pools[i].limit=limit;*index=i;return 1;
}
/* Include raw file hash AND all copied metadata, not just gameplay keys.
 * Pointer values themselves are not fingerprinted. A value must be NULL or
 * point to a terminated token in the genuine definition's own text buffer. */
static int CAR_Metadata(int count,uint32_t *out) {
    uint32_t h=2166136261u;int i,j;const cwDef_t *d;
    if(sizeof(double)!=sizeof(uint64_t))return 0;
    h=CI_Hash32(h,(uint32_t)count);
    for(i=0;i<count;++i) {
        uintptr_t begin,end,p;size_t n;uint64_t bits;
        d=CW_Get(i);if(!d||!memchr(d->name,0,sizeof(d->name)))return 0;
        begin=(uintptr_t)d->text;end=begin+sizeof(d->text);
        h=CI_HashString(h,d->name);h=CI_Hash32(h,d->hash);
        h=CI_Hash32(h,(uint32_t)d->count);h=CI_Hash32(h,(uint32_t)d->unknown);
        h=CI_Hash32(h,(uint32_t)d->altIndex);h=CI_Hash32(h,(uint32_t)CW_FamilyRoot(i));
        h=CI_HashBytes(h,d->text,sizeof(d->text));
        for(j=0;j<CW_FIELD_COUNT;++j) {
            if(d->value[j]) {
                p=(uintptr_t)d->value[j];if(p<begin||p>=end)return 0;
                n=(size_t)(end-p);if(!memchr(d->value[j],0,n))return 0;
                h=CI_Hash32(h,(uint32_t)(p-begin));
            } else h=CI_Hash32(h,UINT32_MAX);
            memcpy(&bits,&d->number[j],sizeof(bits));
            h=CI_Hash32(h,(uint32_t)bits);h=CI_Hash32(h,(uint32_t)(bits>>32));
        }
    }
    *out=h;return 1;
}

/* Both compiled static type2 rows65/66 must have a real loaded target. The
 * original warns/falls back to weapon1 when unmatched; this selected lane
 * refuses that unsupported case and preserves output instead. */
static int CAR_BuildFull(cod1AmmoRegistryReceipt_t *out) {
    cod1AmmoRegistryReceipt_t next;
    const cwDef_t *d;cod1ItemDescriptor_t item;
    uint32_t revision=CW_RegistryRevision(),fingerprint,metadata,again;
    int count=CW_Count(),i,j;int32_t maxAmmo,maxClip,shared,clipOnly;
    if(!revision||count<1||count>CW_MAX_DEFS||!CAR_Output(out,sizeof(*out),count))return 0;
    if(!CAR_Metadata(count,&metadata))return 0;
    fingerprint=CI_Fingerprint();if(!fingerprint)return 0;
    memset(&next,0,sizeof(next));next.version=COD1_AMMO_REGISTRY_VERSION;next.bytes=sizeof(next);
    next.registryRevision=revision;next.descriptorFingerprint=fingerprint;next.metadataFingerprint=metadata;next.weaponCount=count;
    next.ammoCount=next.clipCount=1;strcpy(next.ammo[0].name,"none");strcpy(next.clip[0].name,"none");
    for(i=0;i<count;++i) {
        cod1AmmoRegistryWeapon_t *w=&next.weapons[i+1];
        d=CW_Get(i);if(!d||!d->name[0]||!memchr(d->name,0,sizeof(d->name)))return 0;
        w->cwIndex=i;w->familyRoot=CW_FamilyRoot(i);w->altIndex=d->altIndex;w->definitionHash=d->hash;
        if(w->familyRoot<0||w->familyRoot>=count||w->altIndex< -1||w->altIndex>=count||
           !CAR_Name(w->name,d->name))return 0;
        for(j=1;j<=i;++j)if(!strcmp(next.weapons[j].name,w->name))return 0;
        if(!CAR_Integer(d,"maxAmmo",&maxAmmo)||!CAR_Integer(d,"clipSize",&maxClip)||
           !CAR_Integer(d,"clipOnly",&clipOnly)||clipOnly>1)return 0;
        w->pickup.clipOnly=clipOnly;w->pickup.sharedCapIndex=-1;
        if(!CAR_Pool(next.ammo,&next.ammoCount,CW_String(d,"ammoName"),maxAmmo,&w->pickup.ammoIndex)||
           !CAR_Pool(next.clip,&next.clipCount,CW_String(d,"clipName"),maxClip,&w->pickup.clipIndex))return 0;
        if(CW_String(d,"sharedAmmoCapName")[0]) {
            if(!CAR_Integer(d,"sharedAmmoCap",&shared)||
               !CAR_Pool(next.sharedCap,&next.sharedCapCount,CW_String(d,"sharedAmmoCapName"),shared,&w->pickup.sharedCapIndex))return 0;
        }
    }
    for(i=0;i<COD1_AMMO_REGISTRY_BINDINGS;++i) {
        cod1AmmoRegistryBinding_t *b=&next.bindings[i];const cod1AmmoRegistryWeapon_t *w;
        if(!CI_Descriptor(65+i,&item)||item.dynamic||item.type!=2)return 0;
        for(j=1;j<=count;++j)if(CAR_Prefix(next.weapons[j].name,item.pickup_name))break;
        if(j>count)return 0;
        w=&next.weapons[j];b->itemIndex=item.index;b->weaponIndex=j;b->cwIndex=w->cwIndex;
        b->ammoIndex=w->pickup.ammoIndex;b->clipIndex=w->pickup.clipIndex;
        b->sharedCapIndex=w->pickup.sharedCapIndex;b->clipOnly=w->pickup.clipOnly;b->quantity=item.quantity;
    }
    if(CW_Count()!=count||CW_RegistryRevision()!=revision||CI_Fingerprint()!=fingerprint||
       !CAR_Metadata(count,&again)||again!=metadata)return 0;
    *out=next;return 1;
}
/* R221: repeated currentness fences must not rebuild/hash every definition
 * for every model configstring read. Cache only a successful full projection.
 * The fast path compares EVERY definition byte (including token pointers and
 * unused text), its owner, and family roots. A revision-only cache would miss
 * same-revision corruption. This bounded, per-TU cache owns no live pointers
 * beyond identity checks and never dereferences copied token pointers.
 * CW and these pure projections execute on the game thread; no callbacks run
 * between capture, full validation and comparison. Compiled items are const.
 * Scene/entity/player/resource fences remain with their existing owners. */
static int CAR_CacheMatches(void) {
    int i;const cwDef_t *d;
    if(!carCurrentCache.revision || CW_RegistryRevision()!=carCurrentCache.revision ||
       CW_Count()!=carCurrentCache.count)return 0;
    for(i=0;i<carCurrentCache.count;++i) {
        d=CW_Get(i);
        if(!d || d!=carCurrentCache.owners[i] ||
           CW_FamilyRoot(i)!=carCurrentCache.roots[i] ||
           !CAR_BytesEqual(d,&carCurrentCache.definitions[i],sizeof(*d)))return 0;
    }
    return CW_RegistryRevision()==carCurrentCache.revision && CW_Count()==carCurrentCache.count;
}
/* Reuse the same fully validated projection as CAR_Current. Every live
 * definition byte, token pointer, owner, family root, count and revision is
 * compared before reuse. Compiled item descriptors are const. There are no
 * callbacks between these reads and publication; output/private overlap
 * checks remain mandatory. Any mismatch takes the complete original build. */
static int CAR_Build(cod1AmmoRegistryReceipt_t *out) {
    if(carCurrentCache.valid && CAR_Output(out,sizeof(*out),CW_Count()) && CAR_CacheMatches()) {
        *out=carCurrentCache.receipt;return 1;
    }
    return CAR_BuildFull(out);
}
static int CAR_Current(const cod1AmmoRegistryReceipt_t *receipt) {
    cod1AmmoRegistryReceipt_t current;int i,count;const cwDef_t *d;
    if(!receipt||receipt->version!=COD1_AMMO_REGISTRY_VERSION||receipt->bytes!=sizeof(*receipt)||
       !receipt->registryRevision||receipt->registryRevision!=CW_RegistryRevision()||
       receipt->weaponCount!=CW_Count())return 0;
    if(carCurrentCache.valid && CAR_CacheMatches())
        return !memcmp(receipt,&carCurrentCache.receipt,sizeof(*receipt));
    carCurrentCache.valid=0;count=CW_Count();
    if(count<1 || count>CW_MAX_DEFS)return 0;
    carCurrentCache.count=count;carCurrentCache.revision=CW_RegistryRevision();
    for(i=0;i<count;++i) {
        d=CW_Get(i);if(!d)return 0;
        carCurrentCache.owners[i]=d;carCurrentCache.roots[i]=CW_FamilyRoot(i);
        memcpy(&carCurrentCache.definitions[i],d,sizeof(*d));
    }
    if(!CAR_Build(&current) || !CAR_CacheMatches())return 0;
    carCurrentCache.receipt=current;carCurrentCache.valid=1;
    return !memcmp(receipt,&current,sizeof(current));
}
static int CAR_Binding(const cod1AmmoRegistryReceipt_t *receipt,int itemIndex,
    cod1AmmoRegistryBinding_t *out) {
    cod1AmmoRegistryBinding_t result;
    if(!receipt||itemIndex<65||itemIndex>66||
       !CAR_Output(out,sizeof(*out),CW_Count())||CAR_Overlap(out,sizeof(*out),receipt,sizeof(*receipt))||
       !CAR_Current(receipt))return 0;
    result=receipt->bindings[itemIndex-65];*out=result;return 1;
}
#endif
