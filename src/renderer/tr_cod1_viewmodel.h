/* WeaponDef resource paths -> native XModel/XAnim -> temporary MD3.
 * PATCH64 adds bolt, segmented reload and held-grenade clips. */
#ifndef TR_COD1_VIEWMODEL_H
#define TR_COD1_VIEWMODEL_H
#include "cod1_viewmodel14.h"
#include "tr_cod1_viewtags.h"
#define COD1_WEAPONDEF_IMPLEMENTATION
#include "../game/bg_cod1_weapondef.h"
#include "../game/bg_cod1_viewprofile.h"

/* BREN_VIEWMODEL_HOTFIX_R2: keep the renderer failure reason available to
 * cgame so the modal error does not hide the only useful diagnostic. */
static char r_cod1ViewFailure[256];
static void R_Cod1ViewFailureClear(void) {
    r_cod1ViewFailure[0]=0;
    ri.Cvar_Set("r_cod1ViewFail","");
}
static void R_Cod1ViewFailureSet(const char *model,const char *clip,int sample,const char *stage) {
    Com_sprintf(r_cod1ViewFailure,sizeof(r_cod1ViewFailure),"%s: %s frame %d at %s",
        model&&*model?model:"cod1view",clip&&*clip?clip:"<none>",sample,stage&&*stage?stage:"unknown");
    ri.Cvar_Set("r_cod1ViewFail",r_cod1ViewFailure);
}
static void R_Cod1ViewWarningSet(const char *text) { ri.Cvar_Set("r_cod1ViewWarn",text?text:""); }

/* A second hand selection may reuse a completed MD3 only when every input
 * that could change it still resolves to exactly the same bytes. Receipts
 * are optional, small, renderer-owned and disappear with the model registry. */
#ifdef _MSC_VER
#include <float.h>
#include <xmmintrin.h>
#endif
typedef struct {
    char path[MAX_QPATH];
    byte *data;
    int size;
} cv709File;
typedef struct {
    cv709File files[CW_VIEW_CLIPS+9];
    int count,bytes,refused,index;
    unsigned fp,mxcsr;
    char weapon[CW_VIEW_PATH],hands[MAX_QPATH];
    cwViewProfile key;
    const model_t *model;
} cv709Receipt;

static cv709Receipt cv709Saved[8];
static int cv709Count;

static void R_CV14ReleaseReceipt(cv709Receipt *r) {
    int i;
    for(i=0;i<r->count;i++)free(r->files[i].data);
    memset(r,0,sizeof(*r));
}
static void R_CV14ForgetEquivalences(void) {
    while(cv709Count)R_CV14ReleaseReceipt(&cv709Saved[--cv709Count]);
}
static void R_CV14CaptureFile(cv709Receipt *r,const char *path,int size,const void *data) {
    int i;cv709File *f;
    if(!r||r->refused)return;
    if(!data||size<=0||strlen(path)>=MAX_QPATH){r->refused=1;return;}
    for(i=0;i<r->count;i++)if(!strcmp(r->files[i].path,path)){
        f=&r->files[i];
        if(size!=f->size||memcmp(f->data,data,size))r->refused=1;
        return;
    }
    /* Bound retained bytes across eight default weapons to two MiB. */
    if(r->count==sizeof(r->files)/sizeof(r->files[0])||size>256*1024-r->bytes){r->refused=1;return;}
    f=&r->files[r->count];f->data=malloc(size);
    if(!f->data){r->refused=1;return;}
    memcpy(f->data,data,size);strcpy(f->path,path);f->size=size;r->bytes+=size;r->count++;
}
static int R_CV14SourceKey(cv709Receipt *r,const cwViewProfile *p,const char *weapon) {
#ifdef _MSC_VER
#ifdef _M_IX86
    unsigned x87=0;
    if(!__control87_2(0,0,&x87,NULL))return 0;
    r->fp=x87&(_MCW_PC|_MCW_RC|_MCW_EM|_MCW_DN|_MCW_IC);
#else
    r->fp=0;
#endif
    r->mxcsr=_mm_getcsr()&0xffc0u;
    r->key=*p;memset(r->key.hands,0,sizeof(r->key.hands));r->key.selectedHands=0;
    strcpy(r->weapon,weapon);
    return CW_ViewPath(r->hands,p->hands,"xmodel/",0);
#else
    return 0;
#endif
}

/* Raw geometry, parts and clips are immutable inputs to a single viewmodel
 * bake. Release FS temporary memory immediately, including on allocation
 * failure. The usual readers still decode the live pose for each frame. */
typedef struct {
    char path[MAX_QPATH];
    byte *data;
    int size;
    cv709Receipt *receipt;
} cv14BakeFile;

static int R_CV14ReadInput(cv14BakeFile *files,unsigned index,const char *path,void **data) {
    int size;
    byte *copy;
    cv14BakeFile *slot=files?&files[index]:NULL;
    if(slot && slot->data && !strcmp(slot->path,path)){
        *data=slot->data;
        return slot->size;
    }
    size=ri.FS_ReadFile(path,data);
    if(slot)R_CV14CaptureFile(slot->receipt,path,size,*data);
    if(!slot || !*data || size<=0 || size>1024*1024 || strlen(path)>=sizeof(slot->path) || strstr(path,".cfg"))return size;
    copy=malloc((size_t)size+1);
    if(!copy)return size;
    memcpy(copy,*data,size);
    copy[size]=0;
    ri.FS_FreeFile(*data);
    free(slot->data);
    strcpy(slot->path,path);
    slot->data=copy;
    slot->size=size;
    *data=copy;
    return size;
}

static void R_CV14ReleaseInput(const cv14BakeFile *files,unsigned index,void *data) {
    if(data && (!files || data!=files[index].data))ri.FS_FreeFile(data);
}

static int R_CV14PoseInput(const char *path,cv14Pose *p,float sample,cv14BakeFile *file) {
    void *data=NULL;int n=R_CV14ReadInput(file,0,path,&data),ok=0;
    if(n>0 && data)ok=CV14_SamplePose(data,(size_t)n,sample,p);
    R_CV14ReleaseInput(file,0,data);return ok;
}
static int R_CV14Pose(const char *path,cv14Pose *p,float sample) {
    return R_CV14PoseInput(path,p,sample,NULL);
}
static int R_CV14PartInput(const char *name,const cv14Pose *pose,const cv14Transform *root,
                     xm14Model *model,cv14Skeleton *sk,xm14Mesh *mesh,const char *parentTag,cv14BakeFile *files) {
    char path[MAX_QPATH];void *data=NULL;int n,ok;
    if(!CW_ViewPath(path,name,"xmodel/",0))return 0;n=R_CV14ReadInput(files,0,path,&data);
    ok=n>0 && data && XM14_Model(data,(size_t)n,model);R_CV14ReleaseInput(files,0,data);if(!ok)return 0;data=NULL;
    if(!CW_ViewPath(path,model->lod[0],"xmodelparts/",0))return 0;n=R_CV14ReadInput(files,1,path,&data);
    ok=n>0 && data && CV14_Parts(data,(size_t)n,sk);R_CV14ReleaseInput(files,1,data);if(!ok)return 0;data=NULL;
    if(parentTag)CV14_ApplyAttachedPose(sk,pose,root,parentTag);
    else CV14_ApplyPose(sk,pose,root);
    if(!CW_ViewPath(path,model->lod[0],"xmodelsurfs/",0))return 0;n=R_CV14ReadInput(files,2,path,&data);
    ok=n>0 && data && CV14_Mesh(data,(size_t)n,sk,mesh);R_CV14ReleaseInput(files,2,data);
    return ok && model->ns[0]==mesh->ns;
}
static int R_CV14Part(const char *name,const cv14Pose *pose,const cv14Transform *root,
                     xm14Model *model,cv14Skeleton *sk,xm14Mesh *mesh,const char *parentTag) {
    return R_CV14PartInput(name,pose,root,model,sk,mesh,parentTag,NULL);
}
/* KAR98_CLIP_R1: a narrow compatibility visibility rule for the retail
 * non-scoped Kar98k. Its fire/lastshot tracks park the reload clip to the left,
 * where the port's wider view exposes it. This is NOT a reconstruction of a
 * generic retail hide-parts mechanism. Never change the view/FOV, skeleton,
 * attachments or reload poses to hide this prop.
 *
 * The supplied retail surface is rigidly attached to "k98 clip", uses only
 * metal@k98clip.dds, and has 46 vertices / 38 triangles. Restrict the rule to
 * this known asset layout; unfamiliar/custom layouts remain untouched.
 * Collapse at the part's own first vertex rather than at the camera origin.
 * All vertices coincide in both adjacent baked frames, so MD3 interpolation
 * cannot turn the hidden part into a visible triangle between those frames.
 */
static int R_CV14ViewPartVisibility(const cwViewProfile *profile,const char *clip,
                                  const xm14Model *model,const cv14Skeleton *sk,
                                  xm14Mesh *mesh) {
    int i,j,part=-1,bone=-1;float anchor[3];xm14Surface *surface;
    if(strcmp(profile->gun,"viewmodel_kar98k") ||
       (strcmp(clip,"xanim/viewmodel_kar98_fire") &&
        strcmp(clip,"xanim/viewmodel_kar98_lastshot")))return 0;
    if(strcmp(model->lod[0],"viewmodel_kar982") || model->ns[0]!=mesh->ns)return 0;
    for(i=0;i<sk->n;i++)if(!strcmp(sk->name[i],"k98 clip")){bone=i;break;}
    if(bone<0)return 0;
    for(i=0;i<mesh->ns;i++)if(!strcmp(model->skin[0][i],"metal@k98clip.dds")) {
        if(part>=0)return 0; /* Ambiguous material ownership: do not guess. */
        part=i;
    }
    if(part<0)return 0;surface=&mesh->s[part];
    if(surface->nv!=46 || surface->nt!=38 || !surface->v || !surface->idx)return 0;
    for(j=0;j<3;j++)anchor[j]=surface->v[0].xyz[j];
    for(i=0;i<surface->nv;i++)for(j=0;j<3;j++)surface->v[i].xyz[j]=anchor[j];
    return 1;
}
/* Stock animations move hidden clips/thrown parts far outside the view.
 * MD3 stores 1/64-unit signed shorts. Collapse each connected triangle island
 * that leaves that range, instead of overflowing it or rejecting the weapon.
 * This is only the temporary first-person adapter, never the world mesh. */
static void R_CV14HideFarParts(xm14Mesh *mesh) {
    int parent[CV14_VERTS],hidden[CV14_VERTS],i,j,k,a,b,c;xm14Surface *s;
    for(i=0;i<mesh->ns;i++) {
        s=&mesh->s[i];memset(hidden,0,s->nv*sizeof(int));
        for(j=0;j<s->nv;j++)parent[j]=j;
        for(j=0;j<s->nt;j++)for(k=1;k<3;k++) {
            a=s->idx[j*3];b=s->idx[j*3+k];
            while(parent[a]!=a)a=parent[a];while(parent[b]!=b)b=parent[b];
            if(a!=b)parent[b]=a;
        }
        for(j=0;j<s->nv;j++) {
            a=j;while(parent[a]!=a)a=parent[a];b=j;
            while(parent[b]!=b){c=parent[b];parent[b]=a;b=c;}
            for(k=0;k<3;k++)if(s->v[j].xyz[k]<-500 || s->v[j].xyz[k]>500)hidden[a]=1;
        }
        for(j=0;j<s->nv;j++)if(hidden[parent[j]])memset(s->v[j].xyz,0,sizeof(s->v[j].xyz));
    }
}
static int R_CV14SurfaceSize(const xm14Surface *s) {
    return sizeof(md3Surface_t)+sizeof(md3Shader_t)+s->nt*sizeof(md3Triangle_t)+s->nv*(sizeof(md3St_t)+sizeof(md3XyzNormal_t));
}
static short R_CV14Normal(const float *n) {
    float length=(float)sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]),z;int lat,lng;
    if(length<0.00001f)return 0;z=n[2]/length;if(z>1)z=1;if(z< -1)z=-1;
    lat=(int)(atan2(n[1],n[0])*255.0/(2.0*M_PI));lng=(int)(acos(z)*255.0/(2.0*M_PI));
    return (short)(((lat&255)<<8)|(lng&255));
}
static qboolean R_LoadCod1ViewFrame(model_t *mod,const cwViewProfile *profile,const char *clip,int sample,cv14BakeFile *files) {
    cv14Pose *idle=NULL,*hip=NULL;cv14Skeleton *sk=NULL;xm14Model *models=NULL;xm14Mesh mesh[2];
    cv14Transform root,attachment;md3Tag_t tags[CV14_WEAPON_TAGS];int numTags;md3Header_t *header;md3Frame_t *frame;md3Surface_t *surf;
    md3Shader_t *material;md3Triangle_t *tri;md3St_t *st;md3XyzNormal_t *xyz;shader_t *shader;
    int size,i,j,k,q,found=0,surfaces=0,verts=0,tris=0;char texture[MAX_QPATH];const char *stage="allocation";
    qboolean ok=qfalse;int knownHands=!profile->selectedHands && (!strcmp(profile->hands,"viewmodel_hands_new") || !strcmp(profile->hands,"viewmodel_hands_sten"));memset(mesh,0,sizeof(mesh));
    idle=(cv14Pose*)calloc(1,sizeof(*idle));hip=(cv14Pose*)calloc(1,sizeof(*hip));
    sk=(cv14Skeleton*)calloc(1,sizeof(*sk));models=(xm14Model*)calloc(2,sizeof(*models));
    if(!idle || !hip || !sk || !models)goto done;
    stage="xanim idle";if(!R_CV14PoseInput(clip,idle,(float)sample,files))goto done;
    CV14_Identity(&root);
    if(*profile->hip) {
        stage="xanim hip offset";if(!R_CV14PoseInput(profile->hip,hip,0,files+1))goto done;
        i=CV14_Find(hip,"tag_torso");if(i<0)goto done;root=hip->t[i];
    }
    /* Non-ADS clips contain their own tag_torso track; apply it only once. */
    stage="hands";
    if(!R_CV14PartInput(profile->hands,idle,&root,&models[0],sk,&mesh[0],NULL,files+2))goto done;
    for(i=0;i<sk->n;i++)if(!strcmp(sk->name[i],"tag_weapon")){attachment=sk->world[i];found=1;break;}
    if(!found)goto done;stage="weapon";
    if(!R_CV14PartInput(profile->gun,idle,&attachment,&models[1],sk,&mesh[1],"tag_weapon",files+5))goto done;
    /* Skeleton transforms are already in the SAME posed space as the mesh.
     * Export before reusing/freeing the skeleton, without reapplying ADS or
     * tag_weapon. Missing optional tags do not disable an otherwise valid gun. */
    stage="weapon attachments";numTags=R_CV14ExportTags(sk,tags);if(numTags<0)goto done;
    R_CV14ViewPartVisibility(profile,clip,&models[1],sk,&mesh[1]);
    R_CV14HideFarParts(&mesh[0]);R_CV14HideFarParts(&mesh[1]);
    /* Retail hands' placeholder material is replaced by the player's hand and
     * sleeve materials. Limit this explicit mapping to this known hand model. */
    if(knownHands && mesh[0].ns!=4)goto done;
    stage="MD3 geometry/material limits";
    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++) {
        if((i || !knownHands) && !CW_ViewPath(texture,models[i].skin[0][j],"skins/",0))goto done;
        for(k=0;k<mesh[i].s[j].nv;k++)for(q=0;q<3;q++)
            if(!(mesh[i].s[j].v[k].xyz[q]>=-512 && mesh[i].s[j].v[k].xyz[q]<512))goto done;
    }
    size=sizeof(md3Header_t)+sizeof(md3Frame_t)+numTags*sizeof(md3Tag_t);
    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++){
        size+=R_CV14SurfaceSize(&mesh[i].s[j]);surfaces++;verts+=mesh[i].s[j].nv;tris+=mesh[i].s[j].nt;
    }
    header=(md3Header_t*)calloc(1,size);if(!header)goto done;
    header->ident=MD3_IDENT;header->version=MD3_VERSION;Q_strncpyz(header->name,mod->name,sizeof(header->name));
    header->numFrames=1;header->numSurfaces=surfaces;header->ofsFrames=sizeof(*header);
    header->numTags=numTags;header->ofsTags=sizeof(*header)+sizeof(*frame);
    header->ofsSurfaces=header->ofsTags+numTags*sizeof(md3Tag_t);header->ofsEnd=size;
    if(numTags)memcpy((byte*)header+header->ofsTags,tags,numTags*sizeof(md3Tag_t));
    frame=(md3Frame_t*)((byte*)header+header->ofsFrames);Q_strncpyz(frame->name,"cod1_idle",sizeof(frame->name));
    for(k=0;k<3;k++){frame->bounds[0][k]=999;frame->bounds[1][k]=-999;}
    surf=(md3Surface_t*)((byte*)header+header->ofsSurfaces);
    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++){
        xm14Surface *src=&mesh[i].s[j];surf->ident=SF_MD3;surf->numFrames=1;surf->numShaders=1;
        Com_sprintf(surf->name,sizeof(surf->name),"cod1_%s_%d",i?"weapon":"hands",j);
        surf->numVerts=src->nv;surf->numTriangles=src->nt;surf->ofsShaders=sizeof(*surf);
        surf->ofsTriangles=surf->ofsShaders+sizeof(*material);surf->ofsSt=surf->ofsTriangles+src->nt*sizeof(*tri);
        surf->ofsXyzNormals=surf->ofsSt+src->nv*sizeof(*st);surf->ofsEnd=R_CV14SurfaceSize(src);
        material=(md3Shader_t*)((byte*)surf+surf->ofsShaders);
        if(!i && knownHands)Q_strncpyz(texture,j<2?"skins/viewhands@hand.dds":"skins/viewsleeves_new.tga",sizeof(texture));
        else CW_ViewPath(texture,models[i].skin[0][j],"skins/",0);
        Q_strncpyz(material->name,texture,sizeof(material->name));shader=R_FindShader(texture,LIGHTMAP_NONE,qtrue);
        material->shaderIndex=shader->defaultShader?0:shader->index;
        if(shader->defaultShader)ri.Printf(PRINT_WARNING,"CoD1 viewmodel: missing material %s\n",texture);
        tri=(md3Triangle_t*)((byte*)surf+surf->ofsTriangles);st=(md3St_t*)((byte*)surf+surf->ofsSt);
        xyz=(md3XyzNormal_t*)((byte*)surf+surf->ofsXyzNormals);
        for(k=0;k<src->nt;k++)for(q=0;q<3;q++)tri[k].indexes[q]=src->idx[k*3+q];
        for(k=0;k<src->nv;k++){
            for(q=0;q<2;q++)st[k].st[q]=src->v[k].uv[q];
            for(q=0;q<3;q++){
                float value=src->v[k].xyz[q];xyz[k].xyz[q]=(short)(value/MD3_XYZ_SCALE);
                if(value<frame->bounds[0][q])frame->bounds[0][q]=value;if(value>frame->bounds[1][q])frame->bounds[1][q]=value;
            }
            xyz[k].normal=R_CV14Normal(src->v[k].normal);
        }
        surf=(md3Surface_t*)((byte*)surf+surf->ofsEnd);
    }
    frame->radius=RadiusFromBounds(frame->bounds[0],frame->bounds[1]);
    mod->type=MOD_MESH;mod->numLods=1;mod->dataSize=size;
    for(i=0;i<MD3_MAX_LODS;i++)mod->md3[i]=i?NULL:header;

    ok=qtrue;
done:
    for(i=0;i<2;i++)XM14_Free(&mesh[i]);free(idle);free(hip);free(sk);free(models);
    if(!ok) {
        R_Cod1ViewFailureSet(mod?mod->name:NULL,clip,sample,stage);
        ri.Printf(PRINT_WARNING,"CoD1 viewmodel: failed %s frame %d at %s; viewmodel disabled\n",clip,sample,stage);
    }
    return ok;
}
/* BREN_VIEWMODEL_HOTFIX_R3: bake one temporary pose at a time.  The old
 * implementation kept a complete one-frame MD3 for every animation frame and
 * tried to enforce a 32 MiB temporary-cache budget with
 *     poseSize <= 32 MiB / totalFrames
 * This was only an average-size approximation and rejected a valid heavy frame
 * immediately (Bren idle frame 0) even though the merged MD3 stores static
 * triangles/UVs/materials only once.  Stream dynamic frame/tag/xyz data into
 * the final MD3 instead; peak temporary memory is now one baked pose. */
#define COD1_VIEWMODEL_FINAL_LIMIT (64u*1024u*1024u)
static md3Surface_t *R_CV14SurfaceAt(md3Header_t *h,int index) {
    md3Surface_t *s=(md3Surface_t*)((byte*)h+h->ofsSurfaces);int i;
    for(i=0;i<index;i++)s=(md3Surface_t*)((byte*)s+s->ofsEnd);return s;
}
static const md3Surface_t *R_CV14SurfaceAtConst(const md3Header_t *h,int index) {
    const md3Surface_t *s=(const md3Surface_t*)((const byte*)h+h->ofsSurfaces);int i;
    for(i=0;i<index;i++)s=(const md3Surface_t*)((const byte*)s+s->ofsEnd);return s;
}
static qboolean R_CV14MergedLayoutMatch(const md3Header_t *base,const md3Header_t *pose) {
    int i;
    if(!base||!pose||base->numSurfaces!=pose->numSurfaces||base->numTags!=pose->numTags)return qfalse;
    if(!R_CV14TagsMatch(base,pose))return qfalse;
    for(i=0;i<base->numSurfaces;i++) {
        const md3Surface_t *a=R_CV14SurfaceAtConst(base,i),*b=R_CV14SurfaceAtConst(pose,i);
        if(a->numVerts!=b->numVerts||a->numTriangles!=b->numTriangles||a->numShaders!=b->numShaders||a->ofsXyzNormals!=b->ofsXyzNormals)return qfalse;
    }
    return qtrue;
}
static qboolean R_CV14MergedCopyFrame(md3Header_t *out,const md3Header_t *pose,int frameIndex) {
    md3Frame_t *frames;int i,bytes;
    if(!out||!pose||frameIndex<0||frameIndex>=out->numFrames)return qfalse;
    if(frameIndex && !R_CV14MergedLayoutMatch(out,pose))return qfalse;
    if(out->numTags)memcpy((byte*)out+out->ofsTags+frameIndex*out->numTags*sizeof(md3Tag_t),
        (const byte*)pose+pose->ofsTags,out->numTags*sizeof(md3Tag_t));
    frames=(md3Frame_t*)((byte*)out+out->ofsFrames);
    frames[frameIndex]=*(const md3Frame_t*)((const byte*)pose+pose->ofsFrames);
    for(i=0;i<out->numSurfaces;i++) {
        md3Surface_t *dst=R_CV14SurfaceAt(out,i);
        const md3Surface_t *src=R_CV14SurfaceAtConst(pose,i);
        bytes=dst->numVerts*sizeof(md3XyzNormal_t);
        memcpy((byte*)dst+dst->ofsXyzNormals+frameIndex*bytes,(const byte*)src+src->ofsXyzNormals,bytes);
    }
    return qtrue;
}
static md3Header_t *R_CV14MergedAlloc(const md3Header_t *first,int framesCount,int *outSize) {
    md3Header_t *out;md3Surface_t *dst;const md3Surface_t *src;size_t total;int i,bytes;
    if(!first||framesCount<=0||framesCount>8192)return NULL;
    total=sizeof(*out)+(size_t)framesCount*sizeof(md3Frame_t)+(size_t)framesCount*first->numTags*sizeof(md3Tag_t);
    src=(const md3Surface_t*)((const byte*)first+first->ofsSurfaces);
    for(i=0;i<first->numSurfaces;i++) {
        bytes=src->numVerts*sizeof(md3XyzNormal_t);
        total+=(size_t)src->ofsXyzNormals+(size_t)framesCount*bytes;
        if(total>COD1_VIEWMODEL_FINAL_LIMIT)return NULL;
        src=(const md3Surface_t*)((const byte*)src+src->ofsEnd);
    }
    if(!total||total>COD1_VIEWMODEL_FINAL_LIMIT||total>0x7fffffffU)return NULL;
    out=(md3Header_t*)ri.Hunk_Alloc((int)total,h_low);if(!out)return NULL;memset(out,0,total);*out=*first;
    out->numFrames=framesCount;out->ofsFrames=sizeof(*out);out->ofsTags=out->ofsFrames+framesCount*sizeof(md3Frame_t);
    out->ofsSurfaces=out->ofsTags+framesCount*first->numTags*sizeof(md3Tag_t);out->ofsEnd=(int)total;
    src=(const md3Surface_t*)((const byte*)first+first->ofsSurfaces);dst=(md3Surface_t*)((byte*)out+out->ofsSurfaces);
    for(i=0;i<first->numSurfaces;i++) {
        bytes=src->numVerts*sizeof(md3XyzNormal_t);
        memcpy(dst,src,src->ofsXyzNormals);dst->numFrames=framesCount;
        dst->ofsEnd=src->ofsXyzNormals+framesCount*bytes;
        src=(const md3Surface_t*)((const byte*)src+src->ofsEnd);dst=(md3Surface_t*)((byte*)dst+dst->ofsEnd);
    }
    if(outSize)*outSize=(int)total;return out;
}
static qboolean R_CV14LoadViewPose(model_t *pose,const cwViewProfile *profile,const char *definitionName,
                                  const char *clip,int sample,int clipIndex,cv14BakeFile *files) {
    Q_strncpyz(pose->name,definitionName,sizeof(pose->name));
    if(R_LoadCod1ViewFrame(pose,profile,clip,sample,files))return qtrue;
    /* Presentation-only Bren melee fallback retained from HOTFIX_R2. */
    if(!strcmp(definitionName,"bren") && clipIndex==10+CW_VIEW_MELEE) {
        char why[256],warn[512];Q_strncpyz(why,r_cod1ViewFailure,sizeof(why));
        memset(pose,0,sizeof(*pose));Q_strncpyz(pose->name,definitionName,sizeof(pose->name));
        if(!R_LoadCod1ViewFrame(pose,profile,profile->idle,0,files))return qfalse;
        Com_sprintf(warn,sizeof(warn),"Bren melee pose fallback: %s",why);
        R_Cod1ViewWarningSet(warn);R_Cod1ViewFailureClear();ri.Printf(PRINT_WARNING,"CoD1 viewmodel: %s\n",warn);return qtrue;
    }
    ri.Printf(PRINT_WARNING,"CoD1 PATCH64: failed pose %s frame %d\n",clip,sample);return qfalse;
}
static int R_CV14SameHandModel(const cv709File *captured,const char *selected,const md3Header_t *md3) {
    xm14Model *model;void *data=NULL;int size,i,ok=0;char texture[MAX_QPATH];
    model=malloc(2*sizeof(*model));if(!model)return 0;
    size=ri.FS_ReadFile(selected,&data);
    if(!data||size<=0||!XM14_Model(captured->data,captured->size,&model[0])||
       !XM14_Model(data,size,&model[1])||strcmp(model[0].lod[0],model[1].lod[0])||
       model[0].ns[0]!=model[1].ns[0]||model[1].ns[0]>md3->numSurfaces)goto done;
    for(i=0;i<model[1].ns[0];i++){
        const md3Surface_t *s=R_CV14SurfaceAtConst(md3,i);
        const md3Shader_t *m=(const md3Shader_t *)((const byte *)s+s->ofsShaders);
        if(s->numShaders!=1||!CW_ViewPath(texture,model[1].skin[0][i],"skins/",0)||
           strcmp(texture,m->name))goto done;
    }
    ok=1;
done:
    if(data)ri.FS_FreeFile(data);free(model);return ok;
}
static int R_CV14ReuseViewModel(model_t *mod,const cv709Receipt *wanted) {
    int i,j,size,sawHands;
    for(i=0;i<cv709Count;i++){
        const cv709Receipt *r=&cv709Saved[i];
        const model_t *owner;
        const md3Header_t *md3;
        if(r->index<=0||r->index>=tr.numModels||tr.models[r->index]!=r->model||
           strcmp(r->weapon,wanted->weapon)||r->fp!=wanted->fp||r->mxcsr!=wanted->mxcsr||
           memcmp(&r->key,&wanted->key,sizeof(r->key)))continue;
        owner=r->model;md3=owner->md3[0];
        if(owner->type!=MOD_MESH||owner->numLods!=1||!md3)continue;
        sawHands=0;
        for(j=0;j<r->count;j++){
            const cv709File *f=&r->files[j];
            if(!strcmp(f->path,r->hands)){
                sawHands=R_CV14SameHandModel(f,wanted->hands,md3);
                if(!sawHands)break;
            }else{
                void *data=NULL;
                size=ri.FS_ReadFile(f->path,&data);
                int equal=data&&size==f->size&&!memcmp(data,f->data,size);
                if(data)ri.FS_FreeFile(data);
                if(!equal)break;
            }
        }
        if(j!=r->count||!sawHands)continue;
        /* Shader indices belong to the current renderer. Missing-media
         * warnings and a changed binding take the usual registration path. */
        for(j=0;j<md3->numSurfaces;j++){
            const md3Surface_t *s=R_CV14SurfaceAtConst(md3,j);
            const md3Shader_t *m=(const md3Shader_t *)((const byte *)s+s->ofsShaders);
            shader_t *shader;
            if(s->numShaders!=1||m->shaderIndex<=0)break;
            shader=R_FindShader(m->name,LIGHTMAP_NONE,qtrue);
            if(shader->defaultShader||shader->index!=m->shaderIndex)break;
        }
        if(j!=md3->numSurfaces)continue;
        mod->type=MOD_MESH;mod->numLods=1;mod->dataSize=owner->dataSize;
        for(j=0;j<MD3_MAX_LODS;j++)mod->md3[j]=owner->md3[j];
        return 1;
    }
    return 0;
}

/* One registration per definition name. Frames/fps come from its XAnim. */
static qboolean R_LoadCod1ViewWeapon(model_t *mod,const char *resourceName) {
    cv709Receipt proposal={0},*capture=NULL;
    cv14BakeFile files[8]={0};
    model_t pose={0};md3Header_t *out=NULL;int i,n,size=0; qboolean ok=qfalse;
    cwDef_t *def;cwViewProfile profile;cwViewAnim anim[CW_VIEW_CLIPS];
    const char *clips[CW_VIEW_CLIPS],*clip;int a,sample,clipIndex;
    char path[MAX_QPATH],definitionName[CW_VIEW_PATH],selectedHands[CW_VIEW_PATH];void *data=NULL;
    R_Cod1ViewFailureClear();R_Cod1ViewWarningSet("");
    def=(cwDef_t*)calloc(1,sizeof(*def));if(!def){R_Cod1ViewFailureSet(mod?mod->name:NULL,"<allocation>",0,"weapon definition allocation");return qfalse;}
    if(!CW_ViewResourceParse(resourceName,definitionName,selectedHands) || strlen(definitionName)+11>=sizeof(path)) {free(def);return qfalse;}
    Com_sprintf(path,sizeof(path),"weapons/sp/%s",definitionName);
    size=ri.FS_ReadFile(path,&data);
    i=size>0 && data && CW_Parse(def,definitionName,data,size)>0 && CW_ViewProfile(def,&profile) && CW_ViewSelectHands(&profile,selectedHands);
    if(data)ri.FS_FreeFile(data);free(def);data=NULL;
    if(!i){R_Cod1ViewFailureSet(mod?mod->name:NULL,path,0,"weapon definition/profile");ri.Printf(PRINT_WARNING,"CoD1 PATCH35: invalid viewmodel definition %s\n",path);return qfalse;}
    /* Only the merged view backend uses this identity. Explicit variants
     * retain their own model handles. A missing or changed resource takes
     * the original path including its failure and warning behavior. */
    if(!strncmp(mod->name,"cod1view64/",11) && strcmp(definitionName,"bren") &&
       strcmp(profile.gun,profile.hands) && R_CV14SourceKey(&proposal,&profile,definitionName)){
        if(profile.selectedHands){
            if(R_CV14ReuseViewModel(mod,&proposal))return qtrue;
        }else if(cv709Count<sizeof(cv709Saved)/sizeof(cv709Saved[0]) &&
                 mod->index>0 && mod->index<tr.numModels && tr.models[mod->index]==mod){
            capture=&proposal;
            for(i=0;i<8;i++)files[i].receipt=capture;
        }
    }
    n=1;memset(anim,0,sizeof(anim));
    for(a=0;a<CW_VIEW_CLIPS;a++) {
        clips[a]=CW_ViewClip(&profile,a);if(!*clips[a])continue;
        data=NULL;size=ri.FS_ReadFile(clips[a],&data);i=CW_ViewAnimInfo(data,size,&anim[a]);
        R_CV14CaptureFile(capture,clips[a],size,data);
        if(data)ri.FS_FreeFile(data);
        if(!i){R_Cod1ViewFailureSet(mod?mod->name:NULL,clips[a],0,"clip header");ri.Printf(PRINT_WARNING,"CoD1 PATCH64: unsupported clip %s\n",clips[a]);goto done;}
        if(anim[a].frames<0 || n>8192-anim[a].frames){R_Cod1ViewFailureSet(mod?mod->name:NULL,clips[a],0,"frame count limit");goto done;}
        n+=anim[a].frames;
    }
    memset(&pose,0,sizeof(pose));clip=profile.idle;sample=0;clipIndex=-1;
    if(!R_CV14LoadViewPose(&pose,&profile,definitionName,clip,sample,clipIndex,files))goto done;
    out=R_CV14MergedAlloc(pose.md3[0],n,&size);
    if(!out){R_Cod1ViewFailureSet(mod?mod->name:NULL,clip,sample,"merged viewmodel size limit");goto done;}
    if(!R_CV14MergedCopyFrame(out,pose.md3[0],0)){R_Cod1ViewFailureSet(mod?mod->name:NULL,clip,sample,"initial frame copy");goto done;}
    free(pose.md3[0]);memset(&pose,0,sizeof(pose));
    for(i=1;i<n;i++) {
        sample=i-1;clip=profile.idle;clipIndex=-1;
        for(a=0;a<CW_VIEW_CLIPS;a++){if(sample<anim[a].frames){clip=clips[a];clipIndex=a;break;}sample-=anim[a].frames;}
        if(!R_CV14LoadViewPose(&pose,&profile,definitionName,clip,sample,clipIndex,files))goto done;
        if(!R_CV14MergedLayoutMatch(out,pose.md3[0])) {
            R_Cod1ViewFailureSet(mod?mod->name:NULL,clip,sample,"surface/attachment layout mismatch");goto done;
        }
        if(!R_CV14MergedCopyFrame(out,pose.md3[0],i)) {
            R_Cod1ViewFailureSet(mod?mod->name:NULL,clip,sample,"merged frame copy");goto done;
        }
        free(pose.md3[0]);memset(&pose,0,sizeof(pose));
    }
    mod->type=MOD_MESH;mod->numLods=1;mod->dataSize=size;
    for(i=0;i<MD3_MAX_LODS;i++)mod->md3[i]=i?NULL:out;
    R_Cod1ViewFailureClear();
    ri.Printf(PRINT_ALL,"CoD1 PATCH35 viewmodel %s: fire=%d reload=%d empty=%d switch=%d ADSfire=%d frames, total=%d\n",definitionName,anim[0].frames,anim[1].frames,anim[2].frames,anim[3].frames,anim[4].frames,n);ok=qtrue;
done:
    if(capture && ok && !capture->refused && capture->count && cv709Count<sizeof(cv709Saved)/sizeof(cv709Saved[0])){
        capture->model=mod;capture->index=mod->index;
        cv709Saved[cv709Count++]=*capture;
    }else if(capture)R_CV14ReleaseReceipt(capture);
    for(i=0;i<8;i++)free(files[i].data);
    if(pose.md3[0])free(pose.md3[0]);
    return ok;
}

#endif
