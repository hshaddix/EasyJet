#ifndef VBSHIGGSANALYSIS_BASELINEVARSFULLHADALG_H
#define VBSHIGGSANALYSIS_BASELINEVARSFULLHADALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysListHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
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

  CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{
    this, "met", "AnalysisMET_%SYS%", ""};

  CP::SysReadHandle<xAOD::JetContainer> m_vbsLRJetHandle{
    this, "vbsLRJets", "vbshiggsAnalysisLargeJets_%SYS%", ""};

  CP::SysReadHandle<xAOD::JetContainer> m_signaljetHandle{
    this, "signaljets", "vbshiggsAnalysisSignalJets_%SYS%", ""};

  CP::SysReadHandle<xAOD::JetContainer> m_HCandHandle{
    this, "higgsCandidates", "vbshiggsAnalysisHJets_%SYS%", ""};

  CP::SysReadDecorHandle<char> m_isBtag{
    this, "isBtag", "DFCommonJets_btagged_%SYS%", ""};

  Gaudi::Property<bool> m_doResolved{this, "do_resolved", false};

  Gaudi::Property<std::vector<std::string>> m_floatVariables{
    this, "floatVariableList", {}};

  Gaudi::Property<std::vector<std::string>> m_intVariables{
    this, "intVariableList", {}};

  std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
  std::unordered_map<std::string, CP::SysWriteDecorHandle<int>>   m_Ibranches;
};

}

#endif
