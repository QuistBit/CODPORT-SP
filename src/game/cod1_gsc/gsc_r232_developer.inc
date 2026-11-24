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
/* Final linked parser-owned spans cannot subsequently be extended. Keep
 * the ordered first-match fallback for overlapping or unsorted test IR. */
static void developer_index_code(GscProgram *g) {
 int i,last=0;g->dev_code_ordered=0;
 if(!g->developer_selected||g->developer.enabled||g->dev_count<8)return;
 for(i=0;i<g->dev_count;i++){
  const DevBlock *b=g->dev_blocks+i;
  if(b->code_start<last||b->code_end<b->code_start||b->code_end>g->code_count)return;
  last=b->code_end;
 }
 g->dev_code_ordered=1;
}
static int developer_skip_region(const GscProgram *g,int pc) {
    int n;if(!g->developer_selected||g->developer.enabled)return pc;
    if(g->linked && g->dev_code_ordered) {
        int lo=0,hi=g->dev_count;
        while(lo<hi){int mid=lo+(hi-lo)/2;if(g->dev_blocks[mid].code_end<=pc)lo=mid+1;else hi=mid;}
        if(lo<g->dev_count && g->dev_blocks[lo].code_start<=pc)return g->dev_blocks[lo].code_end;
        return pc;
    }
    for(n=0;n<g->dev_count;n++)if(pc>=g->dev_blocks[n].code_start&&pc<g->dev_blocks[n].code_end)return g->dev_blocks[n].code_end;
    return pc;
}

/* Native developer emission with strict validation of all ordinary functions.
 * This selection does not enable deferred function admission. */
int gsc_program_select_map_developer_script(GscProgram*g,const GscDeveloperScriptSelection*input){
 GscDeveloperScriptSelection copy;
 if(!g||!input||g->invalid||g->linked||g->module_count||!g->animation_namespace_selected||!g->map_animation_names.count||g->function_admission_selected||g->turret_animations_selected||g->developer_selected)return 0;
 copy=*input;if((copy.enabled!=0&&copy.enabled!=1)||!copy.certificate.receipt||!copy.certificate.current)return 0;
 g->developer=copy;g->developer_selected=g->map_developer_selected=1;return 1;
}
