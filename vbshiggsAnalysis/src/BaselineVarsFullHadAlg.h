#ifndef VBSHIGGSANALYSIS_BASELINEVARSFULLHADALG_H
#define VBSHIGGSANALYSIS_BASELINEVARSFULLHADALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysListHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace VBSHIGGS {

class BaselineVarsFullHadAlg final : public AthHistogramAlgorithm {

public:
  using AthHistogramAlgorithm::AthHistogramAlgorithm;
  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) override;

private:
  CP::SysListHandle m_systematicsList{this};

  CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
    this, "event", "EventInfo", ""};

  Gaudi::Property<bool> m_isMC { this, "isMC", false, "Is this simulation?" };
  CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

  CP::SysReadDecorHandle<int> m_PCBT {this, "PCBTDecorName", "", "Name of pseudo-continuous b-tagging decorator"};

  CP::SysReadDecorHandle<int> m_truthFlav { this, "truthFlav", "HadronConeExclTruthLabelID", "Jet truth flavor" };

  CP::SysReadHandle<xAOD::JetContainer> m_smallJetHandle{ this, "smallJets", "vbshiggsAnalysisJets_%SYS%", "All small-R jets for counting" };

  CP::SysReadHandle<xAOD::JetContainer> m_vbsLRJetHandle{ this, "vbsLRJets", "vbshiggsAnalysisLargeJets_%SYS%", "Large R Jet container to read" };



  CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", ""};

    
  CP::SysReadHandle<xAOD::JetContainer> m_RNNjetBoostedHandle{ this, "RNNJets_boosted", "RNNJets_boostedCategory_%SYS%", "input RNN jets container for boosted 20 GeV" };

  CP::SysReadHandle<xAOD::JetContainer> m_RNNjetResolvedHandle{ this, "RNNJets_resolved", "RNNJets_resolvedCategory_%SYS%", "input RNN jets container for resolved 20 GeV" };
  
  
  CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{
    this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", ""};



  Gaudi::Property<bool> m_doResolved{this, "do_resolved", false};

    // W-tagger
    Gaudi::Property<std::string> m_WTag_Type { this, "wtag_type", "", "WTagger type"};
    Gaudi::Property<std::string> m_WTag_WP { this, "wtag_wp", "", "WTagger wp"};
    CP::SysReadDecorHandle<float> m_WTag_score{"", this};
    CP::SysReadDecorHandle<char> m_Pass_WTag{"", this};

  Gaudi::Property<std::vector<std::string>> m_floatVariables{
    this, "floatVariableList", {}};

  Gaudi::Property<std::vector<std::string>> m_intVariables{
    this, "intVariableList", {}};

  std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
  std::unordered_map<std::string, CP::SysWriteDecorHandle<int>>   m_Ibranches;
  Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Not use VBS tagging jets or yes (i.e. use VBF-RNN jets or not)" };
};

}

#endif
