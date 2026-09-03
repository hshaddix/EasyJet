/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHHBBBBTTANALYSIS_JETTRIGGERDECORATORALG
#define HHHBBBBTTANALYSIS_JETTRIGGERDECORATORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

#include "HHHbbbbttEnums.h"

namespace HHHBBBBTT
{

  class JetTriggerDecoratorAlg final : public EL::AnaAlgorithm
  {
    /// \brief The standard constructor
  public:
    using EL::AnaAlgorithm::AnaAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

  private:

    typedef std::unordered_map<HHHBBBBTT::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;
    typedef std::unordered_map<std::string, SG::ReadDecorHandle<xAOD::EventInfo, bool> > trigReadDecoMap;
    
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle{ this, "jets", "bbttAnalysisJets_%SYS%", "Jet container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };
    CP::SysReadDecorHandle<unsigned int> m_year{"dataTakingYear", this};

    Gaudi::Property<std::vector<std::string>> m_triggers 
      { this, "triggerLists", {}, "Name list of trigger" };
    std::unordered_map<std::string, SG::ReadDecorHandleKey<xAOD::EventInfo> >
      m_triggerdecoKeys;
    
    const std::unordered_map<HHHBBBBTT::RunBooleans, std::string> m_runBooleans =
      {
	{HHHBBBBTT::is17PeriodB1_B4, "is2017_periodB1_B4"},
	{HHHBBBBTT::is17PeriodB5_B7, "is2017_periodB5_B7"},
	{HHHBBBBTT::is17PeriodB8_end, "is2017_periodB8_end"},
	{HHHBBBBTT::is18_tau35_medium1, "is2018_tau35_medium1"},
	{HHHBBBBTT::is18PeriodK_end, "is2018_periodK_end"},
	{HHHBBBBTT::is23_from1200bunches, "is2023_from1200bunches"},
	{HHHBBBBTT::is23_first_2400bunches, "is2023_first_2400bunches"},
	{HHHBBBBTT::l1topo_disabled, "l1TopoDisabled"},
      };
    std::map<HHHBBBBTT::RunBooleans, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_runBooleans_key;
    

    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_HLTPTDecor;
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_L1ETDecor;
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_L1EtaDecor;

    CP::SysWriteDecorHandle<float> m_jet_trigMatch_HLTPt{"trigMatch_HLT_onlinept_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_jet_trigMatch_L1Et{"trigMatch_L1_onlinept_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_jet_trigMatch_L1Eta{"trigMatch_L1_onlineeta_%SYS%", this};;

    void checkDiTauTriggers
      (const xAOD::EventInfo* eventInfo, const CP::SystematicSet& sys,
       const runBoolReadDecoMap& runBoolDecos, const trigReadDecoMap& triggerdecos,
       const xAOD::JetContainer* jets) const;

    void checkDiBJetTriggers
      (const xAOD::EventInfo* eventInfo, const CP::SystematicSet& sys,
       const runBoolReadDecoMap& runBoolDecos, const trigReadDecoMap& triggerdecos,
       const xAOD::JetContainer* jets) const;


  };
}

#endif
