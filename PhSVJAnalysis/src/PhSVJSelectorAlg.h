/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef SELECTIONFLAGSPHSVJALG_H
#define SELECTIONFLAGSPHSVJALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODMissingET/MissingETContainer.h>

namespace PhSVJ
{

  /// \brief An algorithm for counting containers
  class PhSVJSelectorAlg final : public AthHistogramAlgorithm {

    public:
      PhSVJSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      //StatusCode execute() override;
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private :
      // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
      // "someInfo"};
      Gaudi::Property<bool> m_bypass
        { this, "bypass", false, "Run selector algorithm in pass-through mode" };

      Gaudi::Property<bool> m_doLargeRJets
        { this, "doLargeRJets", true, "Whether to read and use large-R jets" };

      Gaudi::Property<bool> m_doSmallRJets
        { this, "doSmallRJets", true, "Whether to read and use small-R jets" };

      Gaudi::Property<bool> m_doElectrons
        { this, "doElectrons", false, "Enable electron container reading" };

      Gaudi::Property<bool> m_doMuons
        { this, "doMuons", false, "Enable muoon container reading" };

      Gaudi::Property<bool> m_doPhotons
        { this, "doPhotons", true, "Enable photon container reading" };

      /// \brief Setup syst-aware input container handles
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

      CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle{ this, "jets", "PhSVJ_Jets_%SYS%",   "Jet container to read" };

      CP::SysReadHandle<xAOD::JetContainer>
      m_lrjetHandle{ this, "lrjets", "PhSVJ_LRJets_%SYS%",   "Large-R jet container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer>
      m_electronHandle{ this, "electrons", "PhSVJ_Electrons_%SYS%",   "Electron container to read" };

      CP::SysReadHandle<xAOD::MuonContainer>
      m_muonHandle{ this, "muons", "PhSVJ_Muons_%SYS%",   "Muon container to read" };

      CP::SysReadHandle<xAOD::PhotonContainer>
      m_photonHandle{ this, "photons", "PhSVJ_Photons_%SYS%",   "Photon container to read" };

      CP::SysReadHandle<xAOD::MissingETContainer>
      m_metHandle{ this, "met", "AnalysisMET_%SYS%",   "MET container to read" };

      Gaudi::Property<float> m_minMet
      {this, "minMet", 20000, "Minimum MET cut"};

      Gaudi::Property<std::string> m_eleWPName
      { this, "eleWP", "","Electron ID + Iso working point" };
      CP::SysReadDecorHandle<char> m_eleWPDecorHandle{"", this};

      Gaudi::Property<std::string> m_muonWPName
        { this, "muonWP", "","Muon ID + Iso cuts" };
      CP::SysReadDecorHandle<char> m_muonWPDecorHandle{"", this};

      CP::SysFilterReporterParams m_filterParams {this, "PhSVJ selection"};

      Gaudi::Property<int> m_minLargeRJets
        { this, "minLargeRJets", -1, "Min large-R jets required (bypass=false only; -1 disables)" };

      Gaudi::Property<int> m_minPhotons
        { this, "minPhotons", -1, "Min photons required (bypass=false only; -1 disables)" };

      Gaudi::Property<bool> m_do_trigger_filtering
      { this, "do_trigger_filtering", false, "Flag to apply trigger filtering to events or not"};

      Gaudi::Property<std::vector<std::string>> m_triggerChains
        { this, "triggerChains", {}, "Trigger chains; event passes if any fires (bypass=false only)" };

      std::vector<CP::SysReadDecorHandle<bool>> m_triggerHandles;

  };

}

#endif // SELECTIONFLAGSXBBCALIBALG_H
