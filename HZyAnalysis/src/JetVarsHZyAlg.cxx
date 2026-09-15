/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "JetVarsHZyAlg.h"
#include <AthenaKernel/Units.h>
#include <AthContainers/AuxElement.h>

namespace HZY
{
  StatusCode JetVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("           JetVarsHZyAlg         \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORExtraHandle.initialize(m_systematicsList));
    ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_HZyJetORExtraHandle));
    ATH_CHECK(m_jvtEffSF.initialize(m_systematicsList, m_HZyJetORExtraHandle));

    for (const std::string &string_var : m_floatVariables)
    {
      CP::SysWriteDecorHandle<float> whandle{string_var + "_%SYS%", this};
      m_Fbranches.emplace(string_var, whandle);
      ATH_CHECK(m_Fbranches.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_intVariables)
    {
      CP::SysWriteDecorHandle<int> whandle{var + "_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_floatVecVariables)
    {
      CP::SysWriteDecorHandle<std::vector<float>> whandle{var + "_%SYS%", this};
      m_FvecBranches.emplace(var, whandle);
      ATH_CHECK(m_FvecBranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode JetVarsHZyAlg::execute(const EventContext& ctx)
  {
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::JetContainer *HZyJets = nullptr;
      ANA_CHECK(m_HZyJetHandle.retrieve(HZyJets, sys, ctx));

      const xAOD::JetContainer *HZyJetsOR = nullptr;
      ANA_CHECK(m_HZyJetORHandle.retrieve(HZyJetsOR, sys, ctx));

      const xAOD::JetContainer *HZyJetsORExtra = nullptr;
      ANA_CHECK(m_HZyJetORExtraHandle.retrieve(HZyJetsORExtra, sys, ctx));

      // Reset scalars
      for (const std::string &string_var : m_floatVariables)
        m_Fbranches.at(string_var).set(*event, -99., sys);
      for (const auto &var : m_intVariables)
        m_Ibranches.at(var).set(*event, -99, sys);

      // --- Scalar variables ---
      // Mjj, nJets from HZyJetsORExtra
      m_Ibranches.at("nJets").set(*event, static_cast<int>(HZyJetsORExtra->size()), sys);
      if (HZyJetsORExtra->size() >= 2)
      {
        TLorentzVector jet_pair = HZyJetsORExtra->at(0)->p4() + HZyJetsORExtra->at(1)->p4();
        m_Fbranches.at("Mjj").set(*event, jet_pair.M(), sys);
      }

      // b-jet variables from HZyJetsORExtra
      int nbJets = 0;
      const xAOD::Jet *leadBjet = nullptr;
      const xAOD::Jet *subBjet = nullptr;
      for (const auto *jet : *HZyJetsORExtra) {
        if (!jet) continue;
        if (!m_isBtag.get(*jet, sys)) continue;
        nbJets++;
        if (!leadBjet || jet->pt() > leadBjet->pt()) {
          subBjet = leadBjet;
          leadBjet = jet;
        } else if (!subBjet || jet->pt() > subBjet->pt()) {
          subBjet = jet;
        }
      }
      m_Ibranches.at("nbJets").set(*event, nbJets, sys);
      if (leadBjet) {
        m_Fbranches.at("leadBjet_Pt").set(*event, leadBjet->pt(), sys);
        m_Fbranches.at("leadBjet_Eta").set(*event, leadBjet->eta(), sys);
        m_Fbranches.at("leadBjet_Phi").set(*event, leadBjet->phi(), sys);
        m_Fbranches.at("leadBjet_M").set(*event, leadBjet->m(), sys);
      }
      if (subBjet) {
        m_Fbranches.at("subBjet_Pt").set(*event, subBjet->pt(), sys);
        m_Fbranches.at("subBjet_Eta").set(*event, subBjet->eta(), sys);
        m_Fbranches.at("subBjet_Phi").set(*event, subBjet->phi(), sys);
        m_Fbranches.at("subBjet_M").set(*event, subBjet->m(), sys);
      }

      // --- Per-jet vector variables ---
      // Per-jet attribute accessors (avoid deprecated isAvailable/getAttribute)
      static const SG::ConstAccessor<int> acc_PartonTruthLabelID("PartonTruthLabelID");
      static const SG::ConstAccessor<float> acc_QGScore("QGTransformer_ConstScore");
      static const SG::ConstAccessor<float> acc_NNJvt("NNJvt");
      static const SG::ConstAccessor<std::vector<int>> acc_NumTrkPt500("NumTrkPt500");
      static const SG::ConstAccessor<std::vector<int>> acc_NumTrkPt1000("NumTrkPt1000");
      static const SG::ConstAccessor<int> acc_NTracks("DFCommonJets_QGTagger_NTracks");

      // Local vectors to accumulate per-jet values
      std::vector<float> j_pt, j_eta, j_phi, j_mass, j_e;
      std::vector<float> j_PartonTruthLabelID, j_QGScore, j_NNJvt, j_jvtEffSF;
      std::vector<float> j_NumTrkPt500, j_NumTrkPt1000, j_NTracks;

      for (const auto *jet : *HZyJetsORExtra) {
        if (!jet) continue;
        j_pt.push_back(jet->pt() / 1000.f);
        j_eta.push_back(jet->eta());
        j_phi.push_back(jet->phi());
        j_mass.push_back(jet->m() / 1000.f);
        j_e.push_back(jet->e() / 1000.f);

        j_PartonTruthLabelID.push_back(
            acc_PartonTruthLabelID.isAvailable(*jet)
                ? static_cast<float>(acc_PartonTruthLabelID(*jet)) : -2.f);

        j_QGScore.push_back(
            acc_QGScore.isAvailable(*jet) ? acc_QGScore(*jet) : -1.f);

        j_NNJvt.push_back(
            acc_NNJvt.isAvailable(*jet) ? acc_NNJvt(*jet) : -1.f);

        // jvt_effSF is written by CP::JvtEfficiencyAlg, which is scheduled
        // for MC onl--> reading it on data throws SG::ExcBadAuxVar
        j_jvtEffSF.push_back(m_isMC ? m_jvtEffSF.get(*jet, sys) : -1.f);

        {
          if (acc_NumTrkPt500.isAvailable(*jet)) {
            const auto& v = acc_NumTrkPt500(*jet);
            j_NumTrkPt500.push_back(v.empty() ? -1.f : static_cast<float>(v[0]));
          } else j_NumTrkPt500.push_back(-1.f);
        }
        {
          if (acc_NumTrkPt1000.isAvailable(*jet)) {
            const auto& v = acc_NumTrkPt1000(*jet);
            j_NumTrkPt1000.push_back(v.empty() ? -1.f : static_cast<float>(v[0]));
          } else j_NumTrkPt1000.push_back(-1.f);
        }

        j_NTracks.push_back(
            acc_NTracks.isAvailable(*jet)
                ? static_cast<float>(acc_NTracks(*jet)) : -1.f);

      }

      // Write vector decorations
      m_FvecBranches.at("jets_pt").set(*event, j_pt, sys);
      m_FvecBranches.at("jets_eta").set(*event, j_eta, sys);
      m_FvecBranches.at("jets_phi").set(*event, j_phi, sys);
      m_FvecBranches.at("jets_mass").set(*event, j_mass, sys);
      m_FvecBranches.at("jets_e").set(*event, j_e, sys);
      m_FvecBranches.at("jets_PartonTruthLabelID").set(*event, j_PartonTruthLabelID, sys);
      m_FvecBranches.at("jets_QGScore").set(*event, j_QGScore, sys);
      m_FvecBranches.at("jets_NNJvt").set(*event, j_NNJvt, sys);
      m_FvecBranches.at("jets_jvtEffSF").set(*event, j_jvtEffSF, sys);
      m_FvecBranches.at("jets_NumTrkPt500").set(*event, j_NumTrkPt500, sys);
      m_FvecBranches.at("jets_NumTrkPt1000").set(*event, j_NumTrkPt1000, sys);
      m_FvecBranches.at("jets_NTracks").set(*event, j_NTracks, sys);
    }
    return StatusCode::SUCCESS;
  }
} // namespace HZY
