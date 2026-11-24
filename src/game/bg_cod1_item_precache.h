/* R121 complete original compiled item domain. Static strings/nulls and
 * integer fields derive from native_precache_item/STATIC_RAW_FACTS.json.
 * Dynamic rows use the real mounted CW registry: this port supports at most
 * 63 loaded definitions plus the separate native none0 sentinel.
 * No map-specific substitution or inferred classname prefix exists here. */
#ifndef BG_COD1_ITEM_PRECACHE_H
#define BG_COD1_ITEM_PRECACHE_H
#include "q_shared.h"
#include "cod1_precache.h"
#include "../qcommon/cod1_item_protocol.h"
#include "bg_cod1_weapondef.h"
#include "../qcommon/cod1_ambient_module.h"
#include <stdint.h>
#include <limits.h>
#define COD1_ITEM_COUNT 70
#define COD1_ITEM_BITS_BYTES 18
#define COD1_ITEM_STATE_CAPACITY 128
#if CS_COD1_ITEM_PRECACHE >= MAX_CONFIGSTRINGS
#error CoD1 item state exceeds configstring capacity
#endif
typedef struct {
    const char *classname,*pickup_sound,*model0,*model1,*icon,*ammo_icon,*pickup_name;
    int quantity,type;
} cod1ItemStatic_t;
static const cod1ItemStatic_t cod1ItemCompiled[COD1_ITEM_COUNT] = {
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0 },
    { "emptyitem_\"w01\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w02\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w03\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w04\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w05\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w06\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w07\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w08\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w09\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w10\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w11\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w12\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w13\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w14\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w15\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w16\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w17\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w18\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w19\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w20\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w21\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w22\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w23\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w24\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w25\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w26\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w27\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w28\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w29\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w30\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w31\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w32\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w33\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w34\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w35\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w36\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w37\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w38\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w39\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w40\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w41\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w42\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w43\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w44\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w45\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w46\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w47\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w48\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w49\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w50\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w51\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w52\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w53\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w54\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w55\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w56\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w57\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w58\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w59\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w60\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w61\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w62\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w63\"", "", "", "", "", "", "", 0, 0 },
    { "emptyitem_\"w64\"", "", "", "", "", "", "", 0, 0 },
    { "item_ammo_stielhandgranate_open", "grenade_pickup", "xmodel/ammo_stielhandgranate1", NULL, "gfx/icons/hud@steilhandgrenate", "gfx/icons/hud@steilhandgrenate", "Stielhandgranate Ammo Open", 10, 2 },
    { "item_ammo_stielhandgranate_closed", "grenade_pickup", "xmodel/ammo_stielhandgranate2", NULL, "gfx/icons/hud@steilhandgrenate", "gfx/icons/hud@steilhandgrenate", "Stielhandgranate Ammo Closed", 10, 2 },
    { "item_health_small", "health_pickup_small", "xmodel/health_small", NULL, "icons/iconh_small", NULL, "Small Health", 10, 3 },
    { "item_health", "health_pickup_medium", "xmodel/health_medium", NULL, "icons/iconh_med", NULL, "Med Health", 25, 3 },
    { "item_health_large", "health_pickup_large", "xmodel/health_large", NULL, "icons/iconh_large", NULL, "Large Health", 50, 3 },
};
typedef struct {
    /* Resource descriptor only: quantity is copied metadata, never a gameplay
     * grant. For dynamic rows, models[] are the server registrar's ordered
     * worldModel/projectileModel pair. The original patched client item row
     * has worldModel/NULL; the checked port client lane deliberately acquires
     * the extra projectile dependency before declaring this item complete. */
    int index,type,quantity,dynamic,weapon_index,linked_index;
    int model_present[2],icon_present,pickup_present,ammo_icon_present;
    char internal_name[64],classname[128],pickup_name[128];
    char models[2][MAX_QPATH],icon[MAX_QPATH],ammo_icon[MAX_QPATH],pickup_alias[128];
} cod1ItemDescriptor_t;
typedef struct {
    uint64_t epoch,operation_id;
    uint32_t fingerprint;
    unsigned char registered[COD1_ITEM_COUNT];
} cod1ItemState_t;
static uint32_t CI_Module(uint64_t epoch) {return (uint32_t)(epoch>>32);}
static uint32_t CI_Generation(uint64_t epoch) {return (uint32_t)epoch;}
static int CI_Epoch(uint64_t epoch) {
    return CI_Module(epoch)>0 && CI_Module(epoch)<=COD1_AMBIENT_MODULE_MAX &&
        CI_Generation(epoch)>0 && CI_Generation(epoch)!=UINT32_MAX;
}
static uint32_t CI_HashBytes(uint32_t hash,const void *data,size_t size) {
    const unsigned char *p=(const unsigned char *)data;size_t i;
    for(i=0;i<size;++i) hash=(hash^p[i])*16777619u;
    return hash;
}
static uint32_t CI_Hash32(uint32_t hash,uint32_t value) {
    unsigned char bytes[4];int i;
    for(i=0;i<4;++i)bytes[i]=(unsigned char)(value>>(i*8));
    return CI_HashBytes(hash,bytes,sizeof(bytes));
}
static uint32_t CI_HashString(uint32_t hash,const char *text) {
    unsigned char marker=(unsigned char)(text!=NULL);
    hash=CI_HashBytes(hash,&marker,1);
    return text?CI_HashBytes(hash,text,strlen(text)+1):hash;
}
static uint32_t CI_Fingerprint(void) {
    uint32_t hash=2166136261u;int i,n=CW_Count();const cwDef_t *d;
    if(n<0 || n>CW_MAX_DEFS)return 0;
    hash=CI_Hash32(hash,(uint32_t)n);
    for(i=0;i<COD1_ITEM_COUNT;++i) {
        const cod1ItemStatic_t *r=&cod1ItemCompiled[i];
        hash=CI_HashString(hash,r->classname);hash=CI_HashString(hash,r->pickup_sound);
        hash=CI_HashString(hash,r->model0);hash=CI_HashString(hash,r->model1);
        hash=CI_HashString(hash,r->icon);hash=CI_HashString(hash,r->ammo_icon);
        hash=CI_HashString(hash,r->pickup_name);hash=CI_Hash32(hash,(uint32_t)r->quantity);
        hash=CI_Hash32(hash,(uint32_t)r->type);
    }
    for(i=0;i<n;++i) {
        d=CW_Get(i);if(!d)return 0;
        hash=CI_HashString(hash,d->name);hash=CI_Hash32(hash,(uint32_t)d->hash);
        hash=CI_Hash32(hash,(uint32_t)d->altIndex);
        hash=CI_HashString(hash,CW_String(d,"worldModel"));
        hash=CI_HashString(hash,CW_String(d,"projectileModel"));
        hash=CI_HashString(hash,CW_String(d,"hudIcon"));
        hash=CI_HashString(hash,CW_String(d,"pickupSound"));
        hash=CI_HashString(hash,CW_String(d,"radiantName"));
        hash=CI_HashString(hash,CW_String(d,"displayName"));
        hash=CI_HashString(hash,CW_String(d,"ammoIcon"));
        hash=CI_HashString(hash,CW_String(d,"altWeapon"));
        hash=CI_HashString(hash,CW_String(d,"startAmmo"));
    }
    return hash;
}
static int CI_Copy(char *out,size_t cap,const char *text) {
    size_t n;if(!text || !cap)return 0;n=strlen(text);
    if(n>=cap)return 0;memcpy(out,text,n+1);return 1;
}
/* Resource namespace completion is independent of item-name identity.
 * Existing WeaponDef values may contain either full XModel names or basenames.
 * This selected port treats both as the same declared XModel resource lane. */
static int CI_Model(char out[MAX_QPATH],const char *text) {
    size_t n;
    if(!text)return 0;
    if(!*text) {out[0]=0;return 1;}
    if(!Q_stricmpn(text,"xmodel",6) && (text[6]=='/' || text[6]=='\\')) {
        if(!CI_Copy(out,MAX_QPATH,text))return 0;
    } else {
        n=strlen(text);if(n+7>=MAX_QPATH)return 0;
        memcpy(out,"xmodel/",7);memcpy(out+7,text,n+1);
    }
    return Cod1PrecacheName(out,1);
}
/* Original A9C3 uses CRT81EBF: default locale compares ASCII names without
 * case. Keep native semantic index+1; locale/non-ASCII loader parity is not
 * inferred from this bounded CW name domain. */
static int CI_LinkNameEqual(const char*a,const char*b){unsigned x,y;do{x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}while(x);return 1;}
static int CI_Linked(const cwDef_t *definition) {
    const char *name;const cwDef_t *candidate;int i;
    if(!definition)return 0;name=CW_String(definition,"altWeapon");
    if(!*name)return 0;
    for(i=0;i<CW_Count();++i) {candidate=CW_Get(i);if(candidate && CI_LinkNameEqual(candidate->name,name))return i+1;}
    return 0;
}
/* Mounted metadata may be malformed. Check the numeric domain before the
 * integer conversion; neither gameplay quantity nor retail parser parity is
 * inferred by this resource-only descriptor. */
static int CI_Quantity(const cwDef_t *definition,int *out) {
    double value=CW_Number(definition,"startAmmo");
    if(!out || value!=value || value<(double)INT_MIN || value>(double)INT_MAX)return 0;
    *out=(int)value;return 1;
}
static int CI_Descriptor(int index,cod1ItemDescriptor_t *out) {
    cod1ItemDescriptor_t r;const cod1ItemStatic_t *s;
    const cwDef_t *d;const char *m[2];int i,field,n=CW_Count();
    if(!out || index<1 || index>=COD1_ITEM_COUNT || n<0 || n>CW_MAX_DEFS)return 0;
    memset(&r,0,sizeof(r));r.index=index;r.weapon_index=-1;
    if(index<=n) {
        d=CW_Get(index-1);if(!d)return 0;
        r.dynamic=1;r.type=1;r.weapon_index=index-1;
        r.linked_index=CI_Linked(d);if(!CI_Quantity(d,&r.quantity))return 0;
        if(r.linked_index<0 || r.linked_index>n || !CI_Copy(r.internal_name,sizeof(r.internal_name),d->name))return 0;
        field=CW_FieldIndex("worldModel");m[0]=field>=0?d->value[field]:NULL;
        field=CW_FieldIndex("projectileModel");m[1]=field>=0?d->value[field]:NULL;
        s=NULL;
        r.icon_present=1;r.pickup_present=1;r.ammo_icon_present=1;
        if(!CI_Copy(r.classname,sizeof(r.classname),CW_String(d,"radiantName")) ||
           !CI_Copy(r.pickup_name,sizeof(r.pickup_name),CW_String(d,"displayName")) ||
           !CI_Copy(r.ammo_icon,sizeof(r.ammo_icon),CW_String(d,"ammoIcon")))return 0;
        if(!CI_Copy(r.icon,sizeof(r.icon),CW_String(d,"hudIcon")) ||
           !CI_Copy(r.pickup_alias,sizeof(r.pickup_alias),CW_String(d,"pickupSound")))return 0;
    } else {
        s=&cod1ItemCompiled[index];r.type=s->type;r.quantity=s->quantity;
        m[0]=s->model0;m[1]=s->model1;
        r.icon_present=s->icon!=NULL;r.pickup_present=s->pickup_sound!=NULL;r.ammo_icon_present=s->ammo_icon!=NULL;
        if(s->classname && !CI_Copy(r.classname,sizeof(r.classname),s->classname))return 0;
        if(s->pickup_name && !CI_Copy(r.pickup_name,sizeof(r.pickup_name),s->pickup_name))return 0;
        if(s->ammo_icon && !CI_Copy(r.ammo_icon,sizeof(r.ammo_icon),s->ammo_icon))return 0;
        if(s->icon && !CI_Copy(r.icon,sizeof(r.icon),s->icon))return 0;
        if(s->pickup_sound && !CI_Copy(r.pickup_alias,sizeof(r.pickup_alias),s->pickup_sound))return 0;
    }
    for(i=0;i<2;++i) {
        r.model_present[i]=m[i]!=NULL;
        if(m[i] && !CI_Model(r.models[i],m[i]))return 0;
    }
    *out=r;return 1;
}
/* Native ordered first match: loaded dynamic lane bypasses static fallback
 * on mismatch. Empty request can match the first unloaded placeholder. */
static int CI_Resolve(const char *name,cod1ItemDescriptor_t *out) {
    int i,n=CW_Count();const cwDef_t *d;const cod1ItemStatic_t *r;
    if(!name || !out || n<0 || n>CW_MAX_DEFS)return 0;
    for(i=1;i<COD1_ITEM_COUNT;++i) {
        if(i<=n) {
            d=CW_Get(i-1);if(d && !Q_stricmp(d->name,name))return CI_Descriptor(i,out);
            continue;
        }
        r=&cod1ItemCompiled[i];
        if((r->pickup_name && !Q_stricmp(r->pickup_name,name)) ||
           (r->classname && !Q_stricmp(r->classname,name)))return CI_Descriptor(i,out);
    }
    return 0;
}
static int CI_EncodeBits(char out[COD1_ITEM_BITS_BYTES+1],const unsigned char bits[COD1_ITEM_COUNT]) {
    static const char hex[]="0123456789abcdef";int nibble,j,value;
    if(!out || !bits || bits[0])return 0;
    for(nibble=0;nibble<COD1_ITEM_BITS_BYTES;++nibble) {
        value=0;
        for(j=0;j<4;++j) if(nibble*4+j<COD1_ITEM_COUNT) {
            if(bits[nibble*4+j]>1)return 0;
            value|=(int)bits[nibble*4+j]<<j;
        }
        out[nibble]=hex[value];
    }
    out[COD1_ITEM_BITS_BYTES]=0;return 1;
}
static int CI_Hex(char c) {return c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:-1;}
static int CI_DecodeBits(const char *text,unsigned char out[COD1_ITEM_COUNT]) {
    unsigned char copy[COD1_ITEM_COUNT];int i,j,value;
    if(!text || !out || strlen(text)!=COD1_ITEM_BITS_BYTES)return 0;
    memset(copy,0,sizeof(copy));
    for(i=0;i<COD1_ITEM_BITS_BYTES;++i) {
        value=CI_Hex(text[i]);if(value<0 || (i==17 && value>3))return 0;
        for(j=0;j<4 && i*4+j<COD1_ITEM_COUNT;++j) copy[i*4+j]=(unsigned char)((value>>j)&1);
    }
    if(copy[0])return 0;memcpy(out,copy,sizeof(copy));return 1;
}
/* Private configstring, never the incompatible RTCW CS_ITEMS27/CoD CS8. */
static int CI_EncodeState(char *out,size_t cap,const cod1ItemState_t *state) {
    static const char hex[]="0123456789abcdef";
    char bits[COD1_ITEM_BITS_BYTES+1];uint64_t values[2];int i,j;
    if(!out || cap<67 || !state || !CI_Epoch(state->epoch) || state->operation_id==UINT64_MAX ||
       !CI_EncodeBits(bits,state->registered))return 0;
    if(!state->operation_id)for(i=1;i<COD1_ITEM_COUNT;++i)if(state->registered[i])return 0;
    memcpy(out,"c1i1 ",5);values[0]=state->epoch;values[1]=state->operation_id;
    for(j=0;j<2;++j) for(i=0;i<16;++i) out[5+j*17+i]=hex[(unsigned int)(values[j]>>((15-i)*4))&15];
    out[21]=out[38]=out[47]=' ';
    for(i=0;i<8;++i) out[39+i]=hex[(state->fingerprint>>((7-i)*4))&15];
    memcpy(out+48,bits,COD1_ITEM_BITS_BYTES+1);return 1;
}
static int CI_DecodeState(const char *text,cod1ItemState_t *out) {
    cod1ItemState_t r;char canonical[COD1_ITEM_STATE_CAPACITY];char bits[COD1_ITEM_BITS_BYTES+1];
    uint64_t value;uint32_t fingerprint;int i,j,n,offset;
    if(!text || !out || strlen(text)!=66 || strncmp(text,"c1i1 ",5))return 0;
    memset(&r,0,sizeof(r));
    for(j=0;j<2;++j) {
        offset=j?22:5;value=0;
        for(i=0;i<16;++i) {n=CI_Hex(text[offset+i]);if(n<0)return 0;value=(value<<4)|(unsigned int)n;}
        if(j)r.operation_id=value;else r.epoch=value;
    }
    if(text[21]!=' ' || text[38]!=' ' || text[47]!=' ')return 0;
    fingerprint=0;
    for(i=0;i<8;++i) {n=CI_Hex(text[39+i]);if(n<0)return 0;fingerprint=(fingerprint<<4)|(unsigned int)n;}
    r.fingerprint=fingerprint;memcpy(bits,text+48,COD1_ITEM_BITS_BYTES);bits[COD1_ITEM_BITS_BYTES]=0;
    if(!CI_DecodeBits(bits,r.registered) || !CI_EncodeState(canonical,sizeof(canonical),&r) || strcmp(canonical,text))return 0;
    *out=r;return 1;
}
#endif
