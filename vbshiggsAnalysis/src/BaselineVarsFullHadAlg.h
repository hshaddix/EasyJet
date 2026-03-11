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

#include <xAODMissingET/MissingETContainer.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace VBSHIGGS {

class BaselineVarsFullHadAlg final : public AthHistogramAlgorithm {

public:
  BaselineVarsFullHadAlg(const std::string&, ISvcLocator*);
  StatusCode initialize() override;
  StatusCode execute() override;

private:
  CP::SysListHandle m_systematicsList{this};

  CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
    this, "event", "EventInfo", ""};

  Gaudi::Property<bool> m_isMC { this, "isMC", false, "Is this simulation?" };
  CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};
  //   CP::SysReadDecorHandle<char> m_isBtag{
  //     this, "isBtag", "DFCommonJets_btagged_%SYS%", ""};

  CP::SysReadDecorHandle<int> m_PCBT {this, "PCBTDecorName", "", "Name of pseudo-continuous b-tagging decorator"};

  CP::SysReadDecorHandle<int> m_truthFlav { this, "truthFlav", "HadronConeExclTruthLabelID", "Jet truth flavor" };

 
  CP::SysReadHandle<xAOD::JetContainer> m_vbsLRJetHandle{ this, "vbsLRJets", "vbshiggsAnalysisLargeJets_%SYS%", ""};

  CP::SysReadHandle<xAOD::JetContainer> m_vbsJetHandle{ this, "vbsJets", "",   "VBS Jet container to read" };



  CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{ this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", ""};

    
  CP::SysReadHandle<xAOD::JetContainer> m_RNNjetBoostedHandle{ this, "RNNJets_boosted", "RNNJets_boostedCategory_%SYS%", "input RNN jets container for boosted 20 GeV" };

  CP::SysReadHandle<xAOD::JetContainer> m_RNNjetResolvedHandle{ this, "RNNJets_resolved", "RNNJets_resolvedCategory_%SYS%", "input RNN jets container for resolved 20 GeV" };

  CP::SysReadHandle<xAOD::ElectronContainer> m_vbsElectronHandle{ this, "vbsElectrons", "vbshiggsAnalysisElectrons_%SYS%",   "Electron container to read" };
  CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{ this, "electrons", "AnalysisElectrons_%SYS%", "Original electron container to read" };

  CP::SysReadHandle<xAOD::MuonContainer> m_vbsMuonHandle{ this, "vbsMuons", "vbshiggsAnalysisMuons_%SYS%",   "Muon container to read" };
  CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{ this, "muons", "AnalysisMuons_%SYS%", "Original muon container to read" };
  
  CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{ this, "met", "AnalysisMET_%SYS%",   "MET container to read" };

  
  CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{
    this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", ""};



  Gaudi::Property<bool> m_doResolved{this, "do_resolved", false};

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
