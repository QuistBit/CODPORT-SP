/* Separate host time and original VM bucket cursor. No default activation. */
static int session_r237_frame_private(const GscSession *s,const void *p,size_t n) {
    return session_animation_namespace_private(s,p,n)||
        (s&&s->runtime&&!gsc_runtime_storage_separate(s->runtime,p,n));
}
static int session_r237_frame_resources_current(GscSession *s) {
    return session_animation_resources_current(s)&&
        (!s->actor_bootstrap_enabled||session_actor_map_current(s));
}
int gsc_session_select_r237_frame_buckets(GscSession *s,char *error,size_t cap) {
    if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
    if(!s)return session_reject(error,cap,"R237 frame selection requires Session");
    if(!session_enter(s,error,cap))return 0;
    if(!s->host.enabled||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
       s->r237_frame_buckets_enabled)
        return session_leave_rejected(s,"R237 frame selection requires idle EMPTY Session",error,cap);
    if(!session_r237_frame_resources_current(s)) {
        if(s->abort_requested)return session_failed(s,"R237 frame selection cancelled",error,cap);
        return session_leave_rejected(s,"R237 frame selection resources retired",error,cap);
    }
    s->r237_frame_buckets_enabled=1;s->busy=0;return 1;
}
int gsc_session_r237_frame_buckets_selected(const GscSession *s) {
    return s&&s->r237_frame_buckets_enabled;
}
int gsc_session_r237_frame_view(const GscSession *s,uint64_t id,GscR237FrameView *out) {
    GscR237FrameView copy;
    if(!s||!out||session_r237_frame_private(s,out,sizeof(*out))||
       !s->r237_frame_buckets_enabled||!session_id_matches(s,id)||!s->runtime||
       s->abort_requested||s->view.error[0]||
       (s->view.state!=GSC_SESSION_RUNNING&&s->view.state!=GSC_SESSION_COMPLETED)||
       !gsc_runtime_r237_frame_view(s->runtime,&copy))return 0;
    *out=copy;return 1;
}
int gsc_session_r237_frame_begin(GscSession *s,uint64_t id,int64_t now,
    uint64_t *out_generation,char *error,size_t cap) {
    int64_t delta;uint64_t generation;GscR237FrameView before;
    if(!out_generation||(cap&&!error)||
       session_r237_frame_private(s,out_generation,sizeof(*out_generation))||
       session_r237_frame_private(s,error,cap)||
       session_animation_namespace_overlap(out_generation,sizeof(*out_generation),error,cap))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(!session_id_matches(s,id)||!s->r237_frame_buckets_enabled||!s->runtime||
       (s->view.state!=GSC_SESSION_RUNNING&&s->view.state!=GSC_SESSION_COMPLETED))
        return session_leave_rejected(s,"R237 frame Session unavailable",error,cap);
    if(now<0||now<=s->view.last_host_ms||s->view.origin_host_ms<0)
        return session_leave_rejected(s,"R237 frame host time must advance",error,cap);
    delta=now-s->view.origin_host_ms;
    if(delta<0||delta>0xFFFFFF||
       !gsc_runtime_r237_frame_view(s->runtime,&before)||before.frame_open)
        return session_leave_rejected(s,"R237 frame range or open stage refused",error,cap);
    if(!session_r237_frame_resources_current(s))
        return session_failed(s,"R237 frame resources retired before begin",error,cap);
    if(!gsc_r237_frame_begin(s->runtime,(int)delta,&generation)) {
        if(s->abort_requested||gsc_runtime_error(s->runtime)[0])
            return session_failed(s,gsc_runtime_error(s->runtime)[0]?
                gsc_runtime_error(s->runtime):"R237 frame begin cancelled",error,cap);
        return session_leave_rejected(s,"R237 frame begin refused",error,cap);
    }
    /* VM begin is callback-free. Publish physical host time before any later
     * phase can enter typed hosts or their native resource lease checks. */
    s->view.last_host_ms=now;s->view.logical_ms=(int)delta;
    if(!session_r237_frame_resources_current(s)||s->abort_requested)
        return session_failed(s,"R237 frame resources retired during begin",error,cap);
    *out_generation=generation;s->busy=0;return 1;
}
/* GAME crossed a presentation boundary outside a VM dispatch. Check the
 * original map/animation resources before the next world operation. The VM
 * phase entry already checks these itself when it is the next operation. */
int gsc_session_resume_game_frame(GscSession *s,uint64_t id,int now,uint64_t generation,int open){
    GscR237FrameView frame;
    int valid;
    if(!session_enter(s,NULL,0))return 0;
    valid=session_id_matches(s,id) && s->runtime && !s->view.error[0] &&
          (s->view.state==GSC_SESSION_RUNNING || s->view.state==GSC_SESSION_COMPLETED) &&
          s->view.last_host_ms==now;
    if(valid && generation){
        valid=gsc_runtime_r237_frame_view(s->runtime,&frame) && frame.frame_generation==generation &&
              frame.frame_open==open && frame.host_now_ms==s->view.logical_ms;
    }
    if(valid)valid=session_r237_frame_resources_current(s) && !s->abort_requested;
    s->busy=0;
    return valid;
}
int gsc_session_r237_frame_phase_slice(GscSession *s,uint64_t id,uint64_t generation,
    int exclusive,int (*expired)(void),char *error,size_t cap) {
    GscR237FrameView frame;int ok;
    if((cap&&!error)||session_r237_frame_private(s,error,cap))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(!session_id_matches(s,id)||!s->r237_frame_buckets_enabled||!s->runtime||
       (s->view.state!=GSC_SESSION_RUNNING&&s->view.state!=GSC_SESSION_COMPLETED)||
       !generation||!gsc_runtime_r237_frame_view(s->runtime,&frame)||
       !frame.frame_open||frame.frame_generation!=generation)
        return session_leave_rejected(s,"R237 frame Session or generation stale",error,cap);
    if(frame.host_now_ms!=s->view.logical_ms)
        return session_failed(s,"R237 frame physical host time differs",error,cap);
    if((exclusive!=0 && exclusive!=1) || (s->r237_suspended_phase && s->r237_suspended_phase!=1+exclusive))
        return session_leave_rejected(s,"R237 unfinished frame phase differs",error,cap);
    if(!session_r237_frame_resources_current(s))
        return session_failed(s,"R237 frame resources retired before phase",error,cap);
    ok=gsc_r237_frame_phase_slice(s->runtime,generation,exclusive,expired);
    if(ok==2 && !s->abort_requested){
        s->r237_suspended_phase=1+exclusive;s->busy=0;
        return 2;
    }
    s->r237_suspended_phase=0;
    if(ok&&!s->abort_requested&&!session_r237_frame_resources_current(s))
        return session_failed(s,"R237 frame resources retired during phase",error,cap);
    return session_after_vm(s,ok,error,cap);
}
int gsc_session_r237_run_current_bucket(GscSession *s,uint64_t id,uint64_t generation,
    char *error,size_t cap) {
    return gsc_session_r237_frame_phase_slice(s,id,generation,0,NULL,error,cap);
}
int gsc_session_r237_advance_exclusive(GscSession *s,uint64_t id,uint64_t generation,
    char *error,size_t cap) {
    return gsc_session_r237_frame_phase_slice(s,id,generation,1,NULL,error,cap);
}
