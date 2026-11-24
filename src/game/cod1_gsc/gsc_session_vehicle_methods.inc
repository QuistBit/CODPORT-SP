int gsc_session_select_vehicle_methods(GscSession*s,const GscVehicleMethodHost*input,char*error,size_t cap){GscVehicleMethodHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"vehicle methods require Session/provider");h=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->vehicle_methods_enabled||!s->execution_extensions||!h.invoke)return session_leave_rejected(s,"vehicle methods require idle extended Session",error,cap);
 s->vehicle_methods=h;s->vehicle_methods_enabled=1;s->busy=0;return 1;
}
static int session_vehicle_method(void*u,const GscVehicleMethodRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSession*s=u;GscVehicleMethodRequest q,expected;GscVehicleMethodHost h;SessionPlayerLease l;int ok;
 if(!s||!input||!current||(cap&&!error)||!s->vehicle_methods_enabled||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 q=expected=*input;h=s->vehicle_methods;l.session=s;l.id=s->view.session_id;l.current=current;l.context=ctx;
 if(!session_player_current(&l)||q.now_ms!=s->view.logical_ms)return 0;
 ok=h.invoke(h.user,&q,error,cap,session_player_current,&l);
 return ok==1&&session_player_current(&l)&&q.now_ms==s->view.logical_ms&&!memcmp(&h,&s->vehicle_methods,sizeof(h))&&!memcmp(&q,&expected,sizeof(q))&&!memcmp(input,&expected,sizeof(q));
}
