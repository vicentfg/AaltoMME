/* AaltoMME - Mobility Management Entity for LTE networks
 * Copyright (C) 2013 Vicent Ferrer Guash & Jesus Llorente Santos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   EMMCtx.c
 * @Author Vicent Ferrer
 * @date   July, 2015
 * @brief  EMMCtx Information
 *
 * This module implements the EMMCtx Information
 */

#include <string.h>
#include "EMMCtx.h"
#include "logmgr.h"
#include "ECMSession_priv.h"
#include "EMM_FSMConfig.h"

#include <time.h>
#include <stdlib.h>

void emm_log_(EMMCtx s, int pri, char *fn, const char *func, int ln,
              int en, char *fmt, ...){
    EMMCtx_t *self = (EMMCtx_t*)s;

    va_list args;
    char buf[SYSERR_MSGSIZE];
    size_t len;
    bzero(buf, SYSERR_MSGSIZE);

    snprintf(buf, SYSERR_MSGSIZE, "%s %" PRIu64": ",
             EMMStateName[self->stateName],
             self->imsi);

    len = strlen(buf);
    va_start(args, fmt);
    vsnprintf(buf+len, SYSERR_MSGSIZE, fmt, args);
    buf[SYSERR_MSGSIZE-1] = 0; /* Make sure it is null terminated */
    log_msg_s(pri, fn, func, ln, en, buf);
    va_end(args);
}

void freeESMmsg(gpointer msg){
    GByteArray *array = (GByteArray *)msg;
    g_byte_array_free(array, TRUE);
}

EMMCtx emmCtx_init(){
    EMMCtx_t *self = g_new0(EMMCtx_t, 1);

    self->subs = subs_init();

    self->authQuadrs = g_ptr_array_new_full (5, g_free);
    self->authQuints = g_ptr_array_new_full (5, g_free);

    self->pendingESMmsg = g_ptr_array_new_full (5, freeESMmsg);
    self->attachStarted = FALSE;

    self->ksi = 7;
    self->msg_ksi = 1;

    /* self->t3412 = 0x06; /\* 12 seg*\/ */
    /* self->t3412 = 0x0A; /\* 20 seg*\/ */
    /* self->t3412 = 0x21; /\* 1 min*\/ */
    /* self->t3412 = 0x23; /\* 3 min*\/ */
    self->t3412 = 0x49; /* 54 min, default */

    /* Initialize UE Network Capabilities */
    bzero(self->ueCapabilities, 15);
    /* Initialize MS Network Capabilities */
    bzero(self->msNetCap, 10);

    return self;
}

void emmCtx_free(EMMCtx s){
    EMMCtx_t *self = (EMMCtx_t*)s;
    g_ptr_array_free (self->authQuadrs, TRUE);
    g_ptr_array_free (self->authQuints, TRUE);
    g_ptr_array_free (self->pendingESMmsg, TRUE);

    subs_free(self->subs);
    g_free(self);
}

void emm_setState(EMMCtx emm_h, EMM_State *s, EMMState stateName){
    EMMCtx_t *self = (EMMCtx_t*)emm_h;
    EMMState old = self->stateName;
    self->state = s;
    self->stateName = stateName;
    emm_log(self, LOG_INFO, 0, "state change from %s", EMMStateName[old]);
}

const guint64 emmCtx_getIMSI(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return self->imsi;
}

const guint64 emmCtx_getMSISDN(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return self->msisdn;
}

void emmCtx_setNewAuthQuadruplet(EMMCtx emm, AuthQuadruplet *a){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    g_ptr_array_add(self->authQuadrs, a);
    self->authQuadrsLen++;
}

void emmCtx_freeAuthQuadruplets(EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    g_ptr_array_remove_range (self->authQuadrs,
                          0,
                          self->authQuadrsLen);
    self->authQuadrsLen=0;
}

const AuthQuadruplet *emmCtx_getFirstAuthQuadruplet(EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return g_ptr_array_index(self->authQuadrs, 0);
}

void emmCtx_removeFirstAuthQuadruplet(EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    AuthQuadruplet * a = g_ptr_array_remove_index (self->authQuadrs, 0);
    self->authQuadrsLen--;
}

const guint8 *emmCtx_getServingNetwork_TBCD(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return ecmSession_getServingNetwork_TBCD(self->ecm);
}

Subscription emmCtx_getSubscription(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return self->subs;
}

void emmCtx_setMSISDN(EMMCtx emm, guint64 msisdn){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    self->msisdn = msisdn;
}

void emmCtx_newGUTI(EMMCtx emm, guti_t *guti){
    EMMCtx_t *self = (EMMCtx_t*)emm;

    if(self->guti.mtmsi==0){
        ecmSession_newGUTI(self->ecm, &(self->guti));
    }

    if(guti!=NULL)
        memcpy(guti, &(self->guti), sizeof(guti_t));
}

const guti_t * emmCtx_getGUTI(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return  &(self->guti);
}

void emmCtx_getTAI(const EMMCtx emm, guint8 (*sn)[3], guint16 *tac){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    *tac = self->tac;
    memcpy(sn, self->sn, 3);
}

guint32 *emmCtx_getM_TMSI_p(const EMMCtx emm){
    EMMCtx_t *self = (EMMCtx_t*)emm;
    return &(self->guti.mtmsi);
}

void emmCtx_getSGW(const EMMCtx emm, struct sockaddr *rAddr, socklen_t *rAddrLen){
    EMMCtx_t *self = (EMMCtx_t*)emm;

    memcpy(rAddr, &self->sgwIP, self->sgwIPLen);
    *rAddrLen = self->sgwIPLen;
}

void emmCtx_getPGW(const EMMCtx emm, struct sockaddr *rAddr, socklen_t *rAddrLen){
    EMMCtx_t *self = (EMMCtx_t*)emm;

    memcpy(rAddr, &self->pgwIP, self->pgwIPLen);
    *rAddrLen = self->pgwIPLen;
}
