/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "twoLepSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace VBSSEMILEP{

  twoLepSelectorAlg::twoLepSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator) : EL::AnaAlgorithm(name, pSvcLocator){  }

  StatusCode twoLepSelectorAlg::initialize(){
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     twoLepSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    // Initialise global event filter
    ATH_CHECK (m_filterParams.initialize(m_systematicsList));
    ATH_CHECK (m_passTriggerSLT.initialize(m_systematicsList, m_eventHandle));

    // Initialize the base small jets unconditionally
    ATH_CHECK (m_smalljetHandle.initialize(m_systematicsList));
    
    if (m_doResolved){
      ATH_CHECK (m_signaljetHandle.initialize(m_systematicsList));
    }

    ATH_CHECK (m_vbsjetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_largejetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_GN3Xv01_pqcdcx.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN3Xv01_pWqq.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN3Xv01_pqcdbx.initialize(m_systematicsList, m_largejetHandle));
    ATH_CHECK (m_GN3Xv01_ptop.initialize(m_systematicsList, m_largejetHandle));


    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_smalljetHandle));
    }

    ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));    
    ATH_CHECK (m_year.initialize(m_systematicsList, m_eventHandle));

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
  
  StatusCode twoLepSelectorAlg::execute(const EventContext& ctx){
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);

    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector()){
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));
  
      const xAOD::JetContainer *signalJets = nullptr;

      if (m_doResolved){
        ANA_CHECK (m_signaljetHandle.retrieve (signalJets, sys, ctx));
      }

      const xAOD::JetContainer *vbsjets = nullptr;
      if( !m_UseVBFRNN )
        ANA_CHECK (m_vbsjetHandle.retrieve (vbsjets, sys, ctx));

      const xAOD::JetContainer *largeJets = nullptr;
      ANA_CHECK (m_largejetHandle.retrieve (largeJets, sys, ctx));
      
      const xAOD::JetContainer *smallJets = nullptr;
      ANA_CHECK (m_smalljetHandle.retrieve (smallJets, sys, ctx));

      bool WPgiven = !m_isBtag.empty();
      std::vector<const xAOD::Jet*> bjets;

     /*   for(const xAOD::Jet* jet : *smallJets) {
          if (WPgiven) {
            if (m_isBtag.get(*jet, sys) && std::abs(jet->eta())<2.5) bjets.push_back(jet);
          }
        }
        */
        for(const xAOD::Jet* jet : *signalJets) {
          if (WPgiven) {
            if (m_isBtag.get(*jet, sys)) bjets.push_back(jet);
          }
        }

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK (m_muonHandle.retrieve (muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK (m_electronHandle.retrieve (electrons, sys, ctx));

      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;

      if (m_doResolved){
        if ( signalJets->size() >= 2) m_bools.at(VBSSEMILEP::PASS_TWO_SIGNAL_JETS) = true;  
      }
      if ( largeJets->size() >= 1 )  m_bools.at(VBSSEMILEP::PASS_ONE_LARGE_JET) = true;

      if (!m_passTriggerSLT.empty() and static_cast<bool>(m_passTriggerSLT.get(*event, sys)) ) {
        m_bools.at(VBSSEMILEP::PASS_TRIGGER) = true;
      }

      leptonSelection(electrons, muons);
      
      m_bools.at(VBSSEMILEP::PASS_B_VETO) = (bjets.size() == 0);
      m_bools.at(VBSSEMILEP::PASS_B_TAGGED) = (bjets.size() >= 1);

      bool pass_preselection = m_bools.at(VBSSEMILEP::PASS_TRIGGER) && m_bools.at(VBSSEMILEP::PASS_TWO_LEPTONS);
      //if (!m_doResolved){
      //pass_preselection &= m_bools.at(VBSSEMILEP::PASS_ONE_LARGE_JET);
      //}     
      
      m_passallcuts.set(*event, pass_preselection, sys);
      //m_passallcuts.set(*event, true, sys);

      // do the CUTFLOW only with sys="" -> NOSYS
      if (sys.name()=="" && m_saveCutFlow){
      
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
      //if (!m_bypass && !pass_preselection) continue;
      filter.setPassed(true);
    }
    return StatusCode::SUCCESS;
  }

  StatusCode twoLepSelectorAlg::finalize(){

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

  void twoLepSelectorAlg :: leptonSelection (const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer* muons){

    int n_leptons = 0;

    const xAOD::Electron* ele0 = nullptr;
    const xAOD::Electron* ele1 = nullptr;
    for (const xAOD::Electron *electron : *electrons){
      if(!ele0) ele0 = electron;
      else if(!ele1) ele1 = electron;

      n_leptons += 1;
    }

    if (ele0 && ele1) {
      //if((ele0->p4() + ele1->p4()).M() > 40*Athena::Units::GeV){
      if (ele0->charge() * ele1->charge() > 0)
        m_bools.at(VBSSEMILEP::PASS_TWO_SS_CHARGE_LEPTONS) = true; 
      else
        m_bools.at(VBSSEMILEP::PASS_TWO_OS_CHARGE_LEPTONS) = true;
      //}
    }

    const xAOD::Muon* mu0 = nullptr;
    const xAOD::Muon* mu1 = nullptr;
    for (const xAOD::Muon *muon : *muons){
      if(!mu0) mu0 = muon;
      else if(!mu1) mu1 = muon;

      n_leptons += 1;
    }

    if (mu0 && mu1) {
     // if((mu0->p4() + mu1->p4()).M() > 40*Athena::Units::GeV){
      if (mu0->charge() * mu1->charge() > 0)
        m_bools.at(VBSSEMILEP::PASS_TWO_SS_CHARGE_LEPTONS) = true;
      else
        m_bools.at(VBSSEMILEP::PASS_TWO_OS_CHARGE_LEPTONS) = true;
      //}
    }

    if (ele0 && mu0) {
      // One electron and one muon
      if (ele0->charge() * mu0->charge() > 0)
        m_bools.at(VBSSEMILEP::PASS_TWO_SS_CHARGE_LEPTONS) = true;
      else
        m_bools.at(VBSSEMILEP::PASS_TWO_OS_CHARGE_LEPTONS) = true;
        }

    if (n_leptons == 2)
      m_bools.at(VBSSEMILEP::PASS_TWO_LEPTONS) = true;

  }//Lepton Selection
  
  StatusCode twoLepSelectorAlg::initialiseCutflow(){

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
