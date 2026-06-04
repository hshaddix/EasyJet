/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HZYANALYSIS_HZYSELECTORALG
#define HZYANALYSIS_HZYSELECTORALG

#include "EasyjetHub/CutManager.h"

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysFilterReporterParams.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>

// Load the relevant xAOD classes
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AthContainers/ConstDataVector.h>
#include <TriggerMatchingTool/IMatchingTool.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>

#include <string>
#include <unordered_map>
#include <vector>

class CutManager;

namespace HZY
{
  enum TriggerChannel
  {
    SLT,
    DLT,
  };

  enum Var
  {
    ele = 0,
    mu = 1,
    leadingele = 2,
    leadingmu = 3,
    subleadingele = 4,
    subleadingmu = 5,
    asyleadingmu = 6,
    asysubleadingmu = 7,
  };

  enum Booleans
  {
    pass_trigger_SLT,
    pass_trigger_DLT,
    pass_matching_trigger_SLT,
    pass_matching_trigger_DLT,
    PASS_TRIGGER,
    PASS_LEPTON_ID,
    PASS_LEPTON_ISO,
    SIZE_ZCANDIDATE,
    SIZE_Z_LOOSEPHOTON,
    PASS_TRIGGER_MATCHING,
    PHOTONS_JETS_OR_LEPTONS,
    DILEPTON_MASS_VETO_HMM,
    DILEPTON_MASS_CLOSEZ,
    PASS_RELPT_LOW,
    PHOTON_TIGHT_ID,
    PHOTON_ISOLATION,
    MASS_DILEPTON_PHOTON,
    // PASS_RELPT_HIGH
  };

  /// \brief An algorithm for counting containers
  class HZySelectorAlg final : public AthHistogramAlgorithm
  {

public:
    HZySelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);
    virtual ~HZySelectorAlg() = default;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute() override;
    /// \brief This is the mirror of initialize() and is called after all
    /// events are processed.
    StatusCode
    finalize() override; /// I added this to write the cutflow histogram.

private:
    void setThresholds(const xAOD::EventInfo *event,
                       const CP::SystematicSet &sys);
    void evaluateTriggerCuts(const xAOD::EventInfo *event,
                             const xAOD::ElectronContainer *electrons,
                             const xAOD::MuonContainer *muons, CutManager &HZyCuts,
                             const CP::SystematicSet &sys);
    void evaluateLeptonCuts(const xAOD::EventInfo *event,
                            const xAOD::ElectronContainer *electrons,
                            const xAOD::MuonContainer *muons,
                            const xAOD::PhotonContainer *photons, CutManager &HZyCuts,
                            const CP::SystematicSet &sys);
    void evaluateTriggerMatchingCuts(const xAOD::ElectronContainer *ZCandElectrons,
                                     const xAOD::MuonContainer *ZCandMuons,
                                     CutManager &HZyCuts);

    StatusCode initialiseCutflow();

    /// \brief Setup syst-aware input container handles
    CutManager m_HZyCuts;
    Gaudi::Property<std::vector<std::string>> m_inputCutList{
        this, "cutList", {}};
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};

    long long int m_total_events{0};
    double m_total_mcEventWeight{0.0};
    std::vector<HZY::Booleans> m_inputCutKeys;

    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::PhotonContainer> m_photonHandle{
        this, "HZyPhotons", "HZyAnalysisPhotons_%SYS%", "HZy Photons container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{
        this, "HZyElectrons", "HZyAnalysisElectrons_%SYS%", "HZy Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{
        this, "HZyMuons", "HZyAnalysisMuons_%SYS%", "HZy Muon container to read"};

    CP::SysReadDecorHandle<char> m_is17_periodB5_B8{this, "is2017_periodB5_B8", "is2017_periodB5_B8", ""};
    CP::SysReadDecorHandle<char> m_is22_75bunches{this, "is2022_75bunches", "is2022_75bunches", ""};
    CP::SysReadDecorHandle<char> m_is23_75bunches{this, "is2023_75bunches", "is2023_75bunches", ""};
    CP::SysReadDecorHandle<char> m_is23_400bunches{this, "is2023_400bunches", "is2023_400bunches", ""};

    CP::SysReadDecorHandle<unsigned int> m_year{
        this, "year", "dataTakingYear", ""};
    CP::SysReadDecorHandle<float> m_generatorWeight{
        this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights"};

    CP::SysFilterReporterParams m_filterParams{this, "HZy selection"};

    // Set the Gaudi properties that are transferred from the inputs
    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    Gaudi::Property<bool> m_bypass{this, "bypass", false, "Run the selector algorithm in run-through mode (DEBUG)"};
    Gaudi::Property<std::vector<std::string>> m_leptonTriggers{this, "triggerLists", {}, "Name list of triggers"};

    std::unordered_map<std::string, CP::SysReadDecorHandle<bool>> m_triggerdecos;
    std::unordered_map<HZY::TriggerChannel, std::unordered_map<HZY::Var, float>> m_pt_threshold;
    std::unordered_map<HZY::Booleans, CP::SysWriteDecorHandle<bool>> m_Bbranches;
    std::unordered_map<HZY::Booleans, bool> m_bools;
    std::unordered_map<HZY::Booleans, std::string> m_boolnames{
        {HZY::pass_trigger_SLT, "pass_trigger_SLT"},
        {HZY::pass_trigger_DLT, "pass_trigger_DLT"},
        {HZY::pass_matching_trigger_SLT, "pass_matching_trigger_SLT"},
        {HZY::pass_matching_trigger_DLT, "pass_matching_trigger_DLT"},
        {HZY::PASS_TRIGGER, "PASS_TRIGGER"},
        {HZY::PASS_LEPTON_ID, "PASS_LEPTON_ID"},
        {HZY::PASS_LEPTON_ISO, "PASS_LEPTON_ISO"},
        {HZY::SIZE_ZCANDIDATE, "SIZE_ZCANDIDATE"},
        {HZY::SIZE_Z_LOOSEPHOTON, "SIZE_Z_LOOSEPHOTON"},
        {HZY::PASS_TRIGGER_MATCHING, "PASS_TRIGGER_MATCHING"},
        {HZY::PHOTONS_JETS_OR_LEPTONS, "PHOTONS_JETS_OR_LEPTONS"},
        {HZY::DILEPTON_MASS_VETO_HMM, "DILEPTON_MASS_VETO_HMM"},
        {HZY::DILEPTON_MASS_CLOSEZ, "DILEPTON_MASS_CLOSEZ"},
        {HZY::PASS_RELPT_LOW, "PASS_RELPT_LOW"},
        {HZY::PHOTON_TIGHT_ID, "PHOTON_TIGHT_ID"},
        {HZY::PHOTON_ISOLATION, "PHOTON_ISOLATION"},
        {HZY::MASS_DILEPTON_PHOTON, "MASS_DILEPTON_PHOTON"},
        // {HZY::PASS_RELPT_HIGH, "PASS_RELPT_HIGH"},
    };

    CP::SysWriteDecorHandle<bool> m_passallcuts{"PassAllCuts_%SYS%", this};
    ToolHandle<Trig::IMatchingTool> m_matchingTool{
        this, "trigMatchingTool", "", "Trigger matching tool"};
  };
} // namespace HZY

#endif // HZYANALYSIS_HZYSELECTORALG
