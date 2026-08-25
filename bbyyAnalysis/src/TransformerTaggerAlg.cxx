/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "TransformerTaggerAlg.h"

#include <AthenaKernel/Units.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <utility>

namespace HHBBYY
{
  TransformerTaggerAlg::TransformerTaggerAlg(const std::string &name,
                                 ISvcLocator *pSvcLocator)
    : AthHistogramAlgorithm(name, pSvcLocator)
  {}

  StatusCode TransformerTaggerAlg::initialize()
  {
    ATH_MSG_INFO("*********************************");
    ATH_MSG_INFO("          TransformerTaggerAlg         ");
    ATH_MSG_INFO("*********************************");

    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_photonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));

    if (!m_isBtag.empty()) {
      ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }
    if (!m_PCBT.empty()) {
      ATH_CHECK(m_PCBT.initialize(m_systematicsList, m_jetHandle));
    }

    for (const std::string &string_var : m_floatVariables) {
      CP::SysWriteDecorHandle<float> var {string_var + "_%SYS%", this};
      m_Fbranches.emplace(string_var, var);
      ATH_CHECK(m_Fbranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &string_var: m_intVariables) {
      CP::SysWriteDecorHandle<int> var {string_var+"_%SYS%", this};
      m_Ibranches.emplace(string_var, var);
      ATH_CHECK (m_Ibranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    ATH_CHECK(m_transformerTaggerTool.retrieve());

    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode TransformerTaggerAlg::execute(const EventContext& ctx)
  {
    for (const auto &sys : m_systematicsList.systematicsVector()) {
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.retrieve(jets, sys, ctx));

      const xAOD::PhotonContainer *photons = nullptr;
      ANA_CHECK(m_photonHandle.retrieve(photons, sys, ctx));

      for (const std::string &string_var : m_floatVariables) {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const std::string &string_var: m_intVariables) {
        m_Ibranches.at(string_var).set(*event, -99, sys);
      }

      if (jets->size() < 2 || photons->size() < 2) {
        ATH_MSG_DEBUG("Skip TransformerTaggerAlg: need at least 2 jets and 2 photons");
        continue;
      }

      const bool WPgiven = !m_isBtag.empty();
      const bool PCBTgiven = !m_PCBT.empty();

      int nCentralJets = 0;
      int nBJets = 0;
      for (const xAOD::Jet *jet : *jets) {
        if (std::abs(jet->eta()) < 2.5) {
          ++nCentralJets;
          if (WPgiven && m_isBtag.get(*jet, sys)) ++nBJets;
        }
      }
      if (!WPgiven) nBJets = -1;

      auto eventFeatureValue = [&](const std::string &name) -> float {
        if (name == "nJets") return static_cast<float>(jets->size());
        if (name == "nCentralJets") return static_cast<float>(nCentralJets);
        if (name == "nBJets") return static_cast<float>(nBJets);
        if (name == "averageInteractionsPerCrossing" || name == "pileup") {
          return static_cast<float>(event->averageInteractionsPerCrossing());
        }
        ATH_MSG_ERROR("Unknown event feature '" << name << "' requested by the TransformerTagger IOConfig.");
        return -99.f;
      };

      auto jetFeatureValue = [&](const std::string &name, const xAOD::Jet *jet) -> float {
        if (name == "pt") return static_cast<float>(jet->pt() / Athena::Units::MeV);
        if (name == "eta") return static_cast<float>(jet->eta());
        if (name == "phi") return static_cast<float>(jet->phi());
        if (name == "m") return static_cast<float>(jet->m() / Athena::Units::MeV);
        if (name == "pcbt") return PCBTgiven ? static_cast<float>(m_PCBT.get(*jet, sys)) : -99.f;
        ATH_MSG_ERROR("Unknown jet feature '" << name << "' requested by the TransformerTagger IOConfig.");
        return -99.f;
      };

      float myy = -99.f;
      if (photons->size() >= 2) {
        myy = static_cast<float>((photons->at(0)->p4() + photons->at(1)->p4()).M());
      }

      auto photonFeatureValue = [&](const std::string &name, const xAOD::Photon *photon) -> float {
        if (name == "ptOverMyy") return myy > 0.f ? static_cast<float>(photon->pt()) / myy : -99.f;
        if (name == "eta") return static_cast<float>(photon->eta());
        if (name == "phi") return static_cast<float>(photon->phi());
        ATH_MSG_ERROR("Unknown photon feature '" << name << "' requested by the TransformerTagger IOConfig.");
        return -99.f;
      };

      std::map<std::string, std::pair<std::vector<float>, int64_t>> inputs;

      const std::vector<std::string> &eventFeatureNames = m_transformerTaggerTool->inputFeatures("event_features");
      std::vector<float> eventFeatures;
      eventFeatures.reserve(eventFeatureNames.size());
      for (const std::string &feature : eventFeatureNames) eventFeatures.push_back(eventFeatureValue(feature));
      inputs["event_features"] = {eventFeatures, 1};

      const std::vector<std::string> &jetFeatureNames = m_transformerTaggerTool->inputFeatures("jet_features");
      std::vector<float> jetFeatures;
      jetFeatures.reserve(jets->size() * jetFeatureNames.size());
      for (const xAOD::Jet *jet : *jets)
        for (const std::string &feature : jetFeatureNames) jetFeatures.push_back(jetFeatureValue(feature, jet));
      inputs["jet_features"] = {jetFeatures, static_cast<int64_t>(jets->size())};

      const std::vector<std::string> &photonFeatureNames = m_transformerTaggerTool->inputFeatures("two_photon_features");
      constexpr std::size_t nInputPhotons = 2;
      std::vector<float> photonFeatures;
      photonFeatures.reserve(nInputPhotons * photonFeatureNames.size());
      for (std::size_t photonIdx = 0; photonIdx < nInputPhotons; ++photonIdx) {
        const xAOD::Photon *photon = photons->at(photonIdx);
        for (const std::string &feature : photonFeatureNames) photonFeatures.push_back(photonFeatureValue(feature, photon));
      }
      inputs["two_photon_features"] = {photonFeatures, static_cast<int64_t>(nInputPhotons)};

      std::map<std::string, float> outputs;
      ATH_CHECK(m_transformerTaggerTool->run(inputs, outputs));

      const float HHScore = outputs.at("TransformerTagger_HH_score");
      const float HScore = outputs.at("TransformerTagger_H_score");
      const float jet1Idx = outputs.at("TransformerTagger_jet1_idx");
      const float jet2Idx = outputs.at("TransformerTagger_jet2_idx");
      const float edgeScore = outputs.at("TransformerTagger_edge_score");

      int jet1IdxInt = static_cast<int>(std::lround(jet1Idx));
      int jet2IdxInt = static_cast<int>(std::lround(jet2Idx));
      const bool validJetPair =
        jet1IdxInt >= 0 && jet2IdxInt >= 0 && jet1IdxInt != jet2IdxInt &&
        static_cast<std::size_t>(jet1IdxInt) < jets->size() &&
        static_cast<std::size_t>(jet2IdxInt) < jets->size();
      const bool centralJetPair =
        validJetPair &&
        std::abs(jets->at(jet1IdxInt)->eta()) < 2.5 &&
        std::abs(jets->at(jet2IdxInt)->eta()) < 2.5;
      if (!centralJetPair) {
        jet1IdxInt = -1;
        jet2IdxInt = -1;
      }
      //std::cout << jet1IdxInt << " " << jet2IdxInt << " " << jets->size() << std::endl;

      m_Fbranches.at("TransformerTagger_H_score").set(*event, HScore, sys);
      m_Fbranches.at("TransformerTagger_HH_score").set(*event, HHScore, sys);
      m_Ibranches.at("TransformerTagger_jet1_idx").set(*event, jet1IdxInt, sys);
      m_Ibranches.at("TransformerTagger_jet2_idx").set(*event, jet2IdxInt, sys);
      m_Fbranches.at("TransformerTagger_edge_score").set(*event, edgeScore, sys);
    }

    return StatusCode::SUCCESS;
  }
}
