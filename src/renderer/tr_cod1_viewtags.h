/* SPDX-License-Identifier: GPL-3.0-or-later
 * MUZZLE_R1: retain optional weapon attachments in the baked MD3. No retail
 * assets or proprietary code. Same world-in-model-space transforms as vertices.
 * Included after cod1_viewmodel14.h and the engine's md3Tag_t definition.
 */
#ifndef TR_COD1_VIEWTAGS_H
#define TR_COD1_VIEWTAGS_H
#define CV14_WEAPON_TAGS 6
static int R_CV14ExportTags(const cv14Skeleton *s,md3Tag_t out[CV14_WEAPON_TAGS]) {
    static const char *names[CV14_WEAPON_TAGS]={"tag_flash","tag_brass","tag_weapon",
        "tag_flash_11","tag_flash_2","tag_flash_22"};
    int n=0,i,j,k,a;float unit[3];
    memset(out,0,CV14_WEAPON_TAGS*sizeof(*out));
    for(i=0;i<CV14_WEAPON_TAGS;i++) {
        for(j=0;j<s->n;j++)if(!strcmp(s->name[j],names[i]))break;
        if(j==s->n)continue;
        strcpy(out[n].name,names[i]);
        for(k=0;k<3;k++) {
            if(!(s->world[j].p[k]>=-1000000 && s->world[j].p[k]<=1000000))return -1;
            out[n].origin[k]=s->world[j].p[k];
        }
        for(a=0;a<3;a++) {
            unit[0]=unit[1]=unit[2]=0;unit[a]=1;
            CV14_Rotate(s->world[j].q,unit,out[n].axis[a]);
            for(k=0;k<3;k++)if(!(out[n].axis[a][k]>=-1.001f && out[n].axis[a][k]<=1.001f))return -1;
        }
        n++;
    }
    return n;
}
static int R_CV14TagsMatch(const md3Header_t *a,const md3Header_t *b) {
    const md3Tag_t *ta=(const md3Tag_t*)((const byte*)a+a->ofsTags);
    const md3Tag_t *tb=(const md3Tag_t*)((const byte*)b+b->ofsTags);int i;
    if(a->numTags!=b->numTags)return 0;
    for(i=0;i<a->numTags;i++)if(strcmp(ta[i].name,tb[i].name))return 0;
    return 1;
}
#endif
