int gsc_session_select_turret_spawn(GscSession*s,const GscTurretSpawnHost*input,char*error,size_t cap){
 GscTurretSpawnHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"turret factory requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->turret_spawn_enabled||!s->execution_extensions||!h.prepare||!h.finish)
  return session_leave_rejected(s,"turret factory requires idle extended Session",error,cap);
 s->turret_spawn=h;s->turret_spawn_enabled=1;s->busy=0;return 1;
}
static int session_turret_spawn_prepare(void*user,const GscTurretSpawnRequest*input,GscTurretSpawnTicket*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscTurretSpawnHost host;GscTurretSpawnRequest q,expected;GscTurretSpawnTicket result;int ok,valid;
 if(!s||!input||!out||!current||!s->turret_spawn_enabled||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;expected=q;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->turret_spawn;
 if(!session_player_current(&lease)||q.now_ms!=s->view.logical_ms)return 0;
 memset(&result,0,sizeof(result));
 ok=host.prepare(host.user,&q,&result,error,cap,session_player_current,&lease);
 /* A reservation must survive the wrapper's failure path so the VM can
  * cancel it. This is not publication of a script value. */
 *out=result;
 valid=session_player_current(&lease)&&q.now_ms==s->view.logical_ms&&!memcmp(&host,&s->turret_spawn,sizeof(host))&&!memcmp(&q,&expected,sizeof(q))&&!memcmp(input,&expected,sizeof(q));
 return ok==1&&valid;
}
static int session_turret_spawn_finish(void*user,const GscTurretSpawnTicket*input,int commit,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscTurretSpawnHost host;GscTurretSpawnTicket ticket,expected;int ok;
 if(!s||!input||!current||!s->turret_spawn_enabled||(commit!=0&&commit!=1)||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 ticket=*input;expected=ticket;host=s->turret_spawn;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;
 if(commit&&!session_player_current(&lease))return 0;
 ok=host.finish(host.user,&ticket,commit,error,cap,session_player_current,&lease);
 if(ok!=1||memcmp(&ticket,&expected,sizeof(ticket))||memcmp(input,&expected,sizeof(ticket))||memcmp(&host,&s->turret_spawn,sizeof(host)))return 0;
 return !commit||session_player_current(&lease);
}
