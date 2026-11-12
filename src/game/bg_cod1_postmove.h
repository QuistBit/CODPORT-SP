/* PATCH59: generic native Pmove tail 0x30007b42 / 0x20008493. Preserve PC24
 * arithmetic order when testing displacement against half the final speed. */
static float CMove_Add(float a,float b){volatile float r=a+b;return r;}
static float CMove_Mul(float a,float b){volatile float r=a*b;return r;}
static float CMove_Div(float a,float b){volatile float r=a/b;return r;}
static void PM_Cod1PostMove(void) {
    vec3_t delta;
    float actual,expected,scale;
    int i;
    if(!BG_Cod1PlayerMovementActive(pm))return;
    VectorSubtract(pm->ps->origin,pml.previous_origin,delta);
    actual=CMove_Add(CMove_Add(CMove_Mul(delta[2],delta[2]),
        CMove_Mul(delta[1],delta[1])),CMove_Mul(delta[0],delta[0]));
    actual=CMove_Div(actual,CMove_Mul(pml.frametime,pml.frametime));
    expected=CMove_Add(CMove_Add(CMove_Mul(pm->ps->velocity[0],pm->ps->velocity[0]),
        CMove_Mul(pm->ps->velocity[1],pm->ps->velocity[1])),
        CMove_Mul(pm->ps->velocity[2],pm->ps->velocity[2]));
    expected=CMove_Mul(expected,.25f);
    if(actual<expected) {
        scale=CMove_Div(1.0f,pml.frametime);
        for(i=0;i<3;i++)pm->ps->velocity[i]=CMove_Mul(delta[i],scale);
    }
}
