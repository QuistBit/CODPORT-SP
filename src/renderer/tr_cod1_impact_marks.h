/* SPDX-License-Identifier: GPL-3.0-or-later
 * IMPACT_R1. Compatibility projection onto static CoD triangle soups only.
 * Explicit opt-in prevents changing accepted grenade/RTCW mark behavior.
 * Geometry is clipped, not a floating square pasted in front of the wall.
 * Uses the existing bounded fragment protocol (five floats per output point).
 */
#ifndef TR_COD1_IMPACT_MARKS_H
#define TR_COD1_IMPACT_MARKS_H
static int R_Cod1ImpactMarkFragments(const vec3_t *points,const vec3_t projection,
    int maxPoints,vec3_t pointBuffer,int maxFragments,markFragment_t *fragments) {
    vec3_t normal,center,mins,maxs,normals[6],v1,v2,clips[2][MAX_VERTS_ON_POLY];
    float dists[6],depth;int i,j,k,s,pointCount=0,fragmentCount=0,first,count;
    if(!tr.world || !points || !pointBuffer || !fragments || maxPoints<3 || maxFragments<1)return 0;
    depth=VectorNormalize2(projection,normal)*0.5f;
    if(!(depth>0 && depth<=8192))return 0;
    VectorClear(center);ClearBounds(mins,maxs);
    for(i=0;i<4;i++)VectorAdd(center,points[i],center);
    VectorScale(center,.25f,center);
    for(i=0;i<4;i++) {
        vec3_t a,b;
        VectorSubtract(points[(i+1)%4],points[i],v1);
        CrossProduct(v1,normal,normals[i]);VectorNormalize(normals[i]);
        dists[i]=DotProduct(normals[i],points[i]);
        if(DotProduct(normals[i],center)<dists[i]) {VectorNegate(normals[i],normals[i]);dists[i]=-dists[i];}
        VectorMA(points[i],-depth,normal,a);VectorMA(points[i],depth,normal,b);
        AddPointToBounds(a,mins,maxs);AddPointToBounds(b,mins,maxs);
    }
    VectorCopy(normal,normals[4]);dists[4]=DotProduct(normal,center)-depth;
    VectorNegate(normal,normals[5]);dists[5]=DotProduct(normals[5],center)-depth;
    for(j=0;j<3;j++){mins[j]-=.5f;maxs[j]+=.5f;}
    first=tr.world->cod1FirstWorldSurface;count=tr.world->cod1WorldSurfaceCount;
    if(first<0||count<0||first>tr.world->numsurfaces||count>tr.world->numsurfaces-first)return 0;
    for(s=first;s<first+count;s++) {
        msurface_t *surf=&tr.world->surfaces[s];srfTriangles_t *tri;unsigned f;
        if(!surf->data||*surf->data!=SF_TRIANGLES||!surf->shader)continue;
        f=tr.world->cod1MarkFlags?tr.world->cod1MarkFlags[s]:0;
        if(f&(SURF_NOIMPACT|SURF_NOMARKS|SURF_SKY|SURF_NODRAW))continue;
        if(surf->shader->isSky||surf->shader->surfaceFlags&(SURF_NOIMPACT|SURF_NOMARKS))continue;
        tri=(srfTriangles_t *)surf->data;
        for(j=0;j<3;j++)if(tri->bounds[0][j]>maxs[j]||tri->bounds[1][j]<mins[j])break;
        if(j<3)continue;
        for(k=0;k+2<tri->numIndexes;k+=3) {
            vec3_t face,average,tmin,tmax;
            int indices[3],bad=0;
            ClearBounds(tmin,tmax);VectorClear(average);
            for(j=0;j<3;j++) {
                indices[j]=tri->indexes[k+j];
                if(indices[j]<0||indices[j]>=tri->numVerts){bad=1;break;}
                VectorCopy(tri->verts[indices[j]].xyz,clips[0][j]);
                VectorAdd(average,tri->verts[indices[j]].normal,average);
                AddPointToBounds(clips[0][j],tmin,tmax);
            }
            if(bad)continue;
            for(j=0;j<3;j++)if(tmin[j]>maxs[j]||tmax[j]<mins[j])break;
            if(j<3)continue;
            VectorSubtract(clips[0][1],clips[0][0],v1);VectorSubtract(clips[0][2],clips[0][0],v2);
            CrossProduct(v1,v2,face);if(VectorNormalize(face)<.00001f)continue;
            /* CoD winding may differ from the decoded shading normal. */
            if(DotProduct(face,average)<0)VectorNegate(face,face);
            if(DotProduct(face,normal)<.1f)continue;
            for(j=0;j<3;j++)VectorMA(clips[0][j],.05f,face,clips[0][j]);
            R_AddMarkFragments(3,clips,6,normals,dists,maxPoints,pointBuffer,
                maxFragments,fragments,&pointCount,&fragmentCount,mins,maxs);
            if(fragmentCount>=maxFragments||pointCount>maxPoints-3)return fragmentCount;
        }
    }
    return fragmentCount;
}
#endif
