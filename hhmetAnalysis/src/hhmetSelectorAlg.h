/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef SELECTIONFLAGS4BALG_H
#define SELECTIONFLAGS4BALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

// For custom event weights
#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>

#include <EasyjetHub/CutManager.h>

class CutManager;

namespace HHMET
{
  /// \brief An algorithm for counting containers
  class hhmetSelectorAlg final : public AthHistogramAlgorithm {

    public:
      using AthHistogramAlgorithm::AthHistogramAlgorithm;

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private :

      const std::vector<std::string> m_STANDARD_CUTS{
          "PASS_TRIGGER",
          "AT_LEAST_FOUR_JETS_OR_TWO_LRJETS",
          "AT_LEAST_FOUR_JETS_TWO_BJETS_OR_TWO_LRJETS",
          "AT_LEAST_TWO_LRJETS",
          "AT_LEAST_FOUR_JETS",
          "AT_LEAST_TWO_BJETS",
          "ZERO_LEPTONS",
          "AT_LEAST_FOUR_JETS_THREE_BJETS_OR_AT_LEAST_TWO_JETS_ONE_LRJET"
      };

      void evaluateTriggerCuts(const xAOD::EventInfo& event, 
                          const std::vector<std::string> &Triggers, CutManager& hhmetCuts, const EventContext& ctx);
      void evaluateJetCuts(const int nBoostedJets, const int nResolvedJets, const int nBJets, CutManager& hhmetCuts);

      void evaluateLeptonCuts(const int nElectrons, const int nMuons, CutManager& hhmetCuts);

      StatusCode initialiseCutflow();

      /// \brief Setup syst-aware input container handles
      CutManager m_hhmetCuts;
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer>
        m_electronHandle{ this, "electronToCount", "", "Electron container to count" };
  
      CP::SysReadHandle<xAOD::MuonContainer>
        m_muonHandle{ this, "muonToCount", "", "Muon container to count" };
        
      CP::SysReadHandle<xAOD::JetContainer>
        m_lRjetHandle{ this, "largeRjetToCount", "", "Large-R jet container to count" };
  
      CP::SysReadHandle<xAOD::JetContainer>
        m_jetHandle{ this, "smallRjetToCount", "", "Small-R jet container to count" }; // For counting only

      CP::SysReadDecorHandle<char>
	m_isBtag {this, "btagSelDecor", "", "Name of input dectorator for b-tagging"};

      Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};

      Gaudi::Property<std::vector<std::string>> m_Triggers{this, "Triggers", {}};

      std::unordered_map<std::string,  SG::ReadDecorHandleKey<xAOD::EventInfo>> m_triggerDecorKeys;

      Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
      long long int m_total_events{0};
      double m_total_mcEventWeight{0.0};

      Gaudi::Property<bool> m_isMC
        { this, "isMC", false, "Is this simulation?" };

      CP::SysFilterReporterParams m_filterParams {this, "hhmet selection"};
      Gaudi::Property<bool> m_bypass
        { this, "bypass", false, "Run the selector algorithm in run-through mode" };

      std::unordered_map<std::string, CP::SysWriteDecorHandle<char> > m_Bbranches;

      std::vector<float> eventWeights{};

      CP::SysReadDecorHandle<float>
        m_generatorWeight{ this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights" };

  };

}

#endif // SELECTIONFLAGS4BALG_H
