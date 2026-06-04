/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_PHOTONVARSHZYALG
#define HZYANALYSIS_PHOTONVARSHZYALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <AthenaKernel/Units.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEgamma/PhotonContainer.h>
#include <xAODEventInfo/EventInfo.h>

namespace HZY
{
  /// \brief An algorithm for counting containers
  class PhotonVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    PhotonVarsHZyAlg(const std::string &name, ISvcLocator *pSvcLocator);
    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonHandle{
        this, "HZyPhotons", "HZyAnalysisPhotons_%SYS%",
        "HZy Photon container to read"};

    CP::SysReadDecorHandle<char> m_photonTIWPDecorHandle{
        this, "photonTIWPDecor", "baselineSelection_Tight_FixedCutLoose_%SYS%",
        "Photon tight ID + Iso selection"};

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    CP::SysReadDecorHandle<unsigned int> m_isEMTight{"DFCommonPhotonsIsEMTightIsEMValue", this};

    CP::SysReadDecorHandle<float> m_photon_SF{
        "effSF_Tight_FixedCutLoose_%SYS%", this};

    Gaudi::Property<std::vector<std::string>> m_floatVariables{
        this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables{
        this, "intVariableList", {}, "Name list of integer variables"};

    /// \brief Setup sys-aware output decorations
    Gaudi::Property<bool> m_doSystematics
    {this, "doSystematics", false, "Run on all systematics"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
  };
} // namespace HZY

#endif // HZYANALYSIS_PHOTONVARSHZYALG
