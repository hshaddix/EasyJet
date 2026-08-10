/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


// Always protect against multiple includes!
#ifndef VBSHIGGSANALYSIS_TRIGGERDECORATORALG
#define VBSHIGGSANALYSIS_TRIGGERDECORATORALG

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

#include <EasyjetHub/CutManager.h>
#include <algorithm>
#include "vbshiggsEnums.h"

namespace VBSHIGGS
{
  class TriggerDecoratorAlg final : public EL::AnaAlgorithm
  {
    public:
      using EL::AnaAlgorithm::AnaAlgorithm;

      StatusCode initialize() override;
      StatusCode execute(const EventContext& ctx) override;
      CP::SysListHandle m_systematicsList {this};

      std::unordered_map<VBSHIGGS::TriggerChannel, std::unordered_map<VBSHIGGS::Var_Trigger, float>> m_pt_threshold;

      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

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

      Gaudi::Property<bool> m_doDiLeptonTrigger
      { this, "doDiLeptonTrigger", true,
        "Evaluate di-lepton and asymmetric-lepton triggers (DLT/ASLT) in addition to single-lepton (SLT); "
        "disable for channels (e.g. FullLep) whose triggerLists only contain SLT chains"};

      std::unordered_map<VBSHIGGS::TriggerChannel, CP::SysWriteDecorHandle<char> > m_trig_branches;
      std::unordered_map<VBSHIGGS::TriggerChannel, bool> m_trig_bools;
      std::unordered_map<VBSHIGGS::TriggerChannel, std::string> m_triggerChannels = 
      {
        {VBSHIGGS::SLT, "SLT"},
        {VBSHIGGS::DLT, "DLT"},
        {VBSHIGGS::ASLT1_em, "ASLT1_em"},
        {VBSHIGGS::ASLT1_me, "ASLT1_me"},
        {VBSHIGGS::ASLT2, "ASLT2"},
      };

      std::unordered_map<VBSHIGGS::RunBooleans, std::string> m_runBooleans =
      {
        {VBSHIGGS::is17_periodB5_B8, "is2017_periodB5_B8"},
        {VBSHIGGS::is22_75bunches, "is2022_75bunches"},
        {VBSHIGGS::is23_75bunches, "is2023_75bunches"},
        {VBSHIGGS::is23_400bunches, "is2023_400bunches"},
        {VBSHIGGS::is24_75bunches, "is2024_75bunches"},
        {VBSHIGGS::is24_400bunches_periodD, "is2024_400bunches_periodD"},
        {VBSHIGGS::is24_400bunches_periodL, "is2024_400bunches_periodL"},
      };
      std::map<VBSHIGGS::RunBooleans, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_runBooleans_key;
      typedef std::unordered_map<VBSHIGGS::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;

      CP::SysWriteDecorHandle<char> m_ele0_passSET_decor
	{"ele0_passSET_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_ele1_passSET_decor
	{"ele1_passSET_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_mu0_passSMT_decor
	{"mu0_passSMT_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_mu1_passSMT_decor
	{"mu1_passSMT_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_diele_passDET_decor
	{"diele_passDET_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_dimu_passDMT_decor
	{"dimu_passDMT_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_elemu_passASLT1em_decor
	{"elemu_passASLT1em_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_elemu_passASLT1me_decor
	{"elemu_passASLT1me_%SYS%", this};
      CP::SysWriteDecorHandle<char> m_elemu_passASLT2_decor
	{"elemu_passASLT2_%SYS%", this};

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

      CP::SysWriteDecorHandle<std::vector<std::string>> m_diele_trigPassed_decor
	{"diele_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_diele_trigMatched_decor
	{"diele_trigMatched_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_dimu_trigPassed_decor
	{"dimu_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_dimu_trigMatched_decor
	{"dimu_trigMatched_%SYS%", this};

      CP::SysWriteDecorHandle<std::vector<std::string>> m_elemu_trigPassed_decor
	{"elemu_trigPassed_%SYS%", this};
      CP::SysWriteDecorHandle<std::vector<std::string>> m_elemu_trigMatched_decor
	{"elemu_trigMatched_%SYS%", this};

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
      void evaluateDiLeptonTrigger(const xAOD::EventInfo* event,
				       const runBoolReadDecoMap& runBoolDecos,
				       const xAOD::Electron *ele0, const xAOD::Electron *ele1,
                               const xAOD::Muon *mu0, const xAOD::Muon *mu1,
				       const CP::SystematicSet& sys, 
				       std::vector<std::string>& diele_trigPassed,
				       std::vector<std::string>& dimu_trigPassed,
				       std::vector<std::string>& diele_trigMatched,
				       std::vector<std::string>& dimu_trigMatched,
				       bool& diele_passDET, bool& dimu_passDMT);
      void evaluateAsymmetricLeptonTrigger(const xAOD::EventInfo* event,
                               const runBoolReadDecoMap& runBoolDecos,
					 const xAOD::Electron* ele, const xAOD::Muon* mu,
					 const CP::SystematicSet& sys,
                               std::vector<std::string>& elemu_trigPassed,
                               std::vector<std::string>& elemu_trigMatched,
                               bool& elemu_passASLT1em, bool& elemu_passASLT1me,
                               bool& elemu_passASLT2);

      void setThresholds(const xAOD::EventInfo* event,
			 const runBoolReadDecoMap& runBoolDecos,
			 const CP::SystematicSet& sys);
  };
}

#endif
