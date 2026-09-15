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
#include <SystematicsHandles/SysWriteHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODJet/JetContainer.h>

namespace HZY
{
  /// \brief An algorithm for counting containers
  class PhotonVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;
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

    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonHandle{
        this, "HZyPhotons", "HZyAnalysisPhotons_%SYS%", "HZy Photon container to read (for decorations)"};

    /// Photons after overlap removal with Z-candidate leptons (produced by LeptonVarsHZyAlg)
    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonORInHandle{
        this, "HZyPhotonsOR_LepIn", "HZyPhotonsOR_Lep_%SYS%", "HZy Photons after OR with Z-cand leptons (from LeptonVarsHZyAlg)"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetHandle{
        this, "HZyJets", "HZyAnalysisJets_%SYS%", "HZy Jet container to read"};

    // Read the leptons from HZy electrons with OR and selected Z candidate (Input from from LeptonVarsAlg.cxx tool)
    CP::SysReadHandle<xAOD::ElectronContainer> m_ZCandElectronsHandle{
        this, "HZyElectronsOR_ZCand", "HZyElectronsOR_ZCand_%SYS%", "Input Electrons forming ZCand (from LLgCand tool)"};
    CP::SysReadHandle<xAOD::MuonContainer> m_ZCandMuonsHandle{
        this, "HZyMuonsOR_ZCand", "HZyMuonsOR_ZCand_%SYS%", "Input Muons forming ZCand (from LLgCand tool)"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronORHandle{
        this, "HZyElectronsOR", "HZyElectronsOR_%SYS%", "HZy Electron OR container with Muons to read"};
    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonORHandle{
        this, "HZyMuonsOR", "HZyMuonsOR_%SYS%", "HZy Muon OR container with Electrons to read"};
    
    /// \brief Write these lepton containers for VH and ttH production modes with more than 2 leptons
    CP::SysWriteHandle<ConstDataVector<xAOD::ElectronContainer>> m_HZyElORPhotonsHandle{
        this, "HZyElOR_Photons", "HZyElOR_Photons_%SYS%", "HZy Electrons OR with photons"};
    CP::SysWriteHandle<ConstDataVector<xAOD::MuonContainer>> m_HZyMuORPhotonsHandle{
        this, "HZyMuOR_Photons", "HZyMuOR_Photons_%SYS%", "HZy Muons OR with photons"};

    CP::SysWriteHandle<ConstDataVector<xAOD::PhotonContainer>> m_HZyPhotonORHandle{
        this, "HZyPhotonsOR", "HZyPhotonsOR_%SYS%", "HZy Photons passing OR with leptons and jets"};

    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>> m_HZyJetORHandle{
        this, "HZyJetsOR", "HZyJetsOR_%SYS%", "HZy Jets passing OR with leptons and photons"};

    /// Jets after full OR (Z cand leptons, photons, extra lepton) — no JVT cut.
    /// Used for HZy-specific JVT event SF computation.
    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>> m_HZyJetORNoJvtHandle{
        this, "HZyJetsOR_NoJvt", "HZyJetsOR_NoJvt_%SYS%", "HZy Jets after full OR (no JVT cut)"};

    /// Extra-lepton containers: Z cand leptons + highest-pT extra lepton
    CP::SysWriteHandle<ConstDataVector<xAOD::ElectronContainer>> m_HZyElORPhotonsExtraHandle{
        this, "HZyElOR_Photons_Extra", "HZyElOR_Photons_Extra_%SYS%", "Z cand electrons + extra electron (if any)"};
    CP::SysWriteHandle<ConstDataVector<xAOD::MuonContainer>> m_HZyMuORPhotonsExtraHandle{
        this, "HZyMuOR_Photons_Extra", "HZyMuOR_Photons_Extra_%SYS%", "Z cand muons + extra muon (if any)"};
    /// Jets after OR with Z cand leptons, photons, and the extra lepton
    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>> m_HZyJetORExtraHandle{
        this, "HZyJetsOR_Extra", "HZyJetsOR_Extra_%SYS%", "HZy Jets after OR with Z cand leptons, photons and extra lepton"};

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    Gaudi::Property<double> m_dR_Ph_El{this, "dR_Ph_El", 0.3, "deltaR cut for Photon/Electron OR"};
    Gaudi::Property<double> m_dR_Ph_CaloMu{this, "dR_Ph_CaloMu", 0.3, "deltaR cut for Photon/CaloMuon OR"};
    Gaudi::Property<double> m_dR_Ph_StacoMu{this, "dR_Ph_StacoMu", 0.3, "deltaR cut for Photon/StacoMuon OR"};
    Gaudi::Property<double> m_dR_Jet_Ph{this, "dR_Jet_Ph", 0.2, "deltaR cut for Jet/Photon OR"};
    Gaudi::Property<double> m_dR_Jet_El{this, "dR_Jet_El", 0.2, "deltaR cut for Jet/Electron OR"};
    Gaudi::Property<double> m_dR_Jet_CaloMu{this, "dR_Jet_CaloMu", 0.2, "deltaR cut for Jet/CaloMuon OR"};
    Gaudi::Property<double> m_dR_Jet_StacoMu{this, "dR_Jet_StacoMu", 0.2, "deltaR cut for Jet/StacoMuon OR"};

    /// JVT selection decoration (char, computed by CP JVT selection on raw jets)
    SG::AuxElement::ConstAccessor<char> m_jvtSelection{"jvt_selection"};

    CP::SysReadDecorHandle<char> m_photonTIdNonIsoDecorHandle{
        this, "PhotonTIdNonIsoPDecor", "baselineSelection_Tight_NonIso_%SYS%", "Photon tight ID + non Iso selection"};
    
    CP::SysReadDecorHandle<char> m_photonTIdLIsoDecorHandle{
        this, "photonTIdLIsoDecor", "baselineSelection_Tight_FixedCutLoose_%SYS%",
        "Photon tight ID + loose Iso selection"};

    CP::SysReadDecorHandle<unsigned int> m_isEMTight{"DFCommonPhotonsIsEMTightIsEMValue", this};
    CP::SysReadDecorHandle<float> m_ph_SF{"effSF_Tight_FixedCutLoose_%SYS%", this};

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
