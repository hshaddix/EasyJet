/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef BBTTANALYSIS_HHBBTTSELECTORALG
#define BBTTANALYSIS_HHBBTTSELECTORALG

#include <memory>

#include "EasyjetHub/BaseSelectorAlg.h"
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/ReadDecorHandle.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODTau/TauJetContainer.h>
#include <xAODTau/DiTauJetContainer.h>

#include <EasyjetHub/CutManager.h>

#include "HHbbttEnums.h"

namespace HHBBTT
{

  /// \brief An algorithm for counting containers
  class HHbbttSelectorAlg final : public Easyjet::BaseSelectorAlg
  {
    /// \brief The standard constructor
public:
    using Easyjet::BaseSelectorAlg::BaseSelectorAlg;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// \brief Finalisation method, for cleanup, final print out etc
    StatusCode finalize() override;


private:

    /// \brief Steerable properties
    Gaudi::Property<std::vector<std::string>> m_channel_names
      { this, "channel", {}, "Which channel to run" };

    std::vector<HHBBTT::Channel> m_channels;

    Gaudi::Property<bool> m_useTriggerSel
      { this, "useTriggerSelections", true, "Apply trigger-related selections" };
    Gaudi::Property<bool> m_doAntiIDRegions
      { this, "doAntiIDRegions", false, "Select anti-ID taus for fake estimates" };
    Gaudi::Property<bool> m_do1BRegions
      { this, "do1BRegions", false, "Add 1B signal regions" };
    Gaudi::Property<bool> m_doLTT
      { this, "doLTT", false, "Add LTT signal regions" };
    Gaudi::Property<bool> m_useNonIsoLeptons
      { this, "useNonIsoLeptons", false, "use NonIso lepton wps" };

    Gaudi::Property<bool> m_bypass
      { this, "bypass", false, "Run selector algorithm in pass-through mode" };

    Gaudi::Property<bool> m_doLargeRJets
      { this, "doLargeRJets", false, "Read the Large-R jets, used for boosted channel" };
      

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle{ this, "jets", "bbttAnalysisJets_%SYS%", "Jet container to read" };
    
    CP::SysReadHandle<xAOD::JetContainer>
      m_lRjetHandle{ this, "largeRjets", "bbttAnalysisLargeRJets_%SYS%", "Large-R jet container to read" };
    
    CP::SysReadHandle<xAOD::DiTauJetContainer>
      m_ditauHandle{ this, "ditaus", "bbttAnalysisDitau_%SYS%", "Di-tau jet container to read" };

      
    CP::SysReadHandle<xAOD::ElectronContainer>
    m_electronHandle{ this, "electrons", "bbttAnalysisElectrons_%SYS%", "Electron container to read" };

    CP::SysReadHandle<xAOD::MuonContainer>
    m_muonHandle{ this, "muons", "bbttAnalysisMuons_%SYS%", "Muon container to read" };

    CP::SysReadHandle<xAOD::TauJetContainer>
    m_tauHandle{ this, "taus", "bbttAnalysisTaus_%SYS%", "Tau container to read" };

    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    Gaudi::Property<std::string> m_tauWPName
      { this, "tauWP", "", "Tau ID working point" };
    CP::SysReadDecorHandle<char> m_tauWPDecorHandle{"", this};
    CP::SysReadDecorHandle<char> m_antiTauDecorHandle{"isAntiTau", this};

    CP::SysReadDecorHandle<char> 
    m_isBtag {this, "bTagWPDecorName", "", "Name of input dectorator for b-tagging"};

    CP::SysReadDecorHandle<char> 
    m_Pass_Ditau{this, "ditau_WP", "", "Ditau-tagging working point"}; 
    CP::SysReadDecorHandle<int> 
    m_Pass_GN3XPV01_hbb_bjr{this, "GN3XPV01_hbb_bjr_WP", "", "GN3XPV01 b-tagging working point"}; 
    CP::SysReadDecorHandle<int> 
    m_Pass_GN3XPV01_htt{this, "GN3XPV01_htt_WP", "", "GN3XPV01 tau-tagging working point"}; 

    CP::SysReadDecorHandle<unsigned int> m_year
      {this, "year", "dataTakingYear", ""};

    CP::SysReadDecorHandle<char> m_is2016_periodA
      {this, "is2016_periodA", "is2016_periodA", ""};
    CP::SysReadDecorHandle<char> m_is2016_periodB_D3
      {this, "is2016_periodB_D3", "is2016_periodB_D3", ""};
    CP::SysReadDecorHandle<char> m_is2017_periodB1_B4
      {this, "is2017_periodB1_B4", "is2017_periodB1_B4", ""};
    CP::SysReadDecorHandle<char> m_l1topo_disabled
      {this, "l1TopoDisabled", "l1TopoDisabled", ""};
    CP::SysReadDecorHandle<char> m_is2022_75bunches
      {this, "is2022_75bunches", "is2022_75bunches", ""};
    CP::SysReadDecorHandle<char> m_is2023_first_2400bunches
      {this, "is2023_first_2400bunches", "is2023_first_2400bunches", ""};

    Gaudi::Property<std::vector<std::string>> m_eleWPNames
      { this, "eleWPs", {},"Electron working point names" };
    Gaudi::Property<std::vector<std::string>> m_muonWPNames
      { this, "muonWPs", {},"Muon working point names" };

    typedef std::unordered_map<HHBBTT::LepSelWpDeco, CP::SysReadDecorHandle<char>> leptonDecoMap;
    leptonDecoMap m_eleWPDecorHandleMap;   
    leptonDecoMap m_muonWPDecorHandleMap;

    CP::SysWriteDecorHandle<char> m_selected_el {"selected_el_%SYS%", this};
    CP::SysWriteDecorHandle<char> m_selected_el_isIso {"selected_el_isIso_%SYS%", this};

    CP::SysWriteDecorHandle<char> m_selected_mu {"selected_mu_%SYS%", this};
    CP::SysWriteDecorHandle<char> m_selected_mu_isIso {"selected_mu_isIso_%SYS%", this};

    CP::SysWriteDecorHandle<char> m_selected_tau {"selected_tau_%SYS%", this};

    CP::SysReadDecorHandle<char> bjet_trigMatchDeco{"passTrigMatch_DBT_%SYS%", this};

    std::unordered_map<HHBBTT::TriggerChannel, std::string> m_triggerChannels =
      {
        {HHBBTT::SLT, "SLT"},
        {HHBBTT::STT, "STT"},
        {HHBBTT::DTT, "DTT"},
        {HHBBTT::DTT_2016, "DTT_2016"},
        {HHBBTT::DTT_4J12, "DTT_4J12"},
        {HHBBTT::DTT_L1Topo, "DTT_L1Topo"},
        {HHBBTT::DTT_4J12_delayed, "DTT_4J12_delayed"},
        {HHBBTT::DTT_L1Topo_delayed, "DTT_L1Topo_delayed"},
        {HHBBTT::DBT, "DBT"},
        {HHBBTT::SINGLE_LARGE_R_JETS, "SINGLE_LARGE_R_JETS"},
        {HHBBTT::DI_LARGE_R_JETS, "DI_LARGE_R_JETS"},
        {HHBBTT::MET, "MET"},
        {HHBBTT::RNNTauTrig18, "uniqueRNNTauTrig18"},
      };

    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::EventInfo> > m_trigPass_DecorKey;

    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::MuonContainer> > m_mu_trigMatch_DecorKey;
    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::ElectronContainer> > m_ele_trigMatch_DecorKey;
    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::TauJetContainer> > m_tau_trigMatch_DecorKey;
    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::JetContainer> > m_jet_trigMatch_DecorKey;
    std::unordered_map<HHBBTT::TriggerChannel,
      SG::ReadDecorHandleKey<xAOD::JetContainer> > m_jet_trigMatch_ThresholdKey;

    /// \brief Setup sys-aware output decorations
    CP::SysFilterReporterParams m_filterParams {this, "HHbbtautau selection"};
    
    /// \brief Booleans
    /*
     * We have a lot of booleans. Most of these have to be saved in the output root file and some used for the cutflow algorithm.
     * In both cases, additionally to the value of the boolean, its name is required.
     * In order to easily access both and having both connected, we use the enum Booleans defined above.
     * We then define a map m_bools mapping the enum to the boolean value and a map m_boolnames mapping the enum to the boolean name.
     * Additionally, m_Bbranches is a map of SysWriteDecorHandles taking care of saving the variables.
     */
    std::unordered_map < HHBBTT::Booleans, CP::SysWriteDecorHandle<char> > m_Bbranches;
    std::unordered_map < HHBBTT::Booleans, bool > m_bools;
    std::unordered_map < HHBBTT::Booleans, std::string > m_boolnames{
    {HHBBTT::pass_trigger_SR, "pass_trigger_SR"},
    {HHBBTT::pass_trigger_SLT, "pass_trigger_SLT"},
    {HHBBTT::pass_trigger_STT, "pass_trigger_STT"},
    {HHBBTT::pass_trigger_DTT, "pass_trigger_DTT"},
    {HHBBTT::pass_trigger_DTT_2016, "pass_trigger_DTT_2016"},
    {HHBBTT::pass_trigger_DTT_4J12, "pass_trigger_DTT_4J12"},
    {HHBBTT::pass_trigger_DTT_L1Topo, "pass_trigger_DTT_L1Topo"},
    {HHBBTT::pass_trigger_DTT_4J12_delayed, "pass_trigger_DTT_4J12_delayed"},
    {HHBBTT::pass_trigger_DTT_L1Topo_delayed, "pass_trigger_DTT_L1Topo_delayed"},
    {HHBBTT::pass_trigger_DBT, "pass_trigger_DBT"},
    {HHBBTT::pass_trigger_SINGLE_LARGE_R_JETS, "pass_trigger_SINGLE_LARGE_R_JETS"},
    {HHBBTT::pass_trigger_DI_LARGE_R_JETS, "pass_trigger_DI_LARGE_R_JETS"},
    {HHBBTT::pass_trigger_MET, "pass_trigger_MET"},
    {HHBBTT::pass_trigger_uniqueRNNTauTrig18, "pass_trigger_uniqueRNNTauTrig18"},
    {HHBBTT::TWO_JETS, "TWO_JETS"},
    {HHBBTT::AT_LEAST_TWO_LRJETS, "AT_LEAST_TWO_LRJETS"},
    {HHBBTT::AT_LEAST_ONE_LRJET, "AT_LEAST_ONE_LRJET"},
    {HHBBTT::AT_LEAST_ONE_BTAG_LRJET, "AT_LEAST_ONE_BTAG_LRJET"},
    {HHBBTT::AT_LEAST_ONE_TAUTAG_LRJET, "AT_LEAST_ONE_TAUTAG_LRJET"},
    {HHBBTT::AT_LEAST_ONE_DITAUTAG, "AT_LEAST_ONE_DITAUTAG"},
    {HHBBTT::AT_LEAST_ONE_DITAU, "AT_LEAST_ONE_DITAU"},
    {HHBBTT::pass_Boosted_ditau, "pass_Boosted_ditau"},
    {HHBBTT::pass_Boosted_taujet, "pass_Boosted_taujet"},
    {HHBBTT::AT_LEAST_ONE_TIGHTID_LEP, "AT_LEAST_ONE_TIGHTID_LEP"},
    {HHBBTT::AT_LEAST_ONE_LOOSEID_LEP, "AT_LEAST_ONE_LOOSEID_LEP"},
    {HHBBTT::TWO_BJETS, "TWO_BJETS"},
    {HHBBTT::ONE_BJET, "ONE_BJET"},
    {HHBBTT::MTAUTAU_VIS_MASS_CUT_LEPHAD, "MTAUTAU_VIS_MASS_CUT_LEPHAD"},
    {HHBBTT::N_LEPTONS_CUT_LEPHAD, "N_LEPTONS_CUT_LEPHAD"},
    {HHBBTT::N_LEPTONS_CUT_ANTIISOLEPHAD, "N_LEPTONS_CUT_ANTIISOLEPHAD"},
    {HHBBTT::ONE_TAU, "ONE_TAU"},
    {HHBBTT::OS_CHARGE_LEPHAD, "OS_CHARGE_LEPHAD"},
    {HHBBTT::OS_CHARGE_LEPTONS, "OS_CHARGE_LEPTONS"},
    {HHBBTT::pass_baseline_SLT, "pass_baseline_SLT"},
    {HHBBTT::pass_SLT_2B, "pass_SLT_2B"},
    {HHBBTT::MTAUTAU_VIS_MASS_CUT_HADHAD, "MTAUTAU_VIS_MASS_CUT_HADHAD"},
    {HHBBTT::N_LEPTONS_CUT_HADHAD, "N_LEPTONS_CUT_HADHAD"},
    {HHBBTT::TWO_TAU, "TWO_TAU"},
    {HHBBTT::OS_CHARGE_HADHAD, "OS_CHARGE_HADHAD"},
    {HHBBTT::pass_baseline_STT, "pass_baseline_STT"},
    {HHBBTT::pass_baseline_DTT_2016, "pass_baseline_DTT_2016"},
    {HHBBTT::pass_baseline_DTT_4J12, "pass_baseline_DTT_4J12"},
    {HHBBTT::pass_baseline_DTT_L1Topo, "pass_baseline_DTT_L1Topo"},
    {HHBBTT::pass_baseline_DTT, "pass_baseline_DTT"},
    {HHBBTT::pass_baseline_DBT, "pass_baseline_DBT"},
    {HHBBTT::pass_baseline_SR, "pass_baseline_SR"},
    {HHBBTT::pass_STT_2B, "pass_STT_2B"},
    {HHBBTT::pass_DTT_2016_2B, "pass_DTT_2016_2B"},
    {HHBBTT::pass_DTT_4J12_2B, "pass_DTT_4J12_2B"},
    {HHBBTT::pass_DTT_L1Topo_2B, "pass_DTT_L1Topo_2B"},
    {HHBBTT::pass_DTT_4J12_delayed_2B, "pass_DTT_4J12_delayed_2B"},
    {HHBBTT::pass_DTT_L1Topo_delayed_2B, "pass_DTT_L1Topo_delayed_2B"},
    {HHBBTT::pass_DTT_2B, "pass_DTT_2B"},
    {HHBBTT::pass_DBT_2B, "pass_DBT_2B"},
    {HHBBTT::pass_SR_2B, "pass_SR_2B"},
    {HHBBTT::pass_baseline_LepHad, "pass_baseline_LepHad"},
    {HHBBTT::pass_baseline_HadHad, "pass_baseline_HadHad"},
    {HHBBTT::pass_LepHad_2B, "pass_LepHad_2B"},
    {HHBBTT::pass_HadHad_2B, "pass_HadHad_2B"},
    {HHBBTT::pass_LepHad, "pass_LepHad"},
    {HHBBTT::pass_HadHad, "pass_HadHad"},
    {HHBBTT::pass_ZCR, "pass_ZCR"},
    {HHBBTT::pass_TopEMuCR, "pass_TopEMuCR"},
    {HHBBTT::pass_AntiIsoLepHad, "pass_AntiIsoLepHad"},
    {HHBBTT::pass_Boosted, "pass_Boosted"},
    {HHBBTT::pass_BoostedZCR, "pass_BoostedZCR"},
    };
    std::unordered_map<std::string, HHBBTT::Booleans> m_nameToBool;

    /// \brief Cutflow Variables
    std::vector<HHBBTT::Booleans> m_inputCutKeys;

    CP::SysReadDecorHandle<float>
      m_generatorWeight{ this, "generatorWeight", "generatorWeight_%SYS%", "MC event weights" };

    /// \brief Internal variables

    std::unordered_map<HHBBTT::TriggerChannel, std::unordered_map<HHBBTT::Var, float>> m_pt_threshold;
    std::unordered_map<HHBBTT::TriggerChannel, std::unordered_map<HHBBTT::Var, int>> m_trigger_pt_threshold;

    typedef std::unordered_map<HHBBTT::TriggerChannel, SG::ReadDecorHandle<xAOD::EventInfo, char> > trigPassReadDecoMap;
    typedef std::unordered_map<HHBBTT::TriggerChannel, SG::ReadDecorHandle<xAOD::MuonContainer, char> > muTrigMatchReadDecoMap;
    typedef std::unordered_map<HHBBTT::TriggerChannel, SG::ReadDecorHandle<xAOD::ElectronContainer, char> > eleTrigMatchReadDecoMap;
    typedef std::unordered_map<HHBBTT::TriggerChannel, SG::ReadDecorHandle<xAOD::TauJetContainer, char> > tauTrigMatchReadDecoMap;
    typedef CP::SysReadDecorHandle<char> bjetTrigMatchReadDeco;

    void applyTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::Electron* ele, const eleTrigMatchReadDecoMap& ele_trigMatchDecos,
       const xAOD::Muon* mu, const muTrigMatchReadDecoMap& mu_trigMatchDecos,
       const xAOD::TauJet* tau0, const xAOD::TauJet* tau1,
       const tauTrigMatchReadDecoMap& tau_trigMatchDecos,
       const xAOD::Jet* jet0, const xAOD::Jet* jet1,
       const xAOD::Jet* eta_lt2p5_jet0, const xAOD::Jet* eta_lt2p5_jet1,
       const xAOD::Jet* eta_lt3p2_jet0, const xAOD::Jet* eta_lt2p5_bjet0,
       const xAOD::Jet* eta_lt2p5_bjet1);

    void applySingleLepTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::Electron* ele, const eleTrigMatchReadDecoMap& ele_trigMatchDecos,
       const xAOD::Muon* mu, const muTrigMatchReadDecoMap& mu_trigMatchDecos);
    void applyLepHadTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::Electron* ele, const eleTrigMatchReadDecoMap& ele_trigMatchDecos,
       const xAOD::Muon* mu, const muTrigMatchReadDecoMap& mu_trigMatchDecos,
       const xAOD::TauJet* tau, const tauTrigMatchReadDecoMap& tau_trigMatchDecos,
       const xAOD::Jet* jet0, const xAOD::Jet* jet1);
    void applySingleTauTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::TauJet* tau, const tauTrigMatchReadDecoMap& tau_trigMatchDecos);
    void applyDiTauTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::TauJet* tau0, const xAOD::TauJet* tau1,
       const tauTrigMatchReadDecoMap& tau_trigMatchDecos,
       const xAOD::Jet* jet0, const xAOD::Jet* eta_lt2p5_jet0, 
       const xAOD::Jet* eta_lt2p5_jet1, const xAOD::Jet* eta_lt3p2_jet0);
    void applyDiBJetTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos,
       const xAOD::TauJet* tau0, const xAOD::TauJet* tau1,
       const xAOD::Jet* eta_lt2p5_bjet0, const xAOD::Jet* eta_lt2p5_bjet1);
    void applyLargeRJetsTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos);
    void applyMETTriggerSelection
      (const xAOD::EventInfo* event, const trigPassReadDecoMap& triggerdecos);

    void setThresholds(const xAOD::EventInfo* event,
		       const CP::SystematicSet& sys);
    
    void fillLeptonWpDecoMap(const std::vector<std::string>& wpNames, leptonDecoMap& decoMap);
  };
}

#endif
