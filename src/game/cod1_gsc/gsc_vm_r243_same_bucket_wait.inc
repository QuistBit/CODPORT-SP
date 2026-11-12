/* Original464600..4646D9 permits signedzero / roundedzero milliseconds,
 * serializes the thread and queues it at VMpubTime, rather than running the
 * following statement inline.45DFA0/468BB0 supply the retained LIFO ordering.
 * Explicit selection keeps R237's narrower positive-delay policy unchanged.
 * Nonwrapping24bit cursor and shared instruction budget remain required. */
int gsc_program_select_r243_same_bucket_wait(GscProgram*g){
 if(!g||g->invalid||g->linked||g->r243_same_bucket_wait_selected||!g->r243_thread_start_selected)return 0;
 g->r243_same_bucket_wait_selected=1;return 1;
}
int gsc_runtime_select_r243_same_bucket_wait(GscRuntime*r){
 if(!r||!r->program||!r->program->r243_same_bucket_wait_selected||!r->r243_thread_start_enabled||
    r->r243_same_bucket_wait_enabled||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_field_busy||r->entity_method_busy)return 0;
 r->r243_same_bucket_wait_enabled=1;return 1;
}
int gsc_runtime_r243_same_bucket_wait_selected(const GscRuntime*r){
 return r&&r->program&&r->program->r243_same_bucket_wait_selected&&r->r243_same_bucket_wait_enabled&&!r->error[0];
}
