/* R91 uses the existing restoration EF_NODRAW snapshot flag for CoD entity
 * visibility. Hiding retains snapshots/events/sounds and server collision.
 * No native numeric flag, layout change, model-name or map-name exception. */
#ifndef CG_COD1_ENTITY_VISIBILITY_H
#define CG_COD1_ENTITY_VISIBILITY_H

static qboolean CG_Cod1EntityHidden( const entityState_t *state ) {
    return cgs.cod1MapEntities && state && ( state->eFlags & EF_NODRAW );
}

#endif
