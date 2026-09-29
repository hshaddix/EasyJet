/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
   */

#include "BaselineVarsPhSVJAlg.h"
#include "AthenaBaseComps/AthMsgStreamMacros.h"

namespace PhSVJ
{
    BaselineVarsPhSVJAlg::BaselineVarsPhSVJAlg(const std::string &name,
            ISvcLocator *pSvcLocator)
        : AthHistogramAlgorithm(name, pSvcLocator)
    { }

    StatusCode BaselineVarsPhSVJAlg::initialize()
    {
        ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
        if (m_doLargeRJets) {
            ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
        }
        ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
        if (m_doElectrons) {
            ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
        }
        if (m_doPhotons) {
            ATH_CHECK (m_photonHandle.initialize(m_systematicsList));
        }
        ATH_CHECK (m_metHandle.initialize(m_systematicsList));
        ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

        if(m_isMC){
            if (m_doElectrons) {
                m_ele_SF = CP::SysReadDecorHandle<float>("effSF_"+m_eleWPName+"_%SYS%", this);
                ATH_CHECK (m_ele_SF.initialize(m_systematicsList, m_electronHandle));
            }
            if (m_doMuons) {
                m_mu_SF = CP::SysReadDecorHandle<float>("effSF_"+m_muWPName+"_%SYS%", this);
                ATH_CHECK (m_mu_SF.initialize(m_systematicsList, m_muonHandle));
            }
        }


        if (m_doLargeRJets) {
            for(const auto& wp: m_GN2X_wps)
                m_GN2X_wp_Handles.emplace_back("xbb_select_GN2Xv01_" + wp, this);
            for(auto& handle : m_GN2X_wp_Handles)
                ATH_CHECK(handle.initialize(m_systematicsList,  m_lrjetHandle));
        }



        // Intialise syst-aware output decorators
        for (const std::string &var : m_floatVariables) {
            CP::SysWriteDecorHandle<float> whandle{var+"_%SYS%", this};
            m_Fbranches.emplace(var, whandle);
            ATH_CHECK (m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
        }

        for (const std::string &var : m_intVariables){
            ATH_MSG_DEBUG("initializing integer variable: " << var);
            CP::SysWriteDecorHandle<int> whandle{var+"_%SYS%", this};
            m_Ibranches.emplace(var, whandle);
            ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
        };

        if (m_doLargeRJets) {
            ATH_CHECK (m_GN2Xv01_phbb.initialize(m_systematicsList, m_lrjetHandle));
            ATH_CHECK (m_GN2Xv01_phcc.initialize(m_systematicsList, m_lrjetHandle));
            ATH_CHECK (m_GN2Xv01_pqcd.initialize(m_systematicsList, m_lrjetHandle));
            ATH_CHECK (m_GN2Xv01_ptop.initialize(m_systematicsList, m_lrjetHandle));
        }

        // Intialise syst list (must come after all syst-aware inputs and outputs)
        ATH_CHECK (m_systematicsList.initialize());

        return StatusCode::SUCCESS;
    }

    StatusCode BaselineVarsPhSVJAlg::execute(const EventContext& ctx)
    {
        //Loop over all systs
        for (const auto& sys : m_systematicsList.systematicsVector())
        {
            const xAOD::EventInfo *event = nullptr;
            ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

            const xAOD::JetContainer *jets = nullptr;
            ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

            const xAOD::JetContainer *lrjets = nullptr;
            if (m_doLargeRJets) {
                ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));
            }

            const xAOD::MuonContainer *muons = nullptr;
            if (m_doMuons) {
                ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));
            }

            const xAOD::ElectronContainer *electrons = nullptr;
            if (m_doElectrons) {
              ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));
            }
            const xAOD::PhotonContainer* photons = nullptr;
            if(m_doPhotons) {
                ANA_CHECK(m_photonHandle.retrieve(photons, sys, ctx));
                if (!photons) { // MODIFICATO
                    ATH_MSG_ERROR("Photon container is nullptr!"); // MODIFICATO
                } else { // MODIFICATO
                    ATH_MSG_INFO("Retrieved photon container with " // MODIFICATO
                            << photons->size() << " photons"); // MODIFICATO
                } // MODIFICATO

                static const SG::AuxElement::ConstAccessor<float> acc_topo20("topoetcone20"); // MODIFICATO

                int iph = 0; // MODIFICATO
                for (const xAOD::Photon* photon : *photons) { // MODIFICATO

                    ATH_MSG_INFO("---- Photon #" << iph << " ----"); // MODIFICATO

                    if (!photon) { // MODIFICATO
                        ATH_MSG_ERROR("Photon pointer is nullptr!"); // MODIFICATO
                        ++iph; // MODIFICATO
                        continue; // MODIFICATO
                    } // MODIFICATO

                    ATH_MSG_INFO("pt = " << photon->pt()
                            << " eta = " << photon->eta()
                            << " phi = " << photon->phi()); // MODIFICATO
                    ATH_MSG_INFO("Checking AuxAccessor availability..."); // MODIFICATO

                    if (acc_topo20.isAvailable(*photon)) { // MODIFICATO
                        ATH_MSG_INFO("Aux topoetcone20 = "
                                << acc_topo20(*photon)); // MODIFICATO
                    } else { // MODIFICATO
                        ATH_MSG_INFO("Aux topoetcone20 NOT available"); // MODIFICATO
                    } // MODIFICATO

                    ATH_MSG_INFO("About to call isolationValue()"); // MODIFICATO

                    float topo20 = -999.f; // MODIFICATO

                    bool ok = photon->isolationValue(topo20,
                            xAOD::Iso::topoetcone20); // MODIFICATO

                    ATH_MSG_INFO("Returned from isolationValue()"); // MODIFICATO

                    ATH_MSG_INFO("Return value = " << ok
                            << " topo20 = " << topo20); // MODIFICATO

                    ++iph; // MODIFICATO
                } // MODIFICATO

            }



            const xAOD::MissingETContainer *metCont = nullptr;
            ANA_CHECK (m_metHandle.retrieve (metCont, sys, ctx));
            const xAOD::MissingET* met = (*metCont)["Final"];
            if (!met) {
                ATH_MSG_ERROR("Could not retrieve MET");
                return StatusCode::FAILURE;
            }
            for (const std::string &string_var: m_floatVariables) {
                m_Fbranches.at(string_var).set(*event, -99., sys);
            }

            for (const auto& var: m_intVariables) {
                m_Ibranches.at(var).set(*event, -99, sys);
            }

        }
        return StatusCode::SUCCESS;
    }
}
