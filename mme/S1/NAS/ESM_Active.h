/* This application was initially developed as a Final Project by
 *     Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
 * under the supervision of,
 *     Jukka Manner (jukka.manner@aalto.fi)
 *     Jose Costa-Requena (jose.costa@aalto.fi)
 * in AALTO University and partially funded by EIT ICT labs.
 */

/**
 * @file   ESM_Active.h
 * @Author Vicent Ferrer
 * @date   June, 2015
 * @brief  ESM State
 *
 */

#ifndef ESM_ACTIVE_HFILE
#define ESM_ACTIVE_HFILE

#include "ESM_State.h"

typedef struct{
	ESMSTATE
}ESM_Active;

void linkESMActive(ESM_State* s);

#endif /* ESM_ACTIVE_HFILE */