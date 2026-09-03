/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


#ifndef SELECTIONFLAGS6BALG_H
#define SELECTIONFLAGS6BALG_H

#include "EasyjetHub/BaseSelectorAlg.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

// For custom event weights
#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

#include "bbbbbbEnums.h"

namespace HHH6B
{
  /// \brief An algorithm for counting containers
  class bbbbbbSelectorAlg final : public Easyjet::BaseSelectorAlg {

    public:
      using Easyjet::BaseSelectorAlg::BaseSelectorAlg;

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override;

    private :

      const std::vector<std::string> m_STANDARD_CUTS{
          "PASS_TRIGGER",
          "AT_LEAST_FOUR_JETS",
          "AT_LEAST_SIX_JETS",
          "AT_LEAST_THREE_BJETS",
          "AT_LEAST_FOUR_BJETS",
          "AT_LEAST_SIX_BJETS",
          "AT_LEAST_FOUR_JETS_OR_TWO_LRJETS",
          "AT_LEAST_SIX_JETS_FOUR_BJETS",
          "AT_LEAST_TWO_JETS_AND_TWO_LRJETS",
          "AT_LEAST_FOUR_JETS_AND_ONE_LRJET",
          "AT_LEAST_THREE_LRJETS",
          "AT_LEAST_SIX_JETS_OR_THREE_LRJETS",
      };

      void evaluateTriggerCuts(const xAOD::EventInfo& event, 
			       const std::vector<std::string>& triggers,
			       const EventContext& ctx);
      void evaluateJetCuts(const int nBoostedJets, const int nResolvedJets,
			   const int nBJets);

      /// \brief Setup syst-aware input container handles
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

      CP::SysReadHandle<xAOD::JetContainer>
        m_lRjetHandle{ this, "largeRJetToCount", "", "Large-R jet container to count" };
  
      CP::SysReadHandle<xAOD::JetContainer>
        m_jetHandle{ this, "smallRJetToCount", "", "Small-R jet container to count" }; // For counting only

      CP::SysReadHandle<xAOD::JetContainer>
        m_jetHandleBeforeSel{ this, "smallRJetBeforeSelection", "", "Small-R jet container before selection (for cutflow)" };

      CP::SysReadHandle<xAOD::JetContainer>
        m_truthJetHandle{ this, "truthJets", "", "Truth jet container (for cutflow)" };

      CP::SysReadDecorHandle<char>
	m_isBtag {this, "btagSelDecor", "", "Name of input dectorator for b-tagging"};

      Gaudi::Property<std::vector<std::string>> m_Triggers{this, "Triggers", {}};

      std::unordered_map<std::string,  SG::ReadDecorHandleKey<xAOD::EventInfo>> m_triggerDecorKeys;

      CP::SysFilterReporterParams m_filterParams {this, "bbbb selection"};
      Gaudi::Property<bool> m_bypass
        { this, "bypass", false, "Run the selector algorithm in run-through mode" };

      std::unordered_map < HHH6B::Booleans, CP::SysWriteDecorHandle<char> > m_Bbranches;
      std::unordered_map < HHH6B::Booleans, bool > m_bools;
      std::unordered_map < HHH6B::Booleans, std::string > m_boolnames =
	{
	  {HHH6B::PASS_TRIGGER, "PASS_TRIGGER"},
	  {HHH6B::AT_LEAST_FOUR_JETS, "AT_LEAST_FOUR_JETS"},
	  {HHH6B::AT_LEAST_SIX_JETS, "AT_LEAST_SIX_JETS"},
	  {HHH6B::AT_LEAST_THREE_BJETS, "AT_LEAST_THREE_BJETS"},
	  {HHH6B::AT_LEAST_FOUR_BJETS, "AT_LEAST_FOUR_BJETS"},
	  {HHH6B::AT_LEAST_SIX_BJETS, "AT_LEAST_SIX_BJETS"},
	  {HHH6B::AT_LEAST_FOUR_JETS_OR_TWO_LRJETS, "AT_LEAST_FOUR_JETS_OR_TWO_LRJETS"},
	  {HHH6B::AT_LEAST_SIX_JETS_FOUR_BJETS, "AT_LEAST_SIX_JETS_FOUR_BJETS"},
	  {HHH6B::AT_LEAST_TWO_JETS_AND_TWO_LRJETS, "AT_LEAST_TWO_JETS_AND_TWO_LRJETS"},
	  {HHH6B::AT_LEAST_FOUR_JETS_AND_ONE_LRJET, "AT_LEAST_FOUR_JETS_AND_ONE_LRJET"},
	  {HHH6B::AT_LEAST_THREE_LRJETS, "AT_LEAST_THREE_LRJETS"},
	  {HHH6B::AT_LEAST_SIX_JETS_OR_THREE_LRJETS, "AT_LEAST_SIX_JETS_OR_THREE_LRJETS"},
	};
      std::unordered_map<std::string, HHH6B::Booleans> m_nameToBool;

      // B-jet multiplicity categorization decorators
      CP::SysWriteDecorHandle<bool> m_pass_4b{"pass_4b_%SYS%", this};
      CP::SysWriteDecorHandle<bool> m_pass_5b{"pass_5b_%SYS%", this};
      CP::SysWriteDecorHandle<bool> m_pass_6b{"pass_6b_%SYS%", this};

      // Per-event jet count decorators
      CP::SysWriteDecorHandle<int> m_n_truth_jets{"n_truth_jets_%SYS%", this};
      CP::SysWriteDecorHandle<int> m_n_truth_jets_after_cuts{"n_truth_jets_after_cuts_%SYS%", this};
      CP::SysWriteDecorHandle<int> m_n_reco_jets_before_cuts{"n_reco_jets_before_cuts_%SYS%", this};
      CP::SysWriteDecorHandle<int> m_n_reco_jets_after_cuts{"n_reco_jets_after_cuts_%SYS%", this};

      std::vector<float> eventWeights{};

      CP::SysReadDecorHandle<float>
        m_generatorWeight{ this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights" };

  };

}

#endif // SELECTIONFLAGS6BALG_H
