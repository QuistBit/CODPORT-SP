/* Original 418860 mesh construction, 429650 plane, 426710 normalization,
 * 4273C0/429710 edge basis. Explicit PC53 intermediates and float stores. */
#ifndef V59_MESH_BUILD_MATH_H
#define V59_MESH_BUILD_MATH_H
static float C1MLStore(double x){volatile float v=(float)x;return v;}
static double C1MLDot(const float*a,const float*b,int i,int j,int k){return ((double)a[i]*b[i]+(double)a[j]*b[j])+(double)a[k]*b[k];}
static void C1MLCross(const float*a,const float*b,float*c){int i;for(i=0;i<3;i++)c[i]=C1MLStore((double)a[(i+1)%3]*b[(i+2)%3]-(double)a[(i+2)%3]*b[(i+1)%3]);}
static float C1MLNorm(float*v){float square=C1MLStore(C1MLDot(v,v,0,1,2)),n=C1MLStore(sqrt((double)square)),inv;int i;if(n){inv=C1MLStore(1.0/n);for(i=0;i<3;i++)v[i]=C1MLStore((double)v[i]*inv);}return n;}
static void C1MLBary(const float*origin,float*u,float*q,float lu,float lv,float out[2][4]){
 float uv=C1MLStore(C1MLDot(q,u,2,1,0)),factor=C1MLStore(1.0/(1.0-(double)uv*uv));float us[3],vs[3],iu=C1MLStore(1.0/lu),iv=C1MLStore(1.0/lv);int j;
 for(j=0;j<3;j++){us[j]=C1MLStore((double)factor*u[j]);vs[j]=C1MLStore((double)factor*q[j]);}
 for(j=0;j<3;j++){out[0][j]=C1MLStore(((-(double)uv*vs[j])+us[j])*iu);out[1][j]=C1MLStore(((-(double)uv*us[j])+vs[j])*iv);}
 out[0][3]=C1MLStore(C1MLDot(out[0],origin,1,2,0));out[1][3]=C1MLStore(C1MLDot(out[1],origin,1,2,0));
}
static void C1MLBasis(const float*d,float out[2][3]){
 float axis[3]={0},inv,d0,scaled[3],least=1;int j,k=0;
 for(j=0;j<3;j++)if(fabs(d[j])<least){least=(float)fabs(d[j]);k=j;}axis[k]=1;
 inv=C1MLStore(1.0/C1MLStore(C1MLDot(d,d,0,1,2)));d0=C1MLStore(C1MLDot(axis,d,2,1,0)*inv);
 for(j=0;j<3;j++){scaled[j]=C1MLStore((double)inv*d[j]);out[0][j]=C1MLStore((double)axis[j]-(double)scaled[j]*d0);}
 C1MLNorm(out[0]);C1MLCross(d,out[0],out[1]);
}
#endif
