/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_JETVARSHZYALG
#define HZYANALYSIS_JETVARSHZYALG

#include <AsgDataHandles/ReadHandleKey.h>
#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODTracking/VertexContainer.h>

namespace HZY
{
  class JetVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;
    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetHandle{
        this, "HZyJets", "HZyAnalysisJets_%SYS%", "HZy Jet container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetORHandle{
        this, "HZyJetsOR", "HZyJetsOR_%SYS%", "HZy Jet container after OR"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetORExtraHandle{
        this, "HZyJetsOR_Extra", "HZyJetsOR_Extra_%SYS%", "HZy Jet container after OR with all objs"};

    CP::SysReadDecorHandle<char> m_isBtag{this, "bTagWPDecorName",
        "", "Name of input dectorator for b-tagging"};

    Gaudi::Property<std::vector<std::string>> m_floatVariables
    {this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables
    {this, "intVariableList", {}, "Name list of integer variables"};

    Gaudi::Property<std::vector<std::string>> m_floatVecVariables
    {this, "floatVecVariableList", {}, "Name list of per-jet float vector variables"};

    Gaudi::Property<bool> m_doSystematics
    {this, "doSystematics", false, "Run on all systematics"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<std::vector<float>>> m_FvecBranches;
  };
} // namespace HZY

#endif // HZYANALYSIS_JETVARSHZYALG
