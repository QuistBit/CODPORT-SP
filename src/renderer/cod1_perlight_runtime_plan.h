/* R62: private source plan for the bounded native dynamic perLight replay.
 * Static passes must first satisfy the existing partial projection contract.
 * The output contains only selected authored perLight stages; it does not
 * implement the native prepared static-light producer or claim completion.
 * No material names, image lookup, renderer state or GL calls live here. */
#ifndef COD1_PERLIGHT_RUNTIME_PLAN_H
#define COD1_PERLIGHT_RUNTIME_PLAN_H
#include "cod1_perlight_static.h"

typedef struct {
    unsigned hash, omittedMask;
    int stages, needsBasis, line;
    int dlightBundle[8];
    unsigned bundleAuth[8];
    char reason[160];
} cod1PlRuntimePlan_t;

static int Cod1PLRuntime_Why(cod1PlRuntimePlan_t *plan,int line,const char *why) {
    plan->line=line;Cod1MP_Copy(plan->reason,sizeof(plan->reason),why);return 0;
}

/* Native material bit 2 is source-derived, not RTCW shader.needsNormal.
 * Environment coordinate use is sticky across selected bundles. Diffuse
 * and ambient RGB set it only when they are the final selected RGB mode.
 * Native parser/finalization proof: R62 LIGHT_LIST_CONTRACT evidence. */
static int Cod1PLRuntime_Basis(const cod1MaterialProbe_t *ir,
                               const cod1MpStage_t *stage) {
    int j,environment=0;char rgb[128];rgb[0]=0;
    for(j=0;j<stage->numOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd,arg;char line[512];
        if(!Cod1ME_Line(ir,&stage->op[j],&lx,line))continue;
        cmd=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if(Cod1ME_OneOf(cmd.text,"tcGen texGen") &&
           Cod1MP_Equal(arg.text,"environment"))environment=1;
        if(!stage->op[j].bundle && Cod1MP_Equal(cmd.text,"rgbGen"))
            Cod1MP_Copy(rgb,sizeof(rgb),arg.text);
    }
    return environment || Cod1ME_OneOf(rgb,"lightingDiffuse lightingAmbient");
}

/* The validator's ordinary image grammar deliberately rejects $dlight.
 * Replace only that already-validated final image token in an independent
 * check buffer; the emitted runtime source retains the native symbol. */
static int Cod1PLRuntime_CheckLine(const char *line,char *out,size_t cap,
                                   size_t *used) {
    cod1MpLexer_t lx;cod1MpToken_t cmd,arg;int axes=C1MAP_REPEAT;
    unsigned flags=0;
    memset(&lx,0,sizeof(lx));lx.base=line;lx.cursor=line;lx.line=1;
    cmd=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
    if(Cod1MP_Equal(cmd.text,"map")) {
        while(C1Map_Option(arg.text,&axes,&flags))arg=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(arg.text,"$dlight")) {
            static const char token[]="__cod1_runtime_dlight__";
            return Cod1ST_Append(out,cap,used,line,(size_t)(arg.start-line)) &&
                   Cod1ST_Append(out,cap,used,token,sizeof(token)-1) &&
                   Cod1ST_Append(out,cap,used,arg.end,strlen(arg.end)) &&
                   Cod1ST_Append(out,cap,used,"\n",1);
        }
    }
    return Cod1ST_Append(out,cap,used,line,strlen(line)) &&
           Cod1ST_Append(out,cap,used,"\n",1);
}

static int Cod1PLRuntime_Prepare(const char *body,const cod1MpCaps_t *caps,
                                  char *out,size_t cap,cod1PlRuntimePlan_t *plan) {
    cod1PlStaticResult_t projected;cod1PlRuntimePlan_t candidate;
    cod1MaterialProbe_t *ir=NULL,*checked=NULL;
    char *scratch=NULL,*checkBody=NULL;char line[512],reason[80];
    size_t used=0,checkUsed=0;int i,j,n=0,ok=0;
    if(out && cap)out[0]=0;
    if(!plan)return 0;
    memset(plan,0,sizeof(*plan));memset(&candidate,0,sizeof(candidate));
    if(!body || !caps || !out || !cap || caps->units<1)
        return Cod1PLRuntime_Why(plan,0,"invalid runtime plan input/profile");
    scratch=(char*)calloc(1,COD1_ST_BODY);
    checkBody=(char*)calloc(1,COD1_ST_BODY);
    ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));
    checked=(cod1MaterialProbe_t*)calloc(1,sizeof(*checked));
    if(!scratch || !checkBody || !ir || !checked) {
        Cod1PLRuntime_Why(plan,0,"runtime plan allocation failed");goto done;
    }
    if(!Cod1PL_Prepare(body,caps,scratch,COD1_ST_BODY,&projected)) {
        Cod1PLRuntime_Why(plan,projected.line,projected.reason);goto done;
    }
    if(!Cod1MP_Parse(body,caps,ir)) {
        Cod1PLRuntime_Why(plan,0,ir->error);goto done;
    }
    candidate.hash=projected.hash;candidate.omittedMask=projected.omitted;
    for(j=0;j<ir->topOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd,arg;double value;
        if(!Cod1ME_Line(ir,&ir->topOp[j],&lx,line))goto invalid;
        cmd=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"sort") && Cod1MP_Number(arg.text,&value) &&
           !(value>=-FLT_MAX && value<=FLT_MAX)) {
            Cod1PLRuntime_Why(plan,0,"nonfinite/out-of-range sort unsupported");goto done;
        }
    }
    if(!Cod1ST_Append(out,cap,&used,"{\n",2) ||
       !Cod1ST_Append(checkBody,COD1_ST_BODY,&checkUsed,"{\n",2))goto full;
    for(i=0;i<ir->numStages;++i) {
        const cod1MpStage_t *stage=&ir->stage[i];
        reason[0]=0;
        /* Native requires placement is lexical even on an excluded stage. */
        if(!Cod1ST_RequiresPlaced(ir,stage,reason)) {
            Cod1PLRuntime_Why(plan,stage->line,reason);goto done;
        }
        if(stage->selected==COD1_MP_FALSE)continue;
        candidate.needsBasis|=Cod1PLRuntime_Basis(ir,stage);
        if(!(projected.omitted&(1u<<i))) {
            int multiplyIdentity;
            if(!Cod1PL_CheckStaticStage(ir,stage,caps->units,&multiplyIdentity,reason)) {
                Cod1PLRuntime_Why(plan,stage->line,reason);goto done;
            }
            continue;
        }
        if(n>=8) {Cod1PLRuntime_Why(plan,stage->line,"too many perlight stages");goto done;}
        candidate.dlightBundle[n]=stage->numBundles-1;
        candidate.bundleAuth[n]=Cod1ST_BundleAuth(ir,stage);
        if(!Cod1ST_Append(out,cap,&used,"{\n",2) ||
           !Cod1ST_Append(checkBody,COD1_ST_BODY,&checkUsed,"{\n",2))goto full;
        for(j=0;j<stage->numOps;++j) {
            cod1MpLexer_t lx;cod1MpToken_t cmd;
            if(!Cod1ME_Line(ir,&stage->op[j],&lx,line))goto invalid;
            cmd=Cod1MP_Next(&lx);
            if(Cod1ME_OneOf(cmd.text,"requires perlight"))continue;
            if(!Cod1ST_Append(out,cap,&used,line,strlen(line)) ||
               !Cod1ST_Append(out,cap,&used,"\n",1) ||
               !Cod1PLRuntime_CheckLine(line,checkBody,COD1_ST_BODY,&checkUsed))goto full;
        }
        if(!Cod1ST_Append(out,cap,&used,"}\n",2) ||
           !Cod1ST_Append(checkBody,COD1_ST_BODY,&checkUsed,"}\n",2))goto full;
        ++n;
    }
    if(!Cod1ST_Append(out,cap,&used,"}\n",2) ||
       !Cod1ST_Append(checkBody,COD1_ST_BODY,&checkUsed,"}\n",2))goto full;
    if(!n || !Cod1MP_Parse(checkBody,caps,checked) || checked->numStages!=n)goto invalid;
    for(i=0;i<n;++i) {
        reason[0]=0;
        if(!Cod1ST_CheckStageAuth(checked,&checked->stage[i],caps->units,
                                 candidate.bundleAuth[i],1,reason)) {
            Cod1PLRuntime_Why(plan,checked->stage[i].line,reason);goto done;
        }
    }
    candidate.stages=n;*plan=candidate;ok=1;goto done;
invalid: Cod1PLRuntime_Why(plan,0,"runtime source validation failed");goto done;
full: Cod1PLRuntime_Why(plan,0,"runtime source exceeds output limit");
done:
    if(!ok)out[0]=0;
    free(scratch);free(checkBody);free(ir);free(checked);return ok;
}
#endif
