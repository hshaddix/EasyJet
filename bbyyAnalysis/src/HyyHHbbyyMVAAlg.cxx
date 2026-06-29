/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "HyyHHbbyyMVAAlg.h"
#include <fstream>
#include <AthContainers/ConstDataVector.h>
#include <AthenaKernel/Units.h>
#include "PathResolver/PathResolver.h"
#include "lwtnn/parse_json.hh"
#include "TFile.h"

namespace HHBBYY
{

  HyyHHbbyyMVAAlg::HyyHHbbyyMVAAlg(const std::string &name,
                                   ISvcLocator *pSvcLocator)
    : AthHistogramAlgorithm(name, pSvcLocator)
  {}

  StatusCode HyyHHbbyyMVAAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("       HyyHHbbyyMVAAlg           \n");
    ATH_MSG_INFO("*********************************\n");

    ATH_CHECK(m_jetHandle.initialize(m_systematicsList));
    if (!m_isBtag.empty()) {
      ATH_CHECK(m_isBtag.initialize(m_systematicsList, m_jetHandle));
    }

    ATH_CHECK(m_photonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
    ATH_CHECK (m_metHandle.initialize(m_systematicsList));
    ATH_CHECK(m_met_sig.initialize(m_systematicsList, m_metHandle));

    ATH_CHECK(m_nCentralJets.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_nJets.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_nBJets.initialize(m_systematicsList, m_eventHandle));

    // Setup decorations for BDTInput variables (int)
    for (const std::string &string_var : m_intVariables_BDTInput) {
      CP::SysReadDecorHandle<int> var { string_var + "_%SYS%", this };
      m_Ibranches_BDTInput.emplace(string_var, var);
      ATH_CHECK(m_Ibranches_BDTInput.at(string_var)
                  .initialize(m_systematicsList, m_eventHandle));
    }

    // Setup decorations for BDTInput variables (float)
    for (const std::string &string_var : m_floatVariables_BDTInput) {
      CP::SysReadDecorHandle<float> var {string_var + "_%SYS%", this };
      m_Fbranches_BDTInput.emplace(string_var, var);
      ATH_CHECK(m_Fbranches_BDTInput.at(string_var)
                  .initialize(m_systematicsList, m_eventHandle));
    }

    // Output float decorations
    for (const std::string &string_var : m_floatVariables) {
      CP::SysWriteDecorHandle<float> var { string_var + "_%SYS%", this };
      m_Fbranches.emplace(string_var, var);
      ATH_CHECK(m_Fbranches.at(string_var)
                  .initialize(m_systematicsList, m_eventHandle));
    }

    // Output int decorations
    for (const std::string &string_var : m_intVariables) {
      CP::SysWriteDecorHandle<int> var { string_var + "_%SYS%", this };
      m_Ibranches.emplace(string_var, var);
      ATH_CHECK(m_Ibranches.at(string_var)
                  .initialize(m_systematicsList, m_eventHandle));
    }
    // Init H-yy Couplings multiclass BDT
    if (m_doHyy_multiclass) {
      HyyMultiBDT_init(m_HyyMultiBDT_folder);
    }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode HyyHHbbyyMVAAlg::execute(const EventContext& ctx)
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector()) {

      // In case of special Higgs sample, run only on NOSYS
      if (!m_doSystematics && sys.name() != "")
        continue;

      // Containers we read in
      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_jetHandle.retrieve(jets, sys, ctx));

      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK(m_metHandle.retrieve(metCont, sys, ctx));

      const xAOD::MissingET *met = (*metCont)["Final"];
      if (!met) {
        ATH_MSG_ERROR("Could not retrieve MET");
        return StatusCode::FAILURE;
      }

      TLorentzVector ph1(0.,0.,0.,0.);
      ph1.SetPtEtaPhiE(m_Fbranches_BDTInput.at("Photon1_pt").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon1_eta").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon1_phi").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon1_E").get(*event, sys));
      TLorentzVector ph2(0.,0.,0.,0.);
      ph2.SetPtEtaPhiE(m_Fbranches_BDTInput.at("Photon2_pt").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon2_eta").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon2_phi").get(*event, sys),
                       m_Fbranches_BDTInput.at("Photon2_E").get(*event, sys));
      int n_photons =  m_Ibranches_BDTInput.at("nPhotons").get(*event, sys);

      std::vector<TLorentzVector> Hyy_photons = { ph1, ph2 };
      
      int nCentralJets = m_nCentralJets.get(*event, sys);
      int nJets        = m_nJets.get(*event, sys);
      int nBJets       = m_nBJets.get(*event, sys);

    HyyMultiBDT_createVariableMap(event, sys, met, n_photons, ph1, ph2,
                                  nCentralJets, nJets, nBJets);

      // H-yy Couplings multiclass BDT Implementation
      if (m_doHyy_multiclass) {
        HHBBYY::HyyMultiBDTResults r = HyyMultiBDT_perform();

        m_Ibranches.at("HyyMultiBDT_multiclass_index").set(*event, r.multiclass_index, sys);
        m_Ibranches.at("HyyMultiBDT_binary_index").set(*event, r.binary_index, sys);
        m_Fbranches.at("HyyMultiBDT_binary_score").set(*event, r.binary_score, sys);

        for (std::size_t i = 0; i < m_MulticlassBDT_bins.size(); ++i) {
            m_Fbranches.at(m_MulticlassBDT_bins[i]).set(*event, r.multi_response[i], sys);
        }
      }
      
    } // end loop over systematics

    return StatusCode::SUCCESS;
  }

  void HyyHHbbyyMVAAlg::HyyMultiBDT_init(const std::string &folder)
  {
    std::string resolvedFolder = PathResolverFindCalibDirectory(folder);

    const std::string filename_multiclass = resolvedFolder + "/" + m_MulticlassBDT_model_name;
    TFile *file_multiclass = TFile::Open(filename_multiclass.c_str());
    TTree *tree_multiclass = nullptr;

    file_multiclass->GetObject("lgbm", tree_multiclass);
    HyyMultiBDT_multiclass = std::make_unique<MVAUtils::BDT>(tree_multiclass);
    file_multiclass->Close();

    for (const auto &[index, bin_name] : m_MulticlassBDT_bins) {
      const std::string filename_binary = resolvedFolder + "/model2_" + bin_name + ".root";
      TFile *file_binary = TFile::Open(filename_binary.c_str());
      TTree *tree_binary = nullptr;
      file_binary->GetObject("lgbm", tree_binary);
      std::unique_ptr<MVAUtils::BDT> binary_model =
        std::make_unique<MVAUtils::BDT>(tree_binary);
      HyyMultiBDT_binaries.push_back(std::move(binary_model));
      file_binary->Close();
    }

    int offset = 0;

    for (const auto &[bin, boundary] : m_BinaryBDT_boundaries) {
      HyyMultiBDT_offset_cat.push_back(offset);
      offset += boundary.size() + 1;
    }

  }

  template<typename T>
  T HyyHHbbyyMVAAlg::var_or_nan(T value, T default_value)
  {
    return (value == default_value)
      ? std::numeric_limits<T>::quiet_NaN()
      : value;
  }

  HyyMultiBDTResults
  HyyHHbbyyMVAAlg::HyyMultiBDT_perform()
  {
    std::vector<float> multiBDT_Input =
      get_HyyHHbbyyMVAInput(m_MulticlassBDT_variables);

    const std::vector<float> raw_multi_response =
      HyyMultiBDT_multiclass->GetMultiResponse(
        multiBDT_Input, m_MulticlassBDT_bins.size()
    );

    std::vector<float> multi_response(raw_multi_response.size());
    for (std::size_t i = 0; i != multi_response.size(); ++i) {
      multi_response[i] = raw_multi_response[i] 
      * std::stof(m_BinaryBDT_weights[m_MulticlassBDT_bins[i]]);
    }

    const int multiclass_index =
      std::distance(std::begin(multi_response),
                    std::max_element(std::begin(multi_response),
                                     std::end(multi_response)));

    int   binary_bin   = 0;
    float binary_score = 0;
    const std::string multiclassBDT_bin = m_MulticlassBDT_bins[multiclass_index];
    if (m_BinaryBDT_boundaries[multiclassBDT_bin].size() != 0) {
      std::vector<float> BinaryBDT_Input =
        get_HyyHHbbyyMVAInput(
            m_BinaryBDT_variables[multiclassBDT_bin]
        );

      binary_score =
        HyyMultiBDT_binaries[multiclass_index]->GetClassification(BinaryBDT_Input);

      const auto boundaries = m_BinaryBDT_boundaries[multiclassBDT_bin];

      // remember that pure bins (higher value) have lower index.
      // Extremes (0, 1) and not inside boundaries
      auto upper_it = std::upper_bound(
        boundaries.begin(), 
        boundaries.end(),
        binary_score,
        [](float score, const std::string& boundary_str) {
          float boundary_value = std::stof(boundary_str);
          return score < boundary_value;
        }
      );
      binary_bin = std::distance(upper_it, boundaries.end());
    }

    return HHBBYY::HyyMultiBDTResults(
      { multi_response,
        multiclass_index,
        binary_bin + HyyMultiBDT_offset_cat[multiclass_index],
        binary_score }
    );
  }

  void
  HyyHHbbyyMVAAlg::HyyMultiBDT_createVariableMap(const xAOD::EventInfo *event,
                                                const auto &sys,
                                                const xAOD::MissingET *met,
                                                const int n_photons,
                                                const TLorentzVector &ph1,
                                                const TLorentzVector &ph2,
                                                const int &nCentralJets,
                                                const int &nJets,
                                                const int &nBJets)
  {
    TLorentzVector H_yy(0., 0., 0., 0.);

    float pT_yy     (-99.);
    float yAbs_yy   (-99.);
    float ph0_eta   (-99.);
    float ph1_eta   (-99.);
    float Dy_y_y    (-99.);
    float pTt_yy    (-99.);
    float phiStar_yy(-99.);

    if (n_photons == 1) {

      ph0_eta = m_Fbranches_BDTInput.at("Photon1_eta").get(*event, sys);

    } else if (n_photons > 1) {

      H_yy   = ph1 + ph2;
      pT_yy  = H_yy.Pt();
      yAbs_yy = std::fabs(H_yy.Rapidity());

      ph0_eta = m_Fbranches_BDTInput.at("Photon1_eta").get(*event, sys);
      ph1_eta = m_Fbranches_BDTInput.at("Photon2_eta").get(*event, sys);

      Dy_y_y = std::fabs(ph1.Rapidity() - ph2.Rapidity());

      pTt_yy = std::fabs(ph1.Px() * ph2.Py() - ph2.Px() * ph1.Py())
               / (ph1 - ph2).Pt() * 2.0;

      phiStar_yy =
        tan((TMath::Pi() - std::fabs(ph1.DeltaPhi(ph2))) / 2.0) *
        std::sqrt(1.0 - std::pow(std::tanh((ph1.Eta() - ph2.Eta()) / 2.0), 2.0));
    }

    HyyMultiBDT_VariableMap = {
      {"pT_yy", pT_yy},
      {"yAbs_yy", yAbs_yy},
      {"ph0_eta", ph0_eta},
      {"ph1_eta", ph1_eta},
      {"Dy_y_y", Dy_y_y},
      {"pTt_yy", pTt_yy},
      {"phiStar_yy", phiStar_yy},
      {"N_j_central", nCentralJets},
      {"N_j", nJets},
      {"N_j_btag", std::min(nBJets, 3)},
      {"m_jj", var_or_nan<float>(m_Fbranches_BDTInput.at("m_jj").get(*event, sys), -99)},
      {"pT_yyjj", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_yyjj").get(*event, sys), -99)},
      {"N_j_central30", static_cast<float>(m_Ibranches_BDTInput.at("nCentralJets_30").get(*event, sys))},
      {"N_j_30", static_cast<float>(m_Ibranches_BDTInput.at("nJets_30").get(*event, sys))},
      {"N_j_btag30", static_cast<float>(std::min(m_Ibranches_BDTInput.at("nBJets_30").get(*event, sys), 3))},
      {"m_jj_30", var_or_nan<float>(m_Fbranches_BDTInput.at("m_jj_30").get(*event, sys), -99)},
      {"pT_yyjj_30", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_yyjj_30").get(*event, sys), -99)},
      {"pT_jj", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_jj").get(*event, sys), -99)},
      {"pT_j1_30", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_j1_30").get(*event, sys), -99)},
      {"Dy_j_j", var_or_nan<float>(m_Fbranches_BDTInput.at("Dy_j_j").get(*event, sys), -99)},
      {"Dphi_j_j", var_or_nan<float>(m_Fbranches_BDTInput.at("Dphi_j_j").get(*event, sys), -99)},
      {"Deta_j_j", var_or_nan<float>(m_Fbranches_BDTInput.at("Deta_j_j").get(*event, sys), -99)},
      {"pT_yyj", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_yyj").get(*event, sys), -99)},
      {"m_yyj", var_or_nan<float>(m_Fbranches_BDTInput.at("m_yyj").get(*event, sys), -99)},
      {"m_yyjj", var_or_nan<float>(m_Fbranches_BDTInput.at("m_yyjj").get(*event, sys), -99)},
      {"Dphi_yy_jj", var_or_nan<float>(m_Fbranches_BDTInput.at("Dphi_yy_jj").get(*event, sys), -99)},
      {"DRmin_y_j", var_or_nan<float>(m_Fbranches_BDTInput.at("DRmin_y_j").get(*event, sys), -99)},
      {"Dy_yy_jj", var_or_nan<float>(m_Fbranches_BDTInput.at("Dy_yy_jj").get(*event, sys), -99)},
      {"m_alljet", var_or_nan<float>(m_Fbranches_BDTInput.at("m_alljets").get(*event, sys), -99)},
      {"cosTS_yyjj", var_or_nan<float>(m_Fbranches_BDTInput.at("cosTS_yyjj").get(*event, sys), -99)},
      {"N_lep", static_cast<float>(m_Ibranches_BDTInput.at("nLeptons").get(*event, sys))},
      {"N_e", static_cast<float>(m_Ibranches_BDTInput.at("nElectrons").get(*event, sys))},
      {"N_mu", static_cast<float>(m_Ibranches_BDTInput.at("nMuons").get(*event, sys))},
      {"m_ee", var_or_nan<float>(m_Fbranches_BDTInput.at("m_ee_os").get(*event, sys), -99)},
      {"m_mumu", var_or_nan<float>(m_Fbranches_BDTInput.at("m_mumu_os").get(*event, sys), -99)},
      {"met_TST", var_or_nan<float>(met->met(), -99)},
      {"sumet_TST", var_or_nan<float>(met->sumet(), -99)},
      {"MET_sig", var_or_nan<float>(m_met_sig.get(*met, sys), -99)},
      {"phi_TST", var_or_nan<float>(met->phi(), -99)},
      {"pT_ll", var_or_nan<float>(m_Fbranches_BDTInput.at("pT_ll_os_max").get(*event, sys), -99)},
      {"pTlepMET", var_or_nan<float>(m_Fbranches_BDTInput.at("pTlepMET").get(*event, sys), -99)},
      {"mTlepMET", m_Fbranches_BDTInput.at("mTlepMET").get(*event, sys)},
      {"flag_eTmiss", m_Fbranches_BDTInput.at("metTST_HVTST_flag").get(*event, sys)},
      {"HT_30", var_or_nan<float>(m_Fbranches_BDTInput.at("HT_30").get(*event, sys), 0.)},
      {"Zepp", var_or_nan<float>(m_Fbranches_BDTInput.at("Zepp").get(*event, sys), -99)},
      {"mu", event->actualInteractionsPerCrossing()},
      {"score_recotop", var_or_nan<float>(m_Fbranches_BDTInput.at("score_recotop1").get(*event, sys), -1.)},
      {"recotop_m", var_or_nan<float>(m_Fbranches_BDTInput.at("recotop1_m").get(*event, sys), 0.)},
      {"recotop_phi", var_or_nan<float>(m_Fbranches_BDTInput.at("recotop1_phi").get(*event, sys), 0.)},
      {"recotop_eta", var_or_nan<float>(m_Fbranches_BDTInput.at("recotop1_eta").get(*event, sys), 0.)},
      {"recotop_pT", var_or_nan<float>(m_Fbranches_BDTInput.at("recotop1_pT").get(*event, sys), 0.)},
      {"score_hybrtop", var_or_nan<float>(m_Fbranches_BDTInput.at("score_recotop2").get(*event, sys), -1.)},
      {"hybrtop_m", var_or_nan<float>(m_Fbranches_BDTInput.at("hybrtop_m").get(*event, sys), 0.)},
      {"hybrtop_phi", var_or_nan<float>(m_Fbranches_BDTInput.at("hybrtop_phi").get(*event, sys), 0.)},
      {"hybrtop_eta", var_or_nan<float>(m_Fbranches_BDTInput.at("hybrtop_eta").get(*event, sys), 0.)},
      {"hybrtop_pT", var_or_nan<float>(m_Fbranches_BDTInput.at("hybrtop_pT").get(*event, sys), 0.)},
      {"fwdjet_eta", std::fabs(var_or_nan<float>(m_Fbranches_BDTInput.at("fwdJet_eta").get(*event, sys), -99.))},
      {"m_fwdJyy", var_or_nan<float>(m_Fbranches_BDTInput.at("m_fwdJet_yy").get(*event, sys), -99)},
      {"deltaR_Wb_t2", m_Fbranches_BDTInput.at("dR_Wb_t2").get(*event, sys)}
    };
  }

  std::vector<float>
  HyyHHbbyyMVAAlg::get_HyyHHbbyyMVAInput(const std::vector<std::string> &variables)
  {
    std::vector<float> result;
    result.reserve(variables.size());
    for (const auto& var_name : variables) {
      auto it = HyyMultiBDT_VariableMap.find(var_name);
      if (it != HyyMultiBDT_VariableMap.end()) {
        result.push_back(it->second);
      } else {
        result.push_back(-99.0f);
      }
    }
    return result;
  }

} // namespace HHBBYY

