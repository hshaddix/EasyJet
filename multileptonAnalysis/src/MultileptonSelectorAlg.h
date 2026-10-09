/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef MULTILEPTONANALYSIS_MULTILEPTONSELECTORALG
#define MULTILEPTONANALYSIS_MULTILEPTONSELECTORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysFilterReporterParams.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEgamma/ElectronContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODTau/TauJetContainer.h>

#include "TriggerMatchingTool/IMatchingTool.h"
#include <EasyjetHub/CutManager.h>

#include "SubChannelClassify.h"

namespace MULTILEPTON
{
    enum TriggerChannel
    {
        SLT,
        DLT,
        ASLT,
        LTT,
        MTT,
        ETT,
        ETT_4J12,
        MTT_2016,
        MTT_high,
        MTT_low,
        STT,
        DTT,
        DBT,
        DTT_2016,
        DTT_4J12,
        DTT_L1Topo,
        DTT_4J12_delayed,
        DTT_L1Topo_delayed,
        trigMatch_Tau35,
        trigMatch_Tau25,
        L1,
        HLT,
        SINGLE_LARGE_R_JETS,
        DI_LARGE_R_JETS,
        MET,
        RNNTauTrig18
    };

    enum Var
    {
        ele = 0,
        mu = 1,
        leadingele = 2,
        leadingmu = 3,
        subleadingele = 4,
        subleadingmu = 5,
        tau = 6,
        leadingtau = 7,
        subleadingtau = 8,
    };

    enum RunBooleans
    {
        is16PeriodA,
        is16PeriodB_D3,
        is16PeriodD4_end,
        is17PeriodB1_B4,
        is17PeriodB5_B7,
        is17PeriodB8_end,
        is18PeriodB_end,
        is18_tau35_medium1,
        is18PeriodK_end,
        is22_75bunches,
        is23_75bunches,
        is23_400bunches,
        is23_from1200bunches,
        is23_first_2400bunches,
        is24_timeframeA_C,
        is24_timeframeD,
        is24_timeframeE_F,
        l1topo_disabled
    };

    enum Booleans
    {
        pass_trigger_SLT,
        pass_trigger_DLT,
        pass_trigger_STT,
        pass_trigger_DTT,
        pass_trigger_LTT,
        PASS_TRIGGER,

        pass_2lsc,
        pass_2loc,
        pass_3l,
        pass_bb4l,
        pass_1l2tau,
        pass_2l2tau,
        pass_2lsc1tau,
        pass_1l3tau,
        pass_3l1tau,
    };

    /// \brief An algorithm for counting containers
    class MultileptonSelectorAlg final : public EL::AnaAlgorithm
    {

public:
    using EL::AnaAlgorithm::AnaAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// \brief This is the mirror of initialize() and is called after all
    /// events are processed.
    StatusCode
    finalize() override;


private:
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};

    Gaudi::Property<bool> m_bypass{
        this, "bypass", false, "Run selector algorithm in pass-through mode"};

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::JetContainer> m_jetHandle{
        this, "jets", "hhmlAnalysisJets_%SYS%", "Jet container to read"};

    CP::SysReadDecorHandle<char> m_isBtag{
        this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{
        this, "electrons", "hhmlAnalysisElectrons_%SYS%", "Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{
        this, "muons", "hhmlAnalysisMuons_%SYS%", "Muon container to read"};

    CP::SysReadHandle<xAOD::TauJetContainer> m_tauHandle{ 
        this, "taus", "hhmlAnalysisTaus_%SYS%", "Tau container to read" };


    CP::SysReadDecorHandle<unsigned int> m_year{
        this, "year", "dataTakingYear",""};

    CP::SysReadDecorHandle<char> m_is16_periodA{"is2016_periodA", this};
    CP::SysReadDecorHandle<char> m_is16_periodB_D3{"is2016_periodB_D3", this};
    CP::SysReadDecorHandle<char> m_is16_periodD4_end{"is2016_periodD4_end", this};
    CP::SysReadDecorHandle<char> m_is17_periodB1_B4{"is2017_periodB1_B4", this};
    CP::SysReadDecorHandle<char> m_is17_periodB5_B7{"is2017_periodB5_B7", this};
    CP::SysReadDecorHandle<char> m_is17_periodB8_end{"is2017_periodB8_end", this};
    CP::SysReadDecorHandle<char> m_is18_periodB_K{"is2018_periodB_K", this};
    CP::SysReadDecorHandle<char> m_is18_periodK_end{"is2018_periodK_end", this};
    CP::SysReadDecorHandle<char> m_is22_75bunches{"is2022_75bunches", this};
    CP::SysReadDecorHandle<char> m_is23_75bunches{"is2023_75bunches", this};
    CP::SysReadDecorHandle<char> m_is23_400bunches{"is2023_400bunches", this};
    CP::SysReadDecorHandle<char> m_is23_first_2400bunches{"is2023_first_2400bunches", this};
    CP::SysReadDecorHandle<char> m_is24_timeframeA_C{"is2024_timeframeA_C", this};
    CP::SysReadDecorHandle<char> m_is24_timeframeD{"is2024_timeframeD", this};
    CP::SysReadDecorHandle<char> m_is24_timeframeE_F{"is2024_timeframeE_F", this};
    CP::SysFilterReporterParams m_filterParams{this, "Multilepton selection"};

    Gaudi::Property<std::vector<std::string>> m_triggers 
      { this, "triggerLists", {}, "Name list of trigger" };
		
    ToolHandle<Trig::IMatchingTool> m_matchingTool
      { this, "trigMatchingTool", "", "Trigger matching tool"};

    std::unordered_map<std::string, CP::SysReadDecorHandle<bool> > m_triggerdecos;

    long long int m_total_events{0};
    
    std::unordered_map<MULTILEPTON::Booleans, CP::SysWriteDecorHandle<char> > m_Bbranches;
    std::unordered_map<MULTILEPTON::Booleans, bool> m_bools;
    std::unordered_map<MULTILEPTON::Booleans, std::string> m_boolnames{
        {MULTILEPTON::pass_trigger_SLT, "pass_trigger_SLT"},
        {MULTILEPTON::pass_trigger_DLT, "pass_trigger_DLT"},
        {MULTILEPTON::pass_trigger_STT, "pass_trigger_STT"},
        {MULTILEPTON::pass_trigger_LTT, "pass_trigger_LTT"},
        {MULTILEPTON::pass_trigger_DTT, "pass_trigger_DTT"},
        {MULTILEPTON::PASS_TRIGGER, "PASS_TRIGGER"},

        {MULTILEPTON::pass_2lsc, "pass_2lsc"},
        {MULTILEPTON::pass_2loc, "pass_2loc"},
        {MULTILEPTON::pass_3l, "pass_3l"},
        {MULTILEPTON::pass_bb4l, "pass_bb4l"},
        {MULTILEPTON::pass_1l2tau, "pass_1l2tau"},
        {MULTILEPTON::pass_2l2tau, "pass_2l2tau"},
        {MULTILEPTON::pass_2lsc1tau, "pass_2lsc1tau"},
        {MULTILEPTON::pass_1l3tau, "pass_1l3tau"},
        {MULTILEPTON::pass_3l1tau, "pass_3l1tau"},
    };

    /// \brief Cutflow Variables
    CutManager m_hhmlCuts;
    Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
    std::vector<MULTILEPTON::Booleans> m_inputCutKeys;
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
    double m_total_mcEventWeight{0.0};
    CP::SysReadDecorHandle<float>
		m_generatorWeight{ this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights" };

    std::unordered_map<MULTILEPTON::TriggerChannel, std::unordered_map<MULTILEPTON::Var, float>> m_pt_threshold;
    int m_cachedThresholdKey{-1};
    
    StatusCode initialiseCutflow();

    void evaluateTriggerCuts(
        const xAOD::EventInfo* event,
        const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer *muons,
        const xAOD::TauJetContainer* taus,
        CutManager& hhmlCuts, const CP::SystematicSet& sys);
    
    void evaluateSingleLeptonTrigger(
        const xAOD::EventInfo* event,
        const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer *muons,
        const CP::SystematicSet& sys);
    void evaluateDiLeptonTrigger(
        const xAOD::EventInfo* event,
        const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer *muons,
        const CP::SystematicSet& sys);
    void evaluateSingleTauTrigger(
        const xAOD::EventInfo* event,
        const xAOD::TauJetContainer* taus,
        const CP::SystematicSet& sys);
    void evaluateDiTauTrigger(
        const xAOD::EventInfo* event,
        const xAOD::TauJetContainer* taus,
        const CP::SystematicSet& sys);
    void evaluateLeptonTauTrigger(
        const xAOD::EventInfo* event,
        const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer *muons,
        const xAOD::TauJetContainer* taus,
        const CP::SystematicSet& sys);

    bool evaluate2lscSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluate2locSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluate3lSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluatebb4lSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);
    
    bool evaluate1l2tauSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);
    
    bool evaluate2l2tauSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluate2lsc1tauSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluate1l3tauSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);

    bool evaluate3l1tauSelection(const SubChannelClassify &classify,
        CutManager& hhmlCuts);


    void applyChannelSelection(
        const xAOD::ElectronContainer& electrons,
        const xAOD::MuonContainer& muons,
        const xAOD::TauJetContainer& taus,
        const ConstDataVector<xAOD::JetContainer>& bjets,
        CutManager& hhmlCuts);

    void setThresholds(
        const xAOD::EventInfo* event,
        const CP::SystematicSet& sys);
};

} // namespace MULTILEPTON

#endif // MULTILEPTONANALYSIS_MULTILEPTONSELECTORALG
