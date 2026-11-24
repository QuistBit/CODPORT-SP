/* R135 native configured-key owner. No KeynumToString or direct GAME cache. */
#include <locale.h>
#include "../game/bg_cod1_keybinding.h"
static uint64_t cl_cod1KeyBindingGenerationCounter,cl_cod1KeyBindingGeneration;
static vm_t *cl_cod1KeyBindingOwner;
static int cl_cod1KeyBindingReady,cl_cod1KeyBindingServerId,cl_cod1KeyBindingLastServerId;
static const struct {int retail,port;} cl_cod1KeyBindingKeyMap[]={
    {0,0},
    {1,1},
    {2,2},
    {3,3},
    {4,4},
    {5,5},
    {6,6},
    {7,7},
    {8,8},
    {9,9},
    {10,10},
    {11,11},
    {12,12},
    {13,13},
    {14,14},
    {15,15},
    {16,16},
    {17,17},
    {18,18},
    {19,19},
    {20,20},
    {21,21},
    {22,22},
    {23,23},
    {24,24},
    {25,25},
    {26,26},
    {27,27},
    {28,28},
    {29,29},
    {30,30},
    {31,31},
    {32,32},
    {33,33},
    {34,34},
    {35,35},
    {36,36},
    {37,37},
    {38,38},
    {39,39},
    {40,40},
    {41,41},
    {42,42},
    {43,43},
    {44,44},
    {45,45},
    {46,46},
    {47,47},
    {48,48},
    {49,49},
    {50,50},
    {51,51},
    {52,52},
    {53,53},
    {54,54},
    {55,55},
    {56,56},
    {57,57},
    {58,58},
    {59,59},
    {60,60},
    {61,61},
    {62,62},
    {63,63},
    {64,64},
    {65,65},
    {66,66},
    {67,67},
    {68,68},
    {69,69},
    {70,70},
    {71,71},
    {72,72},
    {73,73},
    {74,74},
    {75,75},
    {76,76},
    {77,77},
    {78,78},
    {79,79},
    {80,80},
    {81,81},
    {82,82},
    {83,83},
    {84,84},
    {85,85},
    {86,86},
    {87,87},
    {88,88},
    {89,89},
    {90,90},
    {91,91},
    {92,92},
    {93,93},
    {94,94},
    {95,95},
    {96,96},
    {97,97},
    {98,98},
    {99,99},
    {100,100},
    {101,101},
    {102,102},
    {103,103},
    {104,104},
    {105,105},
    {106,106},
    {107,107},
    {108,108},
    {109,109},
    {110,110},
    {111,111},
    {112,112},
    {113,113},
    {114,114},
    {115,115},
    {116,116},
    {117,117},
    {118,118},
    {119,119},
    {120,120},
    {121,121},
    {122,122},
    {123,123},
    {124,124},
    {125,125},
    {126,126},
    {127,127},
    {150,K_COMMAND},
    {151,K_CAPSLOCK},
    {153,K_PAUSE},
    {154,K_UPARROW},
    {155,K_DOWNARROW},
    {156,K_LEFTARROW},
    {157,K_RIGHTARROW},
    {158,K_ALT},
    {159,K_CTRL},
    {160,K_SHIFT},
    {161,K_INS},
    {162,K_DEL},
    {163,K_PGDN},
    {164,K_PGUP},
    {165,K_HOME},
    {166,K_END},
    {167,K_F1},
    {168,K_F2},
    {169,K_F3},
    {170,K_F4},
    {171,K_F5},
    {172,K_F6},
    {173,K_F7},
    {174,K_F8},
    {175,K_F9},
    {176,K_F10},
    {177,K_F11},
    {178,K_F12},
    {182,K_KP_HOME},
    {183,K_KP_UPARROW},
    {184,K_KP_PGUP},
    {185,K_KP_LEFTARROW},
    {186,K_KP_5},
    {187,K_KP_RIGHTARROW},
    {188,K_KP_END},
    {189,K_KP_DOWNARROW},
    {190,K_KP_PGDN},
    {191,K_KP_ENTER},
    {192,K_KP_INS},
    {193,K_KP_DEL},
    {194,K_KP_SLASH},
    {195,K_KP_MINUS},
    {196,K_KP_PLUS},
    {197,K_KP_NUMLOCK},
    {198,K_KP_STAR},
    {199,K_KP_EQUALS},
    {200,K_MOUSE1},
    {201,K_MOUSE2},
    {202,K_MOUSE3},
    {203,K_MOUSE4},
    {204,K_MOUSE5},
    {205,K_MWHEELDOWN},
    {206,K_MWHEELUP},
    {207,K_JOY1},
    {208,K_JOY2},
    {209,K_JOY3},
    {210,K_JOY4},
    {211,K_JOY5},
    {212,K_JOY6},
    {213,K_JOY7},
    {214,K_JOY8},
    {215,K_JOY9},
    {216,K_JOY10},
    {217,K_JOY11},
    {218,K_JOY12},
    {219,K_JOY13},
    {220,K_JOY14},
    {221,K_JOY15},
    {222,K_JOY16},
    {223,K_JOY17},
    {224,K_JOY18},
    {225,K_JOY19},
    {226,K_JOY20},
    {227,K_JOY21},
    {228,K_JOY22},
    {229,K_JOY23},
    {230,K_JOY24},
    {231,K_JOY25},
    {232,K_JOY26},
    {233,K_JOY27},
    {234,K_JOY28},
    {235,K_JOY29},
    {236,K_JOY30},
    {237,K_JOY31},
    {238,K_JOY32},
    {239,K_AUX1},
    {240,K_AUX2},
    {241,K_AUX3},
    {242,K_AUX4},
    {243,K_AUX5},
    {244,K_AUX6},
    {245,K_AUX7},
    {246,K_AUX8},
    {247,K_AUX9},
    {248,K_AUX10},
    {249,K_AUX11},
    {250,K_AUX12},
    {251,K_AUX13},
    {252,K_AUX14},
    {253,K_AUX15},
    {254,K_AUX16},
};
static int CL_Cod1KeyBindingPortForRetail(int retail) {
    unsigned n;for(n=0;n<sizeof(cl_cod1KeyBindingKeyMap)/sizeof(cl_cod1KeyBindingKeyMap[0]);n++)
        if(cl_cod1KeyBindingKeyMap[n].retail==retail)return cl_cod1KeyBindingKeyMap[n].port;
    return -1;
}
static int CL_Cod1KeyBindingRetailForPort(int port) {
    unsigned n;for(n=0;n<sizeof(cl_cod1KeyBindingKeyMap)/sizeof(cl_cod1KeyBindingKeyMap[0]);n++)
        if(cl_cod1KeyBindingKeyMap[n].port==port)return cl_cod1KeyBindingKeyMap[n].retail;
    return -1;
}
static void CL_Cod1KeyBindingStop(void) {
    cl_cod1KeyBindingReady=0;cl_cod1KeyBindingGeneration=0;
    cl_cod1KeyBindingOwner=NULL;cl_cod1KeyBindingServerId=0;
}
static void CL_Cod1KeyBindingStart(void) {
    int previous=cl_cod1KeyBindingLastServerId;
    CL_Cod1KeyBindingStop();
    if(cl_cod1KeyBindingGenerationCounter==UINT64_MAX)return;
    cl_cod1KeyBindingGeneration=++cl_cod1KeyBindingGenerationCounter;
    if(cl.serverId>0)cl_cod1KeyBindingLastServerId=cl.serverId;
    if(!cgvm||!cgvm->dllHandle||!cgvm->entryPoint||cl.serverId<=0||cl.serverId==previous)return;
    cl_cod1KeyBindingOwner=cgvm;cl_cod1KeyBindingServerId=cl.serverId;
    cl_cod1KeyBindingReady=1;
}
static int CL_Cod1KeyBindingDomainReady(int inspectBindings) {
    const char *locale;int n;
    if(!cl_cod1KeyBindingReady||!cl_cod1KeyBindingGeneration||!cgvm||
       cgvm!=cl_cod1KeyBindingOwner||!cgvm->dllHandle||!cgvm->entryPoint||
       cls.state!=CA_ACTIVE||!cls.cgameStarted||clc.demoplaying||
       cl.serverId!=cl_cod1KeyBindingServerId||cl.serverId<=0||
       !cl_language||cl_language->integer!=0)return 0;
    locale=setlocale(LC_CTYPE,NULL);
    if(!locale||(strcmp(locale,"C")&&strcmp(locale,"POSIX")))return 0;
    if(inspectBindings)for(n=0;n<MAX_KEYS;n++)
        if(CL_Cod1KeyBindingRetailForPort(n)<0&&keys[n].binding&&keys[n].binding[0])return 0;
    return 1;
}
static int CL_Cod1KeyBindingCommandValid(const char *command) {
    unsigned n;for(n=0;n<COD1_KEYBINDING_NAME_CAP;n++) {
        unsigned char c=(unsigned char)command[n];
        if(!c)return 1;if(c>=128)return 0;
    }return 0;
}
static int CL_Cod1KeyBindingResultValid(const cod1KeyBindingQuery_t *query) {
    if(query->cg_time<0||query->count<0||query->count>2||!query->cgame_generation||
       !memchr(query->key1,0,sizeof(query->key1))||!memchr(query->key2,0,sizeof(query->key2)))return 0;
    if(!query->count)return !strcmp(query->key1,"KEY_UNBOUND")&&!query->key2[0];
    return query->key1[0]&&(query->count==1?!query->key2[0]:query->key2[0]!=0);
}
static void *CL_Cod1KeyBindingPointer(int low,int high) {
    uint64_t raw=(uint64_t)(uint32_t)low|((uint64_t)(uint32_t)high<<32);
    uintptr_t pointer=(uintptr_t)raw;
    return (uint64_t)pointer==raw?(void *)pointer:NULL;
}
qboolean CL_Cod1KeyBindingContext(cod1KeyBindingQuery_t *query) {
    if(!query||currentVM!=cgvm||!CL_Cod1KeyBindingDomainReady(1)||
       query->abi!=COD1_KEYBINDING_WIRE_ABI||!query->operation_id||query->cg_time<0||
       !CL_Cod1KeyBindingCommandValid(query->command)||
       (query->phase!=COD1_KEYBINDING_PHASE_QUERY&&query->phase!=COD1_KEYBINDING_PHASE_VALIDATE))return qfalse;
    if(query->cgame_generation)return query->cgame_generation==cl_cod1KeyBindingGeneration?qtrue:qfalse;
    if(query->phase!=COD1_KEYBINDING_PHASE_QUERY)return qfalse;
    query->cgame_generation=cl_cod1KeyBindingGeneration;return qtrue;
}
/* GSC queries use the action names from the CoD controls menu. */
static const char *CL_Cod1ScriptBinding(const char *binding) {
    if (!binding) return NULL;
    if (!Q_stricmp(binding,"+cod1adstoggle")) return "toggle cl_run";
    if (!Q_stricmp(binding,"+cod1adshold") || !Q_stricmp(binding,"+attack2")) return "+speed";
    if (!Q_stricmp(binding,"+cod1ads"))
        return Cvar_VariableIntegerValue("cl_cod1AdsToggle") ? "toggle cl_run" : "+speed";
    /* Both of these literal engine commands control locomotion. */
    if (!Q_stricmp(binding,"+speed") || !Q_stricmp(binding,"toggle cl_run")) return NULL;
    return binding;
}
qboolean CL_Cod1KeyBindingScan(int retail,char *buffer,int capacity) {
    const char *binding;int port,n;
    if(!buffer||capacity!=COD1_KEYBINDING_BINDING_CAP||retail<0||retail>255||
       currentVM!=cgvm||!CL_Cod1KeyBindingDomainReady(0))return qfalse;
    memset(buffer,0,(size_t)capacity);port=CL_Cod1KeyBindingPortForRetail(retail);
    if(port<0)return qtrue;
    binding=CL_Cod1ScriptBinding(Key_GetBinding(port));
    if(binding)for(n=0;n<capacity-1&&binding[n];n++)buffer[n]=binding[n];
    return qtrue;
}
qboolean CL_Cod1KeyBindingQuery(cod1KeyBindingQuery_t *query) {
    cod1KeyBindingQuery_t before,reply;vm_t *owner;uint64_t generation,pointer;int accepted;
    if(!query||!CL_Cod1KeyBindingDomainReady(1)||query->abi!=COD1_KEYBINDING_WIRE_ABI||
       !query->operation_id||!CL_Cod1KeyBindingCommandValid(query->command))return qfalse;
    if(query->phase==COD1_KEYBINDING_PHASE_QUERY) {
        if(query->cg_time!=-1||query->count!=-1||query->cgame_generation||query->key1[0]||query->key2[0])return qfalse;
    } else if(query->phase!=COD1_KEYBINDING_PHASE_VALIDATE||!CL_Cod1KeyBindingResultValid(query))return qfalse;
    before=*query;reply=before;owner=cgvm;generation=cl_cod1KeyBindingGeneration;
    pointer=(uint64_t)(uintptr_t)&reply;
    accepted=VM_Call(owner,CG_COD1_KEYBINDING_QUERY,(int)(uint32_t)pointer,(int)(uint32_t)(pointer>>32));
    if(accepted!=1||!CL_Cod1KeyBindingDomainReady(1)||cgvm!=owner||
       cl_cod1KeyBindingGeneration!=generation||reply.abi!=before.abi||
       reply.phase!=before.phase||reply.operation_id!=before.operation_id||
       memcmp(reply.command,before.command,sizeof(reply.command))||
       reply.cgame_generation!=generation||!CL_Cod1KeyBindingResultValid(&reply))return qfalse;
    if(before.phase==COD1_KEYBINDING_PHASE_VALIDATE)return !memcmp(&reply,&before,sizeof(reply))?qtrue:qfalse;
    *query=reply;return qtrue;
}
