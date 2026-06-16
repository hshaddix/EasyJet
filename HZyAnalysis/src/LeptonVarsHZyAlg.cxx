/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "LeptonVarsHZyAlg.h"
#include "HZyCommonUtils.h"
#include <AthenaKernel/Units.h>

namespace HZY
{
  LeptonVarsHZyAlg::LeptonVarsHZyAlg(const std::string &name,
                                     ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode LeptonVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("         LeptonVarsHZyAlg        \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElectronORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonORHandle.initialize(m_systematicsList));

    ATH_CHECK(m_ele_SF.initialize(m_systematicsList, m_HZyElectronHandle));
    ATH_CHECK(m_mu_SF.initialize(m_systematicsList, m_HZyMuonHandle));

    ATH_CHECK(m_electronsSTEP3Handle.initialize(m_systematicsList));
    ATH_CHECK(m_photonsSTEP4Handle.initialize(m_systematicsList));

    // For storing electrons/muons used in ZCand
    ATH_CHECK(m_ZCandElectronsHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ZCandMuonsHandle.initialize(m_systematicsList));

    // FSR applied on muons
    ATH_CHECK(m_fsrTool.retrieve());
    HZY::CommonUtils::setFSRTool(m_fsrTool.get());

    // Retrieve Egamma and Muon calib tools for Z mass constraint fit
    ATH_CHECK(m_egammaCalibTool.retrieve());
    ATH_MSG_INFO("Configuring private egamma tool for ZMassConstraintFit tool: " << m_egammaCalibTool.name());
    ATH_CHECK(m_muonCalibTool.retrieve());
    ATH_MSG_INFO("Configuring private muon tool for ZMassConstraintFit tool: " << m_muonCalibTool.name());

    // Add to initialize
    m_ZMassConstraint = std::make_unique<ZMassConstraint::ConstraintFit>("ZMassConstraint");
    ATH_CHECK(m_ZMassConstraint->setProperty("MuonCalibrationAndSmearingTool", "CP::MuonCalibTool/MuonCalibTool"));
    ATH_CHECK(m_ZMassConstraint->initialize());
    ATH_MSG_INFO("Initialized ZMassConstraint tool with private egamma and muon calibration tools");

    // Intialise syst-aware output decorators
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
      ATH_CHECK(
          m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode LeptonVarsHZyAlg::execute()
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "")
        continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys));

      const xAOD::ElectronContainer *HZyElectronOR = nullptr;
      ANA_CHECK(m_HZyElectronORHandle.retrieve(HZyElectronOR, sys));

      const xAOD::MuonContainer *HZyMuonOR = nullptr;
      ANA_CHECK(m_HZyMuonORHandle.retrieve(HZyMuonOR, sys));

      const xAOD::ElectronContainer *correctedSTEP3Electrons = nullptr;
      ANA_CHECK(m_electronsSTEP3Handle.retrieve(correctedSTEP3Electrons, sys));

      const xAOD::PhotonContainer *correctedSTEP4Photons = nullptr;
      ANA_CHECK(m_photonsSTEP4Handle.retrieve(correctedSTEP4Photons, sys));

      // Reset all decorations
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }

      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      // Collect all ll candidates: channel = 1 for ee, 2 for mumu
      std::vector<LLCandidate> ll_candidates;

      // Helper function: apply Z-mass constraint and fill fitted lepton p4
      auto finalizeCandidate = [&](LLCandidate& cand,
                                  const FSR::FsrCandidate* fsrCand,
                                  const TLorentzVector* fsrP4)
      {
        TLorentzVector c1_fit;
        TLorentzVector c2_fit;

        const float mll_constrained = ZMassConstraint(*m_ZMassConstraint,
                            event,
                            cand,
                            fsrCand,
                            fsrP4,
                            c1_fit,
                            c2_fit);
        if (mll_constrained > 0.f){
          cand.mll_constrained = mll_constrained;
          cand.c1_fit = c1_fit;
          cand.c2_fit = c2_fit;
        }
        else{
          cand.mll_constrained = cand.mll_raw;
          // fallback = original leptons
          cand.c1_fit = cand.l1->p4();
          cand.c2_fit = cand.l2->p4();
        }
        ll_candidates.push_back(cand);
      };

      // Electron pairs for Z candidate
      for (std::size_t i = 0; i < HZyElectronOR->size(); ++i)
      {
        const auto* e1 = HZyElectronOR->at(i);
        for (std::size_t j = i + 1; j < HZyElectronOR->size(); ++j)
        {
          const auto* e2 = HZyElectronOR->at(j);
          // Opposite charge
          if (e1->charge() + e2->charge() != 0){
            continue;
          }

          const float mll_raw = (e1->p4() + e2->p4()).M();
          LLCandidate cand{mll_raw, -99.f, 
            1, e1, e2, {}, {}, 
            false, -1, -99.f, {}, {}};
          
          // No FSR for electron channel
          finalizeCandidate(cand, nullptr, nullptr);
        }
      }

      // Muon pairs
      for (std::size_t i = 0; i < HZyMuonOR->size(); ++i)
      {
        const auto* m1 = HZyMuonOR->at(i);
        for (std::size_t j = i + 1; j < HZyMuonOR->size(); ++j)
        {
          const auto* m2 = HZyMuonOR->at(j);
          // Opposite charge
          if (m1->charge() + m2->charge() != 0){
            continue;
          }

          const float mll_raw = (m1->p4() + m2->p4()).M();
          LLCandidate cand{mll_raw, -99.f, 
            2, m1, m2, {}, {}, 
            false, -1, -99.f, {}, {}};

          // FSR correction for muon channel
          const bool hasFSR = applyFSRCorrection(
              m1, m2, correctedSTEP4Photons, correctedSTEP3Electrons, cand.fsrP4,
              cand.fsrIdx, cand.fsrDR, cand.fsrCand);
          cand.hasFSR = hasFSR;

          finalizeCandidate(cand,
                            cand.hasFSR ? &cand.fsrCand : nullptr,
                            cand.hasFSR ? &cand.fsrP4   : nullptr);
        }
      }

      // Save number of ll candidates
      m_Ibranches.at("LepLepZ_Size").set(*event, static_cast<int>(ll_candidates.size()), sys);

      // Select best candidate based on Z mass proximity (using constrained mass if FSR applied, otherwise raw mass)
      const double Z_MASS = 91187.6; // PDG Z mass in GeV
      const LLCandidate *bestCand = nullptr;
      double minDiff = std::numeric_limits<double>::max();
      for (const auto &cand : ll_candidates)
      {
        const double mass_for_sort = m_useConstrainedZMass ? cand.mll_constrained : cand.mll_raw;
        const double diff = std::abs(mass_for_sort - Z_MASS);
        if (diff < minDiff){
          minDiff = diff;
          bestCand = &cand;
        }
      }

      auto electrons_ZCand = std::make_unique<ConstDataVector<xAOD::ElectronContainer>>(SG::VIEW_ELEMENTS);
      auto muons_ZCand = std::make_unique<ConstDataVector<xAOD::MuonContainer>>(SG::VIEW_ELEMENTS);
      if (bestCand){
        const xAOD::IParticle *l1 = bestCand->l1;
        const xAOD::IParticle *l2 = bestCand->l2;

        // Get refitted leptons
        TLorentzVector c1 = bestCand->c1_fit;
        TLorentzVector c2 = bestCand->c2_fit;
        
        // Order leptons and fitted p4 consistently by original lepton pT
        if (l2->pt() > l1->pt()){
          std::swap(l1, l2);
          std::swap(c1, c2);
        }
        TLorentzVector ll_raw = l1->p4() + l2->p4();
        TLorentzVector ll_constrained = c1 + c2;

        // Decorate event with best candidate info
        m_Fbranches.at("LepLepZ_M_Raw").set(*event, bestCand->mll_raw, sys);
        m_Fbranches.at("LepLepZ_Pt_Raw").set(*event, ll_raw.Pt(), sys);
        m_Fbranches.at("LepLepZ_Eta_Raw").set(*event, ll_raw.Eta(), sys);
        m_Fbranches.at("LepLepZ_Phi_Raw").set(*event, ll_raw.Phi(), sys);
        m_Fbranches.at("LepLead_Pt_Raw").set(*event, l1->pt(), sys);
        m_Fbranches.at("LepLead_Eta_Raw").set(*event, l1->eta(), sys);
        m_Fbranches.at("LepLead_Phi_Raw").set(*event, l1->phi(), sys);
        m_Fbranches.at("LepLead_M_Raw").set(*event, l1->m(), sys);
        m_Fbranches.at("LepSub_Pt_Raw").set(*event, l2->pt(), sys);
        m_Fbranches.at("LepSub_Eta_Raw").set(*event, l2->eta(), sys);
        m_Fbranches.at("LepSub_Phi_Raw").set(*event, l2->phi(), sys);
        m_Fbranches.at("LepSub_M_Raw").set(*event, l2->m(), sys);

        float LeadLep_SF = 1.0f;
        float SubLep_SF = 1.0f;

        // Fill Z-candidate containers and lepton SFs
        if (bestCand->channel == 1) {
          const auto* el1 = dynamic_cast<const xAOD::Electron*>(l1);
          const auto *el2 = dynamic_cast<const xAOD::Electron *>(l2);
          electrons_ZCand->push_back(el1);
          electrons_ZCand->push_back(el2);
          if (m_isMC){
            LeadLep_SF = m_ele_SF.get(*el1, sys);
            SubLep_SF = m_ele_SF.get(*el2, sys);
          }
        }
        else if (bestCand->channel == 2) {
          const auto* mu1 = dynamic_cast<const xAOD::Muon*>(l1);
          const auto* mu2 = dynamic_cast<const xAOD::Muon*>(l2);
          muons_ZCand->push_back(mu1);
          muons_ZCand->push_back(mu2);
          if (m_isMC){
            LeadLep_SF = m_mu_SF.get(*mu1, sys);
            SubLep_SF = m_mu_SF.get(*mu2, sys);
          }
        }
        else{
          ATH_MSG_WARNING("Best Z-candidate has invalid channel: " << bestCand->channel);
        }

        m_Fbranches.at("LepLead_effSF").set(*event, LeadLep_SF, sys);
        m_Fbranches.at("LepSub_effSF").set(*event, SubLep_SF, sys);
        m_Ibranches.at("Lep_Channel").set(*event, bestCand->channel, sys);

        // Apply FSR only for muon channel
        if (bestCand->channel == 2){
          m_Ibranches.at("Muon_hasFsr").set(*event, bestCand->hasFSR ? 1 : 0, sys);
          m_Ibranches.at("Muon_Index_Fsr").set(*event, bestCand->hasFSR ? bestCand->fsrIdx : -99, sys);
          m_Fbranches.at("Muon_Fsr_dR").set(*event, bestCand->hasFSR ? bestCand->fsrDR : -99.0, sys);

          if (bestCand->hasFSR){
            m_Fbranches.at("LepLepZ_M_Fsr").set(*event, 
              (bestCand->l1->p4() + bestCand->l2->p4() + bestCand->fsrP4).M(), sys);
          }
          else{
            m_Fbranches.at("LepLepZ_M_Fsr").set(*event, bestCand->mll_raw, sys);
          }
        }

        m_Fbranches.at("LepLepZ_M_Cstr").set(*event, bestCand->mll_constrained, sys);
        m_Fbranches.at("LepLepZ_Pt_Cstr").set(*event, ll_constrained.Pt(), sys);
        m_Fbranches.at("LepLepZ_Eta_Cstr").set(*event, ll_constrained.Eta(), sys);
        m_Fbranches.at("LepLepZ_Phi_Cstr").set(*event, ll_constrained.Phi(), sys);
        m_Fbranches.at("LepLead_Pt_Cstr").set(*event, c1.Pt(), sys);
        m_Fbranches.at("LepLead_Eta_Cstr").set(*event, c1.Eta(), sys);
        m_Fbranches.at("LepLead_Phi_Cstr").set(*event, c1.Phi(), sys);
        m_Fbranches.at("LepLead_M_Cstr").set(*event, c1.M(), sys);
        m_Fbranches.at("LepSub_Pt_Cstr").set(*event, c2.Pt(), sys);
        m_Fbranches.at("LepSub_Eta_Cstr").set(*event, c2.Eta(), sys);
        m_Fbranches.at("LepSub_Phi_Cstr").set(*event, c2.Phi(), sys);
        m_Fbranches.at("LepSub_M_Cstr").set(*event, c2.M(), sys);
      }
      else{
        ATH_MSG_DEBUG("No valid Z candidate found. Setting defaults.");
      }
      // Record both containers for every event.
      // One or both may be empty, but the StoreGate object will exist.
      ANA_CHECK(m_ZCandElectronsHandle.record(std::move(electrons_ZCand), sys));
      ANA_CHECK(m_ZCandMuonsHandle.record(std::move(muons_ZCand), sys));

      // Count electrons and muons
      int n_HZyEle = 0;
      int n_HZyMu = 0;
      n_HZyEle = HZyElectrons->size();
      n_HZyMu = HZyMuons->size();
      m_Ibranches.at("nElectrons").set(*event, n_HZyEle, sys);
      m_Ibranches.at("nMuons").set(*event, n_HZyMu, sys);
      m_Ibranches.at("nLeptons")
          .set(*event, HZyElectrons->size() + HZyMuons->size(), sys);
    }

    return StatusCode::SUCCESS;
  }

  bool LeptonVarsHZyAlg::applyFSRCorrection(
      const xAOD::Muon *mu1, const xAOD::Muon *mu2,
      const xAOD::PhotonContainer *allPhotons,
      const xAOD::ElectronContainer *allElectrons, TLorentzVector &fsrP4_out,
      int &fsrMuonIndex_out, float &dR_fsr_out, FSR::FsrCandidate &fsrCand_out)
  {
    fsrP4_out.SetPtEtaPhiM(0, 0, 0, 0);
    fsrMuonIndex_out = -1;
    dR_fsr_out = -99.0f;

    const TLorentzVector p1 = mu1->p4();
    const TLorentzVector p2 = mu2->p4();
    const double mll = (p1 + p2).M();

    bool hasBest = false;
    double highestEt = -99.;

    FSR::FsrCandidate bestCand;
    TLorentzVector bestP4;

    const std::array<const xAOD::Muon*, 2> muons{mu1, mu2};
    for (std::size_t i = 0; i < muons.size(); ++i)
    {
      FSR::FsrCandidate cand;
      TLorentzVector p4;

      if (!HZY::CommonUtils::findFSR(muons[i], allPhotons, allElectrons, cand, p4))
        continue;

      if (cand.Et > highestEt){
        highestEt = cand.Et;
        bestCand = cand;
        bestP4 = p4;

        fsrMuonIndex_out = static_cast<int>(i); // 0 = mu1, 1 = mu2;
        dR_fsr_out = cand.deltaR;
        hasBest = true;
      }
    }

    if (!hasBest){
      return false;
    }

    const double mll_fsr = ((p1 + p2) + bestP4).M();
    const auto &mllWindow_infsr = m_mllWindow.value();
    ANA_MSG_DEBUG("applyFSRCorrection: dilepton with FSR mass required in [66, 89] and < 100 GeV");
    if ((mll > mllWindow_infsr.at(0) * Athena::Units::GeV &&
         mll < mllWindow_infsr.at(1) * Athena::Units::GeV) &&
        mll_fsr < m_mllFsrThre * Athena::Units::GeV){
      fsrP4_out = bestP4;
      fsrCand_out = bestCand;
      return true;
    }

    // Candidate found but failed FSR mass-window requirement.
    fsrMuonIndex_out = -1;
    dR_fsr_out = -99.0f;
    fsrP4_out.SetPtEtaPhiM(0., 0., 0., 0.);

    return false;
  }

  float LeptonVarsHZyAlg::ZMassConstraint(
      ZMassConstraint::ConstraintFit &fitter, const xAOD::EventInfo *event,
      const LLCandidate &cand, const FSR::FsrCandidate *fsrCand,
      const TLorentzVector *fsrP4,
      TLorentzVector &c1_out,
      TLorentzVector &c2_out)
  {
    constexpr float BAD = -99.0f;
    c1_out.SetPtEtaPhiM(0, 0, 0, 0);
    c2_out.SetPtEtaPhiM(0, 0, 0, 0);
    if (!event){
      ATH_MSG_ERROR("ZMassConstraint: EventInfo pointer is null");
      return BAD;
    }

    if (!cand.l1 || !cand.l2){
      ATH_MSG_WARNING("ZMassConstraint: null lepton pointer");
      return BAD;
    }

    // Determine channel: 1 = ee, 2 = μμ
    // Only ee or μμ pairs make physical sense for Z mass constraint
    if (cand.channel != 1 && cand.channel != 2)
    {
      ATH_MSG_WARNING("ZMassConstraint: candidate is not ee or μμ. Skipping.");
      return BAD;
    }

    // Prepare fit input
    ZMassConstraint::ConstraintFitInput input;

    // -------------------------------
    //  Electron channel
    // -------------------------------
    if (cand.channel == 1)
    {
      const auto* el1 = dynamic_cast<const xAOD::Electron *>(cand.l1);
      const auto* el2 = dynamic_cast<const xAOD::Electron *>(cand.l2);

      if (!el1 || !el2) {
        ATH_MSG_WARNING("ZMassConstraint: ee candidate contains non-electron object(s)");
        return BAD;
      }

      const float Eres1 = el1->caloCluster()->e() * m_egammaCalibTool->getResolution(*el1);
      const float Eres2 = el2->caloCluster()->e() * m_egammaCalibTool->getResolution(*el2);
      fitter.addParticle(*el1, Eres1, input);
      fitter.addParticle(*el2, Eres2, input);
    }

    // -------------------------------
    //  Muon channel
    // -------------------------------
    else if (cand.channel == 2)
    {
      const auto* mu1 = dynamic_cast<const xAOD::Muon *>(cand.l1);
      const auto* mu2 = dynamic_cast<const xAOD::Muon *>(cand.l2);

      if (!mu1 || !mu2){
        ATH_MSG_WARNING("ZMassConstraint: null muon pointer!");
        return BAD;
      }

      // MassConstraint muon enum
      ZMassConstraint::MassConstraintMuonType muType = ZMassConstraint::isCombMCMT;

      // Add muons to fit
      fitter.addParticle(*mu1, input, muType);
      fitter.addParticle(*mu2, input, muType);
    }

    // Optional FSR photon
    if (fsrCand || fsrP4) {
      if (!fsrCand || !fsrP4 || !fsrCand->particle) {
        ATH_MSG_WARNING("ZMassConstraint: incomplete FSR input. Skipping FSR in fit.");
      } else {
        fitter.addFSRParticle(*fsrCand->particle, *fsrP4, input);
      }
    }

    ZMassConstraint::ConstraintFitOutput result;
    if (fitter.doMassFit(input, result).isFailure()){
      ATH_MSG_WARNING("ZMassConstraint: doMassFit failed");
      return BAD;
    }

    c1_out = result.getConstituentFourVector(0);
    c2_out = result.getConstituentFourVector(1);
    auto sane = [](const TLorentzVector &v){
      return std::isfinite(v.Pt()) &&
             std::isfinite(v.Eta()) &&
             std::isfinite(v.Phi()) && 
             std::isfinite(v.M()) && 
             v.Pt() > 0.0;
    };

    if (!sane(c1_out) || !sane(c2_out))
    {
      ATH_MSG_WARNING("ZMassConstraint: non-sane fitted lepton four-vectors");
      return BAD;
    }
    
    TLorentzVector Z_constr;
    result.getCompositeFourVector(Z_constr);

    ATH_MSG_DEBUG("ZMassConstraint: fitted lepton masses = " << c1_out.M() << " , " << c2_out.M());

    float mErrUnc = fitter.getMassError(input);
    float mErrCon = fitter.getMassError(result);
    ATH_MSG_DEBUG("ZMassConstraint: mass error (before/after) = "
                  << mErrUnc << " / " << mErrCon);
    double mll_constrained = (c1_out+c2_out).M();
    ATH_MSG_DEBUG("ZMassConstraint: constrained mass = " << mll_constrained); 
    return mll_constrained;
  }

} // namespace HZY
