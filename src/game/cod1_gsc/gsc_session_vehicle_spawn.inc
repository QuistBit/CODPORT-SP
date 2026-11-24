int gsc_session_select_vehicle_spawn(GscSession*s,const GscVehicleSpawnHost*input,char*error,size_t cap){
 GscVehicleSpawnHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"vehicle factory requires Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->vehicle_spawn_enabled||!s->execution_extensions||!h.prepare||!h.finish)
  return session_leave_rejected(s,"vehicle factory requires idle extended Session",error,cap);
 s->vehicle_spawn=h;s->vehicle_spawn_enabled=1;s->busy=0;return 1;
}
static int session_vehicle_spawn_prepare(void*user,const GscVehicleSpawnRequest*input,GscVehicleSpawnTicket*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscVehicleSpawnHost host;GscVehicleSpawnRequest q,expected;GscVehicleSpawnTicket result;int ok,valid;
 if(!s||!input||!out||!current||!s->vehicle_spawn_enabled||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;expected=q;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;host=s->vehicle_spawn;
 if(!session_player_current(&lease)||q.now_ms!=s->view.logical_ms)return 0;
 memset(&result,0,sizeof(result));
 ok=host.prepare(host.user,&q,&result,error,cap,session_player_current,&lease);
 /* A reservation must survive the wrapper's failure path so the VM can
  * cancel it. This is not publication of a script value. */
 *out=result;
 valid=session_player_current(&lease)&&q.now_ms==s->view.logical_ms&&!memcmp(&host,&s->vehicle_spawn,sizeof(host))&&!memcmp(&q,&expected,sizeof(q))&&!memcmp(input,&expected,sizeof(q));
 return ok==1&&valid;
}
static int session_vehicle_spawn_finish(void*user,const GscVehicleSpawnTicket*input,int commit,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=user;SessionPlayerLease lease;GscVehicleSpawnHost host;GscVehicleSpawnTicket ticket,expected;int ok;
 if(!s||!input||!current||!s->vehicle_spawn_enabled||(commit!=0&&commit!=1)||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 ticket=*input;expected=ticket;host=s->vehicle_spawn;
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=ctx;
 if(commit&&!session_player_current(&lease))return 0;
 ok=host.finish(host.user,&ticket,commit,error,cap,session_player_current,&lease);
 if(ok!=1||memcmp(&ticket,&expected,sizeof(ticket))||memcmp(input,&expected,sizeof(ticket))||memcmp(&host,&s->vehicle_spawn,sizeof(host)))return 0;
 return !commit||session_player_current(&lease);
}
