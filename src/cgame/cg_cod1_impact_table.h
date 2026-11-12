/* SPDX-License-Identifier: GPL-3.0-or-later
 * IMPACT_R1. Three-state CoD impact CSV, independent of engine IO.
 * Uses cePath/ceEqual from cg_cod1_efx_parse.h. No retail data embedded.
 */
#ifndef CG_COD1_IMPACT_TABLE_H
#define CG_COD1_IMPACT_TABLE_H
#define CI_MATERIALS 23
#define CI_CATEGORIES 4
static const char *ciMaterials[CI_MATERIALS]={"default","bark","brick","carpet","cloth",
    "concrete","dirt","flesh","foliage","glass","grass","gravel","ice","metal","mud",
    "paper","plaster","rock","sand","snow","water","wood","asphalt"};
static const char *ciCategories[CI_CATEGORIES]={"bullet_small_normal","bullet_small_reflect",
    "bullet_large_normal","bullet_large_reflect"};
typedef struct { int present; char path[CEFX_PATH]; } ciRow;
typedef struct { ciRow row[CI_CATEGORIES][CI_MATERIALS]; } ciTable;
static int ciIndex(const char *s,const char *const *names,int count) {
    int i;for(i=0;i<count;i++)if(ceEqual(s,names[i]))return i;return -1;
}
/* Caller applies on a copy for atomic overlay. Omitted third column is an
 * explicit empty value, NOT an absent definition. Later rows clear earlier FX. */
static int ciParseCSV(ciTable *table,const char *text,int size,char error[256]) {
    int pos=0,row=0,changed=0;
    error[0]=0;
    if(!text || size<=0 || size>1024*1024 || memchr(text,0,(size_t)size))return -1;
    if(size>=3 && (unsigned char)text[0]==239 && (unsigned char)text[1]==187 && (unsigned char)text[2]==191)pos=3;
    while(pos<size) {
        char col[3][CEFX_PATH];int n=0,k=0,quoted=0,closed=0,bad=0,c,m;char ch;
        memset(col,0,sizeof(col));++row;
        while(pos<size) {
            ch=text[pos++];
            if(quoted) {
                if(ch=='"') { if(pos<size && text[pos]=='"')++pos;else {quoted=0;closed=1;continue;} }
                else if(ch=='\r'||ch=='\n') {bad=1;break;}
            } else {
                if(ch=='\r'||ch=='\n')break;
                if(ch==',' && n<3) {++n;k=0;closed=0;continue;}
                if(ch=='"' && !k && !closed) {quoted=1;continue;}
                if(closed) {if(ch!=' ' && ch!='\t')bad=1;continue;}
            }
            if(n<3) { if(k+1<CEFX_PATH)col[n][k++]=ch;else bad=1; }
            else bad=1;
        }
        while(pos<size && (text[pos]=='\r'||text[pos]=='\n'))++pos;
        /* Trim cells; comments and unrelated categories do not consume slots. */
        for(c=0;c<3;c++) {
            size_t a=0,b=strlen(col[c]);while(a<b && isspace((unsigned char)col[c][a]))++a;
            while(b>a && isspace((unsigned char)col[c][b-1]))--b;
            memmove(col[c],col[c]+a,b-a);col[c][b-a]=0;
        }
        if(!col[0][0] || col[0][0]=='#')continue;
        c=ciIndex(col[0],ciCategories,CI_CATEGORIES);if(c<0)continue;
        m=ciIndex(col[1],ciMaterials,CI_MATERIALS);
        if(bad||quoted||n<1||n>2||m<0) {
            snprintf(error,256,"invalid impact CSV row %d",row);return -1;
        }
        if(col[2][0] && !cePath(col[2],table->row[c][m].path,1)) {
            snprintf(error,256,"unsafe impact path at row %d",row);return -1;
        }
        if(!col[2][0])table->row[c][m].path[0]=0;
        table->row[c][m].present=1;++changed;
    }
    return changed;
}
#endif
