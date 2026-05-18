/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

*/


// Always protect against multiple includes!
#ifndef VBSSEMILEPANALYSIS_TRIGGERDECORATORALG
#define VBSSEMILEPANALYSIS_TRIGGERDECORATORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>
#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include "TriggerMatchingTool/IMatchingTool.h"
#include <xAODJet/JetContainer.h>

#include <EasyjetHub/CutManager.h>
#include <algorithm>
#include "vbssemilepEnums.h"

namespace VBSSEMILEP
{
  
  class TriggerDecoratorAlg final : public EL::AnaAlgorithm
  {
    public:
      TriggerDecoratorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      StatusCode initialize() override;
      StatusCode execute() override;
      CP::SysListHandle m_systematicsList {this};

      std::unordered_map<VBSSEMILEP::TriggerChannel, std::unordered_map<VBSSEMILEP::Var_Trigger, float>> m_pt_threshold;

      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

      CP::SysReadHandle<xAOD::JetContainer> m_jetsHandle{ this, "jets", "",   "jets container to read" };
      
      CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "",   "Electron container to read" };

      CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "",   "Muon container to read" };

      CP::SysReadDecorHandle<unsigned int> m_year {this, "year", "dataTakingYear", ""};

      Gaudi::Property<std::vector<std::string>> m_triggers 
      { this, "triggerLists", {}, "Name list of trigger" };

      std::unordered_map<std::string, CP::SysReadDecorHandle<bool> > m_triggerdecos;

      ToolHandle<Trig::IMatchingTool> m_matchingTool
      { this, "trigMatchingTool", "", "Trigger matching tool"};

      Gaudi::Property<std::vector<std::string>> m_eleTrigSF
      { this, "eleTriggerSF", {}, "List of electron trigger SF" };
      std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_eleTriggerSF;

      Gaudi::Property<std::vector<std::string>> m_muTrigSF
      { this, "muTriggerSF", {}, "List of muon trigger SF" };
      std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_muTriggerSF;
      
      CP::SysWriteDecorHandle<float> m_eventTriggerSF {this, "event_trigEffSF", "event_trigEffSF_%SYS%", "Event-level trigger scale factor"};

      Gaudi::Property<bool> m_saveHighLevelVariables
      { this, "saveHighLevelVariables", false, "Flag to save high level variables"};

      std::unordered_map<VBSSEMILEP::TriggerChannel, CP::SysWriteDecorHandle<char> > m_trig_branches;
      std::unordered_map<VBSSEMILEP::TriggerChannel, bool> m_trig_bools;
      std::unordered_map<VBSSEMILEP::TriggerChannel, std::string> m_triggerChannels = 
      {
        {VBSSEMILEP::SLT, "SLT"},
        {VBSSEMILEP::SJT, "SJT"},
        {VBSSEMILEP::SJTM, "SJTM"},

      };

      std::unordered_map<VBSSEMILEP::RunBooleans, std::string> m_runBooleans =
      {
        {VBSSEMILEP::is22_75bunches, "is2022_75bunches"},
        {VBSSEMILEP::is23_75bunches, "is2023_75bunches"},
        {VBSSEMILEP::is23_400bunches, "is2023_400bunches"},
      };
      std::map<VBSSEMILEP::RunBooleans, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_runBooleans_key;
      typedef std::unordered_map<VBSSEMILEP::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;

      bool ele0_passSET = false;
      bool ele1_passSET = false;
      bool mu0_passSMT = false;
      bool mu1_passSMT = false;

      std::vector<std::string> ele0_trigPassed;
      std::vector<std::string> ele1_trigPassed;
      std::vector<std::string> mu0_trigPassed;
      std::vector<std::string> mu1_trigPassed;

      std::vector<std::string> ele0_trigMatched;
      std::vector<std::string> ele1_trigMatched;
      std::vector<std::string> mu0_trigMatched;
      std::vector<std::string> mu1_trigMatched;

      CP::SysWriteDecorHandle<char> m_ele0_passSET_decor
	{"ele0_passSET_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_ele1_passSET_decor
	{"ele1_passSET_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_mu0_passSMT_decor
	{"mu0_passSMT_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_mu1_passSMT_decor
	{"mu1_passSMT_%SYS%", this};


       CP::SysWriteDecorHandle<std::vector<std::string>> m_ele0_trigPassed_decor
	{"ele0_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_ele1_trigPassed_decor
	{"ele1_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_mu0_trigPassed_decor
	{"mu0_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_mu1_trigPassed_decor
	{"mu1_trigPassed_%SYS%", this};

      CP::SysWriteDecorHandle<std::vector<std::string>> m_ele0_trigMatched_decor
	{"ele0_trigMatched_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_ele1_trigMatched_decor
	{"ele1_trigMatched_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_mu0_trigMatched_decor
	{"mu0_trigMatched_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_mu1_trigMatched_decor
	{"mu1_trigMatched_%SYS%", this};


      void evaluateTriggerCuts(const xAOD::EventInfo *event,
			       const runBoolReadDecoMap& runBoolDecos,
			       const xAOD::ElectronContainer *electrons ,
			       const xAOD::MuonContainer *muons,
			       const CP::SystematicSet& sys);
      void evaluateSingleLeptonTrigger(const xAOD::EventInfo* event,
				       const runBoolReadDecoMap& runBoolDecos,
				       const xAOD::Electron* ele, const xAOD::Muon* mu,
				       const CP::SystematicSet& sys, 
				       std::vector<std::string>& ele_trigPassed,
				       std::vector<std::string>& mu_trigPassed,
				       std::vector<std::string>& ele_trigMatched,
				       std::vector<std::string>& mu_trigMatched,
				       bool& ele_passSET, bool& mu_passSMT);
      void setThresholds(const xAOD::EventInfo* event,
			 const runBoolReadDecoMap& runBoolDecos,
			 const CP::SystematicSet& sys);
			 
      void checkSingleJetTrigger(const xAOD::EventInfo *event, const CP::SystematicSet& sys);
      void checkSingleJetTriggerMatching(const xAOD::EventInfo *event, const xAOD::Jet *jet, const CP::SystematicSet& sys);

  };
}

#endif
