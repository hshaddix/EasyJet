/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ttbarRewVarsAlg.h"

#include "AthContainers/AuxElement.h"
#include <Math/Vector4D.h>

namespace HHHBBBBTT
{

    ttbarRewVarsAlg::ttbarRewVarsAlg(const std::string &name,
                                    ISvcLocator *pSvcLocator)
    : AthHistogramAlgorithm(name, pSvcLocator)
    {
    }

    StatusCode ttbarRewVarsAlg::initialize()
    {

        ATH_CHECK(m_EventHandleKey.initialize());

        ATH_CHECK(m_Pt_top_decor.initialize());
        ATH_CHECK(m_Pt_antitop_decor.initialize());
        ATH_CHECK(m_Pt_ttbar_decor.initialize());
        ATH_CHECK(m_M_ttbar_decor.initialize());

        return StatusCode::SUCCESS;
    }

    StatusCode ttbarRewVarsAlg::execute(const EventContext& ctx)
    {

        SG::ReadHandle<xAOD::EventInfo> evtInfo(m_EventHandleKey, ctx);        ATH_CHECK(evtInfo.isValid());

        const xAOD::TruthParticleContainer* truthTopContainer = nullptr;
        if (!evtStore()->retrieve(truthTopContainer, "TruthTop")) {
            ATH_MSG_ERROR("Could not retrieve TruthTop");
            return StatusCode::FAILURE;
        }

        unsigned int mcChannelNumber = evtInfo->mcChannelNumber();

        float Pt_top = -999.;
        float Pt_antitop = -999.;
        float Pt_ttbar = -999.;
        float M_ttbar = -999.;

        ROOT::Math::PtEtaPhiMVector top, antitop;

        for (const xAOD::TruthParticle *tp : (*truthTopContainer)) {

            if(tp->pdgId() == 6 &&
              ((tp->status() == 62 && tt_pythia8_samples.contains(mcChannelNumber)) ||
               (tp->status() == 11 && tt_herwig7_samples.contains(mcChannelNumber)))){

                Pt_top = tp->pt();
                top = ROOT::Math::PtEtaPhiMVector(tp->pt(), tp->eta(), tp->phi(), tp->m());
                continue;
            }

            if(tp->pdgId() == -6 &&
              ((tp->status() == 62 && tt_pythia8_samples.contains(mcChannelNumber)) ||
               (tp->status() == 11 && tt_herwig7_samples.contains(mcChannelNumber)))){

                Pt_antitop = tp->pt();
                antitop = ROOT::Math::PtEtaPhiMVector(tp->pt(), tp->eta(), tp->phi(), tp->m());
                continue;
            }
        }

        if (Pt_top > 0 && Pt_antitop > 0) {
            Pt_ttbar = (top+antitop).Pt();
            M_ttbar = (top+antitop).M();
        }

        SG::WriteDecorHandle<xAOD::EventInfo,float> d_pt_top(m_Pt_top_decor, ctx);        SG::WriteDecorHandle<xAOD::EventInfo,float> d_pt_antitop(m_Pt_antitop_decor, ctx);        SG::WriteDecorHandle<xAOD::EventInfo,float> d_pt_ttbar(m_Pt_ttbar_decor, ctx);        SG::WriteDecorHandle<xAOD::EventInfo,float> d_m_ttbar(m_M_ttbar_decor, ctx);
        d_pt_top(*evtInfo) = Pt_top;
        d_pt_antitop(*evtInfo) = Pt_antitop;
        d_pt_ttbar(*evtInfo) = Pt_ttbar;
        d_m_ttbar(*evtInfo) = M_ttbar;

        return StatusCode::SUCCESS;
    }
}