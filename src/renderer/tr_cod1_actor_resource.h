/* CoD1 actor pilot: immutable source XModels and weighted meshes.
 *
 * The resource is shared by actor instances.  Scene poses and skinned draw
 * vertices must be allocated by the caller for each scene submission; never
 * mutate the bind skeleton or a source vertex once this loader returns.
 * The pilot alias is deliberately explicit until entity/character scripts
 * provide a validated selection of body, head, gear and world weapon.
 */
#ifndef TR_COD1_ACTOR_RESOURCE_H
#define TR_COD1_ACTOR_RESOURCE_H

#include "cod1_native_source14.h"
#include "../game/bg_cod1_actor_runtime.h"

#define COD1_ACTOR_PARTS 4
#define COD1_ACTOR_LODS 3
#define COD1_ACTOR_RESOURCE_LIMIT (64u * 1024u * 1024u)
#define COD1_ACTOR_RESOURCE_ALIGN 16u

enum {
    COD1_ACTOR_BODY = 0,
    COD1_ACTOR_HEAD,
    COD1_ACTOR_HELMET,
    COD1_ACTOR_WEAPON
};

typedef struct {
    char name[MAX_QPATH];             /* retail skins/ path */
    shader_t *shader;
    int numVerts, numIndexes;
    const cnSourceVertex_t *verts;    /* includes every original influence */
    const int *indexes;               /* RTCW front winding, immutable */
} cod1ActorSurface_t;

typedef struct {
    int numSurfaces;
    cod1ActorSurface_t surface[XM14_SURFS];
} cod1ActorLod_t;

typedef struct {
    char modelName[MAX_QPATH];
    cv14Skeleton bind;                /* LOD0 bone indices apply to every LOD */
    int numLods;
    cod1ActorLod_t lod[COD1_ACTOR_LODS];
} cod1ActorPart_t;

struct cod1ActorModel_s {
    int profile; /* R196: zero retains the original armed pilot. */
    cod1ActorPart_t part[COD1_ACTOR_PARTS];
    vec3_t bounds[2];                /* all parts / LODs, bind pose only */
};

static size_t R_Cod1ActorResourceAlign(size_t n) {
    return (n + COD1_ACTOR_RESOURCE_ALIGN - 1u) & ~(COD1_ACTOR_RESOURCE_ALIGN - 1u);
}

/* Paths come only from bounded retail v14 fields, but still reject escaping
 * the expected game asset namespace.  CoD material names contain '@', and
 * names inside some retail models also contain spaces. */
static qboolean R_Cod1ActorResourcePath(char out[MAX_QPATH],
                                       const char *prefix,const char *name) {
    size_t a,b;
    const unsigned char *p;
    if(!name || !*name || name[0]=='/' || name[0]==' ' ||
       strstr(name,"..") || strchr(name,':') || strchr(name,'\\'))return qfalse;
    a=strlen(prefix);
    b=strlen(name);
    if(!b || a+b>=MAX_QPATH || name[b-1]==' ')return qfalse;
    for(p=(const unsigned char*)name;*p;p++)
        if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') ||
             (*p>='0' && *p<='9') || *p=='_' || *p=='-' ||
             *p=='/' || *p=='.' || *p=='@' || *p==' '))return qfalse;
    memcpy(out,prefix,a);
    memcpy(out+a,name,b+1);
    return qtrue;
}

static int R_Cod1ActorResourceBone(const cv14Skeleton *bind,const char *name) {
    int i;
    for(i=0;i<bind->n;i++)if(!strcmp(bind->name[i],name))return i;
    return -1;
}

static qboolean R_Cod1ActorResourceAnchors(const cod1ActorModel_t *actor) {
    const cv14Skeleton *body=&actor->part[COD1_ACTOR_BODY].bind;
    const cv14Skeleton *head=&actor->part[COD1_ACTOR_HEAD].bind;
    const cv14Skeleton *helmet=&actor->part[COD1_ACTOR_HELMET].bind;
    const cv14Skeleton *weapon=&actor->part[COD1_ACTOR_WEAPON].bind;
    const cv14Skeleton *part[COD1_ACTOR_PARTS];
    const char *rootName[COD1_ACTOR_PARTS]={
        "tag_origin","bip01 spine2","bip01 head","tag_weapon"
    };
    int p,b,roots,left,right,leftHand,rightHand,flash,brass;
    part[0]=body;part[1]=head;part[2]=helmet;part[3]=weapon;
    if(actor->profile!=0){rootName[2]="tag_helmet";rootName[3]=actor->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine";}
    for(p=0;p<COD1_ACTOR_PARTS;p++) {
        roots=0;
        for(b=0;b<part[p]->n;b++)roots+=(part[p]->parent[b]<0);
        if(roots!=1 || strcmp(part[p]->name[0],rootName[p]) ||
           part[p]->parent[0]!=-1)return qfalse;
    }
    if(actor->profile!=0)
        return R_Cod1ActorResourceBone(body,"bip01 spine2")>=0 &&
            R_Cod1ActorResourceBone(body,actor->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine")>=0 &&
            R_Cod1ActorResourceBone(body,"tag_helmet")>=0 &&
            R_Cod1ActorResourceBone(head,"bip01 head")>=0;
    /* The body offers two weapon hands, while this pilot uses the scripted
     * right-hand tag; the world MP40 has its own differently named root. */
    if(R_Cod1ActorResourceBone(body,"bip01 spine2")<0 ||
       R_Cod1ActorResourceBone(body,"bip01 head")<0 ||
       R_Cod1ActorResourceBone(body,"tag_weapon_left")<0 ||
       R_Cod1ActorResourceBone(body,"tag_weapon_right")<0 ||
       R_Cod1ActorResourceBone(head,"bip01 head")<0 ||
       R_Cod1ActorResourceBone(weapon,"tag_flash")<0 ||
       R_Cod1ActorResourceBone(weapon,"tag_brass")<0)return qfalse;
    left=R_Cod1ActorResourceBone(body,"tag_weapon_left");
    right=R_Cod1ActorResourceBone(body,"tag_weapon_right");
    leftHand=R_Cod1ActorResourceBone(body,"bip01 l hand");
    rightHand=R_Cod1ActorResourceBone(body,"bip01 r hand");
    flash=R_Cod1ActorResourceBone(weapon,"tag_flash");
    brass=R_Cod1ActorResourceBone(weapon,"tag_brass");
    if(leftHand<0 || rightHand<0 || body->parent[left]!=leftHand ||
       body->parent[right]!=rightHand || weapon->parent[flash]!=0 ||
       weapon->parent[brass]!=0)return qfalse;
    return qtrue;
}

/* All four parts share one bind-space box for R_ModelBounds.  XModel meshes
 * are local to their own skeletons: their unjoined coordinates cannot be
 * unioned until the head, helmet and world MP40 inherit the correct anchor.
 * The scene path recomputes animated bounds from its private posed vertices. */
static qboolean R_Cod1ActorResourceBounds(cod1ActorModel_t *actor,
                     cnSourcePart_t *source[COD1_ACTOR_PARTS][COD1_ACTOR_LODS],
                     cv14Skeleton posed[COD1_ACTOR_PARTS],const cv14Pose *emptyPose) {
    int p,l,s,v,k,anchor,havePoint=0;
    vec3_t point,normal;
    const cnSourceSurface_t *surface;
    for(p=0;p<COD1_ACTOR_PARTS;p++)posed[p]=actor->part[p].bind;
    anchor=R_Cod1ActorResourceBone(&posed[COD1_ACTOR_BODY],"bip01 spine2");
    if(anchor<0)return qfalse;
    CV14_ApplyAttachedPose(&posed[COD1_ACTOR_HEAD],emptyPose,
                          &posed[COD1_ACTOR_BODY].world[anchor],"bip01 spine2");
    if(actor->profile!=0){
        anchor=R_Cod1ActorResourceBone(&posed[COD1_ACTOR_BODY],"tag_helmet");
        if(anchor<0)return qfalse;
        CV14_ApplyAttachedPose(&posed[COD1_ACTOR_HELMET],emptyPose,
            &posed[COD1_ACTOR_BODY].world[anchor],"tag_helmet");
        anchor=R_Cod1ActorResourceBone(&posed[COD1_ACTOR_BODY],actor->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine");
        if(anchor<0)return qfalse;
        CV14_ApplyAttachedPose(&posed[COD1_ACTOR_WEAPON],emptyPose,
            &posed[COD1_ACTOR_BODY].world[anchor],actor->profile==COD1_ACTOR_MOODY_PROFILE?"bip01 pelvis":"bip01 spine");
    }else{
    anchor=R_Cod1ActorResourceBone(&posed[COD1_ACTOR_HEAD],"bip01 head");
    if(anchor<0)return qfalse;
    CV14_ApplyAttachedPose(&posed[COD1_ACTOR_HELMET],emptyPose,
                          &posed[COD1_ACTOR_HEAD].world[anchor],"bip01 head");
    anchor=R_Cod1ActorResourceBone(&posed[COD1_ACTOR_BODY],"tag_weapon_right");
    if(anchor<0)return qfalse;
    CV14_ApplyAttachedPose(&posed[COD1_ACTOR_WEAPON],emptyPose,
                          &posed[COD1_ACTOR_BODY].world[anchor],"tag_weapon");
    }
    for(p=0;p<COD1_ACTOR_PARTS;p++)
        for(l=0;l<actor->part[p].numLods;l++) {
            if(!source[p][l])return qfalse;
            for(s=0;s<source[p][l]->numSurfaces;s++) {
                surface=&source[p][l]->surfaces[s];
                for(v=0;v<surface->numVerts;v++) {
                    if(!CN_SourceSkinVertex(&surface->verts[v],&posed[p],point,normal))
                        return qfalse;
                    for(k=0;k<3;k++)
                        if(!(point[k]>=-1000000.f && point[k]<=1000000.f))return qfalse;
                    if(!havePoint){
                        VectorCopy(point,actor->bounds[0]);
                        VectorCopy(point,actor->bounds[1]);
                        havePoint=1;
                    } else AddPointToBounds(point,actor->bounds[0],actor->bounds[1]);
                }
            }
        }
    return havePoint?qtrue:qfalse;
}

/* Before Hunk_Alloc every model, surface, bind anchor and material has been
 * validated.  On any read/parse/alloc failure, mod is untouched and all
 * transient FS bytes and weighted meshes are released. */
static qboolean R_LoadCod1ActorPilot(model_t *mod,const char *alias) {
    static const char *pilot[COD1_ACTOR_PARTS]={
        "character_german_WehrmactA","basehead_axis_a_01",
        "gear_german_helmet","weapon_mp40"
    };
    static const char *foley[COD1_ACTOR_PARTS]={
        "character_Foley","head_Foley","gear_US_helmet_scrim","gear_US_Foley"
    };
    static const char *moody[COD1_ACTOR_PARTS]={"character_Moody","head_Moody_training","gear_US_helmet","gear_US_Moody"};
    const char *const *models;int profile;
    cod1ActorModel_t *staging=NULL,*runtime=NULL;
    xm14Model *meta=NULL;
    cnSourcePart_t *source[COD1_ACTOR_PARTS][COD1_ACTOR_LODS];
    cv14Pose *emptyPose=NULL;
    cv14Skeleton *posed=NULL;
    cv14Transform root;
    void *data=NULL;
    char path[MAX_QPATH],failedPath[MAX_QPATH];
    const char *stage="allocation";
    int p,l,s,b,k,n,numLods,sizeRead;
    size_t bytes,vertexBytes,indexBytes,offset;
    unsigned char *cursor;
    qboolean ok=qfalse;

    if(!mod||!alias)return qfalse;
    profile=!Q_stricmp(alias,"moody_training")?COD1_ACTOR_MOODY_PROFILE:!Q_stricmp(alias,"foley_training")?COD1_ACTOR_FOLEY_PROFILE:0;
    if(!profile&&Q_stricmp(alias,"wehrmacht_mp40a"))return qfalse;
    models=profile==COD1_ACTOR_MOODY_PROFILE?moody:profile?foley:pilot;
    memset(source,0,sizeof(source));
    failedPath[0]=0;
    staging=(cod1ActorModel_t*)calloc(1,sizeof(*staging));
    meta=(xm14Model*)calloc(1,sizeof(*meta));
    emptyPose=(cv14Pose*)calloc(1,sizeof(*emptyPose));
    posed=(cv14Skeleton*)calloc(COD1_ACTOR_PARTS,sizeof(*posed));
    if(!staging || !meta || !emptyPose || !posed)goto done;
    staging->profile=profile;
    CV14_Identity(&root);
    bytes=R_Cod1ActorResourceAlign(sizeof(*staging));
    if(bytes>COD1_ACTOR_RESOURCE_LIMIT)goto done;

    for(p=0;p<COD1_ACTOR_PARTS;p++) {
        cod1ActorPart_t *part=&staging->part[p];
        Q_strncpyz(part->modelName,models[p],sizeof(part->modelName));
        stage="xmodel";
        if(!R_Cod1ActorResourcePath(path,"xmodel/",models[p]))goto done;
        Q_strncpyz(failedPath,path,sizeof(failedPath));
        sizeRead=ri.FS_ReadFile(path,&data);
        if(sizeRead<=0 || sizeRead>16*1024*1024 || !data ||
           !XM14_Model(data,(size_t)sizeRead,meta))goto done;
        ri.FS_FreeFile(data);data=NULL;
        stage="xmodelparts LOD0";
        if(!R_Cod1ActorResourcePath(path,"xmodelparts/",meta->lod[0]))goto done;
        Q_strncpyz(failedPath,path,sizeof(failedPath));
        sizeRead=ri.FS_ReadFile(path,&data);
        if(sizeRead<=0 || sizeRead>16*1024*1024 || !data ||
           !CV14_Parts(data,(size_t)sizeRead,&part->bind))goto done;
        ri.FS_FreeFile(data);data=NULL;
        CV14_ApplyPose(&part->bind,emptyPose,&root);
        for(b=0;b<part->bind.n;b++) {
            for(k=0;k<3;k++)
                if(!(part->bind.world[b].p[k]>=-1000000.f &&
                     part->bind.world[b].p[k]<=1000000.f)){
                    stage="bind transform";goto done;
                }
            for(k=0;k<4;k++)
                if(!(part->bind.world[b].q[k]>=-1.001f &&
                     part->bind.world[b].q[k]<=1.001f)){
                    stage="bind rotation";goto done;
                }
        }

        numLods=0;
        for(l=0;l<COD1_ACTOR_LODS;l++) {
            cod1ActorLod_t *lod=&part->lod[l];
            cnSourcePart_t *src;
            if(!meta->lod[l][0]) {
                if(meta->ns[l] || (l<2 && meta->lod[l+1][0])){
                    stage="sparse LOD";goto done;
                }
                continue;
            }
            if(!meta->ns[l] || numLods!=l){stage="LOD surface count";goto done;}
            stage="xmodelsurfs";
            if(!R_Cod1ActorResourcePath(path,"xmodelsurfs/",meta->lod[l]))goto done;
            Q_strncpyz(failedPath,path,sizeof(failedPath));
            sizeRead=ri.FS_ReadFile(path,&data);
            if(sizeRead<=0 || sizeRead>16*1024*1024 || !data)goto done;
            src=(cnSourcePart_t*)calloc(1,sizeof(*src));
            if(!src){stage="source allocation";goto done;}
            source[p][l]=src;
            src->bind=part->bind;
            if(!CN_SourceParseMesh(data,(size_t)sizeRead,&src->bind,src) ||
               src->numSurfaces!=meta->ns[l])goto done;
            ri.FS_FreeFile(data);data=NULL;
            lod->numSurfaces=src->numSurfaces;
            for(s=0;s<lod->numSurfaces;s++) {
                const cnSourceSurface_t *input=&src->surfaces[s];
                cod1ActorSurface_t *out=&lod->surface[s];
                stage="material path";
                if(!R_Cod1ActorResourcePath(out->name,"skins/",meta->skin[l][s]))goto done;
                Q_strncpyz(failedPath,out->name,sizeof(failedPath));
                if(input->numVerts<=0 || input->numIndexes<=0 ||
                   input->numIndexes%3 || !input->verts || !input->indexes){
                    stage="mesh geometry";goto done;
                }
                out->numVerts=input->numVerts;
                out->numIndexes=input->numIndexes;
                for(n=0;n<input->numIndexes;n++)
                    if(input->indexes[n]>=input->numVerts){stage="mesh indices";goto done;}
                stage="material shader";
                out->shader=R_FindShader(out->name,LIGHTMAP_NONE,qtrue);
                if(!out->shader || out->shader->defaultShader)goto done;
                vertexBytes=R_Cod1ActorResourceAlign(
                    (size_t)input->numVerts*sizeof(cnSourceVertex_t));
                indexBytes=R_Cod1ActorResourceAlign(
                    (size_t)input->numIndexes*sizeof(int));
                if(vertexBytes>COD1_ACTOR_RESOURCE_LIMIT-bytes ||
                   indexBytes>COD1_ACTOR_RESOURCE_LIMIT-bytes-vertexBytes){
                    stage="source memory limit";goto done;
                }
                bytes+=vertexBytes+indexBytes;
            }
            numLods++;
        }
        if(!numLods){stage="empty model";goto done;}
        part->numLods=numLods;
    }
    stage="skeleton anchors";
    if(!R_Cod1ActorResourceAnchors(staging))goto done;
    stage="joined bind bounds";
    if(!R_Cod1ActorResourceBounds(staging,source,posed,emptyPose))goto done;
    stage="native allocation";
    if(bytes>0x7fffffffU)goto done;
    runtime=(cod1ActorModel_t*)ri.Hunk_Alloc((int)bytes,h_low);
    if(!runtime)goto done;
    memset(runtime,0,bytes);
    memcpy(runtime,staging,sizeof(*staging));
    cursor=(unsigned char*)runtime;
    offset=R_Cod1ActorResourceAlign(sizeof(*runtime));
    for(p=0;p<COD1_ACTOR_PARTS;p++)for(l=0;l<runtime->part[p].numLods;l++)
        for(s=0;s<runtime->part[p].lod[l].numSurfaces;s++) {
            const cnSourceSurface_t *input=&source[p][l]->surfaces[s];
            cod1ActorSurface_t *out=&runtime->part[p].lod[l].surface[s];
            int *indexes;
            vertexBytes=R_Cod1ActorResourceAlign(
                (size_t)input->numVerts*sizeof(cnSourceVertex_t));
            indexBytes=R_Cod1ActorResourceAlign(
                (size_t)input->numIndexes*sizeof(int));
            out->verts=(const cnSourceVertex_t*)(cursor+offset);
            memcpy(cursor+offset,input->verts,
                   (size_t)input->numVerts*sizeof(cnSourceVertex_t));
            offset+=vertexBytes;
            indexes=(int*)(cursor+offset);
            out->indexes=indexes;
            for(n=0;n<input->numIndexes;n++)indexes[n]=(int)input->indexes[n];
            offset+=indexBytes;
        }
    /* No fallible operation follows Hunk_Alloc. Layout uses the same
     * checked sizes as the validation pass above. */
    mod->type=MOD_COD1_ACTOR_XMODEL;
    mod->numLods=runtime->part[COD1_ACTOR_BODY].numLods;
    mod->dataSize=(int)bytes;
    mod->cod1Actor=runtime;
    ok=qtrue;
done:
    if(data)ri.FS_FreeFile(data);
    for(p=0;p<COD1_ACTOR_PARTS;p++)for(l=0;l<COD1_ACTOR_LODS;l++)
        if(source[p][l]){CN_SourceFree(source[p][l]);free(source[p][l]);}
    free(staging);free(meta);free(emptyPose);free(posed);
    if(!ok)ri.Printf(PRINT_DEVELOPER,
                     "CoD1 actor: %s failed at %s %s (no model committed)\n",
                     alias,stage,failedPath);
    return ok;
}

#endif /* TR_COD1_ACTOR_RESOURCE_H */
