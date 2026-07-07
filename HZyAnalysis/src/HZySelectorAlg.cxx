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
      CP::SysWriteDecorHandle<bool> whandle{value + "_%SYS%", this};
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

    ATH_CHECK(initialiseCutflow());
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
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys, ctx));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys, ctx));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys, ctx));

      // Reset all cut flags to default=false
      for (const auto &[key, value] : m_bools)
      {
        m_bools.at(key) = false;
      }

      // Evaluate trigger passing cuts
      evaluateTriggerCuts(event, HZyElectrons, HZyMuons, m_HZyCuts, sys);

      // Dilepton ID/Iso and dilepton+loose photon selections
      // Check the trigger matching cuts after selected leptons for Z candidate
      // Hmumu and Zgamma selections
      // Photon ID and Isolation selections
      evaluateEventSelections(event, m_HZyCuts, sys);

      bool passedall = !m_inputCutKeys.empty();
      for (const auto &cut : m_inputCutKeys)
      {
        passedall = passedall && m_bools.at(cut);
      }
      m_passallcuts.set(*event, passedall, sys);

      // Global event filter true if any syst passes and controls
      // if event is passed to output writing or not
      if (m_bypass || passedall)
        filter.setPassed(true);

      // do the CUTFLOW only with sys="" -> NOSYS
      if (sys.name().empty() && m_saveCutFlow)
      {
        const float genWeight = m_isMC ? m_generatorWeight.get(*event, sys) : 1.0f;
        // Compute total_events
        ++m_total_events;
        if (m_isMC)
          m_total_mcEventWeight += genWeight;
        for (const auto &cut : m_inputCutKeys)
        {
          const auto &cutName = m_boolnames.at(cut);
          if (!m_HZyCuts.exists(cutName))
            continue;

          auto &cutFlow = m_HZyCuts(cutName);
          cutFlow.passed = m_bools.at(cut);
          if (cutFlow.passed)
          {
            ++cutFlow.counter;
            if (m_isMC)
              cutFlow.w_counter += genWeight;
          }
        }
      }

      // Check how many consecutive cuts are passed by the event.
      unsigned int consecutive_cuts = 0;
      for (size_t i = 0; i < m_HZyCuts.size(); ++i)
      {
        if (m_HZyCuts[i].passed)
          consecutive_cuts++;
        else
          break;
      }

      // Here we basically increment the  N_events(pass_i  AND pass_i-1  AND
      // ... AND pass_0) for the i-cut. I think this is an elegant way to do it
      // :) . Considering the difficulties a configurable cut list imposes.
      for (unsigned int i = 0; i < consecutive_cuts; i++)
      {
        m_HZyCuts[i].relativeCounter += 1;
      }
      for (auto &[key, var] : m_bools)
      {
        m_Bbranches.at(key).set(*event, var, sys);
      }
    }

    return StatusCode::SUCCESS;
  }

  StatusCode HZySelectorAlg::finalize()
  {
    ANA_CHECK(m_filterParams.finalize());
    m_HZyCuts.CheckCutResults(); // Print CheckCutResults

    if (m_saveCutFlow)
    {
      m_HZyCuts.DoAbsoluteEfficiency(m_total_events, efficiency("AbsoluteEfficiency"));
      m_HZyCuts.DoRelativeEfficiency(m_total_events, efficiency("RelativeEfficiency"));
      m_HZyCuts.DoStandardCutFlow(m_total_events, efficiency("StandardCutFlow"));
      if (m_isMC)
      {
        m_HZyCuts.DoWeightedAbsoluteEfficiency(m_total_mcEventWeight, efficiency("WeightedAbsoluteEfficiency"));
        m_HZyCuts.DoWeightedRelativeEfficiency(m_total_mcEventWeight, efficiency("WeightedRelativeEfficiency"));
        m_HZyCuts.DoWeightedStandardCutFlow(m_total_mcEventWeight, efficiency("WeightedStandardCutFlow"));
      }
      m_HZyCuts.DoCutflowLabeling(m_total_events, hist("EventsPassed_BinLabeling"));
    }

    return StatusCode::SUCCESS;
  }

  void HZySelectorAlg::evaluateTriggerCuts(
    const xAOD::EventInfo* event,
    const xAOD::ElectronContainer* electrons,
    const xAOD::MuonContainer* muons,
    CutManager& HZyCuts,
    const CP::SystematicSet& sys)
  {
    if (!HZyCuts.exists("PASS_TRIGGER")) return;

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
      CutManager& HZyCuts,
      const CP::SystematicSet& sys)
  {
    const bool passLeptonID =
        m_Ibranches_leptons.at("nElectrons_LIdNIso").get(*event, sys) >= 2 ||
        m_Ibranches_leptons.at("nMuons_MIdNIso").get(*event, sys) >= 2;
    const bool passLeptonISO =
        m_Ibranches_leptons.at("nElectrons_LIdLIso").get(*event, sys) >= 2 ||
        m_Ibranches_leptons.at("nMuons_MIdLIso").get(*event, sys) >= 2;

    if (!HZyCuts.exists("PASS_LEPTON_ID")) return;
    if (passLeptonID) {
      m_bools.at(HZY::PASS_LEPTON_ID) = true;
    }

    if (!HZyCuts.exists("PASS_LEPTON_ISO")) return;
    if (passLeptonISO) {
      m_bools.at(HZY::PASS_LEPTON_ISO) = true;
    }

        // Z-candidate size
    const int nZCandidates = m_Ibranches_leptons.at("LepLepZ_Size").get(*event, sys);
    if (!HZyCuts.exists("SIZE_ZCANDIDATE")) return;
    if (nZCandidates > 0) {
      m_bools.at(HZY::SIZE_ZCANDIDATE) = true;
    }

    // Z + loose photon size
    if (!HZyCuts.exists("SIZE_Z_LOOSEPHOTON")) return;
    if (passLeptonISO && m_Ibranches_photons.at("nPhotons_LIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::SIZE_Z_LOOSEPHOTON) = true;
    }

    // Trigger matching using the orginal leptons from selected Z candidate
    if (!HZyCuts.exists("PASS_TRIGGER_MATCHING")) return;
    if (m_globalTriggerMatch.get(*event, sys)) {
      m_bools.at(HZY::PASS_TRIGGER_MATCHING) = true;
    }

    // Photon OR lepton or jet selections on the selected Zgamma candidate photon
    if (!HZyCuts.exists("PHOTONS_JETS_OR_LEPTONS")) return;
    if (m_Ibranches_photons.at("nPhotonsOR_LIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTONS_JETS_OR_LEPTONS) = true;
    }

    const float mllRaw = m_Fbranches_leptons.at("LepLepZ_M_Raw").get(*event, sys);
    const float mllCstr = m_Fbranches_leptons.at("LepLepZ_M_Cstr").get(*event, sys);
    const int channel = m_Ibranches_leptons.at("Lep_Channel").get(*event, sys);

    // Hmumu veto
    if (!HZyCuts.exists("DILEPTON_MASS_VETO_HMM")) return;
    if (!(channel == 2 && mllRaw > m_mllRawMax_HmumuVeto * Athena::Units::GeV)) {
      m_bools.at(HZY::DILEPTON_MASS_VETO_HMM) = true;
    }

    // Constrained dilepton mass window
    const auto &mllWindow = m_mllCstrWindow.value();
    if (!HZyCuts.exists("DILEPTON_MASS_CLOSEZ")) return;
    if (mllCstr >= mllWindow.at(0) * Athena::Units::GeV && mllCstr <= mllWindow.at(1) * Athena::Units::GeV) {
      m_bools.at(HZY::DILEPTON_MASS_CLOSEZ) = true;
    }

    // Relative photon pT
    const float photon_relPt = m_Fbranches_baseline.at("LLGam_RelPhpT_Cstr").get(*event, sys);
    if (!HZyCuts.exists("PASS_RELPT_LOW")) return;
    if (photon_relPt > m_relPtMin)
    {
      m_bools.at(HZY::PASS_RELPT_LOW) = true;
    }

    // Photon ID and Isolation selections on the selected Zgamma candidate photon
    if (!HZyCuts.exists("PHOTON_TIGHT_ID")) return;
    if (m_Ibranches_photons.at("nPhotonsOR_TIdNIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTON_TIGHT_ID) = true;
    }

    if (!HZyCuts.exists("PHOTON_ISOLATION")) return;
    if (m_Ibranches_photons.at("nPhotonsOR_TIdLIso").get(*event, sys) >= 1) {
      m_bools.at(HZY::PHOTON_ISOLATION) = true;
    }

    // Constrained mllgamma mass window
    const float mllgCstr = m_Fbranches_baseline.at("LLGam_M_Cstr").get(*event, sys);
    const auto &mllgWindow = m_mllgCstrWindow.value();
    if (!HZyCuts.exists("MASS_DILEPTON_PHOTON")) return;
    if (mllgCstr >= mllgWindow.at(0) * Athena::Units::GeV && mllgCstr <= mllgWindow.at(1) * Athena::Units::GeV)
    {
      m_bools.at(HZY::MASS_DILEPTON_PHOTON) = true;
    }
  }

  StatusCode HZySelectorAlg::initialiseCutflow()
  {
    std::vector<std::string> boolnameslist;
    for (const auto &[key, value] : m_boolnames)
    {
      boolnameslist.push_back(value);
    }
    m_HZyCuts.CheckInputCutList(m_inputCutList, boolnameslist);

    m_inputCutKeys.resize(m_inputCutList.size());

    std::vector<bool> inputWasFound(m_inputCutList.size(), false);
    for (const auto &[key, value] : m_boolnames)
    {
      auto it = std::find(m_inputCutList.begin(), m_inputCutList.end(), value);
      if (it != m_inputCutList.end())
      {
        auto index = it - m_inputCutList.begin();
        m_inputCutKeys.at(index) = key;
        inputWasFound.at(index) = true;
      }
    }

    for (unsigned int index = 0; index < inputWasFound.size(); index++)
    {
      if (inputWasFound.at(index)) continue;
      ATH_MSG_ERROR("Doubled or falsely spelled cuts in CutList (see config file)." + m_inputCutList[index]);
    }

    for (const auto &cut : m_inputCutKeys)
    {
      m_HZyCuts.add(m_boolnames[cut]);
    }

    if (m_saveCutFlow)
    {
      // After filling the CutManager, book your histograms.
      const unsigned int nbins = m_HZyCuts.size() + 1; //  need an extra bin for the total num of events.
      ANA_CHECK(book(TEfficiency("AbsoluteEfficiency", "Absolute Efficiency of H->Z(ll)y cuts.Needs rescaling to total events.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
      ANA_CHECK(book(TEfficiency("RelativeEfficiency", "Relative Efficiency of H->Z(ll)y cuts.Needs rescaling to total events.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
      ANA_CHECK(book(TEfficiency("StandardCutFlow", "StandardCutFlow of H->Z(ll)y cuts.Needs rescaling to total events.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
      if (m_isMC)
      {
        ANA_CHECK(book(TEfficiency("WeightedAbsoluteEfficiency", "Weighted Absolute Efficiency of H->Z(ll)y cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
        ANA_CHECK(book(TEfficiency("WeightedRelativeEfficiency", "Weighted Relative Efficiency of H->Z(ll)y cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
        ANA_CHECK(book(TEfficiency("WeightedStandardCutFlow", "Weighted StandardCutFlow of H->Z(ll)y cuts.Needs rescaling to sumOfWeights.;Cuts;#epsilon", nbins, 0.5, nbins + 0.5)));
      }
      ANA_CHECK(book(TH1F("EventsPassed_BinLabeling", "Events passed by each cut / Bin labeling", nbins, 0.5, nbins + 0.5)));
    }

    return StatusCode::SUCCESS;
  }
} // namespace HZY
