/* Selected native compiler developer_script policy. Parsed source and original
 * IR stay owned and intact. Only exact recorded developer-region spans are
 * absent from validation/emission when the captured setting is zero. */
int gsc_program_select_r232_developer_script(GscProgram *g,const GscDeveloperScriptSelection *input) {
    GscDeveloperScriptSelection copy;
    if(!g||!input||g->invalid||g->linked||g->module_count||!g->function_admission_selected||g->developer_selected)return 0;
    copy=*input;if((copy.enabled!=0&&copy.enabled!=1)||!copy.certificate.receipt||!copy.certificate.current)return 0;
    g->developer=copy;g->developer_selected=1;return 1;
}
int gsc_program_select_developer_value(GscProgram*g,int enabled){
 if(!g||g->invalid||g->linked||g->module_count||!g->function_admission_selected||g->developer_selected||(enabled!=0&&enabled!=1))return 0;
 memset(&g->developer,0,sizeof(g->developer));g->developer.enabled=enabled;
 g->developer_selected=g->developer_value_only=1;return 1;
}
static int developer_code_hidden(const GscProgram *g,int pc) {
    int n;if(!g->developer_selected||g->developer.enabled)return 0;
    for(n=0;n<g->dev_count;n++)if(pc>=g->dev_blocks[n].code_start&&pc<g->dev_blocks[n].code_end)return 1;
    return 0;
}
static int developer_expr_hidden(const GscProgram *g,int expr) {
    int n;if(!g->developer_selected||g->developer.enabled)return 0;
    for(n=0;n<g->dev_count;n++)if(expr>=g->dev_blocks[n].expr_start&&expr<g->dev_blocks[n].expr_end)return 1;
    return 0;
}
static int developer_func_hidden(const GscProgram *g,int fn) {
    int n;if(!g->developer_selected||g->developer.enabled)return 0;
    for(n=0;n<g->dev_count;n++)if(g->dev_blocks[n].scope==GSC_DEV_FILE&&fn>=g->dev_blocks[n].func_start&&fn<g->dev_blocks[n].func_end)return 1;
    return 0;
}
static void developer_validation_view(GscProgram *g) {
    int b,n;if(!g->developer_selected)return;
    g->requires_dev_block=0;
    if(g->developer.enabled)return;
    /* The old per-slot predicate tested membership in the union of these
     * half-open spans. Visit their intersection with the validation view
     * directly; an overlap writes the same canonical empty slot twice. */
    for(b=0;b<g->dev_count;b++){
        const DevBlock *d=&g->dev_blocks[b];
        for(n=d->code_start<0?0:d->code_start;n<d->code_end&&n<g->code_count;n++){
            memset(&g->code[n],0,sizeof(g->code[n]));g->code[n].kind=I_EMPTY;
        }
        for(n=d->expr_start<0?0:d->expr_start;n<d->expr_end&&n<g->expr_count;n++){
            memset(&g->expr[n],0,sizeof(g->expr[n]));g->expr[n].kind=E_NUMBER;
        }
    }
}
static int developer_skip_region(const GscProgram *g,int pc) {
    int n;if(!g->developer_selected||g->developer.enabled)return pc;
    for(n=0;n<g->dev_count;n++)if(pc>=g->dev_blocks[n].code_start&&pc<g->dev_blocks[n].code_end)return g->dev_blocks[n].code_end;
    return pc;
}
