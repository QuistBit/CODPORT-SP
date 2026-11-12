/* Vertical capsule versus triangle soups, swept by conservative advancement.
 * Exact segment/triangle distance; no discrete sampling along the movement. */
static float C26Clamp(float x){return x<0?0:(x>1?1:x);}
static void C26PointTri(const vec3_t p,const cod1CollisionTri_t*t,vec3_t q){
 vec3_t ab,ac,ap,bp,cp;float d1,d2,d3,d4,d5,d6,va,vb,vc,v,w,den;
 VectorSubtract(t->b,t->a,ab);VectorSubtract(t->c,t->a,ac);VectorSubtract(p,t->a,ap);
 d1=DotProduct(ab,ap);d2=DotProduct(ac,ap);
 if(d1<=0&&d2<=0){VectorCopy(t->a,q);return;}
 VectorSubtract(p,t->b,bp);d3=DotProduct(ab,bp);d4=DotProduct(ac,bp);
 if(d3>=0&&d4<=d3){VectorCopy(t->b,q);return;}
 vc=d1*d4-d3*d2;
 if(vc<=0&&d1>=0&&d3<=0){v=d1/(d1-d3);VectorMA(t->a,v,ab,q);return;}
 VectorSubtract(p,t->c,cp);d5=DotProduct(ab,cp);d6=DotProduct(ac,cp);
 if(d6>=0&&d5<=d6){VectorCopy(t->c,q);return;}
 vb=d5*d2-d1*d6;
 if(vb<=0&&d2>=0&&d6<=0){w=d2/(d2-d6);VectorMA(t->a,w,ac,q);return;}
 va=d3*d6-d5*d4;
 if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0){w=(d4-d3)/((d4-d3)+(d5-d6));VectorSubtract(t->c,t->b,ac);VectorMA(t->b,w,ac,q);return;}
 den=1/(va+vb+vc);v=vb*den;w=vc*den;
 VectorMA(t->a,v,ab,q);VectorMA(q,w,ac,q);
}
static void C26Segments(const vec3_t a,const vec3_t b,const vec3_t c,const vec3_t d,vec3_t p,vec3_t q){
 vec3_t u,v,r;float aa,ee,bb,cc,ff,den,s,t;
 VectorSubtract(b,a,u);VectorSubtract(d,c,v);VectorSubtract(a,c,r);
 aa=DotProduct(u,u);ee=DotProduct(v,v);ff=DotProduct(v,r);
 if(aa<1e-12f){s=0;t=ee>1e-12f?C26Clamp(ff/ee):0;}
 else{cc=DotProduct(u,r);if(ee<1e-12f){t=0;s=C26Clamp(-cc/aa);}
 else{bb=DotProduct(u,v);den=aa*ee-bb*bb;s=den>1e-12f?C26Clamp((bb*ff-cc*ee)/den):0;t=(bb*s+ff)/ee;
 if(t<0){t=0;s=C26Clamp(-cc/aa);}else if(t>1){t=1;s=C26Clamp((bb-cc)/aa);}}}
 VectorMA(a,s,u,p);VectorMA(c,t,v,q);
}
static float C26Distance(const vec3_t a,const vec3_t b,const cod1CollisionTri_t*t,vec3_t normal){
 vec3_t p,q,d,best,ab;float best2=FLT_MAX,n2,da,db,f;int i;
 /* A segment can pierce a triangle without approaching any edge. */
 VectorSubtract(a,t->a,d);da=DotProduct(d,t->normal);VectorSubtract(b,t->a,d);db=DotProduct(d,t->normal);
 if((da<=0&&db>=0)||(da>=0&&db<=0)){
  if(fabs(da-db)>1e-10f){f=da/(da-db);VectorSubtract(b,a,ab);VectorMA(a,f,ab,p);C26PointTri(p,t,q);VectorSubtract(p,q,d);
   if(DotProduct(d,d)<1e-8f){VectorCopy(t->normal,normal);return 0;}}
 }
 for(i=0;i<5;i++){
  if(i<2){VectorCopy(i?a:b,p);C26PointTri(p,t,q);}
  else{const float*c=i==2?t->a:(i==3?t->b:t->c);const float*e=i==2?t->b:(i==3?t->c:t->a);C26Segments(a,b,c,e,p,q);}
  VectorSubtract(p,q,d);n2=DotProduct(d,d);if(n2<best2){best2=n2;VectorCopy(d,best);}
 }
 f=(float)sqrt(best2);
 if(f>1e-7f){
  float alignment;
  VectorScale(best,1/f,normal);
  /* Subtracting large world coordinates introduces tiny tangential components
   * into a face contact normal. They must not block a tangent sweep at t=0:
   * PM_StepSlideMove otherwise mistakes a vertical wall for a landing plane.
   * Preserve edge/corner normals except within this near-parallel tolerance. */
  alignment=DotProduct(normal,t->normal);
  if(fabs(alignment)>0.999999f) VectorScale(t->normal,alignment<0?-1.0f:1.0f,normal);
 }else VectorCopy(t->normal,normal);
 return f;
}
void COD1_CollisionTraceShape(const cod1CollisionTri_t*tris,int count,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,trace_t*out,int capsule){
 vec3_t center,delta,extent,low,high,a,b,normal,p;float radius,half,dist,closing,time,step;int i,j,it;
 if(!capsule){COD1_CollisionTrace(tris,count,start,end,mins,maxs,out);return;}
 for(j=0;j<3;j++){extent[j]=(maxs[j]-mins[j])*.5f;center[j]=start[j]+(maxs[j]+mins[j])*.5f;delta[j]=end[j]-start[j];low[j]=center[j]+(delta[j]<0?delta[j]:0)-extent[j]-.125f;high[j]=center[j]+(delta[j]>0?delta[j]:0)+extent[j]+.125f;}
 radius=extent[0];if(extent[1]<radius)radius=extent[1];if(extent[2]<radius)radius=extent[2];
 if(radius<=0){COD1_CollisionTrace(tris,count,start,end,mins,maxs,out);return;}
 half=extent[2]-radius;memset(out,0,sizeof(*out));out->fraction=1;VectorCopy(end,out->endpos);
 for(i=0;i<count;i++){
  const cod1CollisionTri_t*t=&tris[i];
  for(j=0;j<3;j++)if(high[j]<t->mins[j]||low[j]>t->maxs[j])break;if(j!=3)continue;
  time=0;
  for(it=0;it<64;it++){
   VectorMA(center,time,delta,a);VectorCopy(a,b);a[2]-=half;b[2]+=half;
   dist=C26Distance(a,b,t,normal);closing=-DotProduct(delta,normal);
   if(time==0 && dist<radius-.001f){
    out->startsolid=qtrue;VectorAdd(a,delta,a);VectorAdd(b,delta,b);
    if(C26Distance(a,b,t,p)<radius-.001f){out->allsolid=qtrue;out->fraction=0;out->contents=t->contents;}
    break;
   }
   if(closing<=1e-6f)break;
   if(dist-radius<=.1251f || it==63){
    if(time<out->fraction){out->fraction=time;VectorCopy(normal,out->plane.normal);out->plane.dist=DotProduct(normal,t->a);out->plane.type=PlaneTypeForNormal(normal);out->plane.signbits=0;for(j=0;j<3;j++)if(normal[j]<0)out->plane.signbits|=1<<j;out->surfaceFlags=t->surfaceFlags;out->plane.pad[0]=out->plane.pad[1]=0;if(t->cod1SurfaceValid)CI_TraceSurface(out,t->cod1SurfaceFlags);out->contents=t->contents;}
    break;
   }
   step=(dist-radius-.125f)/closing;time+=step;
   if(time>out->fraction||time>1)break;
  }
 }
 for(j=0;j<3;j++)out->endpos[j]=start[j]+out->fraction*delta[j];
}
