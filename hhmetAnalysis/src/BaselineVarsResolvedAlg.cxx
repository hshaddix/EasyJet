/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>

#include "BaselineVarsResolvedAlg.h"
#include "AthContainers/AuxElement.h"
#include "FourMomUtils/xAODP4Helpers.h"

namespace HHMET
{
  StatusCode BaselineVarsResolvedAlg ::initialize()
  {

    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     BaselineVarsResolvedAlg     \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_recoJetHandle.initialize(m_systematicsList));
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    if (m_isBtag.empty()) {
      ATH_MSG_ERROR("bTagSelDecor must be configured");
      return StatusCode::FAILURE;
    }

    ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_jetHandle));

    // initialise common var handles
    for (const std::string &var : m_commonFloatVars)
    {
        const std::string name = baseDecorName(var);
        CP::SysWriteDecorHandle<float> deco{name + "_%SYS%", this};
        m_commonFloatDecos.emplace(name, deco);
        ATH_CHECK(m_commonFloatDecos.at(name).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &var : m_commonIntVars)
    {
        const std::string name = baseDecorName(var);
        CP::SysWriteDecorHandle<int> deco{name + "_%SYS%", this};
        m_commonIntDecos.emplace(name, deco);
        ATH_CHECK(m_commonIntDecos.at(name).initialize(m_systematicsList, m_eventHandle));
    }

    // initialise alg-dependent handles
    for (const std::string &algorithm : m_pairingAlgorithms)
    {
      if (algorithm != "mass_125" && algorithm != "min_deltaR_max")
      {
        ATH_MSG_ERROR("Unknown resolved pairing algorithm: " << algorithm);
        return StatusCode::FAILURE;
      }

      for (const std::string &var : m_floatVars)
      {
        const std::string name = decorName(algorithm, var);
        CP::SysWriteDecorHandle<float> deco{name + "_%SYS%", this};
        m_floatDecos.emplace(name, deco);
        ATH_CHECK(m_floatDecos.at(name).initialize(m_systematicsList, m_eventHandle));
      }

      for (const std::string &var : m_intVars)
      {
        const std::string name = decorName(algorithm, var);
        CP::SysWriteDecorHandle<int> deco{name + "_%SYS%", this};
        m_intDecos.emplace(name, deco);
        ATH_CHECK(m_intDecos.at(name).initialize(m_systematicsList, m_eventHandle));
      }
    }
    
    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsResolvedAlg ::execute(const EventContext& ctx)
  {
    for (const auto& sys : m_systematicsList.systematicsVector())
    {

      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      const xAOD::JetContainer *recoJets = nullptr;
      ANA_CHECK (m_recoJetHandle.retrieve (recoJets, sys, ctx));

      std::unordered_map<const xAOD::Jet*, int> recoJetIndices;
      recoJetIndices.reserve(recoJets->size());
      for (std::size_t index = 0; index < recoJets->size(); ++index)
      {
        recoJetIndices.emplace(recoJets->at(index),static_cast<int>(index));
      }

      setDefaults(*eventInfo, sys);

      int n_ljets = 0;
      int n_bjets = 0;

      // Sort these into the bjets and light jets
      std::vector<const xAOD::Jet*> bjets;
      std::vector<int> bjet_indices;
      std::vector<const xAOD::Jet*> ljets;
      std::vector<int> ljet_indices;

      for (const xAOD::Jet* jet : *jets)
      {
          const auto recoJetIndexIt = recoJetIndices.find(jet);
          if (recoJetIndexIt == recoJetIndices.end())
          {
            ATH_MSG_ERROR("Selected pairing jet is not present in the configured reco-jet output container");
            return StatusCode::FAILURE;
          }
          const int recojet_index = recoJetIndexIt->second;

          if (m_isBtag.get(*jet, sys))
          {
              bjets.push_back(jet);
              bjet_indices.push_back(recojet_index);
              n_bjets++;
          }
          else
          {
              ljets.push_back(jet);
              ljet_indices.push_back(recojet_index);
              n_ljets++;
          }

          if (n_bjets == 4)
          {
              break;
          }
      }

      // Want either 4b OR 3b1l.
      if (!((n_bjets >= 4) || (n_bjets >= 3 && n_ljets >= 1)))
      {
        continue;
      }
      std::vector<const xAOD::Jet*> pairing_jets = bjets;
      std::vector<int> pairing_jet_indices = bjet_indices;
      std::vector<bool> pairing_flav = {1, 1, 1}; 
      if (n_bjets==4)
      {
        pairing_flav.push_back(1);
      }
      else
      {
        pairing_jets.push_back(ljets[0]);
        pairing_jet_indices.push_back(ljet_indices[0]);
        pairing_flav.push_back(0);
      }

      // int nTruthB = -1;
      // int nTruthHiggsB = -1;
      int tStatus = -1;

      // write common vars

      const xAOD::JetFourMom_t hh = pairing_jets.at(0)->jetP4() + pairing_jets.at(1)->jetP4() +
                                    pairing_jets.at(2)->jetP4() + pairing_jets.at(3)->jetP4();
      m_commonIntDecos.at(baseDecorName("reco_success")).set(*eventInfo, 1, sys);
      m_commonFloatDecos.at(baseDecorName("hh_m")).set(*eventInfo, hh.M(), sys);
      m_commonFloatDecos.at(baseDecorName("hh_pt")).set(*eventInfo, hh.Pt(), sys);
      m_commonFloatDecos.at(baseDecorName("hh_eta")).set(*eventInfo, hh.Eta(), sys);
      m_commonFloatDecos.at(baseDecorName("hh_phi")).set(*eventInfo, hh.Phi(), sys);
      if (m_isMC)
      {
        int nTruthB = nTruthBJets(pairing_jets);
        int nTruthHiggsB = nTruthHiggsBJets(pairing_jets);
        tStatus = truthStatus(pairing_jets);
        m_commonIntDecos.at(baseDecorName("n_truth_pairing_bjets")).set(*eventInfo,nTruthB,sys);
        m_commonIntDecos.at(baseDecorName("n_truth_higgs_pairing_bjets")).set(*eventInfo,nTruthHiggsB,sys);
        m_commonIntDecos.at(baseDecorName("truth_status")).set(*eventInfo,tStatus,sys);
      }

      for (const std::string &algorithm : m_pairingAlgorithms)
      {
        const PairingChoice choice = choosePairing(algorithm, pairing_jets);

        const xAOD::JetFourMom_t h1 = pairing_jets[choice.indices[0]]->jetP4() +
                                      pairing_jets[choice.indices[1]]->jetP4();
        const xAOD::JetFourMom_t h2 = pairing_jets[choice.indices[2]]->jetP4() +
                                      pairing_jets[choice.indices[3]]->jetP4();

        m_floatDecos.at(decorName(algorithm, "h1_m")).set(*eventInfo, h1.M(), sys);
        m_floatDecos.at(decorName(algorithm, "h1_pt")).set(*eventInfo, h1.Pt(), sys);
        m_floatDecos.at(decorName(algorithm, "h1_eta")).set(*eventInfo, h1.Eta(), sys);
        m_floatDecos.at(decorName(algorithm, "h1_phi")).set(*eventInfo, h1.Phi(), sys);
        m_floatDecos.at(decorName(algorithm, "h2_m")).set(*eventInfo, h2.M(), sys);
        m_floatDecos.at(decorName(algorithm, "h2_pt")).set(*eventInfo, h2.Pt(), sys);
        m_floatDecos.at(decorName(algorithm, "h2_eta")).set(*eventInfo, h2.Eta(), sys);
        m_floatDecos.at(decorName(algorithm, "h2_phi")).set(*eventInfo, h2.Phi(), sys);
        m_floatDecos.at(decorName(algorithm, "hh_deltaEta")).set(*eventInfo, std::abs(h1.Eta() - h2.Eta()), sys);
        m_floatDecos.at(decorName(algorithm, "h1_deltaR")).set(*eventInfo, choice.dR1, sys);
        m_floatDecos.at(decorName(algorithm, "h2_deltaR")).set(*eventInfo, choice.dR2, sys);
        m_floatDecos.at(decorName(algorithm, "hh_deltaR")).set(*eventInfo, xAOD::P4Helpers::deltaR(h1.Eta(), h1.Phi(), h2.Eta(), h2.Phi()), sys);
        m_floatDecos.at(decorName(algorithm, "metric")).set(*eventInfo, choice.metric, sys);

        m_intDecos.at(decorName(algorithm, "j1_index")).set(*eventInfo, pairing_jet_indices[static_cast<int>(choice.indices[0])], sys);
        m_intDecos.at(decorName(algorithm, "j2_index")).set(*eventInfo, pairing_jet_indices[static_cast<int>(choice.indices[1])], sys);
        m_intDecos.at(decorName(algorithm, "j3_index")).set(*eventInfo, pairing_jet_indices[static_cast<int>(choice.indices[2])], sys);
        m_intDecos.at(decorName(algorithm, "j4_index")).set(*eventInfo, pairing_jet_indices[static_cast<int>(choice.indices[3])], sys);

        m_intDecos.at(decorName(algorithm, "j1_is_b")).set(*eventInfo, pairing_flav[static_cast<int>(choice.indices[0])], sys);
        m_intDecos.at(decorName(algorithm, "j2_is_b")).set(*eventInfo, pairing_flav[static_cast<int>(choice.indices[1])], sys);
        m_intDecos.at(decorName(algorithm, "j3_is_b")).set(*eventInfo, pairing_flav[static_cast<int>(choice.indices[2])], sys);
        m_intDecos.at(decorName(algorithm, "j4_is_b")).set(*eventInfo, pairing_flav[static_cast<int>(choice.indices[3])], sys);

        const int isTruthCorrect = m_isMC ? truthCorrect(pairing_jets, choice, tStatus) : -1;
        m_intDecos.at(decorName(algorithm, "truth_correct")).set(*eventInfo, isTruthCorrect, sys);
        
        if (m_isMC){
          setTruthParentDecorations(*eventInfo, algorithm, pairing_jets, choice, sys);
        }
      }

    }
    return StatusCode::SUCCESS;
  }

  void BaselineVarsResolvedAlg::PairingChoice::orderByPt(const std::vector<const xAOD::Jet*> &jets){

    // Order the two jets inside each candidate.
    if (jets.at(indices[1])->pt() > jets.at(indices[0])->pt())
    {
      std::swap(indices[0], indices[1]);
    }

    if (jets.at(indices[3])->pt() > jets.at(indices[2])->pt())
    {
      std::swap(indices[2], indices[3]);
    }

    // Build the two candidates after the internal jet ordering.
    const xAOD::JetFourMom_t h1 = jets.at(indices[0])->jetP4() + jets.at(indices[1])->jetP4();
    const xAOD::JetFourMom_t h2 = jets.at(indices[2])->jetP4() + jets.at(indices[3])->jetP4();

    // Candidate 1 is the higher-pT candidate.
    if (h2.Pt() > h1.Pt())
    {
      std::swap(indices[0], indices[2]);
      std::swap(indices[1], indices[3]);
      std::swap(dR1, dR2);
    }

  }  

  std::string BaselineVarsResolvedAlg::decorName(const std::string &algorithm, const std::string &var) const
  {
    return "resolved_" + algorithm + "_" + var + "_" + m_bTagWP.value();
  }

  std::string BaselineVarsResolvedAlg::baseDecorName(const std::string &var) const
  {
    return ("resolved_" + var + "_"+ m_bTagWP.value());
  }
  BaselineVarsResolvedAlg::PairingChoice BaselineVarsResolvedAlg::choosePairing(const std::string &algorithm,
                                                      const std::vector<const xAOD::Jet*> &jets) const
  {
    const std::array<std::array<unsigned int, 4>, 3> pairings{{
      {{0, 1, 2, 3}},
      {{0, 2, 1, 3}},
      {{0, 3, 1, 2}}
    }};

    PairingChoice best;
    float bestMetric = std::numeric_limits<float>::max();

    for (const auto &indices : pairings)
    {
      const xAOD::JetFourMom_t h1 = jets[indices[0]]->jetP4() + jets[indices[1]]->jetP4();
      const xAOD::JetFourMom_t h2 = jets[indices[2]]->jetP4() + jets[indices[3]]->jetP4();
      const float dR1 = xAOD::P4Helpers::deltaR(jets[indices[0]], jets[indices[1]]);
      const float dR2 = xAOD::P4Helpers::deltaR(jets[indices[2]], jets[indices[3]]);

      float metric = 0.0;
      if (algorithm == "mass_125")
      {
        metric = std::abs(h1.M() - m_higgsMass.value()) + std::abs(h2.M() - m_higgsMass.value());
      }
      else if (algorithm == "min_deltaR_max")
      {
        metric = std::max(dR1, dR2);
      }

      if (metric < bestMetric)
      {
        bestMetric = metric;
        best.indices = indices;
        best.metric = metric;
        best.dR1 = dR1;
        best.dR2 = dR2;
      }
    }
    best.orderByPt(jets);
    return best;
  }

  bool BaselineVarsResolvedAlg::isTruthBJet(const xAOD::Jet &jet) const
  {
    if (!m_isMC || m_truthBLabelDecorName.value().empty())
    {
      return false;
    }

    const SG::AuxElement::ConstAccessor<int> label(m_truthBLabelDecorName.value());
    if (!label.isAvailable(jet))
    {
      return false;
    }

    return label(jet) == 5;
  }

  unsigned long long BaselineVarsResolvedAlg::truthHiggsParentsMask(const xAOD::Jet &jet) const
  {
    if (!m_isMC || m_truthHiggsParentsMaskDecorName.value().empty())
    {
      return 0;
    }

    const SG::AuxElement::ConstAccessor<unsigned long long>
      mask(m_truthHiggsParentsMaskDecorName.value());
    if (!mask.isAvailable(jet))
    {
      return 0;
    }

    return mask(jet);
  }

  int BaselineVarsResolvedAlg::truthParentIndex(std::vector<const xAOD::Jet*> &jets, std::size_t jetIndex) const
  {
    if (jetIndex >= jets.size())
    {
      return -1;
    }

    const unsigned long long mask = truthHiggsParentsMask(*jets[jetIndex]);
    if (mask == 0)
    {
      return -1;
    }

    std::map<unsigned long long, int> maskToIndex;
    int nextIndex = 0;
    for (std::size_t i = 0; i < std::min<std::size_t>(4, jets.size()); ++i)
    {
      const unsigned long long thisMask = truthHiggsParentsMask(*jets[i]);
      if (thisMask == 0)
      {
        continue;
      }
      maskToIndex.try_emplace(thisMask, nextIndex++);
    }

    const auto it = maskToIndex.find(mask);
    return it == maskToIndex.end() ? -1 : it->second;
  }

  int BaselineVarsResolvedAlg::nTruthBJets(const std::vector<const xAOD::Jet*> &jets) const
  {
    int nTruthB = 0;
    for (std::size_t i = 0; i < std::min<std::size_t>(4, jets.size()); ++i)
    {
      if (isTruthBJet(*jets[i]))
      {
        ++nTruthB;
      }
    }
    return nTruthB;
  }

  int BaselineVarsResolvedAlg::nTruthHiggsBJets(const std::vector<const xAOD::Jet*> &jets) const
  {
    int nTruthHiggsB = 0;
    for (std::size_t i = 0; i < std::min<std::size_t>(4, jets.size()); ++i)
    {
      if (isTruthBJet(*jets[i]) && truthHiggsParentsMask(*jets[i]) != 0)
      {
        ++nTruthHiggsB;
      }
    }
    return nTruthHiggsB;
  }

  int BaselineVarsResolvedAlg::truthStatus(const std::vector<const xAOD::Jet*> &jets) const
  {
    if (!m_isMC)
    {
      return -1;
    }
    if (jets.size() < 4)
    {
      return 1;
    }

    const int nTruthB = nTruthBJets(jets);
    if (nTruthB != 4)
    {
      return 2;
    }

    const int nTruthHiggsB = nTruthHiggsBJets(jets);
    if (nTruthHiggsB != 4)
    {
      return 3;
    }

    std::map<unsigned long long, int> higgsCounts;
    for (std::size_t i = 0; i < 4; ++i)
    {
      ++higgsCounts[truthHiggsParentsMask(*jets[i])];
    }

    if (higgsCounts.size() != 2)
    {
      return 4;
    }
    for (const auto &entry : higgsCounts)
    {
      if (entry.second != 2)
      {
        return 4;
      }
    }

    return 5;
  }


  int BaselineVarsResolvedAlg::truthCorrect(const std::vector<const xAOD::Jet*> &jets, const PairingChoice &choice, int jet_truthStatus) const
  {
    if (jet_truthStatus != 5)
    {
      return -1;
    }

    const unsigned long long h0 = truthHiggsParentsMask(*jets.at(choice.indices[0]));
    const unsigned long long h1 = truthHiggsParentsMask(*jets.at(choice.indices[1]));
    const unsigned long long h2 = truthHiggsParentsMask(*jets.at(choice.indices[2]));
    const unsigned long long h3 = truthHiggsParentsMask(*jets.at(choice.indices[3]));

    return (h0 == h1 && h2 == h3 && h0 != h2) ? 1 : 0;
  }

  void BaselineVarsResolvedAlg::setTruthParentDecorations(const xAOD::EventInfo &eventInfo,
                                             const std::string &algorithm,
                                             std::vector<const xAOD::Jet*> &jets,
                                             const PairingChoice &choice,
                                             const CP::SystematicSet &sys) const
  {
    m_intDecos.at(decorName(algorithm, "truth_parent_j1")).set(eventInfo, truthParentIndex(jets, choice.indices[0]), sys);
    m_intDecos.at(decorName(algorithm, "truth_parent_j2")).set(eventInfo, truthParentIndex(jets, choice.indices[1]), sys);
    m_intDecos.at(decorName(algorithm, "truth_parent_j3")).set(eventInfo, truthParentIndex(jets, choice.indices[2]), sys);
    m_intDecos.at(decorName(algorithm, "truth_parent_j4")).set(eventInfo, truthParentIndex(jets, choice.indices[3]), sys);
  }

  void BaselineVarsResolvedAlg::setDefaults(const xAOD::EventInfo &eventInfo,
                               const CP::SystematicSet &sys) const
  {
    for (const std::string &var : m_commonFloatVars)
    {
      m_commonFloatDecos.at(baseDecorName(var)).set(eventInfo, -99, sys);
    }
    for (const std::string &var : m_commonIntVars)
    {
      m_commonIntDecos.at(baseDecorName(var)).set(eventInfo, -1, sys);
    }

    m_commonIntDecos.at(baseDecorName("reco_success")).set(eventInfo, 0, sys);

    for (const std::string &algorithm : m_pairingAlgorithms)
    {
      for (const std::string &var : m_floatVars)
      {
        m_floatDecos.at(decorName(algorithm, var)).set(eventInfo, -99.0, sys);
      }
      for (const std::string &var : m_intVars)
      {
        m_intDecos.at(decorName(algorithm, var)).set(eventInfo, -1, sys);
      }

    }
  }
}
