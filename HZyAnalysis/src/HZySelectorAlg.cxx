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

  HZySelectorAlg::HZySelectorAlg(const std::string &name,
                                 ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

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

    ATH_CHECK(m_electronLIdLIsoDecorHandle.initialize(m_systematicsList, m_HZyElectronHandle));
    ATH_CHECK(m_muonMIdLIsoDecorHandle.initialize(m_systematicsList, m_HZyMuonHandle));

    ATH_CHECK(m_year.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is17_periodB5_B8.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is22_75bunches.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is23_75bunches.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_is23_400bunches.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK(m_matchingTool.retrieve());

    for (auto &[key, value] : m_boolnames)
    {
      m_bools.emplace(key, false);
      CP::SysWriteDecorHandle<bool> whandle{value + "_%SYS%", this};
      m_Bbranches.emplace(key, whandle);
      ATH_CHECK(
          m_Bbranches.at(key).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise trigger decoration handles
    for (auto trig : m_leptonTriggers)
    {
      std::string trigName = "trigPassed_" + trig;
      CP::SysReadDecorHandle<bool> deco{this, trigName, trigName,
                                        "Name of triggers"};
      m_triggerdecos.emplace(trigName, deco);
      ATH_CHECK(m_triggerdecos.at(trigName).initialize(m_systematicsList,
                                                       m_eventHandle));
    }

    // Setup read decorations for Z candidate dilepton variables (float and int)
    for (const std::string &string_var : m_intVariables_leptons)
    {
      CP::SysReadDecorHandle<int> var{string_var + "_%SYS%", this};
      m_Ibranches_leptons.emplace(string_var, var);
      ATH_CHECK(m_Ibranches_leptons.at(string_var)
                    .initialize(m_systematicsList, m_eventHandle));
    }

    // special flag for all cuts
    ATH_CHECK(m_passallcuts.initialize(m_systematicsList, m_eventHandle));

    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    ATH_CHECK(initialiseCutflow());
    return StatusCode::SUCCESS;
  }

  StatusCode HZySelectorAlg::execute()
  {
    // Global filter originally false
    CP::SysFilterReporterCombiner filterCombiner(m_filterParams, false);

    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      CP::SysFilterReporter filter(filterCombiner, sys);

      // Retrieve inputs
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys));

      // Reset all cut flags to default=false
      for (const auto &[key, value] : m_bools)
      {
        m_bools.at(key) = false;
      }

      // Set trigger thresholds and evaluate trigger cuts
      setThresholds(event, sys);
      evaluateTriggerCuts(event, HZyElectrons, HZyMuons, m_HZyCuts, sys);

      // Dilepton ID/Iso and dilepton+loose photon selections
      evaluateLeptonCuts(event, HZyElectrons, HZyMuons, HZyPhotons, m_HZyCuts, sys);

      // Check the trigger matching cuts after selected leptons for Z
      evaluateTriggerMatchingCuts(HZyElectrons, HZyMuons, m_HZyCuts);

      bool passedall = true;
      for (const auto &cut : m_inputCutKeys)
      {
        passedall &= m_bools.at(cut);
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

  void HZySelectorAlg::evaluateLeptonCuts(
      const xAOD::EventInfo* event,
      const xAOD::ElectronContainer* electrons,
      const xAOD::MuonContainer* muons,
      const xAOD::PhotonContainer *photons,
      CutManager& HZyCuts,
      const CP::SystematicSet& sys)
  {
    int nEleLooseID = 0;
    int nEleLooseIDLooseIso = 0;
    int nMuMediumID = 0;
    int nMuMediumIDLooseIso = 0;

    for (const xAOD::Electron* ele : *electrons) {
      if (!ele) {
        continue;
      }
      ++nEleLooseID;

      if (m_electronLIdLIsoDecorHandle.get(*ele, sys)) {
        ++nEleLooseIDLooseIso;
      }
    }

    for (const xAOD::Muon* mu : *muons) {
      if (!mu) {
        continue;
      }
      ++nMuMediumID;

      if (m_muonMIdLIsoDecorHandle.get(*mu, sys)) {
        ++nMuMediumIDLooseIso;
      }
    }

    const bool passLeptonID = nEleLooseID >= 2 || nMuMediumID >= 2;
    const bool passLeptonISO = nEleLooseIDLooseIso >= 2 || nMuMediumIDLooseIso >= 2;

    if (HZyCuts.exists("PASS_LEPTON_ID")) {
      m_bools.at(HZY::PASS_LEPTON_ID) = passLeptonID;
    }

    if (HZyCuts.exists("PASS_LEPTON_ISO")) {
      m_bools.at(HZY::PASS_LEPTON_ISO) = passLeptonISO;
    }

        // Z-candidate size
    const int nZCandidates = m_Ibranches_leptons.at("LepLepZ_Size").get(*event, sys);
    if (HZyCuts.exists("SIZE_ZCANDIDATE") && nZCandidates > 0) {
      m_bools.at(HZY::SIZE_ZCANDIDATE) = true;
    }

    // Z + loose photon size
    if (HZyCuts.exists("SIZE_Z_LOOSEPHOTON") && passLeptonISO && !photons->empty()) {
      m_bools.at(HZY::SIZE_Z_LOOSEPHOTON) = true;
    }
  }

  void HZySelectorAlg::evaluateTriggerMatchingCuts(
      const xAOD::ElectronContainer* ZCandElectrons,
      const xAOD::MuonContainer* ZCandMuons,
      CutManager& HZyCuts)
  {
    if (!HZyCuts.exists("PASS_TRIGGER_MATCHING")) return;

    const std::size_t nElectrons = ZCandElectrons ? ZCandElectrons->size() : 0;
    const std::size_t nMuons = ZCandMuons ? ZCandMuons->size() : 0;
    if (nElectrons < 2 && nMuons < 2) return;

    bool passSET = false, passSMT = false;
    bool passDET = false, passDMT = false;

    for (const auto& trig : m_leptonTriggers) {
      // single-electron trigger
      if (!passSET && ZCandElectrons &&
          trig.starts_with("HLT_e") && !trig.starts_with("HLT_2e")) {
        for (const auto* ele : *ZCandElectrons) {
          if (ele && ele->pt() > m_pt_threshold[HZY::SLT][HZY::ele] &&
              m_matchingTool->match(*ele, trig, 0.07)) {
            passSET = true;
            break;
          }
        }
      }

      // single-muon trigger
      if (!passSMT && ZCandMuons && trig.starts_with("HLT_mu") && trig != "HLT_mu22_mu8noL1_L1MU14FCH") {
        for (const auto* mu : *ZCandMuons) {
          if (mu && mu->pt() > m_pt_threshold[HZY::SLT][HZY::mu] &&
              m_matchingTool->match(*mu, trig)) {
            passSMT = true;
            break;
          }
        }
      }

      // double-electron trigger
      if (!passDET && ZCandElectrons && trig.starts_with("HLT_2e")) {
        for (const auto* e0 : *ZCandElectrons) {
          for (const auto* e1 : *ZCandElectrons) {
            if (!e0 || !e1 || e0 == e1) continue;

            const bool passPt =
                (e0->pt() > m_pt_threshold[HZY::DLT][HZY::leadingele] && e1->pt() > m_pt_threshold[HZY::DLT][HZY::subleadingele]) ||
                (e1->pt() > m_pt_threshold[HZY::DLT][HZY::leadingele] && e0->pt() > m_pt_threshold[HZY::DLT][HZY::subleadingele]);

            if (passPt && m_matchingTool->match({e0, e1}, trig, 0.07)) {
              passDET = true;
              break;
            }
          }
          if (passDET) break;
        }
      }

      // double-muon trigger
      if (!passDMT && ZCandMuons && (trig.starts_with("HLT_2mu") || trig == "HLT_mu22_mu8noL1_L1MU14FCH")) {
        for (const auto* m0 : *ZCandMuons) {
          for (const auto* m1 : *ZCandMuons) {
            if (!m0 || !m1 || m0 == m1) continue;

            const bool passPt =
                (m0->pt() > m_pt_threshold[HZY::DLT][HZY::leadingmu] && m1->pt() > m_pt_threshold[HZY::DLT][HZY::subleadingmu]) ||
                (m1->pt() > m_pt_threshold[HZY::DLT][HZY::leadingmu] && m0->pt() > m_pt_threshold[HZY::DLT][HZY::subleadingmu]);

            if (passPt && m_matchingTool->match({m0, m1}, trig)) {
              passDMT = true;
              break;
            }
          }
          if (passDMT) break;
        }
      }
    }

    m_bools.at(HZY::pass_matching_trigger_SLT) |= (passSET || passSMT);
    m_bools.at(HZY::pass_matching_trigger_DLT) |= (passDET || passDMT);

    if (m_bools.at(HZY::pass_matching_trigger_SLT) ||
        m_bools.at(HZY::pass_matching_trigger_DLT)) {
      m_bools.at(HZY::PASS_TRIGGER_MATCHING) = true;
    }

    ATH_MSG_DEBUG("Trigger matching: " << "SET=" << passSET << ", SMT=" << passSMT << ", DET=" << passDET << ", DMT=" << passDMT);
  }

  void HZySelectorAlg::setThresholds(const xAOD::EventInfo* event,
                                    const CP::SystematicSet& sys)
  {
    const int year = m_year.get(*event, sys);
    const bool isRun3 = (year >= 2022);
    constexpr float GeV = Athena::Units::GeV;

    // Single-lepton triggers
    m_pt_threshold[HZY::SLT][HZY::ele] =
        m_is22_75bunches.get(*event, sys) ? 18. * GeV : 27. * GeV;

    m_pt_threshold[HZY::SLT][HZY::mu] =
        isRun3 ? 25. * GeV : 27. * GeV;

    // Di-electron triggers
    const float diele_pt = isRun3 ? 18. * GeV : 25. * GeV;
    m_pt_threshold[HZY::DLT][HZY::leadingele] = diele_pt;
    m_pt_threshold[HZY::DLT][HZY::subleadingele] = diele_pt;

    // Di-muon triggers
    if (isRun3) {
      m_pt_threshold[HZY::DLT][HZY::leadingmu] = 15. * GeV;
      m_pt_threshold[HZY::DLT][HZY::subleadingmu] = 15. * GeV;

      m_pt_threshold[HZY::DLT][HZY::asyleadingmu] = 23. * GeV;
      m_pt_threshold[HZY::DLT][HZY::asysubleadingmu] = 9. * GeV;
    } else {
      m_pt_threshold[HZY::DLT][HZY::leadingmu] = 24. * GeV;
      m_pt_threshold[HZY::DLT][HZY::subleadingmu] = 10. * GeV;
    }

    if (year < 2015 || (year > 2018 && year < 2022)) {
      ATH_MSG_WARNING("Using default trigger thresholds for unsupported year: " << year);
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
      if (inputWasFound.at(index))
        continue;
      ATH_MSG_ERROR("Doubled or falsely spelled cuts in CutList (see config file)." + m_inputCutList[index]);
    }

    for (const auto &cut : m_inputCutKeys)
    {
      m_HZyCuts.add(m_boolnames[cut]);
    }

    if (m_saveCutFlow)
    {
      // After filling the CutManager, book your histograms.
      const unsigned int nbins =
          m_HZyCuts.size() +
          1; //  need an extra bin for the total num of events.
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
