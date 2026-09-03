/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "bbbbbbSelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace HHH6B
{
  StatusCode bbbbbbSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         bbbbbbSelectorAlg         \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_filterParams.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    if (!m_lRjetHandle.empty()) {
      ATH_CHECK (m_lRjetHandle.initialize(m_systematicsList));
    }
    if (!m_jetHandle.empty()) {
      ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    }
    if (!m_jetHandleBeforeSel.empty()) {
      ATH_CHECK (m_jetHandleBeforeSel.initialize(m_systematicsList));
    }
    if (!m_truthJetHandle.empty()) {
      ATH_CHECK (m_truthJetHandle.initialize(m_systematicsList));
    }
    if (!m_isBtag.empty()) {
      ATH_CHECK (m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }
    
    if (m_saveCutFlow) ATH_CHECK (m_generatorWeight.initialize(m_systematicsList, m_eventHandle));

    // Initialize trigger decorations
    for (const std::string &trig : m_Triggers)
    {
      std::string triggerDecorName = "trigPassed_"+trig;
      SG::ReadDecorHandleKey< xAOD::EventInfo > triggerDecorKey("EventInfo." + triggerDecorName);
      m_triggerDecorKeys.emplace(trig,triggerDecorKey);
      ATH_CHECK(m_triggerDecorKeys.at(trig).initialize());
    }

    // Intialise booleans with value false. Also initialise syst-aware output decorators
    for (auto& [key, value] : m_boolnames) {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value+"_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    };

    // Initialize b-jet multiplicity categorization decorators
    ATH_CHECK (m_pass_4b.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_pass_5b.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_pass_6b.initialize(m_systematicsList, m_eventHandle));

    // Initialize per-event jet count decorators
    ATH_CHECK (m_n_truth_jets.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_n_truth_jets_after_cuts.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_n_reco_jets_before_cuts.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK (m_n_reco_jets_after_cuts.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    std::vector<std::string> boolNameList;
    for (const auto& [key, value] : m_boolnames) {
      boolNameList.push_back(value);
      m_nameToBool[value] = key;
    }
    if (m_saveCutFlow) ATH_CHECK (initialiseCutflow(boolNameList));

    return StatusCode::SUCCESS;
  }


  StatusCode bbbbbbSelectorAlg::execute(const EventContext& ctx)
  {
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner (m_filterParams, false);
    // Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter (filterCombiner, sys);

      // Retrive inputs
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      for (auto& [key, value] : m_boolnames) m_bools.at(key) = false;
    
      evaluateTriggerCuts(*event, m_Triggers, ctx);
      
      // Get number of truth jets (for cutflow)
      int n_truth_jets = 0;
      int n_truth_jets_after_cuts = 0;
      if (!m_truthJetHandle.empty() && m_isMC) {
        const xAOD::JetContainer *truthJets = nullptr;
        ATH_CHECK (m_truthJetHandle.retrieve (truthJets, sys, ctx));
        n_truth_jets = truthJets->size();
        
        // Apply same pT/eta cuts to truth jets (20 GeV, 4.5 eta based on config)
        for (const xAOD::Jet* jet : *truthJets) {
          if (jet->pt() > 20000. && std::abs(jet->eta()) < 4.5) {
            n_truth_jets_after_cuts++;
          }
        }
      }
      
      // Get number of jets before selection (for cutflow)
      int n_jets_before_selection = 0;
      if (!m_jetHandleBeforeSel.empty()) {
        const xAOD::JetContainer *jetsBeforeSel = nullptr;
        ATH_CHECK (m_jetHandleBeforeSel.retrieve (jetsBeforeSel, sys, ctx));
        n_jets_before_selection = jetsBeforeSel->size();
      }
      
      // Get number of jets
      int n_boosted_jets = 0;
      int n_resolved_jets = 0;
      int n_b_jets = 0;
      if (!m_lRjetHandle.empty()) {
        const xAOD::JetContainer *lRjets = nullptr;
        ATH_CHECK (m_lRjetHandle.retrieve (lRjets, sys, ctx));
        n_boosted_jets = lRjets->size();
      }
      if (!m_jetHandle.empty()) {
        const xAOD::JetContainer *jets = nullptr;
        ATH_CHECK (m_jetHandle.retrieve (jets, sys, ctx));
        n_resolved_jets = jets->size();

        // Count b-tagged jets
        if (!m_isBtag.empty()) {
          for (const xAOD::Jet *jet : *jets) {
            if (m_isBtag.get(*jet, sys)) n_b_jets++;
          }
        }
      }

      evaluateJetCuts(n_boosted_jets, n_resolved_jets, n_b_jets);

      // Categorize events based on b-jet multiplicity (after preselection, but not as cuts)
      // These are labels, not cuts - all events get categorized
      bool pass_4b = (n_b_jets >= 4);
      bool pass_5b = (n_b_jets >= 5);
      bool pass_6b = (n_b_jets >= 6);
      
      // Set the categorization decorators
      m_pass_4b.set(*event, pass_4b, sys);
      m_pass_5b.set(*event, pass_5b, sys);
      m_pass_6b.set(*event, pass_6b, sys);

      // Save per-event jet counts
      m_n_truth_jets.set(*event, n_truth_jets, sys);
      m_n_truth_jets_after_cuts.set(*event, n_truth_jets_after_cuts, sys);
      m_n_reco_jets_before_cuts.set(*event, n_jets_before_selection, sys);
      m_n_reco_jets_after_cuts.set(*event, n_resolved_jets, sys);

      // do the CUTFLOW only with sys="" -> NOSYS
      if (sys.name()=="" && m_saveCutFlow){
        std::vector<bool> cutBools;
        for(const auto& cut : m_inputCutList)
          cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
        double mc_weight = m_isMC ? m_generatorWeight.get(*event, sys) : 0.;
        ATH_CHECK (applyCutFlow(cutBools, mc_weight));
      }

      for (auto& [key, var] : m_bools) {
        m_Bbranches.at(key).set(*event, var, sys);
      }
      bool passedall = true;
      for (const auto& cut : m_inputCutList)
        passedall &= m_bools.at(m_nameToBool.at(cut));

      // discard events not passing
      if (!m_bypass && !passedall) continue;

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode bbbbbbSelectorAlg::finalize()
  {
    //adapt the following for each syst TODO
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);

    ATH_CHECK (m_filterParams.finalize ());
    ATH_CHECK (finaliseCutflow());

    return StatusCode::SUCCESS;

  }

  void bbbbbbSelectorAlg::evaluateTriggerCuts(const xAOD::EventInfo& event,
					      const std::vector<std::string>& triggers,
					      const EventContext& ctx) {
    bool pass_trigger = false;

    for (const std::string &trigger : triggers)
    {
      SG::ReadDecorHandleKey<xAOD::EventInfo>& triggerDecorKey = m_triggerDecorKeys.at(trigger);
      SG::ReadDecorHandle<xAOD::EventInfo, bool> m_triggerDecorHandle(triggerDecorKey, ctx);    // OR between triggers
      pass_trigger |= m_triggerDecorHandle(event);     
    }
    m_bools.at(HHH6B::PASS_TRIGGER) = pass_trigger;
  }

  void bbbbbbSelectorAlg::evaluateJetCuts(const int nBoostedJets,
					  const int nResolvedJets,
					  const int nBJets)
  {
    m_bools.at(HHH6B::AT_LEAST_FOUR_JETS) = nResolvedJets >= 4;
    m_bools.at(HHH6B::AT_LEAST_SIX_JETS) = nResolvedJets >= 6;
    m_bools.at(HHH6B::AT_LEAST_THREE_BJETS) = nBJets >= 3;
    m_bools.at(HHH6B::AT_LEAST_FOUR_BJETS) = nBJets >= 4;
    m_bools.at(HHH6B::AT_LEAST_SIX_BJETS) = nBJets >= 6;
    m_bools.at(HHH6B::AT_LEAST_SIX_JETS_FOUR_BJETS) =
      (nResolvedJets >= 6 && nBJets >= 4);
    m_bools.at(HHH6B::AT_LEAST_SIX_JETS_OR_THREE_LRJETS) =
      (nBoostedJets >= 3 || nResolvedJets >= 6);
    m_bools.at(HHH6B::AT_LEAST_FOUR_JETS_AND_ONE_LRJET) =
      (nBoostedJets == 1 && nResolvedJets >= 4);
    m_bools.at(HHH6B::AT_LEAST_TWO_JETS_AND_TWO_LRJETS) =
      (nBoostedJets == 2 && nResolvedJets >= 2);
    m_bools.at(HHH6B::AT_LEAST_THREE_LRJETS) = nBoostedJets >= 3;
  }

}
