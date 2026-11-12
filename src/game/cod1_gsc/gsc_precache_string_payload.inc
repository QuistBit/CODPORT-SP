/* R129 copied startup publication. Public pool ownership belongs to the GAME
 * configstring service, never to a VM-private string table. Printable ASCII
 * <=127 bytes is explicit port admission, not original getter conversion.
 * Quote breaks the public cs command; percent is lossy in MSG_Read*String.
 * Backslash is excluded by the current public service's narrow wire domain. */
static int precache_string_name_valid(const GscPrecacheStringEvent *event) {
    size_t n;
    if(!event || !memchr(event->name,0,sizeof(event->name)))return 0;
    for(n=0;event->name[n];++n) {
        unsigned char c=(unsigned char)event->name[n];
        if(c<32 || c>126 || c=='"' || c=='%' || c=='\\')return 0;
    }
    return 1;
}
static int precache_string_event_valid(const GscPrecacheStringEvent *event) {
    return event && event->operation_id && event->name[0] &&
        precache_string_name_valid(event);
}
