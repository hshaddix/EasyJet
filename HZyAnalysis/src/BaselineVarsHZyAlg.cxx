/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "BaselineVarsHZyAlg.h"
#include <AthenaKernel/Units.h>
#include <TLorentzVector.h>
#include <PathResolver/PathResolver.h>

namespace HZY
{
  StatusCode BaselineVarsHZyAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     BaselineVarsHZyAlg      \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyJetORExtraHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyPhotonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyElectronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_HZyMuonHandle.initialize(m_systematicsList));

    ATH_CHECK(m_HZyPhotonORHandle.initialize(m_systematicsList));
    ATH_CHECK(m_FsrObjectHandle.initialize(m_systematicsList));

    // Setup read decorations for Z candidate dilepton variables (float)
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
    // Setup read decorations for signal photon with tight ID /loos Iso
    for (const std::string &string_var : m_floatVariables_photons)
    {
      CP::SysReadDecorHandle<float> var{string_var + "_%SYS%", this};
      m_Fbranches_photons.emplace(string_var, var);
      ATH_CHECK(m_Fbranches_photons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }
    for (const std::string &string_var : m_intVariables_photons)
    {
      CP::SysReadDecorHandle<int> var{string_var + "_%SYS%", this};
      m_Ibranches_photons.emplace(string_var, var);
      ATH_CHECK(m_Ibranches_photons.at(string_var).initialize(m_systematicsList, m_eventHandle));
    }

    // Intialise syst-aware output decorators
    for (const std::string &var : m_floatVariables)
    {
      CP::SysWriteDecorHandle<float> whandle{var + "_%SYS%", this};
      m_Fbranches.emplace(var, whandle);
      ATH_CHECK(m_Fbranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    for (const std::string &var : m_intVariables)
    {
      CP::SysWriteDecorHandle<int> whandle{var + "_%SYS%", this};
      m_Ibranches.emplace(var, whandle);
      ATH_CHECK(m_Ibranches.at(var).initialize(m_systematicsList, m_eventHandle));
    }

    // Load all ONNX models: 5 families x 4 folds each
    // VBF: 25 features, 4 folds
    const std::vector<std::string> vbfFiles = {
        "VBF_model_0.onnx", "VBF_model_1.onnx",
        "VBF_model_2.onnx", "VBF_model_3.onnx"};
    for (size_t i = 0; i < vbfFiles.size(); ++i)
      loadModel(ModelFamily::VBF, m_modelFolder + "/" + vbfFiles[i], 25);

    // ggF_ee: 17 features, 4 folds
    const std::vector<std::string> ggFeeFiles = {
        "ggF_BDT_ee_0.onnx", "ggF_BDT_ee_1.onnx",
        "ggF_BDT_ee_2.onnx", "ggF_BDT_ee_3.onnx"};
    for (size_t i = 0; i < ggFeeFiles.size(); ++i)
      loadModel(ModelFamily::ggF_ee, m_modelFolder + "/" + ggFeeFiles[i], 17);

    // ggF_mm: 17 features, 4 folds
    const std::vector<std::string> ggFmmFiles = {
        "ggF_BDT_mumu_0.onnx", "ggF_BDT_mumu_1.onnx",
        "ggF_BDT_mumu_2.onnx", "ggF_BDT_mumu_3.onnx"};
    for (size_t i = 0; i < ggFmmFiles.size(); ++i)
      loadModel(ModelFamily::ggF_mm, m_modelFolder + "/" + ggFmmFiles[i], 17);

    // RelpT_ee: 17 features, 4 folds
    const std::vector<std::string> relpTeeFiles = {
        "ggF_BDT_ee_relpT40_0.onnx", "ggF_BDT_ee_relpT40_1.onnx",
        "ggF_BDT_ee_relpT40_2.onnx", "ggF_BDT_ee_relpT40_3.onnx"};
    for (size_t i = 0; i < relpTeeFiles.size(); ++i)
      loadModel(ModelFamily::RelpT_ee, m_modelFolder + "/" + relpTeeFiles[i], 17);

    // RelpT_mm: 17 features, 4 folds
    const std::vector<std::string> relpTmmFiles = {
        "ggF_BDT_mumu_relpT40_0.onnx", "ggF_BDT_mumu_relpT40_1.onnx",
        "ggF_BDT_mumu_relpT40_2.onnx", "ggF_BDT_mumu_relpT40_3.onnx"};
    for (size_t i = 0; i < relpTmmFiles.size(); ++i)
      loadModel(ModelFamily::RelpT_mm, m_modelFolder + "/" + relpTmmFiles[i], 17);

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsHZyAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "") continue;

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::PhotonContainer *HZyPhotons = nullptr;
      ANA_CHECK(m_HZyPhotonHandle.retrieve(HZyPhotons, sys, ctx));

      const xAOD::ElectronContainer *HZyElectrons = nullptr;
      ANA_CHECK(m_HZyElectronHandle.retrieve(HZyElectrons, sys, ctx));

      const xAOD::MuonContainer *HZyMuons = nullptr;
      ANA_CHECK(m_HZyMuonHandle.retrieve(HZyMuons, sys, ctx));

      const xAOD::JetContainer *HZyJetOR = nullptr;
      ANA_CHECK(m_HZyJetORExtraHandle.retrieve(HZyJetOR, sys, ctx));

      const xAOD::PhotonContainer *HZyPhotonOR = nullptr;
      ANA_CHECK(m_HZyPhotonORHandle.retrieve(HZyPhotonOR, sys, ctx));

      const xAOD::IParticleContainer* fsrObject = nullptr;
      ANA_CHECK(m_FsrObjectHandle.retrieve(fsrObject, sys, ctx));

      // Reset all decorations to default values at the start of each systematics loop
      for (const std::string &string_var : m_floatVariables)
      {
        m_Fbranches.at(string_var).set(*event, -99., sys);
      }
      for (const auto &var : m_intVariables)
      {
        m_Ibranches.at(var).set(*event, -99, sys);
      }

      // Ensure all ONNX inputs always have a defined value.
      FeatureMap features;
      for (const auto& name : vbfFeatures)
        features[name] = -99.f;
      for (const auto& name : ggFRelpTFeatures)
        features[name] = -99.f;

      // PhPy8EG_PDF4LHC21_ggH_NNLOPS with DSID 603798: nominal weight is at index 88
      if (m_isMC){
        const auto &w = event->mcEventWeights();
        size_t i = event->mcChannelNumber() == 603798 ? 88u : 0u;
        m_Fbranches.at("MC_Weight").set(*event, w.size() > i ? w[i] : (w.empty() ? 1.0f : w[0]), sys);
        m_Fbranches.at("BeamSpotWeight").set(*event, event->beamSpotWeight(), sys);
      }

      // Reconstruct dilepton and photon final state
      TLorentzVector llRawP4;
      const float llPtRaw = m_Fbranches_leptons.at("LepLepZ_Pt_Raw").get(*event, sys);
      const float llEtaRaw = m_Fbranches_leptons.at("LepLepZ_Eta_Raw").get(*event, sys);
      const float llPhiRaw = m_Fbranches_leptons.at("LepLepZ_Phi_Raw").get(*event, sys);
      const float llMRaw = m_Fbranches_leptons.at("LepLepZ_M_Raw").get(*event, sys);
      llRawP4.SetPtEtaPhiM(llPtRaw, llEtaRaw, llPhiRaw, llMRaw);
      
      // Z candidate with Fsr correction and ZMass constraint
      TLorentzVector llCstrP4;
      llCstrP4.SetPtEtaPhiM(
          m_Fbranches_leptons.at("LepLepZ_Pt_Cstr").get(*event, sys),
          m_Fbranches_leptons.at("LepLepZ_Eta_Cstr").get(*event, sys),
          m_Fbranches_leptons.at("LepLepZ_Phi_Cstr").get(*event, sys),
          m_Fbranches_leptons.at("LepLepZ_M_Cstr").get(*event, sys));

      // signal photon with tightID and looseISO
      TLorentzVector TIdLIsoPhotonP4;
      const float sigPhotonPt = m_Fbranches_photons.at("Ph_Pt_Raw").get(*event, sys);
      const float sigPhotonEta = m_Fbranches_photons.at("Ph_Eta_Raw").get(*event, sys);
      const float sigPhotonPhi = m_Fbranches_photons.at("Ph_Phi_Raw").get(*event, sys);
      TIdLIsoPhotonP4.SetPtEtaPhiM(sigPhotonPt, sigPhotonEta, sigPhotonPhi, 0.f);
      TLorentzVector llgRawP4_onnx = llRawP4 + TIdLIsoPhotonP4;
      
      // Define P4 for llg system
      TLorentzVector llgCstrP4 = {0, 0, 0, 0};
      const xAOD::Photon* selHZyLoosePhoton =
        (HZyPhotonOR && !HZyPhotonOR->empty()) ? HZyPhotonOR->at(0) : nullptr;
      if (!selHZyLoosePhoton){
        ATH_MSG_DEBUG("No cleaned photon found, skipping photon variables.");
      }
      else {
        const TLorentzVector llgRawP4 = llRawP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Raw").set(*event, llgRawP4.M(), sys);
        m_Fbranches.at("LLGam_Pt_Raw").set(*event, llgRawP4.Pt(), sys);
        m_Fbranches.at("LLGam_Eta_Raw").set(*event, llgRawP4.Eta(), sys);
        m_Fbranches.at("LLGam_Phi_Raw").set(*event, llgRawP4.Phi(), sys);

        const xAOD::IParticle *fsr =
            (fsrObject && !fsrObject->empty()) ? fsrObject->at(0) : nullptr;
        TLorentzVector fsrP4;
        if (fsr){
          fsrP4 = fsr->p4();
        }
        const TLorentzVector llgFsrP4 = llRawP4 + fsrP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Fsr").set(*event, llgFsrP4.M(), sys);

        llgCstrP4 = llCstrP4 + selHZyLoosePhoton->p4();
        m_Fbranches.at("LLGam_M_Cstr").set(*event, llgCstrP4.M(), sys);
        m_Fbranches.at("LLGam_Pt_Cstr").set(*event, llgCstrP4.Pt(), sys);
        m_Fbranches.at("LLGam_Eta_Cstr").set(*event, llgCstrP4.Eta(), sys);
        m_Fbranches.at("LLGam_Phi_Cstr").set(*event, llgCstrP4.Phi(), sys);
        m_Fbranches.at("LLGam_RelPhpT_Cstr").set(*event, selHZyLoosePhoton->pt() / llgCstrP4.M(), sys);
      }

      setFloatAndFeature(event, "ll_pt", llPtRaw > 0.f ? llPtRaw / Athena::Units::GeV : -99.0f, sys, features);
      setFloatAndFeature(event, "ll_eta", llEtaRaw, sys, features);
      setFloatAndFeature(event, "ph_pt", sigPhotonPt > 0.f ? sigPhotonPt / Athena::Units::GeV : -99.0f, sys, features);
      setFloatAndFeature(event, "ph_eta", sigPhotonEta, sys, features);
      if (llPtRaw > 0.f && sigPhotonPt > 0.f) {
        setFloatAndFeature(event, "llg_pt", llgRawP4_onnx.Pt() / Athena::Units::GeV, sys, features);
        setFloatAndFeature(event, "llg_eta", llgRawP4_onnx.Eta(), sys, features);
      }
      // constrained mass for relative-pT denominator
      const float llg_m_Cstr_forRel = llgCstrP4.M() > 0.f ? llgCstrP4.M() : llgRawP4_onnx.M();
      // Relative pT (dimensionless, using constrained mass as denominator)
      const float ll_pt_rel = (llg_m_Cstr_forRel > 0.f) ? llRawP4.Pt() / llg_m_Cstr_forRel : -99.0f;
      const float ph_pt_rel = (llg_m_Cstr_forRel > 0.f) ? TIdLIsoPhotonP4.Pt() / llg_m_Cstr_forRel : -99.0f;
      setFloatAndFeature(event, "ll_pt_rel", ll_pt_rel, sys, features);
      setFloatAndFeature(event, "ph_pt_rel", ph_pt_rel, sys, features);
      setFloatAndFeature(event, "llg_pt_rel", llg_m_Cstr_forRel > 0.f ? 
        static_cast<float>(llgRawP4_onnx.Pt() / llg_m_Cstr_forRel) : -99.f, sys, features);

      // Delta eta and delta phi between Z and photon
      setFloatAndFeature(event, "llg_deta_Zy", std::abs(llRawP4.Eta() - TIdLIsoPhotonP4.Eta()), sys, features);
      setFloatAndFeature(event, "llg_dphi_Zy", std::abs(TVector2::Phi_mpi_pi(llRawP4.Phi() - TIdLIsoPhotonP4.Phi())), sys, features);

      // Read individual lepton 4-vectors from LeptonVarsHZyAlg
      TLorentzVector leadLepP4, subLepP4;
      leadLepP4.SetPtEtaPhiM(
          m_Fbranches_leptons.at("LepLead_Pt_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepLead_Eta_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepLead_Phi_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepLead_M_Raw").get(*event, sys));
      subLepP4.SetPtEtaPhiM(
          m_Fbranches_leptons.at("LepSub_Pt_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepSub_Eta_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepSub_Phi_Raw").get(*event, sys),
          m_Fbranches_leptons.at("LepSub_M_Raw").get(*event, sys));

      // Add CMS angle variables at HZy rest frame
      getCMSAngleVars(event, leadLepP4, subLepP4, TIdLIsoPhotonP4, sys, features);

      // Add VBF topology variables
      getVBFTopologyVars(event, HZyJetOR, llRawP4, TIdLIsoPhotonP4, llgRawP4_onnx, sys, features);

      // Apply BDT models and Event categorization (before this, all variables must be saved in baseline variables list)
      // Read channel from Lep_Channel (from LeptonVarsHZyAlg)
      const int channel = m_Ibranches_leptons.at("Lep_Channel").get(*event, sys);

      // Read VBF_N_j and ph_pt_rel for category
      const float vbf_nj = features.at("VBF_N_j");
      const float ph_pt_rel_val = features.at("ph_pt_rel");

      std::vector<float> vbfInputs;
      vbfInputs.reserve(vbfFeatures.size());

      for (const auto& name : vbfFeatures)
        vbfInputs.push_back(features.at(name));

      std::vector<float> ggFRelpTInputs;
      ggFRelpTInputs.reserve(ggFRelpTFeatures.size());

      for (const auto& name : ggFRelpTFeatures)
        ggFRelpTInputs.push_back(features.at(name));

      const int evtNum = event->eventNumber();
      const float vbfScore = getScore(ModelFamily::VBF, vbfInputs, evtNum);

      float ggFScore = -1.f;
      float relpTScore = -1.f;
      if (channel == 1)
      {
        ggFScore = getScore(ModelFamily::ggF_ee, ggFRelpTInputs, evtNum);
        relpTScore = getScore(ModelFamily::RelpT_ee, ggFRelpTInputs, evtNum);
      }
      else if (channel == 2)
      {
        ggFScore = getScore(ModelFamily::ggF_mm, ggFRelpTInputs, evtNum);
        relpTScore = getScore(ModelFamily::RelpT_mm, ggFRelpTInputs, evtNum);
      }

      m_Fbranches.at("VBF_Score_Run3").set(*event, vbfScore, sys);
      m_Fbranches.at("ggF_Score_Run3").set(*event, ggFScore, sys);
      m_Fbranches.at("RelpT_Score_Run3").set(*event, relpTScore, sys);

      const int nLepIdIso_ORPh = m_Ibranches_photons.at("nElectronsIdIso_ORPh").get(*event, sys) +
          m_Ibranches_photons.at("nMuonsIdIso_ORPh").get(*event, sys);
      const int category = computeCategory(channel, nLepIdIso_ORPh, vbfScore, ggFScore, relpTScore, vbf_nj, ph_pt_rel_val);
      m_Ibranches.at("category_Run3").set(*event, category, sys);
    }
    
    return StatusCode::SUCCESS;
  }

  void BaselineVarsHZyAlg::setFloatAndFeature(const xAOD::EventInfo* event,
                                              const std::string& name,
                                              float value,
                                              const CP::SystematicSet& sys,
                                              FeatureMap& features)
  {
    m_Fbranches.at(name).set(*event, value, sys);
    features[name] = value;
  }

  void BaselineVarsHZyAlg::getCMSAngleVars(const xAOD::EventInfo *event,
                                           TLorentzVector &lepLeadP4,
                                           TLorentzVector &lepSubP4,
                                           const TLorentzVector &photonP4,
                                           const CP::SystematicSet &sys,
                                           FeatureMap &features)
  {
    // pTt — thrust in lab frame, before boost (ref: CalculateCommonKinematic)
    const std::vector<double> thrust_res = thrustCoord(lepLeadP4 + lepSubP4, photonP4);
    setFloatAndFeature(event, "llg_pTt", static_cast<float>(thrust_res[0]) / Athena::Units::GeV, sys, features);

    TLorentzVector vZ_cms = lepLeadP4 + lepSubP4;
    TLorentzVector vH_cms = vZ_cms + photonP4;
    TVector3 boost_vec = -vH_cms.BoostVector();
    vH_cms.Boost(boost_vec);
    vZ_cms.Boost(boost_vec);
    lepLeadP4.Boost(boost_vec);
    lepSubP4.Boost(boost_vec);

    TLorentzVector q, qbar;
    q.SetPxPyPzE(0., 0., vH_cms.M() / 2., vH_cms.M() / 2.);
    qbar.SetPxPyPzE(0., 0., -vH_cms.M() / 2., vH_cms.M() / 2.);

    float cosTheta_ginH = 0.f;
    if (vH_cms.M() > 0.f && vZ_cms.P() > 0.f)
      cosTheta_ginH = static_cast<float>((q - qbar).Dot(vZ_cms) / (vH_cms.M() * vZ_cms.P()));

    float costheta_linZ = -99.f;
    float phi_linZ = -99.f;
    if (vH_cms.M() > 0.f && vZ_cms.P() > 0.f)
    {
      costheta_linZ = static_cast<float>(vH_cms.Dot(lepLeadP4 - lepSubP4) / (vH_cms.M() * vZ_cms.P()));

      TVector3 zCM = vZ_cms.Vect() * (1. / vZ_cms.P());
      TVector3 yCM = q.Vect().Cross(vZ_cms.Vect());
      double yMag = yCM.Mag();
      if (yMag > 0)
      {
        yCM *= (1. / yMag);
        TVector3 xCM = yCM.Cross(zCM);
        TVector3 N = lepLeadP4.Vect().Cross(lepSubP4.Vect());
        double nMag = N.Mag();
        if (nMag > 0)
        {
          N *= (1. / nMag);
          phi_linZ = std::asin(N.Dot(xCM));
        }
      }
    }

    setFloatAndFeature(event, "llg_angles_costheta_ginH", cosTheta_ginH, sys, features);
    setFloatAndFeature(event, "llg_angles_costheta_linZ", costheta_linZ, sys, features);
    // setFloatAndFeature(event, "llg_angles_phi_linZ", phi_linZ, sys, features);
    m_Fbranches.at("llg_angles_phi_linZ").set(*event, phi_linZ, sys);
  }

  /// Thrust as defined in HZgammaHelper.cxx thrust_coord().
  /// Returns {pT_t, pT_l} where pT_t = 2*|vector1 x n̂| is the
  /// transverse thrust scalar.
  std::vector<double> BaselineVarsHZyAlg::thrustCoord(
      const TLorentzVector &dilepton, const TLorentzVector &photon)
  {
    const double dx = dilepton.Px() - photon.Px();
    const double dy = dilepton.Py() - photon.Py();
    TVector2 thrust(dx, dy);
    const double mod = thrust.Mod();
    if (mod > 0)
      thrust = thrust / mod;
    const double pT_t =
        2.0 * std::abs(dilepton.Px() * thrust.Py() - dilepton.Py() * thrust.Px());
    const double pT_l =
        (dilepton.Px() + photon.Px()) * thrust.Px() +
        (dilepton.Py() + photon.Py()) * thrust.Py();
    return {pT_t, pT_l};
  }

  void BaselineVarsHZyAlg::getVBFTopologyVars(
      const xAOD::EventInfo *event, const xAOD::JetContainer *HZyJetOR,
      const TLorentzVector &llRawP4, const TLorentzVector &photonP4,
      const TLorentzVector &llgRawP4, const CP::SystematicSet &sys,
      FeatureMap& features)
  {
    // --- VBF topology variables ---
    const int nJets = static_cast<int>(HZyJetOR->size());
    setFloatAndFeature(event, "VBF_N_j", static_cast<float>(nJets), sys, features);

    if (nJets >= 1)
    {
      const auto *j1 = HZyJetOR->at(0);
      setFloatAndFeature(event, "VBF_pT_j1", j1->pt() / Athena::Units::GeV, sys, features);
      setFloatAndFeature(event, "VBF_eta_j1", j1->eta(), sys, features);
      setFloatAndFeature(event, "VBF_phi_j1", j1->phi(), sys, features);
      setFloatAndFeature(event, "VBF_mass_j1", j1->m() / Athena::Units::GeV, sys, features);
      setFloatAndFeature(event, "Zy_Dphi_j1", std::abs(TVector2::Phi_mpi_pi(llgRawP4.Phi() - j1->phi())), sys, features);
      
      // VBF_DRmin_y_j = min(DeltaR(Z,jets), DeltaR(y,jets))
      float DRmin = 1e9f;
      for (const auto *jet : *HZyJetOR)
      {
        if (!jet) continue;
        const float dR_Z = static_cast<float>(llRawP4.DeltaR(jet->p4()));
        const float dR_Gamma = static_cast<float>(photonP4.DeltaR(jet->p4()));
        DRmin = fmin(DRmin, std::min(dR_Z, dR_Gamma));
      }
      setFloatAndFeature(event, "VBF_DRmin_y_j", DRmin, sys, features);
    }

    if (nJets >= 2)
    {
      const auto *j1 = HZyJetOR->at(0);
      const auto *j2 = HZyJetOR->at(1);
      const TLorentzVector pJJ = j1->p4() + j2->p4();

      setFloatAndFeature(event, "VBF_m_jj", pJJ.M() / Athena::Units::GeV, sys, features);
      setFloatAndFeature(event, "VBF_Dy_j_j", std::abs(j1->rapidity() - j2->rapidity()), sys, features);
      setFloatAndFeature(event, "VBF_pT_jj", pJJ.Pt() / Athena::Units::GeV, sys, features);
      setFloatAndFeature(event, "VBF_pT_j2", j2->pt() / Athena::Units::GeV, sys, features);
      setFloatAndFeature(event, "VBF_eta_j2", j2->eta(), sys, features);
      setFloatAndFeature(event, "VBF_mass_j2", j2->m() / Athena::Units::GeV, sys, features);

      // Zeppenfeld variable: |η(Zy) - 0.5*(η_j1 + η_j2)|
      setFloatAndFeature(event, "VBF_Zepp", std::abs(llgRawP4.Eta() - 0.5f * (j1->eta() + j2->eta())), sys, features);
      // Dphi between Zy and dijet system
      setFloatAndFeature(event, "VBF_Dphi_Zy_jj_Fullrange", std::abs(TVector2::Phi_mpi_pi(llgRawP4.Phi() - pJJ.Phi())), sys, features);
      // Yrel = (j1_eta - llg_eta) * (j2_eta - llg_eta)
      const float Yrel = (j1->eta() - llgRawP4.Eta()) * (j2->eta() - llgRawP4.Eta());
      m_Fbranches.at("VBF_Yrel").set(*event, Yrel, sys);

      // Yrel_ph = (j1_eta - ph_eta) * (j2_eta - ph_eta)
      const float Yrel_ph = (j1->eta() - photonP4.Eta()) * (j2->eta() - photonP4.Eta());
      m_Fbranches.at("VBF_Yrel_ph").set(*event, Yrel_ph, sys);

      // |ph_eta - (j1_eta + j2_eta)/2|
      m_Fbranches.at("VBF_ph_eta_center").set(*event,
          std::abs(photonP4.Eta() - 0.5f * (j1->eta() + j2->eta())), sys);

      // Agap (eta-based gap): sum of pT_jk * exp(-(jk_eta - center)^2 / 2) for jets in the gap
      float Agap = 0;
      const float eta_center = 0.5f * (j1->eta() + j2->eta());
      const float eta_min = std::min(j1->eta(), j2->eta());
      const float eta_max = std::max(j1->eta(), j2->eta());
      for (const auto *jet : *HZyJetOR)
      {
        if (jet == j1 || jet == j2) continue;
        const float j_eta = jet->eta();
        if (j_eta > eta_min && j_eta < eta_max)
        {
          const float dEta = j_eta - eta_center;
          Agap += (jet->pt() / Athena::Units::GeV) * std::exp(-0.5f * dEta * dEta);
        }
      }
      m_Fbranches.at("VBF_Agap").set(*event, Agap, sys);

      // Agap_rapidity
      float Agap_rap = 0;
      const float y_jj = 0.5f * (j1->rapidity() + j2->rapidity());
      const float y_min = std::min(j1->rapidity(), j2->rapidity());
      const float y_max = std::max(j1->rapidity(), j2->rapidity());
      for (const auto *jet : *HZyJetOR)
      {
        if (jet == j1 || jet == j2) continue;
        const float j_y = jet->rapidity();
        if (j_y > y_min && j_y < y_max)
        {
          const float dEta = jet->eta() - y_jj;
          Agap_rap += (jet->pt() / Athena::Units::GeV) * std::exp(-0.5f * dEta * dEta);
        }
      }
      m_Fbranches.at("VBF_Agap_rapidity").set(*event, Agap_rap, sys);

    }

    // VBF_pTt_Zy = 2 * |pT(Z) x pT(y)| / |pT(Z - y)|  (in GeV)
    const double cross = llRawP4.Px() * photonP4.Py() - llRawP4.Py() * photonP4.Px();
    const double deltaPt = (llRawP4 - photonP4).Pt();
    const float VBF_pTt_Zy = deltaPt > 0 ? 2.0f * std::abs(cross) / deltaPt / Athena::Units::GeV : 0.f;
    setFloatAndFeature(event, "VBF_pTt_Zy", VBF_pTt_Zy, sys, features);

    // pt_balance = |sum(pT_vec)| / sum(pT_scalar) for ll+gamma+jets
    float sum_px = 0, sum_py = 0, sum_pt = 0;
    for (const auto *jet : *HZyJetOR)
    {
      if (!jet) continue;
      const float pt = jet->pt() / Athena::Units::GeV;
      sum_px += pt * std::cos(jet->phi());
      sum_py += pt * std::sin(jet->phi());
      sum_pt += pt;
    }
    sum_px += llRawP4.Pt() / Athena::Units::GeV * std::cos(llRawP4.Phi());
    sum_py += llRawP4.Pt() / Athena::Units::GeV * std::sin(llRawP4.Phi());
    sum_pt += llRawP4.Pt() / Athena::Units::GeV;
    sum_px += photonP4.Pt() / Athena::Units::GeV * std::cos(photonP4.Phi());
    sum_py += photonP4.Pt() / Athena::Units::GeV * std::sin(photonP4.Phi());
    sum_pt += photonP4.Pt() / Athena::Units::GeV;
    const float pt_balance = sum_pt > 0
        ? std::sqrt(sum_px * sum_px + sum_py * sum_py) / sum_pt : -1.f;
    m_Fbranches.at("VBF_pt_balance").set(*event, pt_balance, sys);
    
  }

  void BaselineVarsHZyAlg::loadModel(ModelFamily family,
                                     const std::string &fileName,
                                     int nFeatures)
  {
    std::string resolvedPath = PathResolverFindCalibFile(fileName);
    if (resolvedPath.empty()) {
      ATH_MSG_FATAL("Could not resolve ONNX model: " << fileName);
      return;
    }
    ATH_MSG_INFO("Loading ONNX model for family=" << static_cast<int>(family) << ": " << resolvedPath);

    OnnxFold fold;
    fold.env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_FATAL, "");

    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);
    opts.SetLogSeverityLevel(4);
    opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    fold.session = std::make_unique<Ort::Session>(*fold.env, resolvedPath.c_str(), opts);

    Ort::AllocatorWithDefaultOptions alloc;
    size_t nIn = fold.session->GetInputCount();
    fold.inputNames.resize(nIn);
    for (size_t i = 0; i < nIn; ++i)
    {
      char *nm = fold.session->GetInputNameAllocated(i, alloc).release();
      fold.inputNames[i] = nm;
    }
    fold.inputDims = {1, nFeatures};

    size_t nOut = fold.session->GetOutputCount();
    fold.outputNames.resize(nOut);
    for (size_t i = 0; i < nOut; ++i)
    {
      char *nm = fold.session->GetOutputNameAllocated(i, alloc).release();
      fold.outputNames[i] = nm;
    }

    m_folds[family].push_back(std::move(fold));
  }

  float BaselineVarsHZyAlg::getScore(ModelFamily family,
                                     const std::vector<float>& inputs,
                                     const int eventNumber) const
  {
    ATH_MSG_INFO("getScore: family=" << static_cast<int>(family));
    const auto& folds = m_folds.at(family);
    const size_t nfold = folds.size();
    if (nfold == 0) return -1.f;

    constexpr size_t outScoreIndex = 1; // probability tensor and signal probability
    auto evalFold = [&](const auto& fold, size_t ifold) -> float {
      std::vector<float> inCopy = inputs;
      const size_t expectedSize = static_cast<size_t>(fold.inputDims[1]);

      if (inCopy.size() < expectedSize) inCopy.resize(expectedSize, 0.f);
      if (inCopy.size() > expectedSize) inCopy.resize(expectedSize);

      auto memory = Ort::MemoryInfo::CreateCpu(
          OrtArenaAllocator, OrtMemTypeDefault);

      Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
          memory, inCopy.data(), inCopy.size(), fold.inputDims.data(), fold.inputDims.size());

      auto outputs = fold.session->Run(
          Ort::RunOptions{nullptr}, fold.inputNames.data(), &inputTensor, fold.inputNames.size(),
          fold.outputNames.data(), fold.outputNames.size());

      if (outputs.size() <= outScoreIndex) return -1.f;

      const auto& output = outputs[outScoreIndex];
      const size_t nOut = output.GetTensorTypeAndShapeInfo().GetElementCount();

      if (nOut <= outScoreIndex) return -1.f;

      const float* scoreArr =
          output.template GetTensorData<float>();

      ATH_MSG_DEBUG("fold=" << ifold << ", scoreArr[0]=" << scoreArr[0] << ", scoreArr[1]=" << scoreArr[1]);

      return scoreArr[outScoreIndex];
    };

    if (m_isMC) {
      const size_t ifold = static_cast<size_t>(eventNumber >= 0 ? eventNumber : -eventNumber) % nfold;
      
      return evalFold(folds[ifold], ifold);
    }
    else {
      float sumScore = 0.f;
      for (size_t ifold = 0; ifold < nfold; ++ifold) {
        const float score = evalFold(folds[ifold], ifold);
        if (score < 0.f) return -1.f;
        sumScore += score;
      }

      return sumScore / static_cast<float>(nfold);
    }
  }

  int BaselineVarsHZyAlg::computeCategory(int channel,
                                          int nLepIdIso_ORPh,
                                          float vbfScore,
                                          float ggFScore,
                                          float relpTScore,
                                          float vbf_nj,
                                          float ph_pt_rel) const
  {
    ATH_MSG_INFO( "computeCategory: evet categorization for" << " channel=" << channel);
    const float VBF_cut_H = 0.99f;
    const float VBF_cut_M = 0.95f;
    const float ggF_ee_cut_H = 0.85f;
    const float ggF_ee_cut_M = 0.60f;
    const float ggF_mm_cut_H = 0.90f;
    const float ggF_mm_cut_M = 0.65f;
    const float REL_pT_cut = 0.4f;
    const float REL_pT_ee_BDT = 0.65f;
    const float REL_pT_mm_BDT = 0.58f;

    const bool pass2jet = vbf_nj >= 2;
    const bool passRelpT = ph_pt_rel > REL_pT_cut;

    int category = 0;
    if (nLepIdIso_ORPh>=3)
      category = 1;
    else if (pass2jet && vbfScore >= VBF_cut_H)
      category = 2;
    else if (pass2jet && vbfScore >= VBF_cut_M)
      category = 3;

    else if (channel == 1 && passRelpT && relpTScore >= REL_pT_ee_BDT)
      category = 4;
    else if (channel == 1 && passRelpT)
      category = 5;

    else if (channel == 2 && passRelpT && relpTScore >= REL_pT_mm_BDT)
      category = 6;
    else if (channel == 2 && passRelpT)
      category = 7;

    else if (channel == 1 && ggFScore >= ggF_ee_cut_H)
      category = 8;
    else if (channel == 1 && ggFScore >= ggF_ee_cut_M)
      category = 9;
    else if (channel == 1)
      category = 10;

    else if (channel == 2 && ggFScore >= ggF_mm_cut_H)
      category = 11;
    else if (channel == 2 && ggFScore >= ggF_mm_cut_M)
      category = 12;
    else if (channel == 2)
      category = 13;
    return category;
  }

} // namespace HZY
