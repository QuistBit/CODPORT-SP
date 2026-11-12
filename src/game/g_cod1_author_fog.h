/* R78: initial authored atmosphere profile only. This does not execute mission
 * GSC, event threads, entity receivers or restore a mission VM from saves. */
#ifndef G_COD1_AUTHOR_FOG_H
#define G_COD1_AUTHOR_FOG_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
void G_Cod1AuthorFogInitAfterMapBootstrap( int levelTime );
/* Pure current-owner admission; no VM serialization is supplied. */
int G_Cod1AuthorFogBlocksRawSave(void);
#else
#define G_Cod1AuthorFogInitAfterMapBootstrap(t) ((void)0)
#define G_Cod1AuthorFogBlocksRawSave() (0)
#endif
#endif
