/* Typed in-process request for an animated CoD1 actor. This does not change
 * refEntity_t, network entityState_t, or the renderer refexport_t layout. */
#ifndef BG_COD1_ACTOR_RUNTIME_H
#define BG_COD1_ACTOR_RUNTIME_H

#define COD1_ACTOR_CGAME_ABI 59 /* R232: current animated turret tag query. */
#define COD1_ACTOR_STATE_VERSION 1
#define COD1_ACTOR_MODEL_ALIAS "cod1actor/wehrmacht_mp40a"
#define COD1_ACTOR_FOLEY_ALIAS "cod1actor/foley_training"
#define COD1_ACTOR_FOLEY_PROFILE 1
#define COD1_ACTOR_MOODY_PROFILE 2
#define COD1_ACTOR_MOODY_ALIAS "cod1actor/moody_training"
#define COD1_ACTOR_MOODY_WALK_FRAME 223
#define COD1_ACTOR_NET_CLASSNAME "cod1_actor_server_debug"
/* entityState_t.density is sent with 10 bits in the existing protocol. */
#define COD1_ACTOR_NET_MARKER 677
/* Existing network frame field selects the opt-in visual sequence. */
#define COD1_ACTOR_FOLEY_GESTURE_FRAME 197

enum {
    COD1_ACTOR_CLIP_IDLE = 1,
    COD1_ACTOR_CLIP_FOLEY_BREATH = 2,
    COD1_ACTOR_CLIP_FOLEY_GESTURE_SCENE = 3,
    COD1_ACTOR_CLIP_MOODY_IDLE = 4,
    COD1_ACTOR_CLIP_MOODY_WALK = 5
};

typedef struct {
    int version;
    int size;
    int clip;
    int elapsedMs;
} cod1ActorAnimState_t;

#endif
