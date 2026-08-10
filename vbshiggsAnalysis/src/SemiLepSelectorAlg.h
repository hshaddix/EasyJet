/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef VBSHIGGSANALYSIS_SEMILEPSELECTORALG_H
#define VBSHIGGSANALYSIS_SEMILEPSELECTORALG_H

#include <memory>

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMissingET/MissingETContainer.h>

#include <EasyjetHub/CutManager.h>
#include "vbshiggsEnums.h"


namespace VBSHIGGS{

  /// \brief An algorithm for counting containers
  class SemiLepSelectorAlg final : public EL::AnaAlgorithm{
    public:
      using EL::AnaAlgorithm::AnaAlgorithm;

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private:
      const std::vector<std::string> m_STANDARD_CUTS{
        "PASS_TRIGGER",
        "PASS_EXACTLY_ONE_LEPTON", 
        "PASS_TWO_OS_CHARGE_LEPTONS",
        "PASS_ONE_LARGE_JET"

      };

      void leptonSelection(const xAOD::ElectronContainer& electrons,const xAOD::MuonContainer& muons, const xAOD::MissingET *met);
      void vbsjetsSelection(const xAOD::JetContainer * vbsjets);
      void resolvedSelection(const xAOD::JetContainer *HJets, const std::vector<const xAOD::Jet*>& bjets, const CP::SystematicSet& sys);
      void boostedSelection(const xAOD::JetContainer *largeJets, const CP::SystematicSet& sys);
      StatusCode initialiseCutflow();
      
      Gaudi::Property<bool> m_bypass{ this, "bypass", false, "Run selector algorithm in pass-through mode" };

      Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };

      // \brief Setup syst-aware input container handles
      CutManager m_vbshiggsCuts;

      CP::SysListHandle m_systematicsList {this};
      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",  "EventInfo container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "vbshiggsAnalysisElectrons_%SYS%",  "Electron container to read" };
      CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "vbshiggsAnalysisMuons_%SYS%",   "Muon container to read" };

      CP::SysReadDecorHandle<char> m_passTriggerSLT {this, "passTriggerSLT", "pass_trigger_SLT_%SYS%", "events pass any singlep triggers"};
      CP::SysReadDecorHandle<char> m_passTriggerDLT {this, "passTriggerDLT", "pass_trigger_DLT_%SYS%", "events pass any dilep triggers"};
      CP::SysReadDecorHandle<char> m_passTriggerASLT1_em {this, "passTriggerASLT1_em", "pass_trigger_ASLT1_em_%SYS%", "events pass ASLT1_em triggers"};
      CP::SysReadDecorHandle<char> m_passTriggerASLT1_me {this, "passTriggerASLT1_me", "pass_trigger_ASLT1_me_%SYS%", "events pass ASLT1_me triggers"};
      CP::SysReadDecorHandle<char> m_passTriggerASLT2 {this, "passTriggerASLT2", "pass_trigger_ASLT2_%SYS%", "events pass ASLT2 triggers"};

      CP::SysReadHandle<xAOD::JetContainer> m_largejetHandle{ this, "largejets", "vbshiggsAnalysisLargeJets_%SYS%", "Large R Jet container to read"};

      CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", "Signal Jet container to read" };
      CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{ this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", "Higgs Candidates container to read"};

      CP::SysReadHandle<xAOD::JetContainer> m_vbsjetHandle{ this, "vbsjets", "vbshiggsAnalysisVBSJets_%SYS%", "VBS Jet container to read" };

      CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

      CP::SysFilterReporterParams m_filterParams {this, "vbshiggs selection"};

      Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
      std::vector<VBSHIGGS::Booleans> m_inputCutKeys;
      Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
      
      long long int m_total_events{0};

      std::unordered_map<VBSHIGGS::Booleans, CP::SysWriteDecorHandle<char> > m_Bbranches;
      std::unordered_map<VBSHIGGS::Booleans, bool> m_bools;
      CP::SysWriteDecorHandle<char> m_passallcuts {"PassAllCuts_%SYS%", this};
      std::unordered_map<VBSHIGGS::Booleans, std::string> m_boolnames{
        {VBSHIGGS::PASS_TRIGGER, "PASS_TRIGGER"},
        {VBSHIGGS::PASS_EXACTLY_ONE_LEPTON, "PASS_EXACTLY_ONE_LEPTON"},
        {VBSHIGGS::PASS_TWO_OS_CHARGE_LEPTONS, "PASS_TWO_OS_CHARGE_LEPTONS"},
        {VBSHIGGS::PASS_ONE_LARGE_JET, "PASS_ONE_LARGE_JET"},
      };
  };
}
#endif
