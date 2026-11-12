#ifndef BG_COD1_VIEWPROFILE_H
#define BG_COD1_VIEWPROFILE_H
#include "bg_cod1_weapondef.h"
#include "bg_cod1_viewhands.h"
/* Shared resource contract for cgame and renderer; no RTCW weapon IDs here. */
#define CW_VIEW_PATH 64
#define CW_VIEW_EXTRA 6
#define CW_VIEW_CLIPS (10+CW_VIEW_EXTRA)
enum {CW_VIEW_BOLT, CW_VIEW_ADSBOLT, CW_VIEW_RELOAD_START, CW_VIEW_RELOAD_END, CW_VIEW_HOLD, CW_VIEW_MELEE};
#define CW_VIEW_MAX_FIRE_FRAMES 128
typedef struct {
    char gun[CW_VIEW_PATH],hands[CW_VIEW_PATH];
    char idle[CW_VIEW_PATH],fire[CW_VIEW_PATH],hip[CW_VIEW_PATH];
    char reload[2][CW_VIEW_PATH],selector[CW_VIEW_PATH],adsFire[CW_VIEW_PATH],adsDown[CW_VIEW_PATH];
    char drop[CW_VIEW_PATH],raise[CW_VIEW_PATH];
    char lastFire[CW_VIEW_PATH],lastAdsFire[CW_VIEW_PATH],emptyIdle[CW_VIEW_PATH];
    char extra[CW_VIEW_EXTRA][CW_VIEW_PATH];
    int selectedHands; /* Explicit player model; do not substitute default US materials. */
} cwViewProfile;
typedef struct { int frames,fps; } cwViewAnim;
static int CW_ViewPath(char *out,const char *value,const char *prefix,int strip) {
    const char *p;int n;
    if(!value || !*value) return 0;
    n=(int)strlen(prefix);
    if(!strncmp(value,prefix,n)) value+=n;
    if(!*value || value[0]=='/' || strstr(value,"..") || strchr(value,':') || strchr(value,'\\'))return 0;
    for(p=value;*p;p++) if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') ||
        (*p>='0' && *p<='9') || *p=='_' || *p=='-' || *p=='/' || *p=='.' || *p=='@'))return 0;
    if(strlen(value)+n>=CW_VIEW_PATH)return 0;
    if(strip)strcpy(out,value);else {strcpy(out,prefix);strcat(out,value);}
    return 1;
}
/* Internal renderer resource key: weapon@hands. Both components are leaf
 * names; the original GSC string remains xmodel/<hands>. No global renderer
 * mutation: each choice owns a distinct model cache entry. */
static int CW_ViewLeafValid(const char *text) {
    size_t n=0;unsigned char c;if(!text||!*text)return 0;
    while((c=(unsigned char)*text++)!=0) {
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-')||++n>=CW_VIEW_PATH)return 0;
    }
    return 1;
}
static int CW_ViewResourceParse(const char *resource,char *weapon,char *hands) {
    char w[CW_VIEW_PATH],h[CW_VIEW_PATH];const char *at;size_t n;
    if(!resource||!weapon||!hands||strlen(resource)>=CW_VIEW_PATH)return 0;
    at=strchr(resource,'@');n=at?(size_t)(at-resource):strlen(resource);
    if(!n||n>=sizeof(w))return 0;
    memcpy(w,resource,n);w[n]=0;h[0]=0;
    if(!CW_ViewLeafValid(w))return 0;
    if(at) {
        if(!at[1]||strlen(at+1)>=sizeof(h)||!CW_ViewLeafValid(at+1))return 0;
        strcpy(h,at+1);
    }
    strcpy(weapon,w);strcpy(hands,h);return 1;
}
static int CW_ViewSelectHands(cwViewProfile *profile,const char *hands) {
    char path[CW_VIEW_PATH];
    if(!profile||!hands)return 0;
    if(!*hands)return 1;
    if(!CW_ViewLeafValid(hands)||!CW_ViewPath(path,hands,"xmodel/",1))return 0;
    strcpy(profile->hands,path);profile->selectedHands=1;return 1;
}
static int CW_ViewResourceName(char *out,size_t capacity,const char *prefix,const char *weapon,int handsId) {
    const char *hands=Cod1ViewHandsName(handsId);size_t n;
    if(!out||!prefix||!hands||!CW_ViewLeafValid(weapon))return 0;
    n=strlen(prefix)+strlen(weapon)+(*hands?1+strlen(hands):0);
    if(n>=capacity||n>=CW_VIEW_PATH)return 0;
    strcpy(out,prefix);strcat(out,weapon);
    if(*hands){strcat(out,"@");strcat(out,hands);}return 1;
}
static const char *CW_ViewFallback(const cwDef_t *d,const char *key,const char *fallback) {
    const char *s=CW_String(d,key);return *s?s:CW_String(d,fallback);
}
static int CW_ViewProfile(const cwDef_t *d,cwViewProfile *p) {
    static const char *extra[]={"rechamberAnim","adsRechamberAnim","reloadStartAnim","reloadEndAnim","holdFireAnim","meleeAnim"};
    int i;memset(p,0,sizeof(*p));
    if(!CW_ViewPath(p->gun,CW_String(d,"gunModel"),"xmodel/",1) ||
       !CW_ViewPath(p->hands,CW_String(d,"handModel"),"xmodel/",1) ||
       !CW_ViewPath(p->idle,CW_String(d,"idleAnim"),"xanim/",0))return 0;
    if(CW_Number(d,"aimDownSight") &&
       (!CW_ViewPath(p->hip,CW_String(d,"adsUpAnim"),"xanim/",0) ||
        !CW_ViewPath(p->adsDown,CW_String(d,"adsDownAnim"),"xanim/",0)))return 0;
    for(i=0;i<CW_VIEW_EXTRA;i++) {
        int needed=i<2?CW_Number(d,"boltAction")!=0:i<4?CW_Number(d,"segmentedReload")!=0:
            i==CW_VIEW_HOLD?!strcmp(CW_String(d,"weaponType"),"grenade"):CW_Number(d,"meleeDamage")>0;
        if(needed && !CW_ViewPath(p->extra[i],CW_String(d,extra[i]),"xanim/",0))return 0;
    }
    return CW_ViewPath(p->fire,CW_String(d,"fireAnim"),"xanim/",0) &&
        CW_ViewPath(p->reload[0],CW_ViewFallback(d,"reloadAnim","idleAnim"),"xanim/",0) &&
        CW_ViewPath(p->reload[1],CW_ViewFallback(d,"reloadEmptyAnim","idleAnim"),"xanim/",0) &&
        CW_ViewPath(p->selector,CW_ViewFallback(d,"altDropAnim","idleAnim"),"xanim/",0) &&
        CW_ViewPath(p->adsFire,CW_ViewFallback(d,"adsFireAnim","fireAnim"),"xanim/",0) &&
        CW_ViewPath(p->drop,CW_String(d,"dropAnim"),"xanim/",0) &&
        CW_ViewPath(p->raise,CW_String(d,"raiseAnim"),"xanim/",0) &&
        CW_ViewPath(p->lastFire,CW_ViewFallback(d,"lastShotAnim","fireAnim"),"xanim/",0) &&
        CW_ViewPath(p->lastAdsFire,(*CW_String(d,"adsLastShotAnim")?CW_String(d,"adsLastShotAnim"):CW_ViewFallback(d,"adsFireAnim","fireAnim")),"xanim/",0) &&
        CW_ViewPath(p->emptyIdle,CW_ViewFallback(d,"emptyIdleAnim","idleAnim"),"xanim/",0);
}
static const char *CW_ViewClip(const cwViewProfile *p,int index) {
    switch(index) {
    case 0:return p->fire;case 1:return p->reload[0];case 2:return p->reload[1];
    case 3:return p->selector;case 4:return p->adsFire;case 5:return p->drop;case 6:return p->raise;
    case 7:return p->lastFire;case 8:return p->lastAdsFire;case 9:return p->emptyIdle;
    default:return index>=10 && index<CW_VIEW_CLIPS?p->extra[index-10]:"";
    }
}
static int CW_ViewAnimInfo(const void *data,int size,cwViewAnim *a) {
    const unsigned char *b=(const unsigned char*)data;int bones;
    memset(a,0,sizeof(*a));
    if(!b || size<9 || b[0]!=14 || b[1] || (b[6]&~1))return 0;
    a->frames=b[2]+256*b[3]+(b[6]&1);bones=b[4]+256*b[5];a->fps=b[7]+256*b[8];
    return a->frames>0 && a->frames<=CW_VIEW_MAX_FIRE_FRAMES && bones>0 && bones<=128 && a->fps>0 && a->fps<=1000;
}
static void CW_ViewFireFrame(const cwViewAnim *a,int elapsed,int *oldframe,int *frame,float *backlerp) {
    float sample;int first;
    *oldframe=*frame=0;*backlerp=0;
    if(elapsed<0 || a->fps<=0 || a->frames<=0)return;
    sample=elapsed*(a->fps/1000.0f);
    if(sample >= (a->frames>1 ? a->frames-1 : 1))return;
    first=(int)sample;*oldframe=first+1;*frame=*oldframe+1;
    if(*frame>a->frames)*frame=a->frames;
    *backlerp=1-(sample-first);
}
/* Reload poses follow the same predicted countdown as ammo and readiness.
 * CoD1 cgame scales clip playback by its weapon-definition duration. */
static void CW_ViewReloadFrame(const cwViewAnim *a,int base,int remaining,int total,int *oldframe,int *frame,float *backlerp) {
    float sample;int first;
    if(total<=0 || a->frames<=0){*oldframe=*frame=0;*backlerp=0;return;}
    if(remaining<0)remaining=0;if(remaining>total)remaining=total;
    sample=(total-remaining)/(float)total*(a->frames-1);first=(int)sample;
    *oldframe=base+first;*frame=*oldframe+1;
    if(*frame>=base+a->frames)*frame=base+a->frames-1;
    *backlerp=1-(sample-first);
}
#endif
