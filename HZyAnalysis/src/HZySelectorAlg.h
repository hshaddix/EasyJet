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

// Load the relevant xAOD classes
#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODJet/JetContainer.h>

#include <string>
#include <unordered_map>
#include <vector>

class CutManager;

namespace HZY
{
  enum Booleans : uint8_t
  {
    pass_trigger_SLT,
    pass_trigger_DLT,
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
    void evaluateTriggerCuts(
        const xAOD::EventInfo *event,
        const xAOD::ElectronContainer *electrons,
        const xAOD::MuonContainer *muons, CutManager &HZyCuts,
        const CP::SystematicSet &sys);
    void evaluateEventSelections(
        const xAOD::EventInfo *event, 
        CutManager &HZyCuts, const CP::SystematicSet &sys);
        
    StatusCode initialiseCutflow();

    /// \brief Setup syst-aware input container handles
    CutManager m_HZyCuts;

    long long int m_total_events{0};
    double m_total_mcEventWeight{0.0};
    std::vector<HZY::Booleans> m_inputCutKeys;

    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::PhotonContainer> m_HZyPhotonHandle{
        this, "HZyPhotons", "HZyAnalysisPhotons_%SYS%", "HZy Photons container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronHandle{
        this, "HZyElectrons", "HZyAnalysisElectrons_%SYS%", "HZy Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonHandle{
        this, "HZyMuons", "HZyAnalysisMuons_%SYS%", "HZy Muon container to read"};

    CP::SysReadDecorHandle<float> m_generatorWeight{
        this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights"};
    CP::SysReadDecorHandle<char> m_globalTriggerMatch{
        this, "globalTriggerMatch", "globalTriggerMatch_%SYS%", "Global trigger matching"};

    CP::SysFilterReporterParams m_filterParams{this, "HZy selection"};
    CP::SysWriteDecorHandle<bool> m_passallcuts{"Pass_allcuts_%SYS%", this};

    // Set the Gaudi properties that are transferred from the inputs
    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};
    Gaudi::Property<bool> m_bypass{this, "bypass", false, "Run the selector algorithm in run-through mode (DEBUG)"};
    Gaudi::Property<std::vector<std::string>> m_leptonTriggers{this, "triggerLists", {}, "Name list of triggers"};
    Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};

    Gaudi::Property<double> m_mllRawMax_HmumuVeto{this, "max_mll_veto_hmumu", 101.0, "Upper raw mll threshold [GeV] for the Hmumu veto"};
    Gaudi::Property<std::vector<double>> m_mllCstrWindow{this, "mll_cstr_window", {81.18, 101.18}, "Allowed constrained mll window [GeV]"};
    Gaudi::Property<std::vector<double>> m_mllgCstrWindow{this, "mllg_cstr_window", {115.0, 170.0}, "Allowed constrained mllgamma window [GeV]"};
    Gaudi::Property<double> m_relPtMin{this, "photon_relpt_min", 0.09, "Minimum photon relative pT, pTgamma / mllgamma"};

    /// \brief Setup sys-aware read decorations leptons and photons
    Gaudi::Property<std::vector<std::string>> m_floatVariables_leptons{this, "floatVariableList_leptons", {}, "Name list of float variables for Z candidate"};
    Gaudi::Property<std::vector<std::string>> m_intVariables_leptons{this, "intVariableList_leptons", {}, "Name list of int variables for Z candidate"};
    Gaudi::Property<std::vector<std::string>> m_intVariables_photons{this, "intVariableList_photons", {}, "Name list of int variables for photons OR"};
    Gaudi::Property<std::vector<std::string>> m_floatVariables_baseline{this, "floatVariableList_baseline", {}, "Name list of float variables for Zy system"};
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_Fbranches_leptons;
    std::unordered_map<std::string, CP::SysReadDecorHandle<int>> m_Ibranches_leptons;
    std::unordered_map<std::string, CP::SysReadDecorHandle<int>> m_Ibranches_photons;
    std::unordered_map<std::string, CP::SysReadDecorHandle<float>> m_Fbranches_baseline;

    std::unordered_map<std::string, CP::SysReadDecorHandle<bool>> m_triggerdecos;
    std::unordered_map<HZY::Booleans, CP::SysWriteDecorHandle<bool>> m_Bbranches;
    std::unordered_map<HZY::Booleans, bool> m_bools;
    std::unordered_map<HZY::Booleans, std::string> m_boolnames{
        {HZY::pass_trigger_SLT, "pass_trigger_SLT"},
        {HZY::pass_trigger_DLT, "pass_trigger_DLT"},
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

  };
} // namespace HZY

#endif // HZYANALYSIS_HZYSELECTORALG
