#ifndef BG_COD1_ALIAS_SLAVE_H
#define BG_COD1_ALIAS_SLAVE_H
/* Selected current alias parameters travel with each sound, never a global
 * WAV-handle override. Native cap is applied after attenuation, before gain. */
typedef struct cod1AliasSlaveRequest_s {int looping,entity,channel,handle,range,volume;float origin[3],velocity[3],gain,pitch,dist_min,dist_max,slave_cap;} cod1AliasSlaveRequest;
#endif
