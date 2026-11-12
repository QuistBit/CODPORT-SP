#ifndef GSC_SAVE_GAME_PAYLOAD_INCLUDED
#define GSC_SAVE_GAME_PAYLOAD_INCLUDED
/* R123 request payload validation. This proves a copied native request,
 * never successful file writing or restoration of the script VM. */
static int save_game_event_valid(const GscSaveGameEvent *e) {
    return e && e->operation_id && e->operation_id!=UINT64_MAX &&
        memchr(e->filename,0,sizeof(e->filename)) && e->filename[0] &&
        memchr(e->description,0,sizeof(e->description)) &&
        memchr(e->image,0,sizeof(e->image));
}
#endif
