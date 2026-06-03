/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "oneLepSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace VBSSEMILEP{

  oneLepSelectorAlg::oneLepSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator) : EL::AnaAlgorithm(name, pSvcLocator){  }
  StatusCode oneLepSelectorAlg::initialize(){
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     oneLepSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");

     // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_passTriggerSLT.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_vbsjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_largejetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_GN2Xv01_phbb.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN2Xv01_phcc.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN2Xv01_pqcd.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN2Xv01_ptop.initialize(m_systematicsList, m_largejetHandle));

    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_signaljetHandle));
    }

    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<bool> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

     // special flag for all cuts
    ATH_CHECK (m_passallcuts.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    if(m_saveCutFlow) ATH_CHECK (initialiseCutflow());
    return StatusCode::SUCCESS;
  }
  StatusCode oneLepSelectorAlg::execute(){
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));
      
      const xAOD::JetContainer *signaljets = nullptr;
      ANA_CHECK (m_signaljetHandle.retrieve (signaljets, sys));

      const xAOD::JetContainer *vbsjets = nullptr;
      ANA_CHECK (m_vbsjetHandle.retrieve (vbsjets, sys));

      const xAOD::JetContainer *largeJets = nullptr;
      ANA_CHECK (m_largejetHandle.retrieve (largeJets, sys));

      bool WPgiven = !m_isBtag.empty();
      std::vector<const xAOD::Jet*> bjets;
      for(const xAOD::Jet* jet : *signaljets) {
        if (WPgiven) {
          if (m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5) bjets.push_back(jet);
        }
      }

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys));

      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK (m_metHandle.retrieve (metCont, sys));
      const xAOD::MissingET* met = (*metCont)["Final"];
      if (!met) {
        ATH_MSG_ERROR("Could not retrieve MET");
        return StatusCode::FAILURE;
      }

      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;

      if ( signaljets->size() >= 2) m_bools.at(VBSSEMILEP::PASS_TWO_SIGNAL_JETS) = true;
      if ( largeJets->size() >= 1 )  m_bools.at(VBSSEMILEP::PASS_ONE_LARGE_JET) = true;
      
      if (!m_passTriggerSLT.empty() and static_cast<bool>(m_passTriggerSLT.get(*event, sys)) ) {
        m_bools.at(VBSSEMILEP::PASS_TRIGGER) = true;
      }

      leptonSelection(*electrons, *muons, met);
      vbsjetsSelection(vbsjets);
      if (m_bools.at(VBSSEMILEP::PASS_TWO_SIGNAL_JETS)) resolvedSelection(signaljets);
      if (m_bools.at(VBSSEMILEP::PASS_ONE_LARGE_JET)) boostedSelection(largeJets, sys);

      bool pass_preselection = m_bools.at(VBSSEMILEP::PASS_RES_BASELINE) || m_bools.at(VBSSEMILEP::PASS_MERG_BASELINE);

      m_passallcuts.set(*event, pass_preselection, sys);

      // do the CUTFLOW only with sys="" -> NOSYS
      if (sys.name()==""){

        // Compute total_events
        m_total_events+=1;

        // Count how many cuts the event passed and increase the relative counter
        for (const auto &cut : m_inputCutKeys) {
          if(m_vbssemilepCuts.exists(m_boolnames.at(cut))) {
            m_vbssemilepCuts(m_boolnames.at(cut)).passed = m_bools.at(cut);
            if (m_vbssemilepCuts(m_boolnames.at(cut)).passed){
              m_vbssemilepCuts(m_boolnames.at(cut)).counter+=1;
            }
          }
        }

        // Check how many consecutive cuts are passed by the event.
        unsigned int consecutive_cuts = 0;
        for (size_t i = 0; i < m_vbssemilepCuts.size(); ++i) {
          if (m_vbssemilepCuts[i].passed)
            consecutive_cuts++;
          else
            break;
        }

	// Here we basically increment the  N_events(pass_i  AND pass_i-1  AND ... AND pass_0) for the i-cut.
        for (unsigned int i=0; i<consecutive_cuts; i++) {
          m_vbssemilepCuts[i].relativeCounter+=1;
        }
      }

      // Fill syst-aware output decorators
      for (auto& [key, var] : m_bools) {
        m_Bbranches.at(key).set(*event, var, sys);
      }

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      if (!m_bypass && !pass_preselection) continue;
      filter.setPassed(true);

    }
    return StatusCode::SUCCESS;
  }
  StatusCode oneLepSelectorAlg::finalize(){
    
    //adapt the following for each syst TODO
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    ANA_CHECK (m_filterParams.finalize ());
    m_vbssemilepCuts.CheckCutResults(); // Print CheckCutResults

    if(m_saveCutFlow) {
      m_vbssemilepCuts.DoAbsoluteEfficiency(m_total_events, efficiency("AbsoluteEfficiency"));
      m_vbssemilepCuts.DoRelativeEfficiency(m_total_events, efficiency("RelativeEfficiency"));
      m_vbssemilepCuts.DoStandardCutFlow(m_total_events, efficiency("StandardCutFlow"));
      m_vbssemilepCuts.DoCutflowLabeling(m_total_events, hist("EventsPassed_BinLabeling"));

    }

    return StatusCode::SUCCESS;
  }

  void oneLepSelectorAlg :: leptonSelection (const xAOD::ElectronContainer& electrons, const xAOD::MuonContainer& muons, const xAOD::MissingET *met){
    // Exactly one lepton selection
    if (electrons.size() + muons.size() == 1) m_bools.at(VBSSEMILEP::PASS_ONE_LEPTON) = true;

    // met cut
    if (met->met() > 30 * Athena::Units::GeV) m_bools.at(VBSSEMILEP::PASS_MET) = true;
  }//Lepton Selection

  void oneLepSelectorAlg :: vbsjetsSelection(const xAOD::JetContainer * vbsjets){
    if (vbsjets->size() >= 2){
      //TODO, create an object holding the vbs jets passing the mjj + deta selections
      const xAOD::Jet* vbsJet1 = vbsjets->at(0);
      const xAOD::Jet* vbsJet2 = vbsjets->at(1);;
      double mjj = (vbsJet1->p4() + vbsJet2->p4()).M();
      double dEta_jj = std::abs(vbsJet1->eta() - vbsJet2->eta());

      if ( mjj > 300 * Athena::Units::GeV  && dEta_jj > 3.0 ) {
        m_bools.at(VBSSEMILEP::PASS_VBS_BASELINE) = true;
      }
    }
  }//vbsjetsSelection

  //Resolved Analysis
  void oneLepSelectorAlg :: resolvedSelection(const xAOD::JetContainer *signaljets){

    const xAOD::Jet * mSJet1 = signaljets->at(0);
    const xAOD::Jet * mSJet2 = signaljets->at(1);

    //Delta R(s, s) cut 
    float min_dR_j1j2 = 2.;
    float dR_j1j2 = mSJet1->p4().DeltaR(mSJet2->p4());
    if (dR_j1j2 < min_dR_j1j2)
      m_bools.at(VBSSEMILEP::PASS_DELTA_R_SS) = true;
    
    float low_mss = 76.;
    float high_mss = 106.;
    float mss = (mSJet1->p4() + mSJet2->p4()).M();
    if (mss > low_mss * Athena::Units::GeV && mss < high_mss * Athena::Units::GeV ){
      m_bools.at(VBSSEMILEP::PASS_RES_Z_WINDOW) = true;
    }

    bool pass_resolved_baseline = m_bools.at(VBSSEMILEP::PASS_TRIGGER) && m_bools.at(VBSSEMILEP::PASS_ONE_LEPTON);

    m_bools.at(VBSSEMILEP::PASS_RES_BASELINE) = pass_resolved_baseline;

  }

  //boosted Analysis
  void oneLepSelectorAlg :: boostedSelection(const xAOD::JetContainer *largeJets, const CP::SystematicSet& sys){
    //leading Large-R jet in the event
    const xAOD::Jet* largeJet = largeJets->at(0);
    //construct GN2X score
    float phbb = m_GN2Xv01_phbb.get(*largeJet, sys);
    float phcc = m_GN2Xv01_phcc.get(*largeJet, sys);
    float pqcd = m_GN2Xv01_pqcd.get(*largeJet, sys);
    float ptop = m_GN2Xv01_ptop.get(*largeJet, sys);
    float fbb = 0.25;
    float ftop = 0.25;
    float fqcd = 0.50;
    float ZScore = log(phcc / (fbb*phbb + ftop*ptop + fqcd*pqcd));


    bool pass_merged_baseline = m_bools.at(VBSSEMILEP::PASS_TRIGGER) && m_bools.at(VBSSEMILEP::PASS_ONE_LEPTON) && ( (largeJet->p4()).M() > 76 * Athena::Units::GeV && (largeJet->p4()).M() < 106 * Athena::Units::GeV ) && ZScore > 1.0  ;

    m_bools.at(VBSSEMILEP::PASS_MERG_BASELINE) = pass_merged_baseline;

  }

  StatusCode oneLepSelectorAlg::initialiseCutflow(){
        std::vector<std::string> boolnameslist;
    for (const auto& [key, value] : m_boolnames) {
      boolnameslist.push_back(value);
    }
    m_vbssemilepCuts.CheckInputCutList(m_inputCutList, boolnameslist);

    m_inputCutKeys.resize(m_inputCutList.size());
    std::vector<bool> inputWasFound (m_inputCutList.size(), false);
    for (const auto& [key, value]: m_boolnames) {
      auto it = std::find(m_inputCutList.begin(), m_inputCutList.end(), value);
      if (it != m_inputCutList.end()) {
        auto index = it - m_inputCutList.begin();
        m_inputCutKeys.at(index) = key;
        inputWasFound.at(index) = true;
      }
    }
    for (unsigned int index = 0; index < inputWasFound.size(); index++) {
      if(inputWasFound.at(index)) continue;
      ATH_MSG_ERROR("CutLists don't match. Please double check your configuration " + m_inputCutList[index]);
    }

    for (const auto &cut : m_inputCutKeys) {
      m_vbssemilepCuts.add(m_boolnames[cut]);
    }

    //After filling the CutManager, book your histograms.
    const unsigned int nbins = m_vbssemilepCuts.size() + 1; //  need an extra bin for the total num of events.
    ANA_CHECK (book (TEfficiency("AbsoluteEfficiency","Absolute Efficiency of vbssemilep cuts;Cuts;#epsilon",
                                nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("RelativeEfficiency","Relative Efficiency of vbssemilep cuts;Cuts;#epsilon",
                                nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("StandardCutFlow","StandardCutFlow of vbssemilep cuts;Cuts;#epsilon",
                                nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TH1F("EventsPassed_BinLabeling", "Events passed by each cut / Bin labeling", nbins, 0.5, nbins + 0.5)));

    return StatusCode::SUCCESS;
  }

}//name space
