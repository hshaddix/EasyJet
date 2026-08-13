/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/
#include "HZySelectorAlg.h"
#include <AthenaKernel/Units.h>

#include <AthenaBaseComps/AthMsgStreamMacros.h>
#include <GaudiKernel/ISvcLocator.h>
#include <SystematicsHandles/SysFilterReporter.h>
#include <SystematicsHandles/SysFilterReporterCombiner.h>

namespace HZY
{
  StatusCode HZySelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("          HZySelectorAlg         \n");
    ATH_MSG_INFO("*********************************\n");

    // Initialise global event filter
    ATH_CHECK(m_filterParams.initialize(m_systematicsList));

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_generatorWeight.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_globalTriggerMatch.initialize(m_systematicsList, m_eventHandle));

    for (auto &[key, value] : m_boolnames)
    {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<char> whandle{value + "_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise trigger decoration handles
    for (auto trig : m_leptonTriggers)
    {
      std::string trigName = "trigPassed_" + trig;
      CP::SysReadDecorHandle<bool> deco{this, trigName, trigName, "Name of triggers"};
      m_triggerdecos.emplace(trigName, deco);
      ATH_CHECK(m_triggerdecos.at(trigName).initialize(m_systematicsList, m_eventHandle));
    }

    // Setup read decorations for Z candidate dilepton variables (float and int)
    for (const std::string &string_var : m_floatVariables_leptons)
    {
      CP::SysReadDecorHandle<float> var{string_var + "_%SYS%", this};
      m_Fbranches_leptons.emplace(string_var, var);
      ATH_CHECK(m_Fbranches_leptons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &string_var : m_intVariables_leptons)
    {
      CP::SysReadDecorHandle<int> var{string_var + "_%SYS%", this};
      m_Ibranches_leptons.emplace(string_var, var);
      ATH_CHECK(m_Ibranches_leptons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &string_var : m_intVariables_photons)
    {
      CP::SysReadDecorHandle<int> var{string_var + "_%SYS%", this};
      m_Ibranches_photons.emplace(string_var, var);
      ATH_CHECK(m_Ibranches_photons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &string_var : m_floatVariables_baseline)
    {
      CP::SysReadDecorHandle<float> var{string_var + "_%SYS%", this};
      m_Fbranches_baseline.emplace(string_var, var);
      ATH_CHECK(m_Fbranches_baseline.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // special flag for all cuts
    ATH_CHECK(m_passallcuts.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    if(m_saveCutFlow){
      std::vector<std::string> boolNameList;
      for (const auto& [key, value]: m_boolnames){
        boolNameList.push_back(value);
        m_nameToBool[value] = key;
      }
      ATH_CHECK (initialiseCutflow(boolNameList));
    }

    return StatusCode::SUCCESS;
  }

  StatusCode HZySelectorAlg::execute(const EventContext& ctx)
  {
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner(m_filterParams, false);

    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter(filterCombiner, sys);

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ATH_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ATH_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys, ctx));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ATH_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys, ctx));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ATH_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys, ctx));

      // Reset all cut flags to default=false
      for (const auto &[key, value] : m_bools)
      {
        m_bools.at(key) = false;
      }

      // Evaluate trigger passing cuts
      evaluateTriggerCuts(event, HZyElectrons, HZyMuons, sys);

      // Dilepton ID/Iso and dilepton+loose photon selections
      // Check the trigger matching cuts after selected leptons for Z candidate
      // Hmumu and Zgamma selections
      // Photon ID and Isolation selections
      evaluateEventSelections(event, sys);

      bool passedall = !m_bools.empty();
      for (const auto& [key, value]: m_boolnames){
        passedall = passedall && m_bools.at(key);
      }
      m_passallcuts.set(*event, passedall, sys);

      // do the CUTFLOW only with sys="" -> NOSYS
      if (sys.name()=="" && m_saveCutFlow){
        std::vector<bool> cutBools;
        for(const auto& cut : m_inputCutList)
          cutBools.push_back(m_bools.at(m_nameToBool.at(cut)));
        double mc_weight = m_isMC ? m_generatorWeight.get(*event, sys) : 0.;
        ATH_CHECK (applyCutFlow(cutBools, mc_weight));
      }

      for (auto &[key, var] : m_bools)
      {
        m_Bbranches.at(key).set(*event, var, sys);
      }

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      if (!m_bypass && !passedall) continue;
      filter.setPassed(true);
    }

    return StatusCode::SUCCESS;
  }

  StatusCode HZySelectorAlg::finalize()
  {
    ATH_CHECK(m_filterParams.finalize());
    ATH_CHECK (finaliseCutflow());
    return StatusCode::SUCCESS;
  }

  void HZySelectorAlg::evaluateTriggerCuts(
    const xAOD::EventInfo* event,
    const xAOD::ElectronContainer* electrons,
    const xAOD::MuonContainer* muons,
    const CP::SystematicSet& sys)
  {
    bool passSLT = false;
    bool passDLT = false;

    for (const auto& trig : m_leptonTriggers)
    {
      const bool pass = m_triggerdecos.at("trigPassed_" + trig).get(*event, sys);
      if (!pass)
        continue;

      if (electrons) {
        if (trig.starts_with("HLT_2e"))
          passDLT = true;
        else if (trig.starts_with("HLT_e"))
          passSLT = true;
      }

      if (muons) {
        if (trig.starts_with("HLT_2mu") || trig == "HLT_mu22_mu8noL1_L1MU14FCH")
          passDLT = true;
        else if (trig.starts_with("HLT_mu"))
          passSLT = true;
      }
    }

    m_bools.at(HZY::pass_trigger_SLT) |= passSLT;
    m_bools.at(HZY::pass_trigger_DLT) |= passDLT;

    if (passSLT || passDLT) {
      m_bools.at(HZY::PASS_TRIGGER) = true;
    }
  }

  void HZySelectorAlg::evaluateEventSelections(
      const xAOD::EventInfo* event,
      const CP::SystematicSet& sys)
  {
    const bool passLeptonID =
        m_Ibranches_leptons.at("nElectrons_LIdNIso").get(*event, sys) >= 2 ||
        m_Ibranches_leptons.at("nMuons_MIdNIso").get(*event, sys) >= 2;
    const bool passLeptonISO =
        m_Ibranches_leptons.at("nElectrons_LIdLIso").get(*event, sys) >= 2 ||
        m_Ibranches_leptons.at("nMuons_MIdLIso").get(*event, sys) >= 2;

    if (passLeptonID) {
      m_bools.at(HZY::PASS_LEPTON_ID) = true;
    }

    if (passLeptonISO) {
      m_bools.at(HZY::PASS_LEPTON_ISO) = true;
    }

    // Z-candidate size
    const int nZCandidates = m_Ibranches_leptons.at("LepLepZ_Size").get(*event, sys);
    if (nZCandidates > 0) {
      m_bools.at(HZY::SIZE_ZCANDIDATE) = true;
    }

    // Z + loose photon size
    if (passLeptonISO && m_Ibranches_photons.at("nPhotons_LIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::SIZE_Z_LOOSEPHOTON) = true;
    }

    // Trigger matching using the orginal leptons from selected Z candidate
    if (m_globalTriggerMatch.get(*event, sys)) {
      m_bools.at(HZY::PASS_TRIGGER_MATCHING) = true;
    }

    // Photon OR lepton or jet selections on the selected Zgamma candidate photon
    if (m_Ibranches_photons.at("nPhotonsOR_LIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTONS_JETS_OR_LEPTONS) = true;
    }

    const float mllRaw = m_Fbranches_leptons.at("LepLepZ_M_Raw").get(*event, sys);
    const float mllCstr = m_Fbranches_leptons.at("LepLepZ_M_Cstr").get(*event, sys);
    const int channel = m_Ibranches_leptons.at("Lep_Channel").get(*event, sys);

    // Hmumu veto
    if (!(channel == 2 && mllRaw > m_mllRawMax_HmumuVeto * Athena::Units::GeV)) {
      m_bools.at(HZY::DILEPTON_MASS_VETO_HMM) = true;
    }

    // Constrained dilepton mass window
    const auto &mllWindow = m_mllCstrWindow.value();
    if (mllCstr >= mllWindow.at(0) * Athena::Units::GeV && mllCstr <= mllWindow.at(1) * Athena::Units::GeV) {
      m_bools.at(HZY::DILEPTON_MASS_CLOSEZ) = true;
    }

    // Relative photon pT
    const float photon_relPt = m_Fbranches_baseline.at("LLGam_RelPhpT_Cstr").get(*event, sys);
    if (photon_relPt > m_relPtMin)
    {
      m_bools.at(HZY::PASS_RELPT_LOW) = true;
    }

    // Photon ID and Isolation selections on the selected Zgamma candidate photon
    if (m_Ibranches_photons.at("nPhotonsOR_TIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTON_TIGHT_ID) = true;
    }

    if (m_Ibranches_photons.at("nPhotonsOR_TIdLIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTON_ISOLATION) = true;
    }

    // Constrained mllgamma mass window
    const float mllgCstr = m_Fbranches_baseline.at("LLGam_M_Cstr").get(*event, sys);
    const auto &mllgWindow = m_mllgCstrWindow.value();
    if (mllgCstr >= mllgWindow.at(0) * Athena::Units::GeV && mllgCstr <= mllgWindow.at(1) * Athena::Units::GeV)
    {
      m_bools.at(HZY::MASS_DILEPTON_PHOTON) = true;
    }
  }

} // namespace HZY
