int gsc_session_select_r232_randomfloat_range(GscSession*s,char*error,size_t cap){
    if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||
       !s->dynamic_randomfloat_enabled||!s->randomfloat_enabled||!s->randomfloat.draw||s->random_range_enabled)
        return session_leave_rejected(s,"randomFloatRange requires idle two-ATR Session and real shared draw host",error,cap);
    if(!session_animation_resources_current(s))return session_leave_rejected(s,"randomFloatRange map/resource scope retired",error,cap);
    s->random_range_enabled=1;s->busy=0;return 1;
}
