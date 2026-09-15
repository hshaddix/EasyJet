/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes! :)
#ifndef HZYANALYSIS_TRUTHVARSHZYALG
#define HZYANALYSIS_TRUTHVARSHZYALG

#include <AsgDataHandles/ReadHandleKey.h>
#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODTruth/TruthParticleContainer.h>

#include <cstdint>

namespace HZY
{
  /// \brief Truth labelling of the associated Z boson.
  class TruthVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;
    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

    /// Values written to AssocZ_DecayLabel.
    enum class AssocZDecay : std::uint8_t
    {
      None      = 0,  ///< no associated Z in the truth record
      Leptonic  = 1,  ///< Z -> ee, mumu
      Invisible = 2,  ///< Z -> nunu
      Hadronic  = 3,  ///< Z -> qq
      Tau       = 4,  ///< Z -> tautau
    };

private:
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    SG::ReadHandleKey<xAOD::TruthParticleContainer> m_truthBosonsKey{
        this, "TruthBosonsInKey", "TruthBosonsWithDecayParticles",
        "Truth bosons with their decay particles"};

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};

    Gaudi::Property<std::vector<std::string>> m_intVariables
    {this, "intVariableList", {}, "Name list of integer variables"};

    Gaudi::Property<bool> m_doSystematics
    {this, "doSystematics", false, "Run on all systematics"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;

    /// Decay label of the associated Z, or None if the event has not got one.
    AssocZDecay classifyAssociatedZ(
        const xAOD::TruthParticleContainer &truthBosons) const;
  };
} // namespace HZY

#endif // HZYANALYSIS_TRUTHVARSHZYALG
