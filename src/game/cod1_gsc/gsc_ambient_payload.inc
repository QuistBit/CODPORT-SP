#ifndef GSC_AMBIENT_PAYLOAD_INCLUDED
#define GSC_AMBIENT_PAYLOAD_INCLUDED
/* Selected private transport domain; original Scr_GetString coercions and
 * broader Info-string acceptance are not inferred. Preserve admitted bytes. */
static int ambient_event_alias_valid(const GscAmbientEvent *event) {
    size_t n;
    if(event->kind==GSC_AMBIENT_STOP)return event->alias[0]==0;
    if(event->kind!=GSC_AMBIENT_PLAY||!event->alias[0]||
       !memchr(event->alias,0,sizeof(event->alias)))return 0;
    for(n=0;event->alias[n];n++) {
        unsigned char c=(unsigned char)event->alias[n];
        if(c<33||c>126||c=='\\'||c=='"')return 0;
    }
    return 1;
}
#endif
