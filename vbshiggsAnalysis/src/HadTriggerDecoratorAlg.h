/*
   Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/


// Always protect against multiple includes!
#ifndef VBSHIGGSANALYSIS_HADTRIGGERDECORATORALG
#define VBSHIGGSANALYSIS_HADTRIGGERDECORATORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>
#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include "TriggerMatchingTool/IMatchingTool.h"

#include <EasyjetHub/CutManager.h>
#include "vbshiggsEnums.h"

namespace VBSHIGGS
{
  class HadTriggerDecoratorAlg final : public EL::AnaAlgorithm
  {
    public:
      HadTriggerDecoratorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      StatusCode initialize() override;
      StatusCode execute(const EventContext& ctx) override;
      CP::SysListHandle m_systematicsList {this};

      std::unordered_map<VBSHIGGS::TriggerChannel, std::unordered_map<VBSHIGGS::Var_Trigger, float>> m_pt_threshold;

      CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

      CP::SysReadDecorHandle<unsigned int> m_year {this, "year", "dataTakingYear", ""};

      Gaudi::Property<std::vector<std::string>> m_triggers 
      { this, "triggerLists", {}, "Name list of trigger" };

      std::unordered_map<std::string, CP::SysReadDecorHandle<bool> > m_triggerdecos;

      ToolHandle<Trig::IMatchingTool> m_matchingTool
      { this, "trigMatchingTool", "", "Trigger matching tool"};


      CP::SysWriteDecorHandle<float> m_eventTriggerSF {this, "event_trigEffSF", "event_trigEffSF_%SYS%", "Event-level trigger scale factor"};

      Gaudi::Property<bool> m_saveHighLevelVariables
      { this, "saveHighLevelVariables", false, "Flag to save high level variables"};

      std::unordered_map<VBSHIGGS::TriggerChannel, CP::SysWriteDecorHandle<char> > m_trig_branches;
      std::unordered_map<VBSHIGGS::TriggerChannel, bool> m_trig_bools;
      std::unordered_map<VBSHIGGS::TriggerChannel, std::string> m_triggerChannels = 
      {
        {VBSHIGGS::JET, "JET"},
      };

      std::unordered_map<VBSHIGGS::RunBooleans, std::string> m_runBooleans =
      {
        {VBSHIGGS::is22_75bunches, "is2022_75bunches"},
        {VBSHIGGS::is23_75bunches, "is2023_75bunches"},
        {VBSHIGGS::is23_400bunches, "is2023_400bunches"},
        {VBSHIGGS::is24_75bunches, "is2024_75bunches"},
        {VBSHIGGS::is24_400bunches_periodD, "is2024_400bunches_periodD"},
        {VBSHIGGS::is24_400bunches_periodL, "is2024_400bunches_periodL"},
      };
      std::map<VBSHIGGS::RunBooleans, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_runBooleans_key;
      typedef std::unordered_map<VBSHIGGS::RunBooleans, SG::ReadDecorHandle<xAOD::EventInfo, char> > runBoolReadDecoMap;


      void evaluateTriggerCuts(const xAOD::EventInfo *event,
			       const runBoolReadDecoMap& runBoolDecos,
			       const CP::SystematicSet& sys);

      void evaluateSingleJetTrigger(const xAOD::EventInfo* event,
            const std::vector<std::string>& jet_paths,
            bool& jet_pass,
            const CP::SystematicSet& sys);

      void setThresholds(const xAOD::EventInfo* event,
			 const runBoolReadDecoMap& runBoolDecos,
			 const CP::SystematicSet& sys);
  };
}

#endif
