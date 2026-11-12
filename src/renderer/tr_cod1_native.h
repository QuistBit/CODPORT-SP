/* NATIVE_XMODEL_R1: first isolated native CoD1 first-person XModel/XAnim path.
 *
 * Scope is deliberately narrow: Thompson idle only.  The legacy cod1view64
 * XModel/XAnim -> MD3 adapter remains intact and is the fallback/oracle.
 *
 * This path never builds md3Header_t/md3Surface_t/md3Frame_t.  It samples the
 * retail idle XAnim with the already verified v14 pose math, skins hands and
 * gun in float space, and stores ordinary srfTriangles_t surfaces for RTCW's
 * existing tess backend.  Native-2 will move clip sampling/skinning from model
 * registration to per-frame playback; keeping the model type separate now
 * avoids baking that future runtime back into MOD_MESH.
 */
#ifndef TR_COD1_NATIVE_H
#define TR_COD1_NATIVE_H

#define COD1_NATIVE_MAX_SURFACES (XM14_SURFS * 2)
#define COD1_NATIVE_MAX_TAGS CV14_WEAPON_TAGS

typedef struct cod1NativeAnimated_s cod1NativeAnimated_t;

typedef struct {
    char name[MAX_QPATH];
    orientation_t transform;
} cod1NativeTag_t;

typedef struct {
    char name[MAX_QPATH];
    shader_t *shader;
    srfTriangles_t *geometry;
} cod1NativeSurface_t;

struct cod1NativeModel_s {
    int numSurfaces;
    cod1NativeSurface_t surfaces[COD1_NATIVE_MAX_SURFACES];
    int numTags;
    cod1NativeTag_t tags[COD1_NATIVE_MAX_TAGS];
    vec3_t bounds[2];
    vec3_t localOrigin;
    float radius;
    cod1NativeAnimated_t *animated; /* NULL for the accepted fixed-idle path */

    /* Fixed Native-1 bind pose, including both attached model skeletons.
     * Immutable resource data is copied into a submission before drawing. */
    int numBones;
    cv14Transform *boneWorld;

    /* Persistent native animation resource retained for Native-2.  Native-1
     * samples frame 0 during registration but does not discard the XAnim. */
    char idleName[MAX_QPATH];
    int idleFrames;
    int idleFps;
    int idleBytes;
    byte *idleData;
};

struct cod1NativeSubmission_s {
    int numSurfaces;
    srfTriangles_t *surfaces;
    int numTags;
    cod1NativeTag_t tags[COD1_NATIVE_MAX_TAGS];
    int numBones;
    cv14Transform *boneWorld;
    vec3_t bounds[2];
};

static int R_Cod1NativeExportTags(const cv14Skeleton *s,cod1NativeTag_t out[COD1_NATIVE_MAX_TAGS]) {
    static const char *names[COD1_NATIVE_MAX_TAGS]={"tag_flash","tag_brass","tag_weapon",
        "tag_flash_11","tag_flash_2","tag_flash_22"};
    int n=0,i,j,k,a;float unit[3];
    memset(out,0,COD1_NATIVE_MAX_TAGS*sizeof(*out));
    for(i=0;i<COD1_NATIVE_MAX_TAGS;i++) {
        for(j=0;j<s->n;j++)if(!strcmp(s->name[j],names[i]))break;
        if(j==s->n)continue;
        Q_strncpyz(out[n].name,names[i],sizeof(out[n].name));
        for(k=0;k<3;k++) {
            if(!(s->world[j].p[k]>=-1000000 && s->world[j].p[k]<=1000000))return -1;
            out[n].transform.origin[k]=s->world[j].p[k];
        }
        for(a=0;a<3;a++) {
            unit[0]=unit[1]=unit[2]=0;unit[a]=1;
            CV14_Rotate(s->world[j].q,unit,out[n].transform.axis[a]);
            for(k=0;k<3;k++)if(!(out[n].transform.axis[a][k]>=-1.001f && out[n].transform.axis[a][k]<=1.001f))return -1;
        }
        n++;
    }
    return n;
}

static int R_Cod1NativeFogNum(const cod1NativeModel_t *native,trRefEntity_t *ent) {
    int i,j;fog_t *fog;vec3_t localOrigin;
    if((tr.refdef.rdflags&RDF_NOWORLDMODEL) || !native || !tr.world)return 0;
    VectorAdd(ent->e.origin,native->localOrigin,localOrigin);
    for(i=1;i<tr.world->numfogs;i++) {
        fog=&tr.world->fogs[i];
        for(j=0;j<3;j++) {
            if(localOrigin[j]-native->radius>=fog->bounds[1][j])break;
            if(localOrigin[j]+native->radius<=fog->bounds[0][j])break;
        }
        if(j==3)return i;
    }
    return 0;
}

static qboolean R_Cod1NativeBuildPart(const cwViewProfile *profile,int weaponPart,
                                      const cv14Pose *pose,const cv14Transform *root,
                                      xm14Model *model,cv14Skeleton *sk,xm14Mesh *mesh,
                                      const char *parentTag,const char **stage) {
    *stage=weaponPart?"native weapon":"native hands";
    if(!R_CV14Part(weaponPart?profile->gun:profile->hands,pose,root,model,sk,mesh,parentTag))return qfalse;
    if(model->ns[0]!=mesh->ns)return qfalse;
    return qtrue;
}

static qboolean R_LoadCod1NativeViewWeapon(model_t *mod,const char *resourceName) {
    cwDef_t *def=NULL;cwViewProfile profile;cwViewAnim idleInfo;
    cv14Pose *idle=NULL,*hip=NULL;cv14Skeleton *sk=NULL;xm14Model *models=NULL;xm14Mesh mesh[2];
    cv14Transform root,attachment;cod1NativeTag_t tags[COD1_NATIVE_MAX_TAGS];
    shader_t *materials[2][XM14_SURFS];char materialNames[2][XM14_SURFS][MAX_QPATH];
    void *defData=NULL,*idleData=NULL;int defSize=0,idleSize=0;char path[MAX_QPATH];
    int i,j,k,found=0,numTags=0,numSurfaces=0,totalVerts=0,totalIndexes=0,numBones=0;
    cv14Transform poseWorld[2*CV14_BONES];
    size_t bytes=0;byte *cursor;cod1NativeModel_t *native=NULL;const char *stage="allocation";
    qboolean ok=qfalse;int knownHands;
    char definitionName[CW_VIEW_PATH],selectedHands[CW_VIEW_PATH];

    memset(&profile,0,sizeof(profile));memset(&idleInfo,0,sizeof(idleInfo));memset(mesh,0,sizeof(mesh));memset(materials,0,sizeof(materials));memset(materialNames,0,sizeof(materialNames));
    R_Cod1ViewFailureClear();R_Cod1ViewWarningSet("");
    if(!mod || !CW_ViewResourceParse(resourceName,definitionName,selectedHands))return qfalse;

    /* Modes 0/1/2 retain the accepted Thompson/MP40 boundary. Mode 3 opts
     * first-person WeaponDefs into the same bounded native model loader. */
    if(Q_stricmp(definitionName,"thompson") && Q_stricmp(definitionName,"mp40") &&
       (!r_cod1NativeXModel || r_cod1NativeXModel->integer<3)) {
        R_Cod1ViewFailureSet(mod->name,"<native>",0,"Native source requires mode 3 for this weapon");
        return qfalse;
    }

    def=(cwDef_t*)calloc(1,sizeof(*def));idle=(cv14Pose*)calloc(1,sizeof(*idle));hip=(cv14Pose*)calloc(1,sizeof(*hip));
    sk=(cv14Skeleton*)calloc(1,sizeof(*sk));models=(xm14Model*)calloc(2,sizeof(*models));
    if(!def||!idle||!hip||!sk||!models)goto done;

    stage="weapon definition/profile";
    if(!CW_NameValid(definitionName) || strlen(definitionName)+11>=sizeof(path))goto done;
    Com_sprintf(path,sizeof(path),"weapons/sp/%s",definitionName);
    defSize=ri.FS_ReadFile(path,&defData);
    if(defSize<=0 || !defData || CW_Parse(def,definitionName,defData,defSize)<=0 || !CW_ViewProfile(def,&profile) || !CW_ViewSelectHands(&profile,selectedHands))goto done;
    ri.FS_FreeFile(defData);defData=NULL;

    stage="native idle XAnim";
    idleSize=ri.FS_ReadFile(profile.idle,&idleData);
    if(idleSize<=0 || !idleData || !CW_ViewAnimInfo(idleData,idleSize,&idleInfo) ||
       !CV14_SamplePose(idleData,(size_t)idleSize,0,idle))goto done;

    CV14_Identity(&root);
    if(*profile.hip) {
        stage="native hip offset";
        if(!R_CV14Pose(profile.hip,hip,0))goto done;
        i=CV14_Find(hip,"tag_torso");if(i<0)goto done;root=hip->t[i];
    }

    stage="native hands";
    if(!R_Cod1NativeBuildPart(&profile,0,idle,&root,&models[0],sk,&mesh[0],NULL,&stage))goto done;
    numBones=sk->n;
    memcpy(poseWorld,sk->world,(size_t)numBones*sizeof(*poseWorld));
    for(i=0;i<sk->n;i++)if(!strcmp(sk->name[i],"tag_weapon")){attachment=sk->world[i];found=1;break;}
    if(!found){stage="hands tag_weapon";goto done;}

    stage="native weapon";
    if(!R_Cod1NativeBuildPart(&profile,1,idle,&attachment,&models[1],sk,&mesh[1],"tag_weapon",&stage))goto done;
    if(sk->n>2*CV14_BONES-numBones){stage="native composite bone count";goto done;}
    memcpy(poseWorld+numBones,sk->world,(size_t)sk->n*sizeof(*poseWorld));
    numBones+=sk->n;
    stage="native attachments";numTags=R_Cod1NativeExportTags(sk,tags);if(numTags<0)goto done;

    /* Keep exact accepted adapter material mapping, but do not copy any MD3
     * coordinate clamps or hide/collapse workarounds into native rendering. */
    knownHands=!profile.selectedHands && (!strcmp(profile.hands,"viewmodel_hands_new") || !strcmp(profile.hands,"viewmodel_hands_sten"));
    if(knownHands && mesh[0].ns!=4){stage="native hands material layout";goto done;}
    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++) {
        if(numSurfaces>=COD1_NATIVE_MAX_SURFACES){stage="native surface limit";goto done;}
        if(!i && knownHands)Q_strncpyz(materialNames[i][j],j<2?"skins/viewhands@hand.dds":"skins/viewsleeves_new.tga",MAX_QPATH);
        else if(!CW_ViewPath(materialNames[i][j],models[i].skin[0][j],"skins/",0)){stage="native material path";goto done;}
        materials[i][j]=R_FindShader(materialNames[i][j],LIGHTMAP_NONE,qtrue);
        if(materials[i][j]->defaultShader)ri.Printf(PRINT_WARNING,"CoD1 native: missing material %s\n",materialNames[i][j]);
        if(mesh[i].s[j].nv<=0 || mesh[i].s[j].nt<=0){stage="native empty surface";goto done;}
        if(totalVerts>0x7fffffff-mesh[i].s[j].nv || totalIndexes>0x7fffffff-mesh[i].s[j].nt*3){stage="native geometry count overflow";goto done;}
        totalVerts+=mesh[i].s[j].nv;totalIndexes+=mesh[i].s[j].nt*3;numSurfaces++;
    }

    bytes=sizeof(*native);
    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++) {
        /* A surface may have an odd number of 32-bit indexes. Align the next
         * header for its pointer members on both Win32 and 64-bit hosts. */
        bytes=(bytes+sizeof(void*)-1)&~(sizeof(void*)-1);
        bytes+=sizeof(srfTriangles_t)+(size_t)mesh[i].s[j].nv*sizeof(drawVert_t)+(size_t)mesh[i].s[j].nt*3*sizeof(int);
        if(bytes>64u*1024u*1024u){stage="native allocation limit";goto done;}
    }
    bytes+=(size_t)numBones*sizeof(*poseWorld)+(size_t)idleSize;
    if(bytes>64u*1024u*1024u){stage="native allocation limit";goto done;}
    if(bytes>0x7fffffffU){stage="native allocation overflow";goto done;}

    native=(cod1NativeModel_t*)ri.Hunk_Alloc((int)bytes,h_low);if(!native)goto done;memset(native,0,bytes);
    cursor=(byte*)(native+1);
    native->idleBytes=idleSize;
    Q_strncpyz(native->idleName,profile.idle,sizeof(native->idleName));native->idleFrames=idleInfo.frames;native->idleFps=idleInfo.fps;
    native->numTags=numTags;memcpy(native->tags,tags,numTags*sizeof(tags[0]));
    ClearBounds(native->bounds[0],native->bounds[1]);native->numSurfaces=0;

    for(i=0;i<2;i++)for(j=0;j<mesh[i].ns;j++) {
        const xm14Surface *src=&mesh[i].s[j];cod1NativeSurface_t *dst=&native->surfaces[native->numSurfaces++];
        srfTriangles_t *tri;
        cursor=(byte*)(((size_t)cursor+sizeof(void*)-1)&~(sizeof(void*)-1));
        tri=(srfTriangles_t*)cursor;cursor+=sizeof(*tri);
        memset(tri,0,sizeof(*tri));tri->surfaceType=SF_TRIANGLES;tri->numVerts=src->nv;tri->numIndexes=src->nt*3;
        tri->verts=(drawVert_t*)cursor;cursor+=src->nv*sizeof(drawVert_t);
        tri->indexes=(int*)cursor;cursor+=src->nt*3*sizeof(int);ClearBounds(tri->bounds[0],tri->bounds[1]);
        Com_sprintf(dst->name,sizeof(dst->name),"cod1_%s_%d",i?"weapon":"hands",j);dst->shader=materials[i][j];dst->geometry=tri;
        for(k=0;k<src->nv;k++) {
            drawVert_t *v=&tri->verts[k];memset(v,0,sizeof(*v));VectorCopy(src->v[k].xyz,v->xyz);VectorCopy(src->v[k].normal,v->normal);
            VectorNormalize(v->normal);v->st[0]=src->v[k].uv[0];v->st[1]=src->v[k].uv[1];
            v->color[0]=v->color[1]=v->color[2]=v->color[3]=255;
            AddPointToBounds(v->xyz,tri->bounds[0],tri->bounds[1]);AddPointToBounds(v->xyz,native->bounds[0],native->bounds[1]);
        }
        for(k=0;k<tri->numIndexes;k++)tri->indexes[k]=src->idx[k];
    }
    native->numBones=numBones;
    native->boneWorld=(cv14Transform*)cursor;
    memcpy(native->boneWorld,poseWorld,(size_t)numBones*sizeof(*poseWorld));
    cursor+=(size_t)numBones*sizeof(*poseWorld);
    /* Geometry ends on an int boundary. Store float transforms before the
     * byte-sized XAnim; idleSize need not be divisible by four. */
    native->idleData=cursor;memcpy(native->idleData,idleData,idleSize);cursor+=idleSize;
    for(k=0;k<3;k++)native->localOrigin[k]=0.5f*(native->bounds[0][k]+native->bounds[1][k]);
    native->radius=RadiusFromBounds(native->bounds[0],native->bounds[1]);

    mod->type=MOD_COD1_XMODEL;mod->numLods=1;mod->dataSize=(int)bytes;mod->cod1Native=native;
    ri.Printf(PRINT_ALL,"CoD1 Native-1 %s: idle=%s (%d frames @ %d fps), %d surfaces, %d verts, %d indexes, %d tags, %d bytes\n",
        definitionName,native->idleName,native->idleFrames,native->idleFps,native->numSurfaces,totalVerts,totalIndexes,native->numTags,mod->dataSize);
    R_Cod1ViewFailureClear();ok=qtrue;

done:
    if(defData)ri.FS_FreeFile(defData);
    if(idleData)ri.FS_FreeFile(idleData);
    for(i=0;i<2;i++)XM14_Free(&mesh[i]);
    free(def);free(idle);free(hip);free(sk);free(models);
    if(!ok) {
        R_Cod1ViewFailureSet(mod?mod->name:NULL,profile.idle[0]?profile.idle:"<native>",0,stage);
        ri.Printf(PRINT_WARNING,"CoD1 Native-1: failed %s at %s; adapter fallback remains available\n",definitionName?definitionName:"<null>",stage);
    }
    return ok;
}

/* Scene submission owns its final pose, tags and float vertices until the
 * backend has consumed every scene in this SMP frame. The triangle indexes,
 * shaders and source assets stay immutable and shared by the model. */
cod1NativeSubmission_t *R_Cod1NativeCapture(model_t *model,cnsArena_t *arena) {
    cod1NativeModel_t *native=model?model->cod1Native:NULL;
    cod1NativeSubmission_t *out;byte *cursor;size_t bytes;int i;
    if(!native || !arena || native->numSurfaces<=0 ||
       native->numSurfaces>COD1_NATIVE_MAX_SURFACES || native->numBones<=0 ||
       native->numBones>2*CV14_BONES || !native->boneWorld)return NULL;
    bytes=sizeof(*out)+(size_t)native->numSurfaces*sizeof(srfTriangles_t)
         +(size_t)native->numBones*sizeof(cv14Transform);
    if(bytes>arena->capacity)return NULL;
    for(i=0;i<native->numSurfaces;i++) {
        const srfTriangles_t *src=native->surfaces[i].geometry;
        if(!src || src->numVerts<=0 || !src->verts || !src->indexes ||
           (size_t)src->numVerts>(arena->capacity-bytes)/sizeof(drawVert_t))return NULL;
        bytes+=(size_t)src->numVerts*sizeof(drawVert_t);
    }
    out=(cod1NativeSubmission_t*)CNS_Alloc(arena,bytes,sizeof(void*));
    if(!out)return NULL;
    memset(out,0,sizeof(*out));
    out->numSurfaces=native->numSurfaces;out->numTags=native->numTags;
    memcpy(out->tags,native->tags,(size_t)native->numTags*sizeof(*out->tags));
    out->numBones=native->numBones;
    memcpy(out->bounds,native->bounds,sizeof(out->bounds));
    cursor=(byte*)(out+1);out->surfaces=(srfTriangles_t*)cursor;
    cursor+=(size_t)out->numSurfaces*sizeof(*out->surfaces);
    out->boneWorld=(cv14Transform*)cursor;
    memcpy(out->boneWorld,native->boneWorld,(size_t)out->numBones*sizeof(*out->boneWorld));
    cursor+=(size_t)out->numBones*sizeof(*out->boneWorld);
    for(i=0;i<out->numSurfaces;i++) {
        const srfTriangles_t *src=native->surfaces[i].geometry;
        srfTriangles_t *dst=&out->surfaces[i];
        *dst=*src;
        dst->verts=(drawVert_t*)cursor;
        memcpy(dst->verts,src->verts,(size_t)src->numVerts*sizeof(*dst->verts));
        cursor+=(size_t)src->numVerts*sizeof(*dst->verts);
    }
    return out;
}

void R_AddCod1NativeSurfaces(trRefEntity_t *ent) {
    cod1NativeModel_t *native=tr.currentModel?tr.currentModel->cod1Native:NULL;
    cod1NativeSubmission_t *submission=ent->cod1NativeSubmission;
    shader_t *shader;int i,fogNum,cull;
    qboolean personalModel;
    if(!native || !submission || submission->numSurfaces!=native->numSurfaces)return;
    personalModel=(ent->e.renderfx&RF_THIRD_PERSON) && !tr.viewParms.isPortal;
    if(!(ent->e.renderfx&RF_FIRST_PERSON)) {
        cull=R_CullLocalBox(submission->bounds);if(cull==CULL_OUT)return;
    }
    if(!personalModel || r_shadows->integer>1)R_SetupEntityLighting(&tr.refdef,ent);
    fogNum=R_Cod1NativeFogNum(native,ent);
    for(i=0;i<native->numSurfaces;i++) {
        cod1NativeSurface_t *surface=&native->surfaces[i];
        shader=ent->e.customShader?R_GetShaderByHandle(ent->e.customShader):surface->shader;
        if(!shader)shader=tr.defaultShader;
        if(!personalModel && r_shadows->integer==2 && fogNum==0 && !(ent->e.renderfx&(RF_NOSHADOW|RF_DEPTHHACK)) && shader->sort==SS_OPAQUE)
            R_AddDrawSurf((surfaceType_t*)&submission->surfaces[i],tr.shadowShader,0,qfalse,tr.currentModel->ATI_tess);
        if(!personalModel)R_AddDrawSurf((surfaceType_t*)&submission->surfaces[i],shader,fogNum,qfalse,tr.currentModel->ATI_tess);
    }
}

static int R_Cod1NativeLerpTag(const cod1NativeModel_t *native,orientation_t *tag,const char *tagName,int startIndex) {
    int i;if(!native || !tag || !tagName)return -1;
    if(startIndex<0)startIndex=0;
    for(i=startIndex;i<native->numTags;i++)if(!strcmp(native->tags[i].name,tagName)) {*tag=native->tags[i].transform;return i;}
    AxisClear(tag->axis);VectorClear(tag->origin);return -1;
}

#endif
