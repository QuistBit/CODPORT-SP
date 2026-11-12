/* PATCH65: map-driven static XModels, retaining native float geometry.
 * Bake misc_model visuals once at map load. Collision is loaded separately.
 * R_LoadCod1StaticTrees remains the entry point for existing tr_bsp.c callers. */
#include "cod1_viewmodel14.h"
#include "tr_cod1_lighting.h"
#include <limits.h>
#include "cod1_precalc.h"

typedef struct {
    vec3_t origin, angles, scale, light;
    char model[MAX_QPATH];
    int misc, valid;
    int sourceIndex, hasPrecalc; /* R46: diagnostics only; no protocol fields. */
} cod1StaticEntity;

static int R_Cod1StaticPath(char *out,const char *prefix,const char *name) {
    size_t a=strlen(prefix),b=strlen(name);
    if(!b || a+b>=MAX_QPATH || strstr(name,"..") || strchr(name,'\\') || strchr(name,':'))return 0;
    memcpy(out,prefix,a);memcpy(out+a,name,b+1);return 1;
}

static int R_Cod1StaticEntity(char **text,cod1StaticEntity *e) {
    char key[MAX_TOKEN_CHARS],value[MAX_TOKEN_CHARS];char *t;int k,vectorScaleSet=0;
    memset(e,0,sizeof(*e));e->valid=1;e->sourceIndex=-1;
    for(k=0;k<3;k++){e->scale[k]=1;e->light[k]=1;}
    t=COM_ParseExt(text,qtrue);if(strcmp(t,"{"))return 0;
    while(*text){
        t=COM_ParseExt(text,qtrue);if(!strcmp(t,"}"))return 1;
        if(!t[0])return 0;Q_strncpyz(key,t,sizeof(key));
        t=COM_ParseExt(text,qtrue);if(!t[0] || !strcmp(t,"}"))return 0;Q_strncpyz(value,t,sizeof(value));
        if(!Q_stricmp(key,"classname"))e->misc=!Q_stricmp(value,"misc_model");
        else if(!Q_stricmp(key,"model")){
            if(Q_stricmpn(value,"xmodel/",7) || !R_Cod1StaticPath(e->model,"xmodel/",value+7))e->valid=0;
        }else if(!Q_stricmp(key,"origin"))e->valid&=sscanf(value,"%f %f %f",&e->origin[0],&e->origin[1],&e->origin[2])==3;
        else if(!Q_stricmp(key,"angles"))e->valid&=sscanf(value,"%f %f %f",&e->angles[0],&e->angles[1],&e->angles[2])==3;
        else if(!Q_stricmp(key,"angle"))e->angles[1]=(float)atof(value);
        /* The per-axis value takes precedence regardless of BSP key order.
         * Treating both keys as writes to the same vector made later scalar
         * keys inflate some training trees by a factor of 3.5. */
        else if(!Q_stricmp(key,"modelscale")){
            if(!vectorScaleSet)e->scale[0]=e->scale[1]=e->scale[2]=(float)atof(value);
        }else if(!Q_stricmp(key,"modelscale_vec")){
            vectorScaleSet=1;
            e->valid&=sscanf(value,"%f %f %f",&e->scale[0],&e->scale[1],&e->scale[2])==3;
        }
        else if(!Q_stricmp(key,"lightingPrecalc")){e->hasPrecalc=1;e->valid&=sscanf(value,"%f %f %f",&e->light[0],&e->light[1],&e->light[2])==3;}
    }
    return 0;
}
static int R_Cod1StaticValid(const cod1StaticEntity *e) {
    int k;if(!e->valid || !e->model[0])return 0;
    for(k=0;k<3;k++)if(!(e->origin[k]>=-1000000 && e->origin[k]<=1000000) ||
        !(e->angles[k]>=-360000 && e->angles[k]<=360000) ||
        !(e->scale[k]>=0.001f && e->scale[k]<=1000) ||
        !(e->light[k]>=0 && e->light[k]<=1000))return 0;
    return 1;
}
static int R_Cod1StaticCompare(const void *a,const void *b) {
    return Q_stricmp(((const cod1StaticEntity*)a)->model,((const cod1StaticEntity*)b)->model);
}
/* Preserve the existing tree-only debug switch without restricting new models. */
static int R_Cod1StaticLegacyTree(const char *name) {
    return !Q_stricmp(name,"xmodel/TallSkinnyTreeA") || !Q_stricmp(name,"xmodel/HurtgenTallA") ||
           !Q_stricmp(name,"xmodel/LargeHeavyTreeA");
}
static int R_Cod1StaticMesh(const char *name,xm14Model *model,xm14Mesh *mesh,int *posed,const char **stage) {
    void *parts=NULL,*surfs=NULL,*data=NULL;char path[MAX_QPATH];int n,pn,sn,ok=0,rigid;
    cv14Skeleton *sk=NULL;cv14Transform root;int i;
    memset(mesh,0,sizeof(*mesh));*posed=0;*stage="xmodel";
    n=ri.FS_ReadFile(name,&data);
    if(n<=0 || !data || !XM14_Model(data,(size_t)n,model))goto done;
    ri.FS_FreeFile(data);data=NULL;*stage="xmodelparts";
    if(!R_Cod1StaticPath(path,"xmodelparts/",model->lod[0]))goto done;
    pn=ri.FS_ReadFile(path,&parts);if(pn<=0 || !parts)goto done;
    *stage="xmodelsurfs";if(!R_Cod1StaticPath(path,"xmodelsurfs/",model->lod[0]))goto done;
    sn=ri.FS_ReadFile(path,&surfs);if(sn<=0 || !surfs)goto done;
    *stage="v14 mesh/bind skeleton";
    rigid=XM14_Parts(parts,(size_t)pn);
    if(rigid && XM14_Mesh(surfs,(size_t)sn,mesh)){
        /* XM14's original static decoder uses the opposite winding. */
        int j;for(i=0;i<mesh->ns;i++)for(j=0;j<mesh->s[i].nt*3;j+=3){
            unsigned short swap=mesh->s[i].idx[j+1];mesh->s[i].idx[j+1]=mesh->s[i].idx[j+2];mesh->s[i].idx[j+2]=swap;
        }
    }else{
        sk=(cv14Skeleton*)calloc(1,sizeof(*sk));if(!sk)goto done;
        if(!CV14_Parts(parts,(size_t)pn,sk))goto done;
        CV14_Identity(&root);
        for(i=0;i<sk->n;i++)CV14_Compose(sk->parent[i]<0?&root:&sk->world[sk->parent[i]],&sk->local[i],&sk->world[i]);
        if(!CV14_Mesh(surfs,(size_t)sn,sk,mesh))goto done;*posed=1;
    }
    *stage="surface/material count";if(mesh->ns!=model->ns[0])goto done;
    for(i=0;i<mesh->ns;i++)if(!R_Cod1StaticPath(path,"skins/",model->skin[0][i]))goto done;
    ok=1;
done:
    if(data)ri.FS_FreeFile(data);if(parts)ri.FS_FreeFile(parts);if(surfs)ri.FS_FreeFile(surfs);free(sk);
    if(!ok)XM14_Free(mesh);return ok;
}

/* Registration/bake diagnostics, not a capture of the final GPU fragment.
 * Log every skin once per model type, and every instance once after baking.
 * This connects the image path to the same entity's source and final RGB. */
static void R_Cod1StaticReportSkin(const char *model,int surface,const shader_t *sh) {
    int s,b;
    if(!c1LightTrace)return;
    ri.Printf(PRINT_ALL,
        "CoD1 static material: model=%s meshSurface=%d shader=%s default=%d route=%s lm=%d passes=%d sort=%g\n",
        model,surface,sh?sh->name:"<null>",sh?sh->defaultShader:1,
        sh?R_Cod1MaterialRouteName(sh->cod1MaterialRoute):"unresolved",
        sh?sh->lightmapIndex:0,sh?sh->numUnfoggedPasses:0,sh?sh->sort:0.0f);
    if(!sh)return;
    if(sh->cod1ShaderType)ri.Printf(PRINT_ALL,"CoD1 static type source: path=%s hash=%08x bakedRole=%d\n",
        sh->cod1ShaderTypePath,sh->cod1ShaderTypeHash,sh->cod1StaticModelSource);
    for(s=0;s<sh->numUnfoggedPasses && s<MAX_SHADER_STAGES;s++){
        const shaderStage_t *stage=sh->stages[s];
        if(!stage)continue;
        for(b=0;b<NUM_TEXTURE_BUNDLES;b++){
            const image_t *image=stage->bundle[b].image[0];
            if(!image)continue;
            ri.Printf(PRINT_ALL,
                "CoD1 static stage: model=%s meshSurface=%d shader=%s stage=%d bundle=%d image=%s texnum=%u size=%dx%d upload=%dx%d format=%x lightmap=%d rgbGen=%d alphaGen=%d state=%08x\n",
                model,surface,sh->name,s,b,image->imgName,(unsigned)image->texnum,
                image->width,image->height,image->uploadWidth,image->uploadHeight,
                image->internalFormat,stage->bundle[b].isLightmap,
                stage->rgbGen,stage->alphaGen,stage->stateBits);
        }
    }
}

static void R_Cod1StaticReportBake(const cod1StaticEntity *e,const xm14Mesh *mesh,
                                  const msurface_t *surfaces,const float *fallback) {
    int i,j,k,n=0,black=0,nearBlack=0,lo[3]={255,255,255},hi[3]={0,0,0};
    if(!c1LightTrace)return;
    for(i=0;i<mesh->ns;i++){
        const srfTriangles_t *tri=(const srfTriangles_t*)surfaces[i].data;
        for(j=0;j<tri->numVerts;j++){
            const byte *rgb=tri->verts[j].color;int peak=0;
            for(k=0;k<3;k++){
                if(rgb[k]<lo[k])lo[k]=rgb[k];if(rgb[k]>hi[k])hi[k]=rgb[k];
                if(rgb[k]>peak)peak=rgb[k];
            }
            n++;if(!peak)black++;if(peak<=2)nearBlack++;
        }
        if (surfaces[i].shader && !surfaces[i].shader->defaultShader) {
            int stage;
            for(stage=0;stage<surfaces[i].shader->numUnfoggedPasses;stage++)
                if(surfaces[i].shader->stages[stage]->rgbGen==CGEN_COD1_PRECALC) {
                    ri.Printf(PRINT_ALL,"CoD1 R52 precalc: entity=%d model=%s surface=%d shader=%s source=(%.9g %.9g %.9g) identity=%.9g packed=(%u %u %u %u) valid=%d\n",
                        e->sourceIndex,e->model,i,surfaces[i].shader->name,
                        e->light[0],e->light[1],e->light[2],tr.identityLight,
                        tri->cod1PrecalcRGBA[0],tri->cod1PrecalcRGBA[1],
                        tri->cod1PrecalcRGBA[2],tri->cod1PrecalcRGBA[3],tri->cod1PrecalcValid);
                    break;
                }
        }
    }
    ri.Printf(PRINT_ALL,
        "CoD1 static RGB: entity=%d model=%s origin=(%.6g %.6g %.6g) precalcPresent=%d precalc=(%.6g %.6g %.6g) native=%d reason=%s centerCoverage=%.6g debugWhite=%d verts=%d zero=%d nearBlackLE2=%d min=(%d %d %d) max=(%d %d %d)\n",
        e->sourceIndex,e->model,e->origin[0],e->origin[1],e->origin[2],e->hasPrecalc,
        e->light[0],e->light[1],e->light[2],c1Lighting,c1LightInitReason,fallback[2],
        c1StaticLightDebug,n,black,nearBlack,lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]);
}

static void R_Cod1StaticBake(const cod1StaticEntity *e,const xm14Mesh *mesh,shader_t **skins,
                             const vec3_t localBounds[2],msurface_t *out,vec3_t bounds[2]) {
    vec3_t axis[3],sample,tight[2],point;float fallback[3]={0,0,0};int i,j,k,q;
    c1mModelLightFrame frame;cod1ModelLightSource_t *source=NULL;
    msurface_t *first=out;
    (void)localBounds;memset(&frame,0,sizeof(frame));
    AnglesToAxis(e->angles,axis);
    /* Native cache bounds are extrema of transformed actual vertices. */
    ClearBounds(tight[0],tight[1]);
    for(i=0;i<mesh->ns;i++)for(j=0;j<mesh->s[i].nv;j++) {
        for(k=0;k<3;k++) {
            point[k]=e->origin[k];
            for(q=0;q<3;q++)point[k]+=mesh->s[i].v[j].xyz[q]*e->scale[q]*axis[q][k];
        }
        AddPointToBounds(point,tight[0],tight[1]);
    }
    for(k=0;k<3;k++)sample[k]=0.5f*(tight[0][k]+tight[1][k]);
    if(c1Lighting) {
        source=R_Cod1ModelSource(sample,&frame);
        if(source)fallback[2]=source->candidates.coverage;
        if(c1LightTrace)ri.Printf(PRINT_ALL,
            "CoD1 R72 model prepare: entity=%d center=(%.9g %.9g %.9g) prepared=%d candidates=%d selected=%d sky=%.6g coverage=%.6g\n",
            e->sourceIndex,sample[0],sample[1],sample[2],frame.valid,
            source?source->candidates.count:0,frame.prepared.count,
            source?source->candidates.remainingSky:0,fallback[2]);
    }
    for(i=0;i<mesh->ns;i++,out++){
        const xm14Surface *src=&mesh->s[i];srfTriangles_t *tri;
        memset(out,0,sizeof(*out));out->shader=skins[i];
        tri=ri.Hunk_Alloc(sizeof(*tri)+src->nv*sizeof(drawVert_t)+src->nt*3*sizeof(int),h_low);
        memset(tri,0,sizeof(*tri));tri->surfaceType=SF_TRIANGLES;tri->numVerts=src->nv;tri->numIndexes=src->nt*3;
        C1Precalc_Pack(e->light,tr.identityLight,tri->cod1PrecalcRGBA);
        tri->cod1PrecalcValid=qtrue;tri->cod1ModelLightSource=source;
        tri->cod1ModelLightBakedValid=source && frame.valid && !c1StaticLightDebug;
        if(source)tri->cod1ModelLightColors=R_Cod1ModelColorCacheAlloc(src->nv);
        tri->verts=(drawVert_t*)(tri+1);tri->indexes=(int*)(tri->verts+src->nv);out->data=(surfaceType_t*)tri;
        ClearBounds(tri->bounds[0],tri->bounds[1]);
        for(j=0;j<src->nv;j++){
            drawVert_t *v=&tri->verts[j];memset(v,0,sizeof(*v));
            for(k=0;k<3;k++){
                v->xyz[k]=e->origin[k];v->normal[k]=0;
                for(q=0;q<3;q++){
                    v->xyz[k]+=src->v[j].xyz[q]*e->scale[q]*axis[q][k];
                    v->normal[k]+=src->v[j].normal[q]/e->scale[q]*axis[q][k];
                }
                v->color[k]=(byte)(255*(e->light[k]>1?1:e->light[k]));
            }
            VectorNormalize(v->normal);v->color[3]=255;
            if(frame.valid && R_Cod1ShadePreparedVertex(&frame,v->xyz,v->normal,v->color))c1LitVerts++;
            else {tri->cod1ModelLightBakedValid=qfalse;c1UncachedVerts++;R_Cod1LightVertex(v,e->light,fallback);}
            if(c1StaticLightDebug)for(k=0;k<3;k++)v->color[k]=255;
            v->st[0]=src->v[j].uv[0];v->st[1]=src->v[j].uv[1];
            AddPointToBounds(v->xyz,tri->bounds[0],tri->bounds[1]);AddPointToBounds(v->xyz,bounds[0],bounds[1]);
        }
        for(j=0;j<tri->numIndexes;j++)tri->indexes[j]=src->idx[j];
    }
    R_Cod1StaticReportBake(e,mesh,first,fallback);
}

static void R_LoadCod1StaticTrees(const cod1Bsp_t *bsp,vec3_t bounds[2]) {
    const byte *ent;char *text,*copy;int len,n=0,capacity=0,invalid=0,disabled=0,i,j,k,end,count,posed,ns=0,scap=0;
    int added=0,missing=0,types=0,failed=0,old=s_worldData.numsurfaces,entityIndex=0;
    cod1StaticEntity e,*entities=NULL;msurface_t *pending=NULL,*all;xm14Model *model=NULL;xm14Mesh mesh;
    shader_t *skins[XM14_SURFS];vec3_t localBounds[2];char path[MAX_QPATH];const char *stage;
    cvar_t *enabled=ri.Cvar_Get("r_cod1StaticModels","1",CVAR_ARCHIVE);
    cvar_t *trees=ri.Cvar_Get("r_cod1StaticTrees","1",CVAR_ARCHIVE);
    R_Cod1LightingInit(bsp);if(!enabled->integer){R_Cod1LightingFinish();return;}
    ent=COD1_BSP_LumpData(bsp,COD1_BSP_LUMP_ENTITIES,&len);if(!ent || len<1 || ent[len-1]){R_Cod1LightingFinish();return;}
    copy=(char*)malloc(len);if(!copy){R_Cod1LightingFinish();ri.Error(ERR_DROP,"CoD1 static models: entity allocation failed");return;}
    memcpy(copy,ent,len);text=copy;
    while(R_Cod1StaticEntity(&text,&e)){
        e.sourceIndex=entityIndex++;
        if(!e.misc)continue;if(!R_Cod1StaticValid(&e)){invalid++;continue;}
        if(!trees->integer && R_Cod1StaticLegacyTree(e.model)){disabled++;continue;}
        if(n==capacity){
            cod1StaticEntity *grown;int next;
            if(capacity>INT_MAX/2)goto allocation_failed;next=capacity?capacity*2:128;
            if(next>INT_MAX/(int)sizeof(*entities))goto allocation_failed;
            grown=(cod1StaticEntity*)realloc(entities,next*sizeof(*entities));if(!grown)goto allocation_failed;
            entities=grown;capacity=next;
        }
        entities[n++]=e;
    }
    free(copy);copy=NULL;if(n>1)qsort(entities,n,sizeof(*entities),R_Cod1StaticCompare);
    model=(xm14Model*)calloc(1,sizeof(*model));if(n && !model)goto allocation_failed;
    for(i=0;i<n;i=end){
        for(end=i+1;end<n && !Q_stricmp(entities[i].model,entities[end].model);end++);count=end-i;
        if(!R_Cod1StaticMesh(entities[i].model,model,&mesh,&posed,&stage)){
            ri.Printf(PRINT_WARNING,"CoD1 static model: %s unavailable/unsupported at %s; skipped %d instances\n",entities[i].model,stage,count);
            missing+=count;failed++;continue;
        }
        if(mesh.ns>(INT_MAX-ns)/count || old>INT_MAX-ns-count*mesh.ns ||
           old+ns+count*mesh.ns>INT_MAX/(int)sizeof(*pending)){XM14_Free(&mesh);goto allocation_failed;}
        if(ns+count*mesh.ns>scap){
            msurface_t *grown;int next=ns+count*mesh.ns;
            grown=(msurface_t*)realloc(pending,next*sizeof(*pending));if(!grown){XM14_Free(&mesh);goto allocation_failed;}
            pending=grown;scap=next;
        }
        ClearBounds(localBounds[0],localBounds[1]);
        for(j=0;j<mesh.ns;j++){
            R_Cod1StaticPath(path,"skins/",model->skin[0][j]);skins[j]=R_FindCod1StaticModelShader(path,qtrue);
            if(skins[j]->defaultShader)ri.Printf(PRINT_WARNING,"CoD1 static model: %s unresolved/unsupported skin %s\n",entities[i].model,path);
            R_Cod1StaticReportSkin(entities[i].model,j,skins[j]);
            for(k=0;k<mesh.s[j].nv;k++)AddPointToBounds(mesh.s[j].v[k].xyz,localBounds[0],localBounds[1]);
        }
        for(j=i;j<end;j++){R_Cod1StaticBake(&entities[j],&mesh,skins,localBounds,pending+ns,bounds);ns+=mesh.ns;}
        ri.Printf(PRINT_ALL,"CoD1 static model: %s: %d instances, %d surfaces (%s, LOD0, visual only)\n",
                  entities[i].model,count,count*mesh.ns,posed?"bind pose":"rigid");
        added+=count;types++;XM14_Free(&mesh);
    }
    if(ns){
        all=ri.Hunk_Alloc((old+ns)*sizeof(*all),h_low);
        if(old)memcpy(all,s_worldData.surfaces,old*sizeof(*all));memcpy(all+old,pending,ns*sizeof(*all));
        s_worldData.surfaces=all;s_worldData.numsurfaces=old+ns;
    }
    ri.Printf(PRINT_ALL,"CoD1 static models: %d instances/%d types loaded, %d surfaces; %d instances/%d types missing or unsupported, %d invalid, %d disabled\n",
              added,types,ns,missing,failed,invalid,disabled);
    if(c1Lighting)ri.Printf(PRINT_ALL,"CoD1 R73 model lighting (cached static colors): queries=%u candidateLists=%u unavailable=%u; %d vertices shaded, %d vertices using incomplete legacy fallback\n",c1ModelQueryAttempts,c1ModelPrepared,c1ModelUnavailable,c1LitVerts,c1UncachedVerts);
    R_Cod1LightingFinish();free(pending);free(model);free(entities);return;
allocation_failed:
    R_Cod1LightingFinish();free(copy);free(pending);free(model);free(entities);ri.Error(ERR_DROP,"CoD1 static models: allocation/size limit");
}
