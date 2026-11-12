/* PATCH64: unanimated XModel v14 for handheld pickups and projectiles.
 * Uses the bind skeleton and the existing temporary MD3 mesh adapter. */
static qboolean R_LoadCod1WorldModel(model_t *mod,const char *name) {
    xm14Model *model=NULL;cv14Skeleton *sk=NULL;cv14Pose *pose=NULL;xm14Mesh mesh;
    cv14Transform root;md3Header_t *h;md3Frame_t *fr;md3Surface_t *surf;
    int i,j,k,size;char texture[MAX_QPATH];qboolean ok=qfalse;memset(&mesh,0,sizeof(mesh));
    model=(xm14Model*)calloc(1,sizeof(*model));sk=(cv14Skeleton*)calloc(1,sizeof(*sk));pose=(cv14Pose*)calloc(1,sizeof(*pose));
    if(!model || !sk || !pose)goto done;CV14_Identity(&root);
    if(!R_CV14Part(name,pose,&root,model,sk,&mesh,NULL))goto done;
    size=sizeof(*h)+sizeof(*fr);
    for(i=0;i<mesh.ns;i++) {
        if(!CW_ViewPath(texture,model->skin[0][i],"skins/",0))goto done;
        for(j=0;j<mesh.s[i].nv;j++)for(k=0;k<3;k++)if(!(mesh.s[i].v[j].xyz[k]>=-512 && mesh.s[i].v[j].xyz[k]<512))goto done;
        size+=R_CV14SurfaceSize(&mesh.s[i]);
    }
    h=(md3Header_t*)ri.Hunk_Alloc(size,h_low);memset(h,0,size);
    h->ident=MD3_IDENT;h->version=MD3_VERSION;Q_strncpyz(h->name,mod->name,sizeof(h->name));
    h->numFrames=1;h->numSurfaces=mesh.ns;h->ofsFrames=sizeof(*h);h->ofsTags=h->ofsSurfaces=sizeof(*h)+sizeof(*fr);h->ofsEnd=size;
    fr=(md3Frame_t*)((byte*)h+h->ofsFrames);for(k=0;k<3;k++){fr->bounds[0][k]=999;fr->bounds[1][k]=-999;}
    surf=(md3Surface_t*)((byte*)h+h->ofsSurfaces);
    for(i=0;i<mesh.ns;i++) {
        xm14Surface *s=&mesh.s[i];md3Shader_t *material;md3Triangle_t *tri;md3St_t *st;md3XyzNormal_t *xyz;shader_t *shader;
        surf->ident=SF_MD3;surf->numFrames=surf->numShaders=1;surf->numVerts=s->nv;surf->numTriangles=s->nt;
        Com_sprintf(surf->name,sizeof(surf->name),"cod1_world_%d",i);surf->ofsShaders=sizeof(*surf);
        surf->ofsTriangles=surf->ofsShaders+sizeof(*material);surf->ofsSt=surf->ofsTriangles+s->nt*sizeof(*tri);
        surf->ofsXyzNormals=surf->ofsSt+s->nv*sizeof(*st);surf->ofsEnd=R_CV14SurfaceSize(s);
        material=(md3Shader_t*)((byte*)surf+surf->ofsShaders);CW_ViewPath(texture,model->skin[0][i],"skins/",0);
        Q_strncpyz(material->name,texture,sizeof(material->name));shader=R_FindShader(texture,LIGHTMAP_NONE,qtrue);material->shaderIndex=shader->defaultShader?0:shader->index;
        tri=(md3Triangle_t*)((byte*)surf+surf->ofsTriangles);st=(md3St_t*)((byte*)surf+surf->ofsSt);xyz=(md3XyzNormal_t*)((byte*)surf+surf->ofsXyzNormals);
        for(j=0;j<s->nt;j++)for(k=0;k<3;k++)tri[j].indexes[k]=s->idx[j*3+k];
        for(j=0;j<s->nv;j++) {
            for(k=0;k<2;k++)st[j].st[k]=s->v[j].uv[k];
            for(k=0;k<3;k++){float v=s->v[j].xyz[k];xyz[j].xyz[k]=(short)(v/MD3_XYZ_SCALE);if(v<fr->bounds[0][k])fr->bounds[0][k]=v;if(v>fr->bounds[1][k])fr->bounds[1][k]=v;}
            xyz[j].normal=R_CV14Normal(s->v[j].normal);
        }
        surf=(md3Surface_t*)((byte*)surf+surf->ofsEnd);
    }
    fr->radius=RadiusFromBounds(fr->bounds[0],fr->bounds[1]);mod->type=MOD_MESH;mod->numLods=1;mod->dataSize=size;
    for(i=0;i<MD3_MAX_LODS;i++)mod->md3[i]=i?NULL:h;ok=qtrue;
done:
    XM14_Free(&mesh);free(model);free(sk);free(pose);return ok;
}
