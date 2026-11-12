/* Original GAME alpha20019B50 and fade2001A450, finite admitted domain.
 * Preserve float32 product and caller x87 rounding mode, like native. */
static int HudIntroQuantize(float value,float scale,int *out){
#if defined(_MSC_VER) && defined(_M_IX86)
    float product;double epsilon=9.31322574615478515625e-10;int n;
    __asm {fld value
        fmul scale
        fstp product
        fld product
        fadd epsilon
        fistp n}
    *out=n;return 1;
#else
    return 0;
#endif
}
static int HudIntroApply(cod1ScriptHud_t *state,const GscHudElemRequest *r,int now,GscHudElemResult *result){
    float v=r->number;int n;
    if(r->kind==GSC_HUDELEM_WRITE){
        if(r->value_kind!=GSC_HUDELEM_VALUE_NUMBER||!CQ_Finite(v))return 0;
        if(r->field==GSC_HUDELEM_FIELD_ALPHA){if(v<0)v=0;if(v>1)v=1;if(!HudIntroQuantize(v,255,&n))return 0;state->alpha=(uint32_t)n;}
        else if(r->field==GSC_HUDELEM_FIELD_FONT_SCALE){if(v<=0||v>16)return 0;state->font_scale=v;}
        else if(r->field==GSC_HUDELEM_FIELD_SORT){if(v< -8192||v>8192)return 0;state->sort=v;}
        else return 0;
    }else if(r->kind==GSC_HUDELEM_READ){
        result->kind=GSC_HUDELEM_RESULT_NUMBER;
        if(r->field==GSC_HUDELEM_FIELD_ALPHA)result->number=state->alpha*(1.0f/255.0f);
        else if(r->field==GSC_HUDELEM_FIELD_FONT_SCALE)result->number=state->font_scale;
        else if(r->field==GSC_HUDELEM_FIELD_SORT)result->number=state->sort;
        else return 0;
    }else if(r->kind==GSC_HUDELEM_FADE){
        if(!CQ_Finite(v)||v<=0||v>60||!HudIntroQuantize(v,1000,&n)||n<0||n>60000)return 0;
        state->from_alpha=state->alpha;state->fade_start=now;state->fade_ms=n;
    }else if(r->kind==GSC_HUDELEM_SET_SHADER){
        if(strcmp(r->text,"black")||r->width<0||r->height<0||r->width>1024||r->height>1024)return 0;
        state->content=1;state->clock=0;strcpy(state->text,"black");state->width=r->width;state->height=r->height;
    }else if(r->kind==GSC_HUDELEM_SET_TEXT){
        state->content=2;state->clock=0;state->width=state->height=0;strcpy(state->text,r->text);
    }else return 0;
    return 1;
}
