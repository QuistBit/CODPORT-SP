/* First native collision-brush bridge: axis-aligned, six-sided world ladders.
 * IBSP59: brush sides lump 3 (8 bytes), brushes lump 4 (u16 count/material).
 * The first six sides store bounds as floats; later sides use plane indices.
 * Only six-sided ladders are handled here. No guessed boxes for sloped brushes. */
static int COD1_LadderTriangles(const cod1Bsp_t*bsp,cod1CollisionTri_t*out,int capacity){
 const byte *brushes,*sides,*materials,*models;int bl,sl,ml,modl,i,j,k,axis,face,part,written=0;
 unsigned first,count,side=0,n,mat;vec3_t lo,hi,vertices[8];
 static const int faces[6][4]={{0,2,6,4},{1,5,7,3},{0,4,5,1},{2,3,7,6},{0,1,3,2},{4,6,7,5}};
 brushes=COD1_BSP_LumpData(bsp,4,&bl);sides=COD1_BSP_LumpData(bsp,3,&sl);
 materials=COD1_BSP_LumpData(bsp,0,&ml);models=COD1_BSP_LumpData(bsp,27,&modl);
 if(!brushes||!sides||!materials||!models||bl%4||sl%8||modl<48)return 0;
 first=COD1_CollisionReadLE32(models+40);count=COD1_CollisionReadLE32(models+44);
 if(first>(unsigned)(bl/4)||count>(unsigned)(bl/4)-first)return 0;
 for(i=0;i<(int)(first+count);i++){
  n=COD1_CollisionReadLE16(brushes+i*4);mat=COD1_CollisionReadLE16(brushes+i*4+2);
  if(side>(unsigned)(sl/8)||n>(unsigned)(sl/8)-side)return -1;
  if(i>=(int)first && n==6 && mat<(unsigned)(ml/72) &&
     (COD1_CollisionReadLE32(materials+mat*72+64)&8)){
   for(axis=0;axis<3;axis++){
    lo[axis]=COD1_CollisionFloat(sides+(side+axis*2)*8);
    hi[axis]=COD1_CollisionFloat(sides+(side+axis*2+1)*8);
    if(!(lo[axis]>-FLT_MAX&&hi[axis]<FLT_MAX&&lo[axis]<hi[axis]))return -1;
   }
   if(out){
    if(written>capacity-12)return -1;
    for(j=0;j<8;j++)for(k=0;k<3;k++)vertices[j][k]=(j&(1<<k))?hi[k]:lo[k];
    for(face=0;face<6;face++)for(part=0;part<2;part++){
     cod1CollisionTri_t*t=&out[written++];memset(t,0,sizeof(*t));
     VectorCopy(vertices[faces[face][0]],t->a);
     VectorCopy(vertices[faces[face][part?2:1]],t->b);
     VectorCopy(vertices[faces[face][part?3:2]],t->c);
     t->normal[face/2]=(face&1)?1:-1;t->contents=CONTENTS_SOLID;t->surfaceFlags=SURF_LADDER;t->cod1SurfaceFlags=COD1_CollisionReadLE32(materials+mat*72+64);t->cod1SurfaceValid=1;
     for(k=0;k<3;k++){
      t->mins[k]=t->maxs[k]=t->a[k];
      if(t->b[k]<t->mins[k])t->mins[k]=t->b[k];if(t->c[k]<t->mins[k])t->mins[k]=t->c[k];
      if(t->b[k]>t->maxs[k])t->maxs[k]=t->b[k];if(t->c[k]>t->maxs[k])t->maxs[k]=t->c[k];
     }
    }
   }else written+=12;
  }
  side+=n;
 }
 return written;
}
