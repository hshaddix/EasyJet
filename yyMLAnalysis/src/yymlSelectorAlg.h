/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef YYMLANALYSIS_YYMLSELECTORALG
#define YYMLANALYSIS_YYMLSELECTORALG

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysFilterReporterParams.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <AsgDataHandles/ReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODJet/JetContainer.h>
#include <xAODTau/TauJetContainer.h>

#include "TriggerMatchingTool/IMatchingTool.h"
#include <EasyjetHub/CutManager.h>

#include "SubChannelClassify.h"

namespace HHYYML
{

    enum Booleans
    {
        pass_trigger_diphoton,
        pass_matching_trigger_diphoton,
        PASS_TRIGGER,

        pass_any_subchannel,
        pass_1l0tau,
        pass_0l1tau,
        pass_2l0tau,
        pass_1l1tau,
        pass_0l2tau,
        pass_0l0tau,
    };

    /// \brief An algorithm for counting containers
    class yymlSelectorAlg final : public EL::AnaAlgorithm
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

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    // TODO: are these rather "yyML" in the name?
    CP::SysReadHandle<xAOD::PhotonContainer> m_photonHandle{
        this, "photons", "yymlAnalysisPhotons_%SYS%", "Photons container to read" };

    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{
        this, "electrons", "yymlAnalysisElectrons_%SYS%", "Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{
        this, "muons", "yymlAnalysisMuons_%SYS%", "Muon container to read"};

    CP::SysReadHandle<xAOD::JetContainer> m_jetHandle{
        this, "jets", "yymlAnalysisJets_%SYS%", "Jet container to read"};

    CP::SysReadHandle<xAOD::TauJetContainer> m_tauHandle{
        this, "taus", "yymlAnalysisTaus_%SYS%", "Tau container to read" };

    CP::SysReadDecorHandle<char> m_isBtag{
        this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

    CP::SysReadDecorHandle<unsigned int> m_year{
        this, "year", "dataTakingYear",""};

    CP::SysFilterReporterParams m_filterParams{
        this, "yyML selection"};

    Gaudi::Property<std::vector<std::string>> m_photonTriggers{this, "photonTriggers", {}, "Name list of photon trigger" };

    std::unordered_map<std::string,  SG::ReadDecorHandleKey<xAOD::EventInfo>> m_triggerDecorKeys;

    ToolHandle<Trig::IMatchingTool> m_matchingTool{
        this, "trigMatchingTool", "", "Trigger matching tool"};

    long long int m_total_events{0};

    std::unordered_map<HHYYML::Booleans, CP::SysWriteDecorHandle<char> > m_Bbranches;
    std::unordered_map < HHYYML::Booleans, bool > m_bools;
    std::unordered_map < HHYYML::Booleans, std::string > m_boolnames{
        {HHYYML::pass_trigger_diphoton, "pass_trigger_diphoton"},
        {HHYYML::pass_matching_trigger_diphoton, "pass_matching_trigger_diphoton"},
        {HHYYML::PASS_TRIGGER, "PASS_TRIGGER"},

        {HHYYML::pass_any_subchannel, "pass_any_subchannel"},
        {HHYYML::pass_1l0tau, "pass_1l0tau"},
        {HHYYML::pass_0l1tau, "pass_0l1tau"},
        {HHYYML::pass_2l0tau, "pass_2l0tau"},
        {HHYYML::pass_1l1tau, "pass_1l1tau"},
        {HHYYML::pass_0l2tau, "pass_0l2tau"},
        {HHYYML::pass_0l0tau, "pass_0l0tau"},
    };

    /// \brief Cutflow Variables
    CutManager m_yymlCuts;
    Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "cutList", {}};
    std::vector<HHYYML::Booleans> m_inputCutKeys;
    Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
    double m_total_mcEventWeight{0.0};
    CP::SysReadDecorHandle<float> m_generatorWeight{"generatorWeight_%SYS%", this};

    StatusCode initialiseCutflow();

    void evaluateTriggerCuts(const xAOD::EventInfo& eventInfo,
        const std::vector<std::string> &photonTriggers, const EventContext& ctx);

    void evaluateTriggerMatchingCuts(const std::vector<std::string> &photonTriggers,
        const xAOD::PhotonContainer* photons);


    bool evaluate1l0tauSelection(const SubChannelClassify &classify);

    bool evaluate0l1tauSelection(const SubChannelClassify &classify);

    bool evaluate2l0tauSelection(const SubChannelClassify &classify);

    bool evaluate1l1tauSelection(const SubChannelClassify &classify);

    bool evaluate0l2tauSelection(const SubChannelClassify &classify);
    
    bool evaluate0l0tauSelection(const SubChannelClassify &classify);

    void applyChannelSelection(
        const xAOD::ElectronContainer& electrons,
        const xAOD::MuonContainer& muons,
        const xAOD::TauJetContainer& taus);

    // TODO: used together with m_pt_threshold above for Multilepton and bbll, probably not needed for diphoton triggers
    // void setThresholds(
    //     const xAOD::EventInfo* event,
    //     const CP::SystematicSet& sys);

};

} // namespace HHYYML

#endif // YYMLANALYSIS_YYMLSELECTORALG
