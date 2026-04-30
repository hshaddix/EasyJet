/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef VBSSEMILEPANALYSIS_SEMILEPSELECTORALG_H
#define VBSSEMILEPANALYSIS_SEMILEPSELECTORALG_H

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
#include "vbssemilepEnums.h"


namespace VBSSEMILEP{

  /// \brief An algorithm for counting containers
  class oneLepSelectorAlg final : public EL::AnaAlgorithm{
    public:
      oneLepSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

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
        "PASS_ONE_LEPTON",
        "PASS_MET",
        "PASS_VBS_BASELINE",
      };

      void leptonSelection(const xAOD::ElectronContainer& electrons,const xAOD::MuonContainer& muons, const xAOD::MissingET *met);
      void vbsjetsSelection(const xAOD::JetContainer * vbsjets);
      void resolvedSelection(const xAOD::JetContainer * signaljets);
      void boostedSelection(const xAOD::JetContainer *largeJets, const CP::SystematicSet& sys);
      StatusCode initialiseCutflow();
      
      Gaudi::Property<bool> m_bypass{ this, "bypass", false, "Run selector algorithm in pass-through mode" };

      // \brief Setup syst-aware input container handles
      CutManager m_vbssemilepCuts;

      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::JetContainer> m_largejetHandle{ this, "largejets", "vbssemilepAnalysisLargeJets_%SYS%", "Large R Jet container to read"};
      CP::SysReadDecorHandle<float> m_GN2Xv01_phbb = {this, "phbb", "GN2Xv01_phbb", "GN2Xv01_phbb"};
      CP::SysReadDecorHandle<float> m_GN2Xv01_phcc = {this, "phcc", "GN2Xv01_phcc", "GN2Xv01_phcc"};
      CP::SysReadDecorHandle<float> m_GN2Xv01_pqcd = {this, "pqcd", "GN2Xv01_pqcd", "GN2Xv01_pqcd"};
      CP::SysReadDecorHandle<float> m_GN2Xv01_ptop = {this, "ptop", "GN2Xv01_ptop", "GN2Xv01_ptop"};

      CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbssemilepAnalysisSignalJets_%SYS%", "Signal Jet container to read" };

      CP::SysReadHandle<xAOD::JetContainer> m_vbsjetHandle{ this, "vbsjets", "vbssemilepAnalysisVBSJets_%SYS%", "VBS Jet container to read" };

      CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",  "EventInfo container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "vbssemilepAnalysisElectrons_%SYS%",  "Electron container to read" };

      CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "vbssemilepAnalysisMuons_%SYS%",   "Muon container to read" };

      CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{ this, "met", "AnalysisMET_%SYS%",   "MET container to read" };

      CP::SysReadDecorHandle<bool> m_passTriggerSLT {this, "passTriggerSLT", "pass_trigger_SLT_%SYS%", "events pass any singlep triggers"};
      
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
        {VBSSEMILEP::PASS_ONE_LEPTON, "PASS_ONE_LEPTON"},
        {VBSSEMILEP::PASS_MET, "PASS_MET"},
        {VBSSEMILEP::PASS_ONE_LARGE_JET, "PASS_ONE_LARGE_JET"},
        {VBSSEMILEP::PASS_TWO_SIGNAL_JETS, "PASS_TWO_SIGNAL_JETS"},
        {VBSSEMILEP::PASS_VBS_BASELINE, "PASS_VBS_BASELINE"},
        {VBSSEMILEP::PASS_RES_BASELINE, "PASS_RES_BASELINE"},
        {VBSSEMILEP::PASS_MERG_BASELINE, "PASS_MERG_BASELINE"},
        {VBSSEMILEP::PASS_B_VETO, "PASS_B_VETO"},
        {VBSSEMILEP::PASS_B_TAGGED, "PASS_B_TAGGED"},
        {VBSSEMILEP::PASS_DELTA_R_SS, "PASS_DELTA_R_SS"},
        {VBSSEMILEP::PASS_RES_Z_WINDOW, "PASS_RES_Z_WINDOW"},
      };
  };
}
#endif
