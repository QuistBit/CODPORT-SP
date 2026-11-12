/* R135 original42 command cache, clocked by actual cg.time. */
#include <ctype.h>
#include <locale.h>
static const char *const cg_cod1KeyBindingCommands[]={
    "+scores",
    "+speed",
    "+forward",
    "+back",
    "+moveleft",
    "+moveright",
    "+moveup",
    "+movedown",
    "+left",
    "+right",
    "+strafe",
    "+lookup",
    "+lookdown",
    "+mlook",
    "centerview",
    "toggle cl_run",
    "+melee",
    "+prone",
    "lowerstance",
    "raisestance",
    "togglecrouch",
    "toggleprone",
    "goprone",
    "gocrouch",
    "+gostand",
    "weaponslot primary",
    "weaponslot primaryb",
    "weaponslot pistol",
    "weaponslot grenade",
    "weaponslot smokegrenade",
    "+attack",
    "weapprev",
    "weapnext",
    "weapalt",
    "savegame quicksave",
    "loadgame quicksave",
    "+activate",
    "+reload",
    "+leanleft",
    "+leanright",
    "screenshot",
    "screenshotJPEG",
};
static const struct {int retail;const char *name;} cg_cod1KeyBindingNames[]={
    {9,"KEY_TAB"},
    {13,"KEY_ENTER"},
    {27,"KEY_ESCAPE"},
    {32,"KEY_SPACE"},
    {127,"KEY_BACKSPACE"},
    {154,"KEY_UPARROW"},
    {155,"KEY_DOWNARROW"},
    {156,"KEY_LEFTARROW"},
    {157,"KEY_RIGHTARROW"},
    {158,"KEY_ALT"},
    {159,"KEY_CTRL"},
    {160,"KEY_SHIFT"},
    {151,"KEY_CAPSLOCK"},
    {167,"KEY_F1"},
    {168,"KEY_F2"},
    {169,"KEY_F3"},
    {170,"KEY_F4"},
    {171,"KEY_F5"},
    {172,"KEY_F6"},
    {173,"KEY_F7"},
    {174,"KEY_F8"},
    {175,"KEY_F9"},
    {176,"KEY_F10"},
    {177,"KEY_F11"},
    {178,"KEY_F12"},
    {161,"KEY_INS"},
    {162,"KEY_DEL"},
    {163,"KEY_PGDN"},
    {164,"KEY_PGUP"},
    {165,"KEY_HOME"},
    {166,"KEY_END"},
    {200,"KEY_MOUSE1"},
    {201,"KEY_MOUSE2"},
    {202,"KEY_MOUSE3"},
    {203,"KEY_MOUSE4"},
    {204,"KEY_MOUSE5"},
    {206,"KEY_MWHEELUP"},
    {205,"KEY_MWHEELDOWN"},
    {207,"KEY_JOY1"},
    {208,"KEY_JOY2"},
    {209,"KEY_JOY3"},
    {210,"KEY_JOY4"},
    {211,"KEY_JOY5"},
    {212,"KEY_JOY6"},
    {213,"KEY_JOY7"},
    {214,"KEY_JOY8"},
    {215,"KEY_JOY9"},
    {216,"KEY_JOY10"},
    {217,"KEY_JOY11"},
    {218,"KEY_JOY12"},
    {219,"KEY_JOY13"},
    {220,"KEY_JOY14"},
    {221,"KEY_JOY15"},
    {222,"KEY_JOY16"},
    {223,"KEY_JOY17"},
    {224,"KEY_JOY18"},
    {225,"KEY_JOY19"},
    {226,"KEY_JOY20"},
    {227,"KEY_JOY21"},
    {228,"KEY_JOY22"},
    {229,"KEY_JOY23"},
    {230,"KEY_JOY24"},
    {231,"KEY_JOY25"},
    {232,"KEY_JOY26"},
    {233,"KEY_JOY27"},
    {234,"KEY_JOY28"},
    {235,"KEY_JOY29"},
    {236,"KEY_JOY30"},
    {237,"KEY_JOY31"},
    {238,"KEY_JOY32"},
    {239,"KEY_AUX1"},
    {240,"KEY_AUX2"},
    {241,"KEY_AUX3"},
    {242,"KEY_AUX4"},
    {243,"KEY_AUX5"},
    {244,"KEY_AUX6"},
    {245,"KEY_AUX7"},
    {246,"KEY_AUX8"},
    {247,"KEY_AUX9"},
    {248,"KEY_AUX10"},
    {249,"KEY_AUX11"},
    {250,"KEY_AUX12"},
    {251,"KEY_AUX13"},
    {252,"KEY_AUX14"},
    {253,"KEY_AUX15"},
    {254,"KEY_AUX16"},
    {182,"KEY_KP_HOME"},
    {183,"KEY_KP_UPARROW"},
    {184,"KEY_KP_PGUP"},
    {185,"KEY_KP_LEFTARROW"},
    {186,"KEY_KP_5"},
    {187,"KEY_KP_RIGHTARROW"},
    {188,"KEY_KP_END"},
    {189,"KEY_KP_DOWNARROW"},
    {190,"KEY_KP_PGDN"},
    {191,"KEY_KP_ENTER"},
    {192,"KEY_KP_INS"},
    {193,"KEY_KP_DEL"},
    {194,"KEY_KP_SLASH"},
    {195,"KEY_KP_MINUS"},
    {196,"KEY_KP_PLUS"},
    {197,"KEY_KP_NUMLOCK"},
    {198,"KEY_KP_STAR"},
    {199,"KEY_KP_EQUALS"},
    {153,"KEY_PAUSE"},
    {59,"KEY_SEMICOLON"},
    {150,"KEY_COMMAND"},
    {128,"\265"},
    {129,"\277"},
    {130,"\337"},
    {131,"\340"},
    {132,"\341"},
    {133,"\344"},
    {134,"\345"},
    {135,"\346"},
    {136,"\347"},
    {137,"\350"},
    {138,"\351"},
    {139,"\354"},
    {140,"\361"},
    {141,"\362"},
    {142,"\363"},
    {143,"\366"},
    {144,"\370"},
    {145,"\371"},
    {146,"\372"},
    {147,"\374"},
};
static int cg_cod1KeyBindingReady,cg_cod1KeyBindingCachedTime=-1;
static int cg_cod1KeyBindingPairs[42][2],cg_cod1KeyBindingCounts[42];
static uint64_t cg_cod1KeyBindingGeneration;
static int CG_Cod1KeyBindingCTypeReady(void) {
    const char *locale=setlocale(LC_CTYPE,NULL);
    return locale&&(!strcmp(locale,"C")||!strcmp(locale,"POSIX"));
}
static int CG_Cod1KeyBindingEqual(const char *a,const char *b) {
    unsigned char x,y;
    do {x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='a'&&x<='z')x=(unsigned char)(x-'a'+'A');
        if(y>='a'&&y<='z')y=(unsigned char)(y-'a'+'A');
        if(x!=y)return 0;
    }while(x);return 1;
}
static int CG_Cod1KeyBindingCommand(const char *command) {
    unsigned n;for(n=0;n<42;n++)if(CG_Cod1KeyBindingEqual(command,cg_cod1KeyBindingCommands[n]))return (int)n;
    return -1;
}
qboolean CG_Cod1KeyBindingName(int retail,char *out,int capacity) {
    const char *name=NULL;unsigned n;char tiny[5];static const char hex[]="0123456789abcdef";
    if(!out||capacity!=COD1_KEYBINDING_NAME_CAP||retail<0||retail>255||!CG_Cod1KeyBindingCTypeReady())return qfalse;
    memset(out,0,(size_t)capacity);
    if(retail>32&&retail<127&&retail!='"') {out[0]=(char)toupper((unsigned char)retail);return qtrue;}
    for(n=0;n<sizeof(cg_cod1KeyBindingNames)/sizeof(cg_cod1KeyBindingNames[0]);n++)
        if(cg_cod1KeyBindingNames[n].retail==retail){name=cg_cod1KeyBindingNames[n].name;break;}
    if(!name){tiny[0]='0';tiny[1]='x';tiny[2]=hex[(retail>>4)&15];tiny[3]=hex[retail&15];tiny[4]=0;name=tiny;}
    for(n=0;name[n]&&n<(unsigned)capacity-1;n++)out[n]=name[n];
    return name[n]?qfalse:qtrue;
}
void CG_Cod1KeyBindingReset(qboolean fresh) {
    int n;cg_cod1KeyBindingReady=fresh?1:0;cg_cod1KeyBindingCachedTime=-1;
    cg_cod1KeyBindingGeneration=0;
    for(n=0;n<42;n++){cg_cod1KeyBindingPairs[n][0]=-1;cg_cod1KeyBindingPairs[n][1]=0;cg_cod1KeyBindingCounts[n]=0;}
}
static int CG_Cod1KeyBindingRefresh(int now) {
    int n,key,count,pair[2];char binding[COD1_KEYBINDING_BINDING_CAP];
    for(n=0;n<42;n++) {
        count=0;pair[0]=pair[1]=-1;
        for(key=0;key<=255;key++) {
            if(!trap_Cod1KeyBindingScan(key,binding,sizeof(binding))||
               !memchr(binding,0,sizeof(binding)))return 0;
            if(!binding[0]||!CG_Cod1KeyBindingEqual(binding,cg_cod1KeyBindingCommands[n]))continue;
            pair[count++]=key;if(count==2)break;
        }
        /* A later refusal retains the rows already refreshed, never rollback. */
        cg_cod1KeyBindingPairs[n][0]=pair[0];cg_cod1KeyBindingPairs[n][1]=pair[1];cg_cod1KeyBindingCounts[n]=count;
    }
    cg_cod1KeyBindingCachedTime=now;return 1;
}
static int CG_Cod1KeyBindingExpected(const char *command,char key1[128],char key2[128],int *count) {
    int index=CG_Cod1KeyBindingCommand(command);
    memset(key1,0,128);memset(key2,0,128);*count=index<0?0:cg_cod1KeyBindingCounts[index];
    if(!*count){memcpy(key1,"KEY_UNBOUND",sizeof("KEY_UNBOUND"));return 1;}
    if(*count<0||*count>2||!CG_Cod1KeyBindingName(cg_cod1KeyBindingPairs[index][0],key1,128))return 0;
    return *count==1||CG_Cod1KeyBindingName(cg_cod1KeyBindingPairs[index][1],key2,128);
}
qboolean CG_Cod1KeyBindingQuery(cod1KeyBindingQuery_t *query) {
    cod1KeyBindingQuery_t stamp,result;char key1[128],key2[128];int now,count;
    if(!query||!cg_cod1KeyBindingReady||cg.time<0||cg.mapRestart||cgs.gametype!=GT_SINGLE_PLAYER||
       !CG_Cod1KeyBindingCTypeReady()||query->abi!=COD1_KEYBINDING_WIRE_ABI||!query->operation_id||
       !memchr(query->command,0,sizeof(query->command)))return qfalse;
    now=cg.time;stamp=*query;stamp.cg_time=now;
    if(query->phase==COD1_KEYBINDING_PHASE_QUERY) {
        if(query->cg_time!=-1||query->count!=-1||query->cgame_generation||query->key1[0]||query->key2[0])return qfalse;
        if(!trap_Cod1KeyBindingContext(&stamp)||cg.time!=now||!cg_cod1KeyBindingReady||
           cg.mapRestart||cgs.gametype!=GT_SINGLE_PLAYER||!CG_Cod1KeyBindingCTypeReady()||
           !stamp.cgame_generation)return qfalse;
        if(cg_cod1KeyBindingGeneration&&cg_cod1KeyBindingGeneration!=stamp.cgame_generation)return qfalse;
        cg_cod1KeyBindingGeneration=stamp.cgame_generation;
        if(cg_cod1KeyBindingCachedTime!=now&&!CG_Cod1KeyBindingRefresh(now))return qfalse;
    } else if(query->phase==COD1_KEYBINDING_PHASE_VALIDATE) {
        if(query->cg_time!=now||cg_cod1KeyBindingCachedTime!=now||
           query->cgame_generation!=cg_cod1KeyBindingGeneration||!trap_Cod1KeyBindingContext(&stamp))return qfalse;
    } else return qfalse;
    if(!CG_Cod1KeyBindingExpected(query->command,key1,key2,&count))return qfalse;
    if(!trap_Cod1KeyBindingContext(&stamp))return qfalse;
    if(cg.time!=now||!cg_cod1KeyBindingReady||cg.mapRestart||cgs.gametype!=GT_SINGLE_PLAYER||
       !CG_Cod1KeyBindingCTypeReady()||stamp.cgame_generation!=cg_cod1KeyBindingGeneration)return qfalse;
    if(query->phase==COD1_KEYBINDING_PHASE_VALIDATE)
        return query->count==count&&!memcmp(query->key1,key1,128)&&!memcmp(query->key2,key2,128)?qtrue:qfalse;
    result=*query;result.cg_time=now;result.count=count;result.cgame_generation=stamp.cgame_generation;
    memcpy(result.key1,key1,128);memcpy(result.key2,key2,128);*query=result;return qtrue;
}
qboolean CG_Cod1KeyBindingQueryNative(int low,int high) {
#if defined(Q3_VM)
    (void)low;(void)high;return qfalse;
#else
    uint64_t raw=(uint64_t)(uint32_t)low|((uint64_t)(uint32_t)high<<32);
    uintptr_t pointer=(uintptr_t)raw;
    if((uint64_t)pointer!=raw||!pointer)return qfalse;
    return CG_Cod1KeyBindingQuery((cod1KeyBindingQuery_t *)pointer);
#endif
}
