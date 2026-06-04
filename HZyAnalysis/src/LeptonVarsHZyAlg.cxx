/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "LeptonVarsHZyAlg.h"
#include <AthenaKernel/Units.h>

namespace HZY
{
    LeptonVarsHZyAlg::LeptonVarsHZyAlg(const std::string& name,
        ISvcLocator* pSvcLocator)
        : AthHistogramAlgorithm(name, pSvcLocator)
    {
    }

    StatusCode LeptonVarsHZyAlg::initialize()
    {
        ATH_MSG_INFO("*********************************\n");
        ATH_MSG_INFO("         LeptonVarsHZyAlg        \n");
        ATH_MSG_INFO("*********************************\n");

        ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
        ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
        ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));

        // Intialise syst-aware output decorators
        for (const std::string& var : m_intVariables)
        {
            CP::SysWriteDecorHandle<int> whandle{ var + "_%SYS%", this };
            m_Ibranches.emplace(var, whandle);
            ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
        }

        // Initialise syst list (must come after all syst-aware inputs and outputs)
        ATH_CHECK(m_systematicsList.initialize());

        return StatusCode::SUCCESS;
    }

    StatusCode LeptonVarsHZyAlg::execute()
    {
        // Loop over all systs
        for (const auto& sys : m_systematicsList.systematicsVector())
        {
            // In case of special Higgs sample, run only on NOSYS
            if (!m_doSystematics && sys.name() != "") continue;

            const xAOD::EventInfo* event = nullptr;
            ANA_CHECK(m_eventHandle.retrieve(event, sys));

            const xAOD::ElectronContainer* HZyElectrons = nullptr;
            ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys));
            
            const xAOD::MuonContainer* HZyMuons = nullptr;
            ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys));

            for (const auto& var : m_intVariables)
            {
                m_Ibranches.at(var).set(*event, -99, sys);
            }
            
            // Count electrons and muons
            int n_HZyEle = 0;
            int n_HZyMu = 0;
            n_HZyEle = HZyElectrons->size();
            n_HZyMu = HZyMuons->size();
            m_Ibranches.at("nElectrons").set(*event, n_HZyEle, sys);
            m_Ibranches.at("nMuons").set(*event, n_HZyMu, sys);
            m_Ibranches.at("nLeptons").set(*event, HZyElectrons->size() + HZyMuons->size(), sys);
        }

        return StatusCode::SUCCESS;
    }
} // namespace HZY
