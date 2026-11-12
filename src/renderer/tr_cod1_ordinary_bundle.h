/* R61 bounded native ordinary secondary bundles. Included after ParseTexMod,
 * ParseVector and the image policy helpers; only the CoD1 source route uses
 * it. Native nextbundle defaults GL_MODULATE (0x4ee5b5); coordinates and
 * modifiers belong to this bundle, while RGB/alpha/depth belong to stage. */
static qboolean R_Cod1OrdinaryBundle(shaderStage_t *stage,char **text) {
    shaderStage_t local[NUM_TEXTURE_BUNDLES-1];
    texModInfo_t mods[NUM_TEXTURE_BUNDLES-1][TR_MAX_TEXMODS];
    textureBundle_t *bundle;
    char *token;
    int mapped[NUM_TEXTURE_BUNDLES-1],source[NUM_TEXTURE_BUNDLES-1];
    int env[NUM_TEXTURE_BUNDLES-1],b=0,k;
    unsigned auth=R_Cod1StageBundleAuth(stage);
    memset(local,0,sizeof(local));memset(mods,0,sizeof(mods));
    memset(mapped,0,sizeof(mapped));memset(source,0,sizeof(source));
    for(k=0;k<NUM_TEXTURE_BUNDLES-1;++k) {
        if(stage->bundle[k+1].image[0] || !stage->bundle[k+1].texMods)return qfalse;
        local[k].bundle[0].texMods=mods[k];env[k]=GL_MODULATE;
    }
    bundle=&local[0].bundle[0];
    while(1) {
        token=COM_ParseExt(text,qtrue);
        if(!token[0])return qfalse;
        if(!strcmp(token,"}"))break;
        if(!Q_stricmp(token,"nextbundle")) {
            if(!mapped[b] || b+1>=NUM_TEXTURE_BUNDLES-1 || !(auth&COD1_ST_AUTH_THIRD))return qfalse;
            ++b;bundle=&local[b].bundle[0];continue;
        }
        if(!Q_stricmp(token,"map")) {
            int axes=C1MAP_REPEAT,wrap;unsigned flags=0;
            if(mapped[b]++)return qfalse;
            token=COM_ParseExt(text,qfalse);
            while(C1Map_Option(token,&axes,&flags))token=COM_ParseExt(text,qfalse);
            if(!token[0])return qfalse;
            bundle->cod1MapLoadFlags=flags;source[b]=C1Map_Source(token);
            if(source[b]==C1MAP_LIGHTMAP) {
                if(shader.lightmapIndex>=0) {
                    if(shader.lightmapIndex>=tr.numLightmaps)return qfalse;
                    bundle->image[0]=tr.lightmaps[shader.lightmapIndex];
                } else bundle->image[0]=tr.identityLightImage;
                bundle->isLightmap=qtrue;
            } else if(source[b]==C1MAP_WHITE)bundle->image[0]=tr.whiteImage;
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
            else if(source[b]==C1MAP_DLIGHT && cod1PerlightCompiling)
                bundle->image[0]=tr.cod1DlightImage;
#endif
            else if(source[b]==C1MAP_FILE) {
                wrap=C1Map_Wrap(axes,GL_REPEAT,GL_CLAMP_TO_EDGE,COD1_IMAGE_CLAMP_X,COD1_IMAGE_CLAMP_Y);
                bundle->image[0]=R_Cod1StageImage(token,!shader.noMipMaps,!shader.noPicMip,shader.characterMip,wrap);
                R_Cod1MapPolicyTrace(token,flags,bundle->image[0]);
            } else return qfalse;
            if(!bundle->image[0])return qfalse;
        } else if(!Q_stricmp(token,"tcGen")) {
            if(!mapped[b])return qfalse;
            token=COM_ParseExt(text,qfalse);
            if(!Q_stricmp(token,"texture") || !Q_stricmp(token,"base"))bundle->tcGen=TCGEN_TEXTURE;
            else if(!Q_stricmp(token,"lightmap"))bundle->tcGen=TCGEN_LIGHTMAP;
            else if(!Q_stricmp(token,"environment"))bundle->tcGen=TCGEN_ENVIRONMENT_MAPPED;
            else if(!Q_stricmp(token,"vector")) {
                if(!ParseVector(text,3,bundle->tcGenVectors[0]) ||
                   !ParseVector(text,3,bundle->tcGenVectors[1]))return qfalse;
                bundle->tcGen=TCGEN_VECTOR;
            } else return qfalse;
        } else if(!Q_stricmp(token,"tcMod")) {
            char line[1024];size_t used=0,n;line[0]=0;
            if(!mapped[b])return qfalse;
            while(1) {
                token=COM_ParseExt(text,qfalse);if(!token[0])break;
                n=strlen(token);if(n+2>sizeof(line)-used)return qfalse;
                memcpy(line+used,token,n);used+=n;line[used++]=' ';line[used]=0;
            }
            ParseTexMod(line,&local[b]);
        } else if(!Q_stricmp(token,"blendFunc")) {
            token=COM_ParseExt(text,qfalse);
            if(!Q_stricmp(token,"filter"))env[b]=GL_MODULATE;
            else if(!Q_stricmp(token,"blend"))env[b]=GL_DECAL;
            else if(!Q_stricmp(token,"add") && glConfig.textureEnvAddAvailable &&
                    (!cod1BundleAuthStrict || (auth&COD1_ST_AUTH_ENV_ADD)))env[b]=GL_ADD;
            else return qfalse;
        } else {
            ri.Printf(PRINT_WARNING,"CoD1 R60 ordinary bundle: unsupported selected operation '%s' in shader '%s'\n",token,shader.name);
            return qfalse;
        }
    }
    if(!mapped[b])return qfalse;
    /* Terminal neutral white keeps the established two-bundle output. An
     * intermediate white must remain: the third combiner consumes it. */
    bundle=&local[0].bundle[0];
    if(!b && source[0]==C1MAP_WHITE && env[0]==GL_MODULATE && !bundle->numTexMods && bundle->tcGen==TCGEN_BAD) {
        stage->bundle[1].cod1MapLoadFlags=bundle->cod1MapLoadFlags;
        ri.Printf(PRINT_ALL,"CoD1 neutral white bundle: %s\n",shader.name);
        return qtrue;
    }
    if(!qglActiveTextureARB || glConfig.maxActiveTextures<b+2)return qfalse;
    if(b && (!qglClientActiveTextureARB || !(auth&COD1_ST_AUTH_THIRD)))return qfalse;
    for(k=0;k<=b;++k) {
        bundle=&local[k].bundle[0];
        if(!mapped[k])return qfalse;
        if(b && env[k]==GL_ADD && !(auth&COD1_ST_AUTH_ENV_ADD))return qfalse;
        if(bundle->tcGen==TCGEN_BAD)bundle->tcGen=bundle->isLightmap?TCGEN_LIGHTMAP:TCGEN_TEXTURE;
    }
    /* Publish only after the entire stage-local continuation is valid. */
    for(k=0;k<=b;++k) {
        bundle=&local[k].bundle[0];
        memcpy(stage->bundle[k+1].texMods,mods[k],sizeof(mods[k]));
        bundle->texMods=stage->bundle[k+1].texMods;stage->bundle[k+1]=*bundle;
    }
    shader.multitextureEnv=env[0];stage->cod1TexEnv=env[0];
    if(b) {
        stage->cod1TexEnv2=env[1];
        ri.Printf(PRINT_ALL,"CoD1 R61 ordinary three bundles: %s env1=%d env2=%d auth=%x\n",shader.name,env[0],env[1],auth);
    } else if(source[0]==C1MAP_LIGHTMAP && !bundle->numTexMods && bundle->tcGen==TCGEN_LIGHTMAP && env[0]==GL_MODULATE)
        ri.Printf(PRINT_ALL,"CoD1 lightmap bundle: %s\n",shader.name);
    else {
        ri.Printf(PRINT_ALL,"CoD1 R60 ordinary bundle: %s independent tcGen=%d mods=%d env=%d\n",shader.name,bundle->tcGen,bundle->numTexMods,env[0]);
    }
    return qtrue;
}
