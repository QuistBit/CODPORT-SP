#ifndef BG_COD1_VOICE_STREAM_H
#define BG_COD1_VOICE_STREAM_H
#include "bg_cod1_sound_notify_packet.h"
enum {C1V_STREAM_PLAY=1,C1V_STREAM_STOP=2,C1V_STREAM_RESET=3,C1V_STREAM_STATUS=4};
typedef struct cod1VoiceRequest_s {unsigned operation;uint64_t incarnation;C1NPacket packet;unsigned mix_mode;float slave_cap;} cod1VoiceRequest;
/* Return duration only after a real PCM stream has been accepted. Stop/reset
 * return zero on success, -1 on rejection. Identity is never a path command. */
static int C1V_RequestValid(const cod1VoiceRequest *r){
 if(!r||!r->incarnation||r->incarnation==UINT64_MAX||r->mix_mode>2||!CQ_Finite(r->slave_cap)||r->slave_cap<0||(r->mix_mode!=2&&r->slave_cap!=0))return 0;
 if(r->operation==C1V_STREAM_RESET)return r->packet.sound.epoch&&!r->packet.sound.sequence&&!r->packet.session&&!r->packet.generation;
 return (r->operation==C1V_STREAM_PLAY||r->operation==C1V_STREAM_STOP||r->operation==C1V_STREAM_STATUS)&&C1S_Valid(&r->packet.sound)&&r->packet.session&&r->packet.generation&&r->packet.sound.channel==3;
}
#endif
