/* Native467F50 consumes only a matched delivered event registration before
 * queueing continuation. Task-self, frame-self, execution/map certificates and
 * endon subscriptions are independent. Legacy mapless profiles stay bounded. */
int gsc_program_select_owned_wait_delivery(GscProgram*g){
 if(!g||g->invalid||g->linked||g->owned_wait_delivery_selected||
    !g->event_payload_syntax||!g->lifetime_syntax||!g->names_syntax)return 0;
 g->owned_wait_delivery_selected=1;return 1;
}
int gsc_runtime_select_owned_wait_delivery(GscRuntime*r){
 if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
    r->owned_wait_delivery_enabled||!r->program->owned_wait_delivery_selected||
    !r->events_enabled||!r->event_payloads_enabled||!r->state_enabled||
    !r->results_enabled||!r->lifetime_enabled||!r->endon_enabled||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
    r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->hud_elements_busy)return 0;
 r->owned_wait_delivery_enabled=1;return 1;
}
