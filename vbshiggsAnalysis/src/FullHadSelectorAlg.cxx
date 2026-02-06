/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "FullHadSelectorAlg.h"
#include <AthenaKernel/Units.h>
#include <TVector3.h>
#include <TH1F.h>
#include <TEfficiency.h>

namespace VBSHIGGS {

FullHadSelectorAlg::FullHadSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator)
: EL::AnaAlgorithm(name, pSvcLocator) {}

StatusCode FullHadSelectorAlg::initialize() {
    ATH_MSG_INFO("*********************************");
    ATH_MSG_INFO("     FullHadSelectorAlg         ");
    ATH_MSG_INFO("*********************************");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_smallJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_largeJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_metHandle.initialize(m_systematicsList));

    
    ATH_CHECK(initialiseCutflow());

    return StatusCode::SUCCESS;
}

StatusCode FullHadSelectorAlg::execute() {
    for (const auto& sys : m_systematicsList.systematicsVector()) {

        const xAOD::EventInfo* event = nullptr;
        ANA_CHECK(m_eventHandle.retrieve(event, sys));

        const xAOD::JetContainer* smallJets = nullptr;
        const xAOD::JetContainer* largeJets = nullptr;
	const xAOD::ElectronContainer* electrons = nullptr;
	const xAOD::MuonContainer* muons = nullptr;
	const xAOD::MissingETContainer* metCont = nullptr;

	ANA_CHECK(m_smallJetHandle.retrieve(smallJets, sys));
        ANA_CHECK(m_largeJetHandle.retrieve(largeJets, sys));
	ANA_CHECK(m_electronHandle.retrieve(electrons, sys));
	ANA_CHECK(m_muonHandle.retrieve(muons, sys));
	ANA_CHECK(m_metHandle.retrieve(metCont, sys));


        for (auto& [key, _] : m_bools) m_bools[key] = false;


        if (smallJets->size() == 2) m_bools[PASS_EXACTLY_TWO_SMALL_JETS] = true;
        if (largeJets->size() >= 2) m_bools[PASS_TWO_LARGE_JETS] = true;

        bool passAll = true;
        for (auto& [key, val] : m_bools) passAll &= val;
        m_bools[PASS_ALL_CUTS] = passAll;


        for (const auto& [key, name] : m_boolnames) {
            if (m_bools[key]) {
                m_vbshiggsCuts[name].passed = true;
                m_vbshiggsCuts[name].counter++;
            }
        }

        m_totalEvents++;
    }

    return StatusCode::SUCCESS;
}

StatusCode FullHadSelectorAlg::finalize() {
    ATH_MSG_INFO("Total events processed = " << m_totalEvents);
    m_vbshiggsCuts.CheckCutResults();
    return StatusCode::SUCCESS;
}

StatusCode FullHadSelectorAlg::initialiseCutflow() {
    std::vector<std::string> boolnamesList;
    for (const auto& [key, name] : m_boolnames) boolnamesList.push_back(name);

    m_vbshiggsCuts.CheckInputCutList(boolnamesList, boolnamesList);

    for (const auto& [key, name] : m_boolnames)
        m_vbshiggsCuts.add(name);


    unsigned int nbins = m_vbshiggsCuts.size() + 1;
    ANA_CHECK(book(TEfficiency("AbsoluteEfficiency", "Absolute Efficiency;Cuts;#epsilon",
                                nbins, 0.5, nbins+0.5)));
    ANA_CHECK(book(TEfficiency("RelativeEfficiency", "Relative Efficiency;Cuts;#epsilon",
                                nbins, 0.5, nbins+0.5)));
    ANA_CHECK(book(TEfficiency("StandardCutFlow", "Standard CutFlow;Cuts;#events",
                                nbins, 0.5, nbins+0.5)));
    ANA_CHECK(book(TH1F("EventsPassed_BinLabeling", "Events passed by cut/bin",
                        nbins, 0.5, nbins+0.5)));

    return StatusCode::SUCCESS;
}

} // namespace VBSHIGGS
