/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef VBSSEMILEPANALYSIS_FULLLEPSELECTORALG_H
#define VBSSEMILEPANALYSIS_FULLLEPSELECTORALG_H

#include <memory>

#include "AnaAlgorithm/AnaAlgorithm.h"
#include <AsgDataHandles/ReadDecorHandleKey.h>

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

#include "TriggerMatchingTool/IMatchingTool.h"
#include <EasyjetHub/CutManager.h>
#include "vbssemilepEnums.h"


namespace VBSSEMILEP{

  /// \brief An algorithm for counting containers
  class twoLepSelectorAlg final : public EL::AnaAlgorithm{
    public:
      twoLepSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute() override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private:
      const std::vector<std::string> m_STANDARD_CUTS{
        "PASS_TRIGGER",
        "PASS_TWO_LEPTONS",
      };
      std::vector<std::string> m_Bvarnames{};
     
      void setThresholds(const xAOD::EventInfo* event, const CP::SystematicSet& sys);
      void leptonSelection(const xAOD::ElectronContainer* electrons,const xAOD::MuonContainer* muons);
      StatusCode initialiseCutflow();
      
      Gaudi::Property<bool> m_bypass{ this, "bypass", false, "Run selector algorithm in pass-through mode" };

      /// \brief Setup syst-aware input container handles
      CutManager m_vbssemilepCuts;

      CP::SysListHandle m_systematicsList {this};

      // Add this alongside your signaljetHandle
      CP::SysReadHandle<xAOD::JetContainer> m_smalljetHandle{ this, "smalljets", "vbssemilepAnalysisJets_%SYS%", "Base small-R jets to check for b-tags" };
      CP::SysReadHandle<xAOD::JetContainer> m_largejetHandle{ this, "largejets", "vbssemilepAnalysisLargeJets_%SYS%", "Large R Jet container to read"};
      CP::SysReadDecorHandle<float> m_GN3Xv01_pqcdcx = {this, "pqcdcx", "GN3XPV01_pqcdcx", "GN3XPV01_pqcdcx"};
      CP::SysReadDecorHandle<float> m_GN3Xv01_pWqq = {this, "pWqq", "GN3XPV01_pWqq", "GN3XPV01_pWqq"};
      CP::SysReadDecorHandle<float> m_GN3Xv01_pqcdbx = {this, "pqcdbx", "GN3XPV01_pqcdbx", "GN3XPV01_pqcdbx"};
      CP::SysReadDecorHandle<float> m_GN3Xv01_ptop = {this, "ptop", "GN3XPV01_ptop", "GN3XPV01_ptop"};

      CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbssemilepAnalysisSignalJets_%SYS%", "Signal Jet container to read" };

      CP::SysReadHandle<xAOD::JetContainer> m_vbsjetHandle{ this, "vbsjets", "vbssemilepAnalysisVBSJets_%SYS%", "VBS Jet container to read" };

      CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",  "EventInfo container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "vbssemilepAnalysisElectrons_%SYS%",  "Electron container to read" };

      CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "vbssemilepAnalysisMuons_%SYS%",   "Muon container to read" };
      
      CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{ this, "met", "AnalysisMET_%SYS%",   "MET container to read" };

      CP::SysReadDecorHandle<char> m_passTriggerSLT {this, "passTriggerSLT", "pass_trigger_SLT_%SYS%", "events pass any singlep triggers"};
      
      CP::SysFilterReporterParams m_filterParams {this, "vbssemilep selection"};

      Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
      std::vector<VBSSEMILEP::Booleans> m_inputCutKeys;
      Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
      
      long long int m_total_events{0};

      std::unordered_map<VBSSEMILEP::Booleans, CP::SysWriteDecorHandle<bool> > m_Bbranches;
      std::unordered_map<VBSSEMILEP::Booleans, bool> m_bools;
      CP::SysWriteDecorHandle<bool> m_passallcuts {"PassAllCuts_%SYS%", this};
      std::unordered_map<VBSSEMILEP::Booleans, std::string> m_boolnames{
        {VBSSEMILEP::PASS_TRIGGER, "PASS_TRIGGER"},
        {VBSSEMILEP::PASS_TWO_LEPTONS, "PASS_TWO_LEPTONS"},
        {VBSSEMILEP::PASS_TWO_SS_CHARGE_LEPTONS, "PASS_TWO_SS_CHARGE_LEPTONS"},
        {VBSSEMILEP::PASS_TWO_OS_CHARGE_LEPTONS, "PASS_TWO_OS_CHARGE_LEPTONS"},
        {VBSSEMILEP::PASS_ONE_LARGE_JET, "PASS_ONE_LARGE_JET"},
        {VBSSEMILEP::PASS_TWO_SIGNAL_JETS, "PASS_TWO_SIGNAL_JETS"},
        {VBSSEMILEP::PASS_B_VETO, "PASS_B_VETO"},
        {VBSSEMILEP::PASS_B_TAGGED, "PASS_B_TAGGED"},
      };

      CP::SysReadDecorHandle<unsigned int> m_year {this, "year", "dataTakingYear", ""};

      Gaudi::Property<std::vector<std::string>> m_triggers 
      { this, "triggerLists", {}, "Name list of trigger" };

      Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };
      // do resolved category
      Gaudi::Property<bool> m_doResolved { this, "do_resolved", false, "Run Resolved category" };

  };
}
#endif
