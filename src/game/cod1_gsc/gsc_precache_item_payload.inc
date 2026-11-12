#ifndef GSC_PRECACHE_ITEM_PAYLOAD_INCLUDED
#define GSC_PRECACHE_ITEM_PAYLOAD_INCLUDED
/* R121 selected bounded item-name admission. Pickup names and the native
 * vector format may contain spaces. Preserve spelling for host lookup. */
static int precache_item_event_name_valid(const GscPrecacheItemEvent *event) {
    size_t n;
    if(!memchr(event->name,0,sizeof(event->name)))return 0;
    for(n=0;event->name[n];n++) {
        unsigned char c=(unsigned char)event->name[n];
        if(c<32||c>126)return 0;
    }
    return 1;
}
#endif
