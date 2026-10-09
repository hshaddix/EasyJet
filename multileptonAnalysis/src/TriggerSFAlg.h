/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHMLANALYSIS_TRIGGERSFALG
#define HHMLANALYSIS_TRIGGERSFALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODTau/TauJetContainer.h>

#include "MultileptonSelectorAlg.h"

namespace MULTILEPTON
{

  /// \brief An algorithm for counting containers
  class TriggerSFAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:

    typedef std::unordered_map<MULTILEPTON::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;
    typedef std::unordered_map<MULTILEPTON::Booleans, CP::SysReadDecorHandle<char>> categoryReadDecoMap;

    void computeTriggerSF(const xAOD::EventInfo *event, const CP::SystematicSet& sys,
			  const runBoolReadDecoMap& runBoolDecos,
			  const xAOD::Electron* ele0, const xAOD::Muon* mu0,
			  const xAOD::TauJet* tau0, const xAOD::TauJet* tau1);

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle{ this, "jets", "hhmlAnalysisJets_%SYS%", "Jet container to read" };

    CP::SysReadHandle<xAOD::ElectronContainer>
      m_electronHandle{ this, "electrons", "AnalysisElectrons_%SYS%", "Original electron container to read" };
    CP::SysReadHandle<xAOD::ElectronContainer>
      m_hhmlEleHandle{ this, "hhmlElectrons", "hhmlAnalysisElectrons_%SYS%", "hhml electron container to read" };

    CP::SysReadHandle<xAOD::MuonContainer>
      m_muonHandle{ this, "muons", "AnalysisMuons_%SYS%", "Original muon container to read" };
    CP::SysReadHandle<xAOD::MuonContainer>
      m_hhmlMuonHandle{ this, "hhmlMuons", "hhmlAnalysisMuons_%SYS%", "hhml muon container to read" };

    CP::SysReadHandle<xAOD::TauJetContainer>
      m_tauHandle{ this, "taus", "AnalysisTaus_%SYS%", "Original tau container to read" };
    CP::SysReadHandle<xAOD::TauJetContainer>
      m_hhmlTauHandle{ this, "hhmlTaus", "hhmlAnalysisTaus_%SYS%", "hhml tau container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    Gaudi::Property<bool> m_doLTT
      { this, "doLTT", false, "Add LTT signal regions" };

    Gaudi::Property<std::vector<std::string>> m_eleTrigSF
      {this, "eleTriggerSF", {}, "List of electron trigger SF"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_eleTriggerSF;

    Gaudi::Property<std::vector<std::string>> m_muonTrigSF
      {this, "muonTriggerSF", {}, "List of muon trigger SF"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_muonTriggerSF;

    Gaudi::Property<std::vector<std::string>> m_tauTrigSF
      {this, "tauTriggerSF", {}, "List of tau trigger SF"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_tauTriggerSF;
    
    CP::SysReadDecorHandle<unsigned int> m_year{this, "year", "dataTakingYear", ""};

     const std::unordered_map<MULTILEPTON::RunBooleans, std::string> m_runBooleans = {
	      {MULTILEPTON::is16PeriodA, "is2016_periodA"},
	      {MULTILEPTON::is16PeriodB_D3, "is2016_periodB_D3"},
	      {MULTILEPTON::is16PeriodD4_end, "is2016_periodD4_end"},
	      {MULTILEPTON::is17PeriodB1_B4, "is2017_periodB1_B4"},
	      {MULTILEPTON::is17PeriodB5_B7, "is2017_periodB5_B7"},
	      {MULTILEPTON::is17PeriodB8_end, "is2017_periodB8_end"},
	      {MULTILEPTON::is18PeriodB_end, "is2018_periodB_end"},
	      {MULTILEPTON::is18_tau35_medium1, "is2018_tau35_medium1"},
	      {MULTILEPTON::is18PeriodK_end, "is2018_periodK_end"},
	      {MULTILEPTON::is22_75bunches, "is2022_75bunches"},
	      {MULTILEPTON::is23_75bunches, "is2023_75bunches"},
	      {MULTILEPTON::is23_400bunches, "is2023_400bunches"},
	      {MULTILEPTON::is23_from1200bunches, "is2023_from1200bunches"},
	      {MULTILEPTON::is23_first_2400bunches, "is2023_first_2400bunches"},
        {MULTILEPTON::is24_timeframeA_C, "is2024_timeframeA_C"},
        {MULTILEPTON::is24_timeframeD, "is2024_timeframeD"},
        {MULTILEPTON::is24_timeframeE_F, "is2024_timeframeE_F"},
	      {MULTILEPTON::l1topo_disabled, "l1TopoDisabled"},
      };
     std::map<MULTILEPTON::RunBooleans, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_runBooleans_key;

    categoryReadDecoMap m_categoryBranches;
    std::unordered_map < MULTILEPTON::Booleans, std::string > m_boolnames{
      {MULTILEPTON::pass_trigger_SLT, "pass_trigger_SLT"},
      {MULTILEPTON::pass_trigger_DLT, "pass_trigger_DLT"},
      {MULTILEPTON::pass_trigger_STT, "pass_trigger_STT"},
      {MULTILEPTON::pass_trigger_DTT, "pass_trigger_DTT"},
      {MULTILEPTON::pass_trigger_LTT, "pass_trigger_LTT"},
    };

    /// \brief Setup sys-aware output decorations
    CP::SysWriteDecorHandle<float> m_eventTriggerSF {"eventTriggerSF_%SYS%", this};
    // Individual leg SFs entering eventTriggerSF (1 if the leg is not used)
    CP::SysWriteDecorHandle<float> m_triggerSF_ele {"triggerSF_ele_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_triggerSF_mu {"triggerSF_mu_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_triggerSF_tau0 {"triggerSF_tau0_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_triggerSF_tau1 {"triggerSF_tau1_%SYS%", this};

  };
} // namespace MULTILEPTON

#endif

