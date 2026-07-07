/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_BASELINEVARSHZYALG
#define HZYANALYSIS_BASELINEVARSHZYALG

#include <AthContainers/ConstDataVector.h>
#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODJet/JetContainer.h>
#include <xAODBase/IParticleContainer.h>

#include <onnxruntime_cxx_api.h>

namespace HZY
{
  /// Model family identifier
  enum class ModelFamily : uint8_t
  {
    VBF = 0,
    ggF_ee,
    ggF_mm,
    RelpT_ee,
    RelpT_mm
  };

  /// Per-fold ONNX session
  struct OnnxFold
  {
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::vector<const char *> inputNames;
    std::vector<const char *> outputNames;
    std::vector<int64_t> inputDims;
  };

  // Read MVA input features. Order matters — must match training order.
  // getVBFScore feature list (26 features):
  const std::vector<std::string> vbfFeatures = {
      "VBF_DRmin_y_j",    "VBF_Dphi_Zy_jj_Fullrange",
      "VBF_Dy_j_j",       "VBF_N_j",
      "VBF_Zepp",         "VBF_eta_j1",
      "VBF_eta_j2",       "VBF_m_jj",
      "VBF_mass_j1",      "VBF_mass_j2",
      "VBF_pT_j1",        "VBF_pT_j2",
      "VBF_pT_jj",        "VBF_pTt_Zy",
      "Zy_Dphi_j1",       "ll_eta",
      "ll_pt",            "ph_eta",
      "ph_pt",            "llg_angles_costheta_ginH",
      "llg_angles_costheta_linZ", "llg_deta_Zy",
      "llg_dphi_Zy",      "llg_eta",
      "llg_pt",
  };

  // getGGFScore / getRelpTScore feature list (17 features):
  const std::vector<std::string> ggFRelpTFeatures = {
      "VBF_DRmin_y_j",        "VBF_N_j",
      "VBF_eta_j1",           "VBF_pT_j1",
      "VBF_phi_j1",           "Zy_Dphi_j1",
      "ll_eta",               "ll_pt_rel",
      "ph_eta",               "ph_pt_rel",
      "llg_angles_costheta_ginH", "llg_angles_costheta_linZ",
      "llg_deta_Zy",          "llg_dphi_Zy",
      "llg_eta",              "llg_pTt",
      "llg_pt_rel",
  };

  /// \brief An algorithm for counting containers
  class BaselineVarsHZyAlg final : public AthHistogramAlgorithm
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

    /// \brief Collect the features
    using FeatureMap = std::unordered_map<std::string, float>;
    void setFloatAndFeature(
        const xAOD::EventInfo* event,
        const std::string& name,
        float value,
        const CP::SystematicSet& sys,
        FeatureMap& features
    );
    /// \brief CMS angle at HZy rest frame and Thrust as defined in HZgammaHelper.cxx thrust_coord()
    void getCMSAngleVars(const xAOD::EventInfo *event,
                         TLorentzVector &lepLeadP4,
                         TLorentzVector &lepSubP4,
                         const TLorentzVector &photonP4,
                         const CP::SystematicSet &sys,
                         FeatureMap& features);
    void getVBFTopologyVars(const xAOD::EventInfo *event,
                            const xAOD::JetContainer *HZyJetOR,
                            const TLorentzVector &llRawP4,
                            const TLorentzVector &photonP4,
                            const TLorentzVector &llgRawP4,
                            const CP::SystematicSet &sys,
                            FeatureMap& features);
    std::vector<double> thrustCoord(const TLorentzVector &dilepton,
                                    const TLorentzVector &photon);

    /// \brief Event categorization
    void loadModel(ModelFamily family, const std::string &fileName, int nFeatures);
    float getScore(ModelFamily family, const std::vector<float> &inputs, int eventNumber) const;
    int computeCategory(int channel, int nLepIdIso_ORPh, 
        float vbfScore, float ggFScore, float relpTScore, float vbf_nj, float ph_pt_rel) const;

private:
    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonHandle{
        this, "HZyPhotons", "HZyAnalysisPhotons_%SYS%", "HZy Photon container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronHandle{
        this, "HZyElectrons", "HZyAnalysisElectrons_%SYS%", "HZy Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonHandle{
        this, "HZyMuons", "HZyAnalysisMuons_%SYS%", "HZy Muon container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_HZyJetORHandle{
        this, "HZyJetsOR", "HZyJetsOR_%SYS%", "HZy Jet container with photon and leptons overlap-removal"};

    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonORHandle{
        this, "HZyPhotonsOR", "HZyPhotonsOR_%SYS%", "HZy Photon container with LooseIDnonISO after photons and jets OR"};

    CP::SysReadHandle<xAOD::IParticleContainer> m_FsrObjectHandle{
        this, "HZyFsrObject", "HZyFsrObject_%SYS%", "Selected FSR object view container"};
    
    /// \brief Setup sys-aware output decorations
    Gaudi::Property<std::vector<std::string>> m_floatVariables_leptons{this, "floatVariableList_leptons", {}, "Name list of float variables for Z candidate"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_Fbranches_leptons;
    Gaudi::Property<std::vector<std::string>> m_intVariables_leptons{this, "intVariableList_leptons", {}, "Name list of int variables for Z candidate"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<int>> m_Ibranches_leptons;
    Gaudi::Property<std::vector<std::string>> m_floatVariables_photons{this, "floatVariableList_photons", {}, "Name list of float variables for signal photon"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_Fbranches_photons;
    Gaudi::Property<std::vector<std::string>> m_intVariables_photons{this, "intVariableList_photons", {}, "Name list of int variables for signal photon"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<int>> m_Ibranches_photons;

    /// 5 families x 4 folds = 20 sessions
    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    Gaudi::Property<std::string> m_modelFolder{
        this, "ModelFolder", "HZyAnalysis/model_run3", "Path prefix for ONNX models"};
    std::map<ModelFamily, std::vector<OnnxFold>> m_folds;
    
    Gaudi::Property<std::vector<std::string>> m_floatVariables{
        this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables{
        this, "intVariableList", {}, "Name list of integer variables"};

    /// \brief Setup sys-aware output decorations
    Gaudi::Property<bool> m_doSystematics{this, "doSystematics", false, "Run on all systematics"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;
  };
} // namespace HZY

#endif // HZYANALYSIS_BASELINEVARSHZYALG
