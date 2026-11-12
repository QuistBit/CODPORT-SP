/* R135 selected native-DLL keybinding transport; no QVM pointer ABI. */
#ifndef BG_COD1_KEYBINDING_H
#define BG_COD1_KEYBINDING_H
#include <stdint.h>

#define COD1_KEYBINDING_WIRE_ABI 1u
#define COD1_KEYBINDING_NAME_CAP 128
#define COD1_KEYBINDING_BINDING_CAP 256
#define COD1_KEYBINDING_PHASE_QUERY 1u
#define COD1_KEYBINDING_PHASE_VALIDATE 2u

typedef struct {
    uint32_t abi, phase;
    uint64_t operation_id;
    char command[COD1_KEYBINDING_NAME_CAP];
    int32_t cg_time, count;
    uint64_t cgame_generation;
    char key1[COD1_KEYBINDING_NAME_CAP], key2[COD1_KEYBINDING_NAME_CAP];
} cod1KeyBindingQuery_t;

#endif
