/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_JETVARSHZYALG
#define HZYANALYSIS_JETVARSHZYALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HZY
{
  /// \brief An algorithm for counting containers
  class JetVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    JetVarsHZyAlg(const std::string &name, ISvcLocator *pSvcLocator);
    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetHandle{
        this, "HZyJets", "HZyAnalysisJets_%SYS%", "HZy Jet container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetORHandle{
        this, "HZyJetsOR", "HZyJetsOR_%SYS%", "HZy Jet container after jets and photons/leptons OR"};

    Gaudi::Property<std::vector<std::string>> m_floatVariables
    {this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables
    {this, "intVariableList", {}, "Name list of integer variables"};

    /// \brief Setup sys-aware output decorations
    Gaudi::Property<bool> m_doSystematics
    {this, "doSystematics", false, "Run on all systematics"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
  };
} // namespace HZY

#endif // HZYANALYSIS_JETVARSHZYALG
