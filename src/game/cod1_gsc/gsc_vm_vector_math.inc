/* R198 pure finite math is portable restoration policy, not retail FPU parity.
 * Double intermediates avoid overflow for finite vector inputs whose final
 * binary32 result is representable. Normalizing zero returns the zero vector.
 * Trigonometric scalar angles use degrees. vectoangles keeps the engine's
 * float intermediate and branch conventions, including zero -> (-270,0,0).
 * Every output is staged, including when it aliases an input argument. */
int gsc_runtime_select_vector_math(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->vector_math_enabled||!r->program->vector_math_selected||!r->entities_enabled||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->vector_math_enabled=1;return 1;
}
static int vector_math_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    Value result;double x[3],sum=0,value,sp,cp,sy,cy,sr,cr,pitch,yaw,roll;float scalar;int k;
    if(!r->vector_math_enabled||!out||!args||argc!=vector_math_arity(kind)||!argc)
        return rfail(r,line,"invalid selected vector math capability or arity");
    if(kind>=VECTOR_MATH_SIN&&kind<=VECTOR_MATH_ASIN) {
        if(!scalar_float(r,&args[0],line,&scalar))return 0;
    } else for(k=0;k<argc;k++)if(!vector_arithmetic_finite(r,&args[k],line))return 0;
    memset(&result,0,sizeof(result));
    if(kind>=VECTOR_MATH_SIN&&kind<=VECTOR_MATH_ASIN) {
        const double radians=0.017453292519943295769236907684886;
        result.kind=V_NUMBER;
        if(kind==VECTOR_MATH_ASIN) {
            if(scalar<-1||scalar>1)return rfail(r,line,"asin input outside [-1,1]");
            result.number=(float)(asin((double)scalar)/radians);
        } else {
            value=fmod((double)scalar,360.0)*radians;
            result.number=(float)(kind==VECTOR_MATH_SIN?sin(value):cos(value));
        }
    } else if(kind==VECTOR_MATH_TO_ANGLES) {
        /* Match q_math.c vectoangles, including float forward/pitch/yaw.
         * Finite extreme coordinates may overflow forward exactly as in that
         * engine routine; atan2 still produces its finite orientation. */
        const float pi=3.14159265358979323846f; /* q_shared.h M_PI */
        float forward,vector_yaw,vector_pitch;
        const float *v=args[0].vector;
        if(v[1]==0&&v[0]==0) {
            vector_yaw=0;vector_pitch=v[2]>0?90.0f:270.0f;
        } else {
            if(v[0])vector_yaw=(float)(atan2(v[1],v[0])*180/pi);
            else vector_yaw=v[1]>0?90.0f:270.0f;
            if(vector_yaw<0)vector_yaw+=360;
            forward=(float)sqrt(v[0]*v[0]+v[1]*v[1]);
            vector_pitch=(float)(atan2(v[2],forward)*180/pi);
            if(vector_pitch<0)vector_pitch+=360;
        }
        result.kind=V_VECTOR;
        result.vector[0]=-vector_pitch;result.vector[1]=vector_yaw;result.vector[2]=0;
    } else if(kind>=VECTOR_MATH_FORWARD&&kind<=VECTOR_MATH_UP) {
        const double radians=0.017453292519943295769236907684886;
        pitch=fmod((double)args[0].vector[0],360.0)*radians;
        yaw=fmod((double)args[0].vector[1],360.0)*radians;
        roll=fmod((double)args[0].vector[2],360.0)*radians;
        sp=sin(pitch);cp=cos(pitch);sy=sin(yaw);cy=cos(yaw);sr=sin(roll);cr=cos(roll);
        if(kind==VECTOR_MATH_FORWARD){x[0]=cp*cy;x[1]=cp*sy;x[2]=-sp;}
        else if(kind==VECTOR_MATH_RIGHT){x[0]=-sr*sp*cy+cr*sy;x[1]=-sr*sp*sy-cr*cy;x[2]=-sr*cp;}
        else{x[0]=cr*sp*cy+sr*sy;x[1]=cr*sp*sy-sr*cy;x[2]=cr*cp;}
        result.kind=V_VECTOR;for(k=0;k<3;k++)result.vector[k]=(float)x[k];
    } else if(kind==VECTOR_MATH_DOT) {
        for(k=0;k<3;k++)sum+=(double)args[0].vector[k]*(double)args[1].vector[k];
        result.kind=V_NUMBER;result.number=(float)sum;
    } else if(kind==VECTOR_MATH_DISTANCE_SQUARED) {
        /* Original GAME 20034E70 stores each difference as binary32, then
         * reduces Z/Y/X. Keep the portable finite double accumulator; this
         * does not claim all x87 precision/control-word/exception parity. */
        volatile float delta[3];
        for(k=0;k<3;k++)
            delta[k]=(float)((double)args[0].vector[k]-(double)args[1].vector[k]);
        sum=(double)delta[2]*(double)delta[2]+(double)delta[1]*(double)delta[1];
        sum+=(double)delta[0]*(double)delta[0];
        result.kind=V_NUMBER;result.number=(float)sum;
    } else {
        for(k=0;k<3;k++) {
            x[k]=(double)args[0].vector[k];
            if(kind==VECTOR_MATH_DISTANCE||kind==VECTOR_MATH_DISTANCE_SQUARED)x[k]-=(double)args[1].vector[k];
            sum+=x[k]*x[k];
        }
        if(kind==VECTOR_MATH_NORMALIZE) {
            result.kind=V_VECTOR;value=sqrt(sum);
            if(value!=0)for(k=0;k<3;k++)result.vector[k]=(float)(x[k]/value);
        } else {
            result.kind=V_NUMBER;
            result.number=(float)((kind==VECTOR_MATH_LENGTH_SQUARED||kind==VECTOR_MATH_DISTANCE_SQUARED)?sum:sqrt(sum));
        }
    }
    if(result.kind==V_VECTOR) {
        for(k=0;k<3;k++)if(!isfinite(result.vector[k]))return rfail(r,line,"nonfinite vector math result");
    } else if(!isfinite(result.number))return rfail(r,line,"vector math result exceeds finite binary32 range");
    *out=result;return 1;
}
