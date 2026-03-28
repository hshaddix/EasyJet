/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "DataEnrichmentAlg.h"

#include <algorithm>
#include <AthContainers/ConstAccessor.h>

StatusCode DataEnrichmentAlg::initialize() {
    ATH_CHECK(m_jetKey.initialize());
    ATH_CHECK(m_photonKey.initialize());
    return StatusCode::SUCCESS;
}

StatusCode DataEnrichmentAlg::execute(const EventContext& ctx) const {
    static const SG::ConstAccessor<char> s_isTight(
        "DFCommonPhotonsIsEMTight");
    static const SG::ConstAccessor<char> s_isCleaned(
        "DFCommonPhotonsCleaning");
    auto jets    = SG::makeHandle(m_jetKey, ctx);
    auto photons = SG::makeHandle(m_photonKey, ctx);
    ATH_CHECK(jets.isValid());
    ATH_CHECK(photons.isValid());
    bool pass =
        std::any_of(jets->begin(), jets->end(),
            [this](const xAOD::Jet* j){ return j->pt() > m_minJetPt; })
        &&
        std::any_of(photons->begin(), photons->end(),
            [this](const xAOD::Photon* ph){
                return ph->pt() > m_minPhotonPt
                    && (!m_requirePhotonTight    || s_isTight(*ph))
                    && (!m_requirePhotonCleaning || s_isCleaned(*ph));
            });
    if (pass && m_maxWritten >= 0) {
        // fetch_add returns the previous value; if it already reached
        // the limit, reject this event (and undo the increment).
        if (m_nWritten.fetch_add(1) >= m_maxWritten) {
            m_nWritten.fetch_sub(1);
            pass = false;
        }
    }
    setFilterPassed(pass, ctx);
    return StatusCode::SUCCESS;
}
