/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author

#include "BaselineVarsZlljCalibAlg.h"
#include "FourVectorOutBlock.h"
#include "DefaultOutputs.h"
#include "SystVariableCopier.h"

#include <format>

namespace XBBCALIB
{
  template <typename RecoLep> const xAOD::TruthParticle* matchTruthLepton(
      const RecoLep* reco,
      const xAOD::TruthParticleContainer* truthLeptons,
      double maxDR = 0.1)
  {
    if (!reco || !truthLeptons)
      return nullptr;
    const xAOD::TruthParticle* matched = nullptr;
    double minDR = maxDR;
    for (const xAOD::TruthParticle* truth : *truthLeptons)
    {
      if (!truth)
        continue;
      const double dR = reco->p4().DeltaR(truth->p4());
      if (dR < minDR)
      {
        minDR = dR;
        matched = truth;
      }
    }
    return matched;
  }

  StatusCode BaselineVarsZlljCalibAlg::initialize()
  {
    ATH_CHECK (m_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_XbbCalib_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_XbbCalib_muonHandle.initialize(m_systematicsList));

    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));
    
    ATH_CHECK(m_nLRJetsHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_nElectronsHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_nMuonsHandle.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK (m_passallcuts.initialize(m_systematicsList, m_eventHandle));
    
    // Initialize muons and electron with SF
    if (m_isMC)
    {
      // Leptons
      ATH_CHECK (m_electronHandle.initialize(m_systematicsList));
      ATH_CHECK (m_muonHandle.initialize(m_systematicsList));

      m_ele_SF = CP::SysReadDecorHandle<float>("effSF_" + m_eleWPName + "_%SYS%", this);
      ATH_CHECK(m_ele_SF.initialize(m_systematicsList, m_electronHandle));

      m_mu_SF = CP::SysReadDecorHandle<float>("effSF_" + m_muWPName + "_%SYS%", this);
      ATH_CHECK(m_mu_SF.initialize(m_systematicsList, m_muonHandle));

      ATH_CHECK(m_truthElectronHandle.initialize(m_systematicsList));
      ATH_CHECK(m_truthMuonHandle.initialize(m_systematicsList));

    }
  
    ATH_CHECK(m_Electron1SFHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_Electron2SFHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_Muon1SFHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_Muon2SFHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_SelectedLepton1SFHandle.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_SelectedLepton2SFHandle.initialize(m_systematicsList, m_eventHandle));

    m_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Zcand_", m_systematicsList, m_eventHandle);
    m_truth_z_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Zcand_truth_", m_systematicsList, m_eventHandle);

    // Maybe it is better to only save leding/sub-leadin lep?
    m_e0_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Electron1_", m_systematicsList, m_eventHandle);
    m_e1_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Electron2_", m_systematicsList, m_eventHandle);

    m_mu0_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Muon1_", m_systematicsList, m_eventHandle);
    m_mu1_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Muon2_", m_systematicsList, m_eventHandle);

    // Initialize other float branches
    // to do: implement new variables initializion with SystVariableCopier 
    for (const auto &var : m_floats_to_copy)
    {
      auto handle = std::make_unique<CP::SysWriteDecorHandle<float>>(
          m_copied_variable_prefix + var + "_%SYS%", this);
      ATH_CHECK(handle->initialize(m_systematicsList, m_eventHandle));
      m_Fbranches.emplace(m_copied_variable_prefix + var, std::move(handle));
    }
    // Initialize other int branches
    for (const auto &var : m_ints_to_copy)
    {
      auto handle = std::make_unique<CP::SysWriteDecorHandle<int>>(
          m_copied_variable_prefix + var + "_%SYS%", this);
      ATH_CHECK(handle->initialize(m_systematicsList, m_eventHandle));
      m_Ibranches.emplace(m_copied_variable_prefix + var, std::move(handle));
    }
    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());
    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsZlljCalibAlg::execute(const EventContext& ctx)
  {
    //Loop over all systs
    for (const auto& sys : m_systematicsList.systematicsVector())
    {
      
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (event, sys, ctx));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK (m_lrjetHandle.retrieve (lrjets, sys, ctx));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK(m_XbbCalib_muonHandle.retrieve(muons, sys, ctx));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK(m_XbbCalib_electronHandle.retrieve(electrons, sys, ctx));

      const xAOD::TruthParticleContainer *truthElectrons = nullptr;
      const xAOD::TruthParticleContainer *truthMuons = nullptr;
      if (m_isMC)
      {
        ANA_CHECK(m_truthElectronHandle.retrieve(truthElectrons, sys, ctx));
        ANA_CHECK(m_truthMuonHandle.retrieve(truthMuons, sys, ctx));
      }

      for (const std::string &string_var : m_floats_to_copy)
      {
        m_Fbranches.at(m_copied_variable_prefix + string_var)
            ->set(*event, defaults::def_float, sys);
      }

      for (const std::string &string_var : m_ints_to_copy)
      {
        m_Ibranches.at(m_copied_variable_prefix + string_var)
            ->set(*event, defaults::def_int, sys);
      }

      // dilepton kinematics
      TLorentzVector ll;
      TLorentzVector Leading_lep;
      TLorentzVector Subleading_lep;
      bool isMuonPair = false;
      bool foundTruthMatch = false;

      // Count electrons and muons
      size_t n_electrons = electrons->size();
      size_t n_muons = muons->size();
      m_nElectronsHandle.set(*event, n_electrons, sys);
      m_nMuonsHandle.set(*event, n_muons, sys);

      // Require exactly one same-flavour pair and no extra leptons of the
      // other flavour. This rejects 3(+)-lepton events (e.g. ee+mu, eemumu)
      // that would otherwise slip through if checked independently.
      const bool pass_ee   = (n_electrons == 2 && n_muons == 0);
      const bool pass_mumu = (n_muons == 2 && n_electrons == 0);
      const bool pass_selection = m_passallcuts.get(*event, sys);

      const xAOD::Electron *ele0 = nullptr;
      const xAOD::Electron *ele1 = nullptr;
      const xAOD::Muon *mu0 = nullptr;
      const xAOD::Muon *mu1 = nullptr;

      if (pass_ee && pass_selection) {
        ele0 = electrons->at(0);
        ele1 = electrons->at(1);
        m_e0_candidate_4vec->set(*event, *ele0, sys);
        m_e1_candidate_4vec->set(*event, *ele1, sys);
        Leading_lep = ele0->p4();
        Subleading_lep = ele1->p4();
        isMuonPair = false;
        if (m_isMC) {
          m_Electron1SFHandle.set(*event, m_ele_SF.get(*ele0, sys), sys);
          m_Electron2SFHandle.set(*event, m_ele_SF.get(*ele1, sys), sys);
          m_Muon1SFHandle.set(*event, defaults::def_float, sys);
          m_Muon2SFHandle.set(*event, defaults::def_float, sys);
          m_SelectedLepton1SFHandle.set(*event, m_ele_SF.get(*ele0, sys), sys);
          m_SelectedLepton2SFHandle.set(*event, m_ele_SF.get(*ele1, sys), sys);
        }
      } else if (pass_mumu && pass_selection) {
        mu0 = muons->at(0);
        mu1 = muons->at(1);
        m_mu0_candidate_4vec->set(*event, *mu0, sys);
        m_mu1_candidate_4vec->set(*event, *mu1, sys);
        Leading_lep = mu0->p4();
        Subleading_lep = mu1->p4();
        isMuonPair = true;
        if (m_isMC) {
          m_Muon1SFHandle.set(*event, m_mu_SF.get(*mu0, sys), sys);
          m_Muon2SFHandle.set(*event, m_mu_SF.get(*mu1, sys), sys);
          m_Electron1SFHandle.set(*event, defaults::def_float, sys);
          m_Electron2SFHandle.set(*event, defaults::def_float, sys);
          m_SelectedLepton1SFHandle.set(*event, m_mu_SF.get(*mu0, sys), sys);
          m_SelectedLepton2SFHandle.set(*event, m_mu_SF.get(*mu1, sys), sys);
        }
      } else if (m_isMC) {
        m_Electron1SFHandle.set(*event, defaults::def_float, sys);
        m_Electron2SFHandle.set(*event, defaults::def_float, sys);
        m_Muon1SFHandle.set(*event, defaults::def_float, sys);
        m_Muon2SFHandle.set(*event, defaults::def_float, sys);
        m_SelectedLepton1SFHandle.set(*event, defaults::def_float, sys);
        m_SelectedLepton2SFHandle.set(*event, defaults::def_float, sys);
      }

      ll = Leading_lep + Subleading_lep;

      // passed selection
      if (pass_selection) {
        // Truth Z from the selected reconstructed lepton pair
        if (m_isMC) {
          const xAOD::TruthParticle* truthLep0 = nullptr;
          const xAOD::TruthParticle* truthLep1 = nullptr;
          if (isMuonPair) {
            truthLep0 = matchTruthLepton(mu0, truthMuons);
            truthLep1 = matchTruthLepton(mu1, truthMuons);
          } else {
            truthLep0 = matchTruthLepton(ele0, truthElectrons);
            truthLep1 = matchTruthLepton(ele1, truthElectrons);
          }
          if (truthLep0 && truthLep1)  {
            const TLorentzVector truthZ = truthLep0->p4() + truthLep1->p4();
            m_truth_z_candidate_4vec->set(*event, truthZ, sys);
            foundTruthMatch = true;
          }
        }

      m_Ibranches.at(m_copied_variable_prefix + "isMuonPair")
            ->set(*event, isMuonPair, sys);
      m_z_candidate_4vec->set(*event, ll, sys);
      m_Fbranches.at(m_copied_variable_prefix + "pt_balance")
            ->set(*event, (Leading_lep.Pt() - Subleading_lep.Pt()) / ll.Pt(), sys);
      m_Fbranches.at(m_copied_variable_prefix + "dR")
            ->set(*event, Leading_lep.DeltaR(Subleading_lep), sys);
      m_Fbranches.at(m_copied_variable_prefix + "dPhi")
            ->set(*event, Leading_lep.DeltaPhi(Subleading_lep), sys);
      m_Fbranches.at(m_copied_variable_prefix + "dEta")
            ->set(*event, Leading_lep.Eta() - Subleading_lep.Eta(), sys);
      } else {
        m_z_candidate_4vec->setDefault(*event, sys);
      }

      if (!foundTruthMatch) m_truth_z_candidate_4vec->setDefault(*event, sys);

      // Count jets
      size_t n_lrjets = lrjets->size();
      m_nLRJetsHandle.set(*event, n_lrjets, sys); 
      
      // Define additional variables
      if (n_lrjets == 1)
      {
      m_Fbranches.at(m_copied_variable_prefix + "leplep_LR_ptsymmetry")
            ->set(*event, (ll.Pt() - lrjets->at(0)->pt()) / (ll.Pt() + lrjets->at(0)->pt()), sys);
      m_Fbranches.at(m_copied_variable_prefix + "leplep_LR_dEta")
            ->set(*event, abs(ll.Eta() - lrjets->at(0)->eta()), sys);
      } 
    }
    return StatusCode::SUCCESS;
  }

}
