/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "ssWWTruthSelectorAlg.h"
#include <AthenaKernel/Units.h>
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AthContainers/ConstDataVector.h>
#include <AthContainers/AuxTypeRegistry.h>


namespace ssWWVBS
{
  
  ssWWTruthSelectorAlg::ssWWTruthSelectorAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : EL::AnaAlgorithm(name, pSvcLocator)
  {
  }


  StatusCode ssWWTruthSelectorAlg::initialize()
  {
    ATH_MSG_INFO("*********************************\n");
    ATH_MSG_INFO("     ssWWTruthSelectorAlg      \n");
    ATH_MSG_INFO("*********************************\n");


    ATH_CHECK (m_truthjetsInKey.initialize());
    ATH_CHECK (m_truthelectronInKey.initialize());
    ATH_CHECK (m_truthmuonInKey.initialize());
    // ATH_CHECK (m_truthmetInKey.initialize());
    ATH_CHECK (m_eventInKey.initialize());   
    ATH_CHECK (m_passTruthCutsKey.initialize());
    SG::AuxTypeRegistry::instance().getAuxID<bool>("PassTruthCuts");

    for (auto& [boolKey, boolName] : m_boolnames) {   
      m_bools.emplace(boolKey, false);
      m_BbranchKeys.emplace(boolKey, m_eventInKey.key() + "." + boolName);
      ATH_CHECK(m_BbranchKeys.at(boolKey).initialize());
      SG::AuxTypeRegistry::instance().getAuxID<bool>(boolName);
    }

    for (const std::string &var : m_floatVariables) {
      m_FbranchesKeys.emplace(var, m_eventInKey.key() + "." + var);
      ATH_CHECK (m_FbranchesKeys.at(var).initialize());
      SG::AuxTypeRegistry::instance().getAuxID<float>(var);
    }

    ATH_MSG_INFO("Decorating object: " << m_eventInKey.key());
    
    if(m_saveCutFlow) ATH_CHECK (initialiseCutflow());

    return StatusCode::SUCCESS;

  }


  StatusCode ssWWTruthSelectorAlg::execute()
  {

    // FilterReporter filter(m_filterParams, false);

    SG::ReadHandle<xAOD::EventInfo> truthEvents(m_eventInKey);
    SG::ReadHandle<xAOD::JetContainer> truthJets(m_truthjetsInKey);
    // SG::ReadHandle<xAOD::MissingETContainer> truthMet(m_truthmetInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthElectrons(m_truthelectronInKey);
    SG::ReadHandle<xAOD::TruthParticleContainer> truthMuons(m_truthmuonInKey);

    auto jets_after_overlap = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
    auto truthjets = std::make_unique<ConstDataVector<xAOD::JetContainer>>(SG::VIEW_ELEMENTS);
    auto truthmet = std::make_unique<ConstDataVector<xAOD::MissingETContainer>>(SG::VIEW_ELEMENTS);
    
    for (auto [var, key] : m_FbranchesKeys){
      m_Fbranches.emplace(var, key);
    }

    const xAOD::EventInfo* truthevent = truthEvents.get();

    for (const std::string &string_var: m_floatVariables) {
        m_Fbranches.at(string_var)(*truthevent) = -99.;
    }

    auto muons_after_selection = std::make_unique<ConstDataVector<xAOD::TruthParticleContainer>>(SG::VIEW_ELEMENTS);
    auto electrons_after_selection = std::make_unique<ConstDataVector<xAOD::TruthParticleContainer>>(SG::VIEW_ELEMENTS);
    
    m_bools.at(ssWWVBS::IS_ee_TRUTH) = false;
    m_bools.at(ssWWVBS::IS_mm_TRUTH) = false;
    m_bools.at(ssWWVBS::IS_em_TRUTH) = false;

    m_bools.at(ssWWVBS::EXACTLY_TWO_LEPTONS_TRUTH) = false;
    m_bools.at(ssWWVBS::SAME_CHARGE_LEPTONS_TRUTH) = false;
    m_bools.at(ssWWVBS::DILEPTON_MASS_TRUTH) = false;

    m_bools.at(ssWWVBS::MET_TRUTH) = false;
    m_bools.at(ssWWVBS::TWO_JETS_TRUTH) = false;
    m_bools.at(ssWWVBS::DIJET_MASS_TRUTH) = false;
    m_bools.at(ssWWVBS::DIJET_RAPIDITY_TRUTH) = false;

    evaluateTruthLeptonCuts(*truthevent, *truthElectrons, *truthMuons, m_ssWWTruthCuts, *electrons_after_selection, *muons_after_selection);
    truthOverlapRemoval(*truthJets, *electrons_after_selection, *muons_after_selection, *jets_after_overlap);

    for (const xAOD::Jet *jet : *jets_after_overlap){
        truthjets->push_back(jet);
    }

    int n_jets = jets_after_overlap->size();
    for (int i=0; i<std::min(n_jets, 2); ++i) {
      std::string prefix = "TruthJet_"+std::to_string(i+1);
      m_Fbranches.at(prefix+"_pt")(*truthevent) = jets_after_overlap->at(i)->pt();
      m_Fbranches.at(prefix+"_eta")(*truthevent) = jets_after_overlap->at(i)->eta();
      m_Fbranches.at(prefix+"_phi")(*truthevent) = jets_after_overlap->at(i)->phi();
      m_Fbranches.at(prefix+"_E")(*truthevent) = jets_after_overlap->at(i)->e();
    }

    // Leptons
    const xAOD::TruthParticle* ele0 = nullptr;
    const xAOD::TruthParticle* ele1 = nullptr;

    for (const xAOD::TruthParticle* electron : *electrons_after_selection){
      if (!ele0) ele0 = electron;
      else {
        ele1 = electron;
        break;
      }
    }

    const xAOD::TruthParticle* mu0 = nullptr;
    const xAOD::TruthParticle* mu1 = nullptr;

    for (const xAOD::TruthParticle* muon : *muons_after_selection){
      if (!mu0) mu0 = muon;
      else {
        mu1 = muon;
        break;
      }
    }

    std::vector<const xAOD::TruthParticle*> leptons;
    if(ele0) leptons.emplace_back(ele0);
    if(mu0) leptons.emplace_back(mu0);
    if(ele1) leptons.emplace_back(ele1);
    if(mu1) leptons.emplace_back(mu1);

    std::sort(leptons.begin(), leptons.end(),
    [](const xAOD::TruthParticle*& a,
        const xAOD::TruthParticle*& b) {
          SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
          return dressedLeptonPt(*a) > dressedLeptonPt(*b); });

    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    SG::ConstAccessor<float> leptonM("m");

    int n_leptons = leptons.size();
    for (int i = 0; i < std::min(n_leptons, 2); ++i){
      std::string prefix = "TruthLepton_"+std::to_string(i+1);
      m_Fbranches.at(prefix+"_pt")(*truthevent) = dressedLeptonPt(*leptons.at(i));
      m_Fbranches.at(prefix+"_eta")(*truthevent) = dressedLeptonEta(*leptons.at(i));
      m_Fbranches.at(prefix+"_phi")(*truthevent) = dressedLeptonPhi(*leptons.at(i));
      m_Fbranches.at(prefix+"_E")(*truthevent) = dressedLeptonE(*leptons.at(i));
      m_Fbranches.at(prefix+"_m")(*truthevent) = leptonM(*leptons.at(i));
    }

    evaluateTruthJetCuts(*truthjets, m_ssWWTruthCuts);


    // for (const xAOD::MissingET *met : *truthMet){
    //     truthmet->push_back(met);
    // }
    // evaluateTruthMetCuts(*truthmet, m_ssWWTruthCuts);

    bool pass_truth_baseline=true;
    for (const auto& [key, value] : m_boolnames) {
      auto it = std::find(m_BASELINE_CUTS.begin(), m_BASELINE_CUTS.end(), value);
      if (it != m_BASELINE_CUTS.end()) {
        pass_truth_baseline &= m_bools.at(key);
      }
    }
  
    // Compute total_events
    m_total_events+=1;

    for (const auto &cut : m_inputCutKeys) {
      if (m_ssWWTruthCuts.exists(m_boolnames.at(cut))) {
        m_ssWWTruthCuts(m_boolnames.at(cut)).passed = m_bools.at(cut);
        if (m_ssWWTruthCuts(m_boolnames.at(cut)).passed) {
          m_ssWWTruthCuts(m_boolnames.at(cut)).counter += 1;
        }
      }
    }
    
    unsigned int consecutive_cuts = 0;
    for (size_t i = 0; i < m_ssWWTruthCuts.size(); ++i) {
      if (m_ssWWTruthCuts[i].passed)
        consecutive_cuts++;
      else
        break;
    }

    for (unsigned int i=0; i<consecutive_cuts; i++) {
      m_ssWWTruthCuts[i].relativeCounter+=1;
    }

    ATH_MSG_VERBOSE("pass_truth_baseline = " << pass_truth_baseline);
    SG::WriteDecorHandle<xAOD::EventInfo, bool> passTruthCutsHandle(m_passTruthCutsKey);
    passTruthCutsHandle(*truthevent) = pass_truth_baseline && jets_after_overlap->size() > 0;

    for (auto& [key, handleKey] : m_BbranchKeys) {
      SG::WriteDecorHandle<xAOD::EventInfo, bool> handle(handleKey);
      handle(*truthevent) = m_bools.at(key);
    }

  return StatusCode::SUCCESS;
  }

  StatusCode ssWWTruthSelectorAlg::finalize()
  {
    ATH_MSG_INFO("Total events = " << m_total_events <<std::endl);
    m_ssWWTruthCuts.CheckCutResults(); // Print CheckCutResults

    if(m_saveCutFlow) {
      m_ssWWTruthCuts.DoAbsoluteEfficiency(m_total_events, efficiency("AbsoluteTruthEfficiency"));
      m_ssWWTruthCuts.DoRelativeEfficiency(m_total_events, efficiency("RelativeTruthEfficiency"));
      m_ssWWTruthCuts.DoStandardCutFlow(m_total_events, efficiency("StandardTruthCutFlow"));
      m_ssWWTruthCuts.DoCutflowLabeling(m_total_events, hist("TruthEventsPassed_BinLabeling"));

    }

    return StatusCode::SUCCESS;
  }


  void ssWWTruthSelectorAlg::truthOverlapRemoval(const xAOD::JetContainer& truthJets, ConstDataVector<xAOD::TruthParticleContainer>& truthElectrons, ConstDataVector<xAOD::TruthParticleContainer>& truthMuons, ConstDataVector<xAOD::JetContainer>& jetsAfterOverlap){

    float dRcut = 0.4;
    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    for (const xAOD::Jet* jet : truthJets){
      TLorentzVector jet_p4 = jet->p4();
      bool keep = true;
      for (const xAOD::TruthParticle* ele : truthElectrons){
        TLorentzVector ele_p4;
        ele_p4.SetPtEtaPhiE(dressedLeptonPt(*ele),
                            dressedLeptonEta(*ele),
                            dressedLeptonPhi(*ele),
                            dressedLeptonE(*ele));
        if (jet_p4.DeltaR(ele_p4) < dRcut){
          keep = false;
          break;
        }
      }
      if (!keep) continue;
      for (const xAOD::TruthParticle* mu : truthMuons){
        TLorentzVector mu_p4;
        mu_p4.SetPtEtaPhiE(dressedLeptonPt(*mu),
                           dressedLeptonEta(*mu),
                           dressedLeptonPhi(*mu),
                           dressedLeptonE(*mu));
        if (jet_p4.DeltaR(mu_p4) < dRcut){
          keep = false;
          break;
        }
      }
      if (keep){
        jetsAfterOverlap.push_back(jet);
      }
    }

  }

  void ssWWTruthSelectorAlg::evaluateTruthLeptonCuts(const xAOD::EventInfo& truthevent, const xAOD::TruthParticleContainer& truthelectrons, const xAOD::TruthParticleContainer& truthmuons, CutManager& ssWWTruthCuts, ConstDataVector<xAOD::TruthParticleContainer>& electronsAfterSelection, ConstDataVector<xAOD::TruthParticleContainer>& muonsAfterSelection)
  {
  
    TLorentzVector ll;
    double mll = -99;

    bool SAME_CHARGE_LEPTONS_TRUTH = false;

    SG::ConstAccessor<float> dressedLeptonPt("pt_dressed");
    SG::ConstAccessor<float> dressedLeptonEta("eta_dressed");
    SG::ConstAccessor<float> dressedLeptonPhi("phi_dressed");
    SG::ConstAccessor<float> dressedLeptonE("e_dressed");
    
    for (const xAOD::TruthParticle* electron : truthelectrons){
      if (dressedLeptonPt(*electron) < 27.*Athena::Units::GeV || std::abs(dressedLeptonEta(*electron)) > 2.5) continue;
      electronsAfterSelection.push_back(electron);
    }

    for (const xAOD::TruthParticle* muon : truthmuons){
      if (dressedLeptonPt(*muon) < 27.*Athena::Units::GeV || std::abs(dressedLeptonEta(*muon)) > 2.5) continue;
      muonsAfterSelection.push_back(muon);
    }

    if (electronsAfterSelection.size() + muonsAfterSelection.size() == 2) {
      m_bools.at(ssWWVBS::EXACTLY_TWO_LEPTONS_TRUTH) = true;

    }
      
    if (electronsAfterSelection.size() >= 2 && muonsAfterSelection.size() == 0)
    {
      m_bools.at(ssWWVBS::IS_ee_TRUTH) = true;
      TLorentzVector ele0;
      ele0.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(0)),
                        dressedLeptonEta(*electronsAfterSelection.at(0)),
                        dressedLeptonPhi(*electronsAfterSelection.at(0)),
                        dressedLeptonE(*electronsAfterSelection.at(0)));
      TLorentzVector ele1;
      ele1.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(1)),
                        dressedLeptonEta(*electronsAfterSelection.at(1)),
                        dressedLeptonPhi(*electronsAfterSelection.at(1)),
                        dressedLeptonE(*electronsAfterSelection.at(1)));
      ll = ele0 + ele1;
      SAME_CHARGE_LEPTONS_TRUTH = electronsAfterSelection.at(0)->charge()*electronsAfterSelection.at(1)->charge() > 0;
    }

    if (muonsAfterSelection.size() >= 2 && electronsAfterSelection.size() == 0)
    {
      m_bools.at(ssWWVBS::IS_mm_TRUTH) = true;
      TLorentzVector mu0;
      mu0.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(0)),
                       dressedLeptonEta(*muonsAfterSelection.at(0)),
                       dressedLeptonPhi(*muonsAfterSelection.at(0)),
                       dressedLeptonE(*muonsAfterSelection.at(0)));
      TLorentzVector mu1;
      mu1.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(1)),
                       dressedLeptonEta(*muonsAfterSelection.at(1)),
                       dressedLeptonPhi(*muonsAfterSelection.at(1)),
                       dressedLeptonE(*muonsAfterSelection.at(1)));
      ll = mu0 + mu1;
      SAME_CHARGE_LEPTONS_TRUTH = muonsAfterSelection.at(0)->charge()*muonsAfterSelection.at(1)->charge() > 0;
    }

    if (electronsAfterSelection.size() == 1 && muonsAfterSelection.size() == 1)
    {
      m_bools.at(ssWWVBS::IS_em_TRUTH) = true;
      TLorentzVector ele0;
      ele0.SetPtEtaPhiE(dressedLeptonPt(*electronsAfterSelection.at(0)),
                        dressedLeptonEta(*electronsAfterSelection.at(0)),
                        dressedLeptonPhi(*electronsAfterSelection.at(0)),
                        dressedLeptonE(*electronsAfterSelection.at(0)));
      TLorentzVector mu0;
      mu0.SetPtEtaPhiE(dressedLeptonPt(*muonsAfterSelection.at(0)),
                       dressedLeptonEta(*muonsAfterSelection.at(0)),
                       dressedLeptonPhi(*muonsAfterSelection.at(0)),
                       dressedLeptonE(*muonsAfterSelection.at(0)));
      ll = ele0 + mu0;
      SAME_CHARGE_LEPTONS_TRUTH = electronsAfterSelection.at(0)->charge()*muonsAfterSelection.at(0)->charge() > 0;
    }
    
    mll = ll.M();

    std::string prefix = "Truth_ll";
    m_Fbranches.at(prefix+"_pt")(truthevent) = ll.Pt();
    m_Fbranches.at(prefix+"_eta")(truthevent) = ll.Eta();
    m_Fbranches.at(prefix+"_phi")(truthevent) = ll.Phi();
    m_Fbranches.at(prefix+"_E")(truthevent) = ll.E();
    m_Fbranches.at(prefix+"_m")(truthevent) = mll;


    if(ssWWTruthCuts.exists("SAME_CHARGE_LEPTONS_TRUTH")) m_bools.at(ssWWVBS::SAME_CHARGE_LEPTONS_TRUTH) = SAME_CHARGE_LEPTONS_TRUTH;
    if(ssWWTruthCuts.exists("DILEPTON_MASS_TRUTH")) m_bools.at(ssWWVBS::DILEPTON_MASS_TRUTH) = ( mll >= 20.*Athena::Units::GeV );

  }

  void ssWWTruthSelectorAlg::evaluateTruthJetCuts
  (const ConstDataVector<xAOD::JetContainer>& truthjets, CutManager& ssWWTruthCuts)
  { 

    ///All jets in the containers should have pT>20GeV. Check minPt of your JetSelectorAlg in the ssWW_config file.
    if (truthjets.size() >= 2 && ssWWTruthCuts.exists("TWO_JETS_TRUTH")){
      if (truthjets.at(0)->pt() > 65 *Athena::Units::GeV && truthjets.at(1)->pt() > 35 *Athena::Units::GeV){
        m_bools.at(ssWWVBS::TWO_JETS_TRUTH) = true;
      }
    }
  }  

  // void ssWWTruthSelectorAlg::evaluateTruthMetCuts
  // (const ConstDataVector<xAOD::MissingETContainer>& truthmet, CutManager& ssWWTruthCuts)
  // { 
  //   if ((truthmet.size() && ssWWTruthCuts.exists("MET_TRUTH"))){
  //       m_bools.at(ssWWVBS::MET_TRUTH) = true;
  //   }

  // }
  
  StatusCode ssWWTruthSelectorAlg::initialiseCutflow(){


    std::vector<std::string> boolnameslist;
    for (const auto& [key, value] : m_boolnames) {
      boolnameslist.push_back(value);
    }
    m_ssWWTruthCuts.CheckInputCutList(m_inputCutList, boolnameslist);

    m_inputCutKeys.resize(m_inputCutList.size());
    std::vector<bool> inputWasFound (m_inputCutList.size(), false);
    for (const auto& [key, value]: m_boolnames) {
      auto it = std::find(m_inputCutList.begin(), m_inputCutList.end(), value);
      if (it != m_inputCutList.end()) {
        auto index = it - m_inputCutList.begin();
        m_inputCutKeys.at(index) = key;
        inputWasFound.at(index) = true;
      }
    }

    for (unsigned int index = 0; index < inputWasFound.size(); index++) {
      if(inputWasFound.at(index)) continue;
      ATH_MSG_ERROR("Doubled or falsely spelled cuts in CutList (see config file)." + m_inputCutList[index]);
    }

    for (const auto &cut : m_inputCutKeys) {
      m_ssWWTruthCuts.add(m_boolnames[cut]);
    }

    //After filling the CutManager, book your histograms.
    const unsigned int nbins = m_ssWWTruthCuts.size() + 1; //  need an extra bin for the total num of events.
    ATH_MSG_DEBUG("Booking AbsoluteTruthEfficiency histogram...");
    ANA_CHECK (book (TEfficiency("AbsoluteTruthEfficiency","Absolute Efficiency of ssWW truth cuts;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ATH_MSG_DEBUG("Booking RelativeTruthEfficiency histogram...");
    ANA_CHECK (book (TEfficiency("RelativeTruthEfficiency","Relative Efficiency of ssWW truth cuts;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ATH_MSG_DEBUG("Booking StandardTruthCutFlow histogram...");
    ANA_CHECK (book (TEfficiency("StandardTruthCutFlow","StandardCutFlow of ssWW truth cuts;Cuts;#epsilon",
				 nbins, 0.5, nbins + 0.5)));
    ATH_MSG_DEBUG("Booking TruthEventsPassed_BinLabeling histogram...");
    ANA_CHECK (book (TH1F("TruthEventsPassed_BinLabeling", "Events passed by each truth cut / Bin labeling", nbins, 0.5, nbins + 0.5)));
    ATH_MSG_DEBUG("Histograms booked successfully");
    return StatusCode::SUCCESS;
  }

}
