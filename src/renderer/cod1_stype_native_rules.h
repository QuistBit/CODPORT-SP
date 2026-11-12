/* R48 native runtime metadata, not material-family render-state guesses.
 * CoDSP.exe SHA256 af73842d...; parser 0x4efb00, table 0x5646d0.
 * The native runtime ORs ONLY surfaceFlags (+8); clearSolid/contents belong
 * to other consumers. Unknown surfaceparm is a no-op in this parser.
 * Keep native flags separate from RTCW bits and authoritative BSP flags. */
#ifndef COD1_STYPE_NATIVE_RULES_H
#define COD1_STYPE_NATIVE_RULES_H
static unsigned Cod1ST_SurfaceFlag(const char *name,int *known) {
    static const struct {const char *name;unsigned flags;} rows[]={
        {"bark",0x00100000u},
        {"brick",0x00200000u},
        {"carpet",0x00300000u},
        {"cloth",0x00400000u},
        {"concrete",0x00500000u},
        {"dirt",0x00600000u},
        {"flesh",0x00700000u},
        {"foliage",0x00800000u},
        {"glass",0x00900000u},
        {"grass",0x00a00000u},
        {"gravel",0x00b00000u},
        {"ice",0x00c00000u},
        {"metal",0x00d00000u},
        {"mud",0x00e00000u},
        {"paper",0x00f00000u},
        {"plaster",0x01000000u},
        {"rock",0x01100000u},
        {"sand",0x01200000u},
        {"snow",0x01300000u},
        {"water",0x01400000u},
        {"wood",0x01500000u},
        {"asphalt",0x01600000u},
        {"opaqueglass",0x00900000u},
        {"clipmissile",0x00000000u},
        {"ai_nosight",0x00000000u},
        {"clipshot",0x00000000u},
        {"playerclip",0x00000000u},
        {"monsterclip",0x00000000u},
        {"vehicleclip",0x00000000u},
        {"itemclip",0x00000000u},
        {"nodrop",0x00000000u},
        {"nonsolid",0x00004000u},
        {"origin",0x00000000u},
        {"detail",0x00000000u},
        {"structural",0x00000000u},
        {"portal",0x80000000u},
        {"canshootclip",0x00000000u},
        {"sky",0x00000004u},
        {"lightfilter",0x00008000u},
        {"alphashadow",0x00010000u},
        {"castshadow",0x00040000u},
        {"hint",0x00000100u},
        {"slick",0x00000002u},
        {"noimpact",0x00000010u},
        {"nomarks",0x00000020u},
        {"ladder",0x00000008u},
        {"nodamage",0x00000001u},
        {"nosteps",0x00002000u},
        {"nodraw",0x00000080u},
        {"pointlight",0x00000800u},
        {"nolightmap",0x00000400u},
        {"nodlight",0x00020000u},
    };
    unsigned i;
    for(i=0;i<sizeof(rows)/sizeof(rows[0]);++i)if(Cod1MP_Equal(name,rows[i].name)) {
        *known=1;return rows[i].flags;
    }
    *known=0;return 0;
}
#endif
