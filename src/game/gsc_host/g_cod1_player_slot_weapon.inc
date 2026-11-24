/* GAME1.4 16370: case-insensitive fixed slot name; empty slot -> none.
 * Use the occupied position directly. A read must never repair/reorder slots.
 * CoD alternate definitions share a port WP, so decode that slot's mode. */
static int PlayerNativeSlotWeapon(const char *name,const playerState_t *ps,char *out,size_t cap){
 static const char *const names[CW_SLOT_COUNT]={"none","primary","primaryb","pistol","grenade","smokegrenade"};
 const cwDef_t *def;playerState_t selected;int slot,weapon;
 if(!name||!ps||!out||cap<5)return 0;
 for(slot=1;slot<CW_SLOT_COUNT;slot++)if(!Q_stricmp(name,names[slot]))break;
 if(slot==CW_SLOT_COUNT)return 0;
 weapon=ps->cod1Slots[slot];
 if(weapon==WP_NONE){memcpy(out,"none",5);return 1;}
 if(weapon<WP_NONE||weapon>=WP_NUM_WEAPONS||!COM_BitCheck(ps->weapons,weapon)||!CW_SlotCompatible(slot,weapon))return 0;
 selected=*ps;selected.weapon=weapon;def=CW_PSDef(&selected);
 if(!def||!memchr(def->name,0,sizeof(def->name))||!def->name[0]||strlen(def->name)>=cap)return 0;
 memcpy(out,def->name,strlen(def->name)+1);return 1;
}
