/* This application was initially developed as a Final Project by
 *     Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
 * under the supervision of,
 *     Jukka Manner (jukka.manner@aalto.fi)
 *     Jose Costa-Requena (jose.costa@aalto.fi)
 * in AALTO University and partially funded by EIT ICT labs.
 */

/**
 * @file   EMM_Registered.c
 * @Author Vicent Ferrer
 * @date   August, 2015
 * @brief  EMM State
 *
 */

#include "EMM_Registered.h"
#include "logmgr.h"
#include "EMM_FSMConfig.h"
#include "NAS.h"
#include "NAS_EMM_priv.h"

static void processDetachReq(EMMCtx_t *emm, GenericNASMsg_t *msg);

static void emmProcessMsg(gpointer emm_h, GenericNASMsg_t* msg){
    log_msg(LOG_ERR, 0, "Not Implemented");
}


static void emm_processSecMsg(gpointer emm_h, gpointer buf, gsize len){
    EMMCtx_t *emm = (EMMCtx_t*)emm_h;

    GenericNASMsg_t msg;
    SecurityHeaderType_t s;
    ProtocolDiscriminator_t p;
    gboolean isAuth = FALSE, res;

    nas_getHeader(buf, len, &s, &p);

    if(emm->sci){
        res = nas_authenticateMsg(emm->parser, buf, len, NAS_UpLink, (uint8_t*)&isAuth);
        if(res==2){
            log_msg(LOG_WARNING, 0, "Wrong SQN Count");
            return;
        }else if(res==0){
            g_error("NAS Authentication Error");
        }
    }
    if(!isAuth){
        log_msg(LOG_INFO, 0, "Received Message with wrong MAC");
        return;
    }

    if(!dec_secNAS(emm->parser, &msg, NAS_UpLink, buf, len)){
        g_error("NAS Decyphering Error");
    }

    nas_incrementNASCount(emm->parser, NAS_UpLink);

    switch(msg.plain.eMM.messageType){
    case DetachRequest:
        log_msg(LOG_DEBUG, 0, "Received DetachRequest");
        processDetachReq(emm, &msg);
        break;
    default:
        log_msg(LOG_WARNING, 0,
                "NAS Message type (%u) not recognized in this context",
                msg.plain.eMM.messageType);
    }

}


void linkEMMRegistered(EMM_State* s){
    s->processMsg = emmProcessMsg;
    /* s->authInfoAvailable = emmAuthInfoAvailable; */
    s->attachAccept = NULL;
    s->processSecMsg = emm_processSecMsg;
    s->sendESM = emm_internalSendESM;
}

static void emm_detach(EMMCtx_t *emm){
    uint8_t *pointer, buffer[150];

    /* Forge Detach Accept*/
    pointer = buffer;
    newNASMsg_EMM(&pointer, EPSMobilityManagementMessages, PlainNAS);
    encaps_EMM(&pointer, DetachAccept);

    ecm_send(emm->ecm, buffer, pointer-buffer);
    emmChangeState(emm, EMM_Deregistered);
    ecm_sendUEContextReleaseCommand(emm->ecm, CauseNas, CauseNas_detach);
    /*TODO, move EMM context to MME structure*/
}

static void processDetachReq(EMMCtx_t *emm, GenericNASMsg_t *msg){
    uint64_t mobid=0ULL;
    guti_t  *guti;
    guint i;
    DetachRequestUEOrig_t *detachMsg = (DetachRequestUEOrig_t*)&(msg->plain.eMM);

    /*ePSAttachType*/
    /*detachMsg->detachType.v;*/

    /*nASKeySetId*/
    /*if(detachMsg->nASKeySetId.v != PDATA->user_ctx->ksi.id){
        log_msg(LOG_ERR, 0, "Incorrect KSI: %u Ignoring detach", detachMsg->nASKeySetId.v);
        return 1;
     }*/

    /*EPSMobileId*/
    if(((ePSMobileId_header_t*)detachMsg->ePSMobileId.v)->type == 1 ){  /* IMSI*/
        for(i=0; i<detachMsg->ePSMobileId.l-1; i++){
            mobid = mobid*10 + ((detachMsg->ePSMobileId.v[i])>>4);
            mobid = mobid*10 + ((detachMsg->ePSMobileId.v[i+1])&0x0F);
        }
        if(((ePSMobileId_header_t*)detachMsg->ePSMobileId.v)->parity == 1){
            mobid = mobid*10 + ((detachMsg->ePSMobileId.v[i])>>4);
        }
        if(emm->imsi != mobid){
            log_msg(LOG_ERR, 0, "received IMSI doesn't match.");
            return;
        }
    }else if(((ePSMobileId_header_t*)detachMsg->ePSMobileId.v)->type == 1 ){    /*GUTI*/
        guti = (guti_t  *)(detachMsg->ePSMobileId.v+1);
        if(memcmp(guti, &(emm->guti), 10)!=0){
            log_msg(LOG_ERR, 0, "GUTI incorrect. GUTI reallocation not implemented yet.");
            return;
        }
    }

    esm_detach(emm->esm, emm_detach, emm);
}
