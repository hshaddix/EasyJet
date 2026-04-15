/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#include "FullHadSelectorAlg.h"
#include <AthenaKernel/Units.h>
#include <TVector3.h>
#include <TH1F.h>
#include <TEfficiency.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>


namespace VBSHIGGS {

FullHadSelectorAlg::FullHadSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator) : EL::AnaAlgorithm(name, pSvcLocator){}
StatusCode FullHadSelectorAlg::initialize() {
    ATH_MSG_INFO("*********************************");
    ATH_MSG_INFO("     FullHadSelectorAlg         ");
    ATH_MSG_INFO("*********************************");

    //  ATH_CHECK(m_systematicsList.initialize());
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    if (m_doResolved){
      ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
      ATH_CHECK (m_HCandHandle.initialize(m_systematicsList));
    }


    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));

    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

    
    ATH_CHECK(m_passallcuts.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK(m_metHandle.initialize(m_systematicsList));


    ATH_CHECK(m_smallJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_largeJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
    // ATH_CHECK (m_vbsjetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_metHandle.initialize(m_systematicsList));
    if (!m_isBtag.empty() && m_doResolved) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_signaljetHandle));
    }

    if (!m_UseVBFRNN) {
      ATH_CHECK(m_vbsjetHandle.initialize(m_systematicsList));
  }

    ATH_CHECK (m_systematicsList.initialize());
    if (m_saveCutFlow) ATH_CHECK(initialiseCutflow());


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
	const xAOD::JetContainer *signalJets = nullptr;
	const xAOD::JetContainer *HJets = nullptr;

	if (m_doResolved){
	  ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys));
	  ANA_CHECK (m_HCandHandle.retrieve (HJets, sys));
	}
	const xAOD::JetContainer *vbsjets = nullptr;
	if( !m_UseVBFRNN )
	  ANA_CHECK (m_vbsjetHandle.retrieve (vbsjets, sys));


	ANA_CHECK(m_smallJetHandle.retrieve(smallJets, sys));
  ANA_CHECK(m_largeJetHandle.retrieve(largeJets, sys));
	ANA_CHECK(m_electronHandle.retrieve(electrons, sys));
	ANA_CHECK(m_muonHandle.retrieve(muons, sys));
	ANA_CHECK(m_metHandle.retrieve(metCont, sys));
	bool WPgiven = !m_isBtag.empty();
	std::vector<const xAOD::Jet*> bjets;

	if (m_doResolved){
	  for(const xAOD::Jet* jet : *signalJets) {
	    if (WPgiven) {
	      if (m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5) bjets.push_back(jet);
	    }
	  }
	}



	const xAOD::MissingET* met = (*metCont)["Final"];
	if (!met) {
	  ATH_MSG_ERROR("Could not retrieve MET");
	  return StatusCode::FAILURE;
	}

  for (auto& [key, _] : m_bools) m_bools[key] = false;

	m_bools[PASS_LEPTON_VETO] = (muons->empty() && electrons->empty());
	if (smallJets->size() == 2) m_bools[PASS_EXACTLY_TWO_SMALL_JETS] = true;
        if (largeJets->size() >= 2) m_bools[PASS_TWO_LARGE_JETS] = true;

        bool passAll = true;
        for (auto& [key, val] : m_bools) passAll &= val;
        m_bools[PASS_ALL_CUTS] = passAll;
  m_passallcuts.set(*event, passAll, sys);


	// for (const auto& [key, name] : m_boolnames) {
  //           if (m_bools[key]) {
  //               m_vbshiggsCuts[name].passed = true;
  //               m_vbshiggsCuts[name].counter++;
  //           }
  //       }

    // Count how many cuts the event passed and increase the relative counter
  for (const auto &cut : m_inputCutKeys) {
    if(m_vbshiggsCuts.exists(m_boolnames.at(cut))) {
      m_vbshiggsCuts(m_boolnames.at(cut)).passed = m_bools.at(cut);
      if (m_vbshiggsCuts(m_boolnames.at(cut)).passed){
        m_vbshiggsCuts(m_boolnames.at(cut)).counter+=1;
      }
    }
  }
  for (const auto& [key, value] : m_bools) {
    m_Bbranches.at(key).set(*event, value, sys);
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

    // m_vbshiggsCuts.CheckInputCutList(boolnamesList, boolnamesList);
    m_vbshiggsCuts.CheckInputCutList(m_inputCutList, boolnamesList);


    m_inputCutKeys.resize(m_inputCutList.size());
    std::vector<bool> inputWasFound(m_inputCutList.size(), false);

    for (const auto& [key, name] : m_boolnames) {
    auto it = std::find(m_inputCutList.begin(), m_inputCutList.end(), name);
    if (it != m_inputCutList.end()) {
      auto index = it - m_inputCutList.begin();
      m_inputCutKeys.at(index) = key;
      inputWasFound.at(index) = true;
    }
  }


    for (unsigned int index = 0; index < inputWasFound.size(); index++) {
      if (inputWasFound.at(index)) continue;
      ATH_MSG_ERROR("CutLists don't match. Please double check your configuration " + m_inputCutList[index]);
    }

    for (const auto& cut : m_inputCutKeys) {
      m_vbshiggsCuts.add(m_boolnames.at(cut));
    }
    // for (const auto& [key, name] : m_boolnames)
    //     m_vbshiggsCuts.add(name);


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
