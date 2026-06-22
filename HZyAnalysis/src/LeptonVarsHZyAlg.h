/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_LEPTONVARSHZYALG
#define HZYANALYSIS_LEPTONVARSHZYALG

#include <AthContainers/ConstDataVector.h>
#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODBase/IParticleContainer.h>

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/ToolHandle.h>

#include <EgammaAnalysisInterfaces/IEgammaCalibrationAndSmearingTool.h>
#include <FsrUtils/FsrPhotonTool.h>
#include <MuonAnalysisInterfaces/IMuonCalibrationAndSmearingTool.h>
#include <ZMassConstraint/ConstraintFit.h>

namespace HZY
{
  struct LLCandidate
  {
    float mll_raw = -99.0f;
    float mll_constrained = -99.0f;
    int channel = -1; // 1 for ee, 2 for mumu
    const xAOD::IParticle *l1 = nullptr;
    const xAOD::IParticle *l2 = nullptr;
    TLorentzVector c1_fit;
    TLorentzVector c2_fit;

    bool hasFSR = false;
    int fsrIdx = -1;
    float fsrDR = -99.f;
    FSR::FsrCandidate fsrCand;
    TLorentzVector fsrP4;
  };

  /// \brief An algorithm for counting containers
  class LeptonVarsHZyAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    LeptonVarsHZyAlg(const std::string &name, ISvcLocator *pSvcLocator);
    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronHandle{
        this, "HZyElectrons", "HZyAnalysisElectrons_%SYS%",
        "HZy Electrons container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonHandle{
        this, "HZyMuons", "HZyAnalysisMuons_%SYS%",
        "HZy Muons container to read"};

    // Electrons with various ID and isolation WPs: preselection
    CP::SysReadDecorHandle<char> m_electronLIdLIsoDecorHandle{
        this, "ElectronLIdLIsoDecor", "baselineSelection_LooseBLayerLH_Loose_VarRad_%SYS%", "Electron loose ID + loose Iso selection"};

    // Muons with various ID and isolation WPs: preselection
    CP::SysReadDecorHandle<char> m_muonMIdLIsoDecorHandle{
        this, "MuonMIdLIsoDecor", "baselineSelection_Medium_Loose_VarRad_%SYS%", "Muon medium ID + loose Iso selection"};

    CP::SysReadDecorHandle<float> m_ele_SF{"effSF_LooseBLayerLH_Loose_VarRad_%SYS%", this};
    CP::SysReadDecorHandle<float> m_mu_SF{"effSF_Medium_Loose_VarRad_%SYS%", this};

    // Read lepton containers from HZyLeptonORAlg, which will be used to build
    // Z candidates
    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronORHandle{
        this, "HZyElectronsOR", "HZyElectronsOR_%SYS%",
        "HZy Electron OR container to read"};
    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonORHandle{
        this, "HZyMuonsOR", "HZyMuonsOR_%SYS%",
        "HZy Muon OR container to read"};

    // Store leptons for building candidate Z to be used in HZyPhotonJetORAlg
    CP::SysWriteHandle<ConstDataVector<xAOD::ElectronContainer>> m_ZCandElectronsHandle{
        this, "HZyElectronsOR_ZCand", "HZyElectronsOR_ZCand_%SYS%", "Electrons to form ZCand container"};
    CP::SysWriteHandle<ConstDataVector<xAOD::MuonContainer>> m_ZCandMuonsHandle{
        this, "HZyMuonsOR_ZCand", "HZyMuonsOR_ZCand_%SYS%", "Muons to form ZCand container"};

    ToolHandle<CP::IEgammaCalibrationAndSmearingTool> m_egammaCalibTool{
        this, "EgammaCalibTool", "","Egamma CalibSmearing Tool for Fsr correction and ZMassConstraint fit"};
    ToolHandle<CP::IMuonCalibrationAndSmearingTool> m_muonCalibTool{
        this, "MuonCalibTool", "", "Muon CalibTool for ZMassConstraint fit"};
    
    ToolHandle<FSR::FsrPhotonTool> m_fsrTool{
        this, "FsrPhotonTool", "FSR::FsrPhotonTool/FsrPhotonTool", "FSR Photon correction tool"};
    std::unique_ptr<ZMassConstraint::ConstraintFit> m_ZMassConstraint;

    CP::SysWriteHandle<ConstDataVector<xAOD::IParticleContainer>> m_FsrObjectHandle{
        this,"HZyFsrObject", "HZyFsrObject_%SYS%", "Selected FSR object view container"};
    
    // Used for FSR correction tool
    CP::SysReadHandle<xAOD::ElectronContainer> m_electronsSTEP3Handle{
        this, "AnalysisElectrons_STEP3", "AnalysisElectrons_STEP3_%SYS%", ""};
    CP::SysReadHandle<xAOD::PhotonContainer> m_photonsSTEP4Handle{
        this, "AnalysisPhotons_STEP4", "AnalysisPhotons_STEP4_%SYS%", ""};

    float ZMassConstraint(ZMassConstraint::ConstraintFit &fitter,
                          const xAOD::EventInfo *event,
                          const LLCandidate &cand,
                          const FSR::FsrCandidate *fsrCand,
                          const TLorentzVector *fsrP4, TLorentzVector &c1_out,
                          TLorentzVector &c2_out);

    bool applyFSRCorrection(const xAOD::Muon *mu1, const xAOD::Muon *mu2,
                            const xAOD::PhotonContainer *allPhotons,
                            const xAOD::ElectronContainer *allElectrons,
                            TLorentzVector &fsrP4_out, int &fsrMuonIndex_out,
                            float &dR_fsr_out, FSR::FsrCandidate &fsrCand_out);

    Gaudi::Property<bool> m_useConstrainedZMass {
        this, "useConstrainedZMass", true, "Use constrained Z mass to select the best candidate (if false, use raw mass)." 
          "Constrained mass will be set to raw mass if fit fails."};
    Gaudi::Property<std::vector<double>> m_mllWindow{
        this, "mllWindowInfsr", {66.0, 89.0}, "Allowed mll window [GeV] for FSR correction"};
    Gaudi::Property<double> m_mllFsrThre{this, "mllFsrThreshold", 100.0, "Allowed FSR mll window [GeV]"};
    
    Gaudi::Property<std::vector<std::string>> m_floatVariables
    {this, "floatVariableList", {}, "Name list of floating variables"};
    
    Gaudi::Property<std::vector<std::string>> m_intVariables{
        this, "intVariableList", {}, "Name list of integer variables"};

    /// \brief Setup sys-aware output decorations
    Gaudi::Property<bool> m_doSystematics{this, "doSystematics", false,
                                          "Run on all systematics"};

    /// \brief Setup sys-aware output decorations
    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
  };
} // namespace HZY

#endif // HZYANALYSIS_LEPTONVARSHZYALG
