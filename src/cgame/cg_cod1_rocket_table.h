/* SPDX-License-Identifier: GPL-3.0-or-later
 * CoD1 rocket impact CSV selector. Retail data is read from VFS at runtime;
 * no original table contents are embedded in the engine.
 */
#ifndef CG_COD1_ROCKET_TABLE_H
#define CG_COD1_ROCKET_TABLE_H
#include "cg_cod1_impact_table.h"

typedef struct { int present; char path[CEFX_PATH]; } crRow;
typedef struct { crRow row[CI_MATERIALS]; } crTable;

/* Parse only rocket_explode rows. Unrelated categories are ignored. Later
 * rows replace earlier rows so sorted CSV overlays can use normal VFS order. */
static int crParseCSV(crTable *table,const char *text,int size,char error[256]) {
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
                if(ch=='\"') { if(pos<size && text[pos]=='\"')++pos;else {quoted=0;closed=1;continue;} }
                else if(ch=='\r'||ch=='\n') {bad=1;break;}
            } else {
                if(ch=='\r'||ch=='\n')break;
                if(ch==',' && n<3) {++n;k=0;closed=0;continue;}
                if(ch=='\"' && !k && !closed) {quoted=1;continue;}
                if(closed) {if(ch!=' ' && ch!='\t')bad=1;continue;}
            }
            if(n<3) { if(k+1<CEFX_PATH)col[n][k++]=ch;else bad=1; }
            else bad=1;
        }
        while(pos<size && (text[pos]=='\r'||text[pos]=='\n'))++pos;
        for(c=0;c<3;c++) {
            size_t a=0,b=strlen(col[c]);while(a<b && isspace((unsigned char)col[c][a]))++a;
            while(b>a && isspace((unsigned char)col[c][b-1]))--b;
            memmove(col[c],col[c]+a,b-a);col[c][b-a]=0;
        }
        if(!col[0][0] || col[0][0]=='#' || !ceEqual(col[0],"rocket_explode"))continue;
        m=ciIndex(col[1],ciMaterials,CI_MATERIALS);
        if(bad||quoted||n<1||n>2||m<0) {
            snprintf(error,256,"invalid rocket impact CSV row %d",row);return -1;
        }
        if(col[2][0] && !cePath(col[2],table->row[m].path,1)) {
            snprintf(error,256,"unsafe rocket impact path at row %d",row);return -1;
        }
        if(!col[2][0])table->row[m].path[0]=0;
        table->row[m].present=1;++changed;
    }
    return changed;
}
#endif
