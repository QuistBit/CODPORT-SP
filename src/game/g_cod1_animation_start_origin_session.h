#ifndef G_COD1_ANIMATION_START_ORIGIN_SESSION_H
#define G_COD1_ANIMATION_START_ORIGIN_SESSION_H
#include "g_cod1_animation_start_origin.h"
#include "cod1_gsc/gsc_fixture.h"
/* R209 selected typed facade over the actual R208 owned GAME resource bank.
 * The certificate is pure and remains readable during an active query. Its
 * receipt cannot migrate across MapBegin/End or resource-bank replacement.
 * The accessor grants no resource selection and retains no Session pointer. */
int G_Cod1StartOriginMapCertificate(GscMapExecutionCertificate *,char *,size_t);
const GscStartOriginHost *G_Cod1StartOriginQueryHost(void);
/* Separate optional R212 fixed Guy+gear facade; grants no resource selection. */
const GscStartOriginHost *G_Cod1NamespaceStartOriginQueryHost(void);
#endif
