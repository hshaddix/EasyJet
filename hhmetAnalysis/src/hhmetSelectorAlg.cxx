/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "hhmetSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace HHMET
{
  hhmetSelectorAlg::hhmetSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }


  StatusCode hhmetSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         hhmetSelectorAlg         \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    if (!m_lRjetHandle.empty()) {
      ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    }
    if (!m_jetHandle.empty()) {
      ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    }
    
    if (!m_electronHandle.empty()) {
      ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
    }
    
    if (!m_muonHandle.empty()) {
      ATH_CHECK (m_muonHandle.initialize(m_systematicsList));
    }
    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }
    
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    //Initialize trigger decorations
    for (const std::string &trig : m_Triggers)
    {
      std::string triggerDecorName = "trigPassed_"+trig;
      SG::ReadDecorHandleKey< xAOD::EventInfo > triggerDecorKey("EventInfo." + triggerDecorName);
      m_triggerDecorKeys.emplace(trig,triggerDecorKey);
      ATH_CHECK(m_triggerDecorKeys.at(trig).initialize());
    }
    
    for (const std::string &string_var: m_inputCutList) {
      CP::SysWriteDecorHandle<char> var {string_var+"_%SYS%", this};
      m_Bbranches.emplace(string_var, var);
      ATH_CHECK (m_Bbranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    if(m_saveCutFlow) ATH_CHECK (initialiseCutflow());
    return StatusCode::SUCCESS;
  }


  StatusCode hhmetSelectorAlg::execute(const EventContext& ctx)
  {
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys));

      // reset all cut flags to default=false
      for (CutEntry& cut : m_hhmetCuts) {
        cut.passed = false;
        m_Bbranches.at(cut.name).set(*event, cut.passed, sys);
      }
    
      evaluateTriggerCuts(*event, m_Triggers, m_hhmetCuts, ctx);
      
      // Get number of jets
      int n_boosted_jets = 0;
      int n_resolved_jets = 0;
      int n_b_jets = 0;
      if (!m_lRjetHandle.empty()) {
        const xAOD::JetContainer *lRjets = nullptr;
        ANA_CHECK (m_lRjetHandle.retrieve (lRjets, sys));
        n_boosted_jets = lRjets->size();
      }
      if (!m_jetHandle.empty()) {
        const xAOD::JetContainer *jets = nullptr;
        ANA_CHECK (m_jetHandle.retrieve (jets, sys));
        n_resolved_jets = jets->size();
        // Count b-tagged jets
        if (!m_isBtag.empty()) {
          for (const xAOD::Jet *jet : *jets) {
            if (m_isBtag.get(*jet, sys)) n_b_jets++;
          }
        }
      }

      evaluateJetCuts(n_boosted_jets, n_resolved_jets, n_b_jets, m_hhmetCuts);

      int n_electrons = 0;
      if (!m_electronHandle.empty()) {
        const xAOD::ElectronContainer *electrons = nullptr;
        ANA_CHECK (m_electronHandle.retrieve (electrons, sys));
        n_electrons = electrons->size();
      }

      int n_muons = 0;
      if (!m_muonHandle.empty()) {
        const xAOD::MuonContainer *muons = nullptr;
        ANA_CHECK (m_muonHandle.retrieve (muons, sys));
        n_muons = muons->size();
      }

      evaluateLeptonCuts(n_electrons, n_muons, m_hhmetCuts);

      bool passedall = true;
      for (CutEntry& cut : m_hhmetCuts) {
        passedall = passedall && cut.passed;
        m_Bbranches.at(cut.name).set(*event, cut.passed, sys);
      }

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      if (m_bypass || passedall) filter.setPassed(true);

      // Only store cutflow for NOSYS
      if (sys.name()=="" && m_saveCutFlow){

        // Compute total_events
        m_total_events+=1;
        if (m_isMC) m_total_mcEventWeight+= m_generatorWeight.get(*event, sys);

        // Count how many cuts the event passed and increase the relative counter
        for (const auto &cut : m_inputCutList) {
          if(m_hhmetCuts.exists(cut)) {
            if (m_hhmetCuts(cut).passed) {
              m_hhmetCuts(cut).counter += 1;
              if (m_isMC) m_hhmetCuts(cut).w_counter += m_generatorWeight.get(*event, sys);
            }
          }
        }

        // Check how many consecutive cuts are passed by the event.
        unsigned int consecutive_cuts = 0;
        for (size_t i = 0; i < m_hhmetCuts.size(); ++i) {
          if (m_hhmetCuts[i].passed) consecutive_cuts++;
          else break;
        }

        // Here we basically increment the  N_events(pass_i  AND pass_i-1  AND ... AND pass_0) for the i-cut.
        // I think this is an elegant way to do it :) . Considering the difficulties a configurable cut list imposes. 
        for (unsigned int i=0; i < consecutive_cuts; i++) {
          m_hhmetCuts[i].relativeCounter += 1;
          if (m_isMC) m_hhmetCuts[i].w_relativeCounter += m_generatorWeight.get(*event, sys);
        }
      }

    }

    return StatusCode::SUCCESS;
  }

  StatusCode hhmetSelectorAlg::finalize()
  {
    //adapt the following for each syst TODO
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    m_hhmetCuts.CheckCutResults(); // Print CheckCutResults

    if(m_saveCutFlow) {
      m_hhmetCuts.DoAbsoluteEfficiency(m_total_events, efficiency("AbsoluteEfficiency"));
      m_hhmetCuts.DoRelativeEfficiency(m_total_events, efficiency("RelativeEfficiency"));
      m_hhmetCuts.DoStandardCutFlow(m_total_events, efficiency("StandardCutFlow"));
      if (m_isMC) {
        m_hhmetCuts.DoWeightedAbsoluteEfficiency(m_total_mcEventWeight, efficiency("WeightedAbsoluteEfficiency"));
        m_hhmetCuts.DoWeightedRelativeEfficiency(m_total_mcEventWeight, efficiency("WeightedRelativeEfficiency"));
        m_hhmetCuts.DoWeightedStandardCutFlow(m_total_mcEventWeight, efficiency("WeightedStandardCutFlow"));
      }
      m_hhmetCuts.DoCutflowLabeling(m_total_events, hist("EventsPassed_BinLabeling"));
    }

    return StatusCode::SUCCESS;

  }

  void hhmetSelectorAlg::evaluateTriggerCuts(const xAOD::EventInfo& event, const std::vector<std::string> &Triggers, 
                                            CutManager& hhmetCuts, const EventContext& ctx) {
    if (!hhmetCuts.exists("PASS_TRIGGER"))
        return;
    
    bool pass_trigger = false;

    for (const std::string &trigger : Triggers)
    {
      SG::ReadDecorHandleKey<xAOD::EventInfo>& triggerDecorKey = m_triggerDecorKeys.at(trigger);
      SG::ReadDecorHandle<xAOD::EventInfo, bool> m_triggerDecorHandle(triggerDecorKey, ctx);    // OR between triggers
      pass_trigger |= m_triggerDecorHandle(event);     
    }
    hhmetCuts("PASS_TRIGGER").passed = pass_trigger;
  }

  void hhmetSelectorAlg::evaluateJetCuts(const int nBoostedJets, const int nResolvedJets, const int nBJets, CutManager& hhmetCuts)
  {
    if (((nResolvedJets >= 2 && nBoostedJets >= 1) || (nResolvedJets >= 4 && nBJets >= 3)) && hhmetCuts.exists("AT_LEAST_FOUR_JETS_THREE_BJETS_OR_AT_LEAST_TWO_JETS_ONE_LRJET"))
      hhmetCuts("AT_LEAST_FOUR_JETS_THREE_BJETS_OR_AT_LEAST_TWO_JETS_ONE_LRJET").passed = true;
  }

    
  void hhmetSelectorAlg::evaluateLeptonCuts(const int nElectrons, const int nMuons, CutManager& hhmetCuts)
  {
    if (nElectrons == 0 && nMuons == 0 && hhmetCuts.exists("ZERO_LEPTONS"))
      hhmetCuts("ZERO_LEPTONS").passed = true;
  }

  StatusCode hhmetSelectorAlg::initialiseCutflow(){

    m_hhmetCuts.CheckInputCutList(m_inputCutList,m_STANDARD_CUTS);

    for (const std::string &cut : m_inputCutList)  {
      // Initialize a vector of CutEntry structs based on the input Cut List
      m_hhmetCuts.add(cut);
    }

    //After filling the CutManager, book your histograms.
    const unsigned int nbins = m_hhmetCuts.size() + 1; //  need an extra bin for the total num of events.
    ANA_CHECK (book (TEfficiency("AbsoluteEfficiency","Absolute Efficiency of HH-> cuts.Needs rescaling to total events.;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("RelativeEfficiency","Relative Efficiency of HH-> cuts.Needs rescaling to total events.;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ANA_CHECK (book (TEfficiency("StandardCutFlow","StandardCutFlow of HH-> cuts.Needs rescaling to total events.;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    if (m_isMC) {
      ANA_CHECK (book (TEfficiency("WeightedAbsoluteEfficiency","Weighted Absolute Efficiency of HH-> cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
      ANA_CHECK (book (TEfficiency("WeightedRelativeEfficiency","Weighted Relative Efficiency of HH-> cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
      ANA_CHECK (book (TEfficiency("WeightedStandardCutFlow","Weighted StandardCutFlow of HH-> cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon",
				   nbins, 0.5, nbins + 0.5)));
    }
    ANA_CHECK (book (TH1F("EventsPassed_BinLabeling", "Events passed by each cut / Bin labeling", nbins, 0.5, nbins + 0.5)));

    return StatusCode::SUCCESS;
  }

}
