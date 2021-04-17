//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "encode.hpp"
#include "task.hpp"
#include "utils.hpp"

#include <gnb/gtp/task.hpp>
#include <gnb/rrc/task.hpp>

#include <asn/ngap/ASN_NGAP_FiveG-S-TMSI.h>
#include <asn/ngap/ASN_NGAP_Paging.h>

namespace nr::gnb
{

void NgapTask::handleRadioLinkFailure(int ueId)
{
    // Notify GTP task
    auto *w2 = new NwGnbNgapToGtp(NwGnbNgapToGtp::UE_CONTEXT_RELEASE);
    w2->ueId = ueId;
    m_base->gtpTask->push(w2);

    // Notify AMF
    sendContextRelease(ueId, NgapCause::RadioNetwork_radio_connection_with_ue_lost);
}

void NgapTask::receivePaging(int amfId, ASN_NGAP_Paging *msg)
{
    m_logger->debug("Paging received");

    auto *amf = findAmfContext(amfId);
    if (amf == nullptr)
        return;

    auto *ieUePagingIdentity = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_UEPagingIdentity);
    auto *ieTaiListForPaging = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_TAIListForPaging);

    if (ieUePagingIdentity == nullptr || ieTaiListForPaging == nullptr ||
        ieUePagingIdentity->UEPagingIdentity.present != ASN_NGAP_UEPagingIdentity_PR_fiveG_S_TMSI)
    {
        m_logger->err("Invalid parameters received in Paging message");
        return;
    }

    auto *w = new NwGnbNgapToRrc(NwGnbNgapToRrc::PAGING);
    w->uePagingTmsi =
        ngap_encode::EncodeS(asn_DEF_ASN_NGAP_FiveG_S_TMSI, ieUePagingIdentity->UEPagingIdentity.choice.fiveG_S_TMSI);
    w->taiListForPaging = asn::UniqueCopy(ieTaiListForPaging->TAIListForPaging, asn_DEF_ASN_NGAP_TAIListForPaging);
    if (w->uePagingTmsi.length() == 0)
    {
        m_logger->err("FiveG-S-TMSI encoding failed");
        delete w;
        return;
    }

    m_base->rrcTask->push(w);
}

} // namespace nr::gnb
