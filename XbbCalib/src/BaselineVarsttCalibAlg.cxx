/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/
/// @author Derrick Allen
/// @author Jason Oliver - systematics, scale factors, handle aliasing.
#include "BaselineVarsttCalibAlg.h"
#include "AthenaBaseComps/AthMsgStreamMacros.h"

#include "DefaultOutputs.h"
#include "FourVectorOutBlock.h"
#include "SystVariableCopier.h"

#include <format>
namespace XBBCALIB
{
  BaselineVarsttCalibAlg::BaselineVarsttCalibAlg(const std::string &name,
                                                 ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }
  BaselineVarsttCalibAlg::~BaselineVarsttCalibAlg() = default;

  StatusCode BaselineVarsttCalibAlg::initialize()
  {
    ATH_CHECK(m_ttcalib_jetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ttcalib_lrjetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ttcalib_muonHandle.initialize(m_systematicsList));
    ATH_CHECK(m_ttcalib_electronHandle.initialize(m_systematicsList));
    ATH_CHECK(m_metHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventHandle.initialize(m_systematicsList));

    m_ProbeJet_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "ProbeJet_", m_systematicsList, m_eventHandle);
    m_TagJet_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "TagJet_", m_systematicsList, m_eventHandle);
    m_Lepton_candidate_4vec = std::make_unique<FourVectorOutBlock>(
        this, "Lepton_", m_systematicsList, m_eventHandle);

    // set up the generic float copying
    m_ProbeJet_float_copier = std::make_unique<SystVariableCopier<float>>(
        this, m_floats_to_copy, m_systematicsList, m_ttcalib_lrjetHandle,
        m_eventHandle, m_copied_variable_prefix0);

    m_ProbeJet_int_copier = std::make_unique<SystVariableCopier<int>>(
        this, m_ints_to_copy, m_systematicsList, m_ttcalib_lrjetHandle,
        m_eventHandle, m_copied_variable_prefix0);

    ATH_CHECK(m_tagjet_lep_m.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_dRJetLep.initialize(m_systematicsList, m_eventHandle));

    ATH_CHECK(m_nEl.initialize(m_systematicsList, m_eventHandle));
    ATH_CHECK(m_nMu.initialize(m_systematicsList, m_eventHandle));

    if (m_isMC)
    {

      // retreive the AnalysisElectrons and AnalysisMuon Containers for scale
      // factor extraction
      ATH_CHECK(m_electronHandle.initialize(m_systematicsList));
      ATH_CHECK(m_muonHandle.initialize(m_systematicsList));

      // scale factor Read handles m_ele_SF =
      m_ele_SF = CP::SysReadDecorHandle<float>(
          "effSF_" + m_electronWPName + "_%SYS%", this);
      m_mu_SF = CP::SysReadDecorHandle<float>(
          "effSF_" + m_muonWPName + "_%SYS%", this);

      // initialize the scale factos from AnalysisElectrons and AnalysisMuons
      // initialize the scale factos from AnalysisElectrons and AnalysisMuons
      // NOTE: use the `electronHandle` and `muonHandle` here because scale
      // factor systematics are not registered on the `ttcalib_*` handles. We
      // have to read from the original `Analysis*` lepton containers to pick
      // up the systematics.
      ATH_CHECK(m_ele_SF.initialize(m_systematicsList, m_electronHandle));
      ATH_CHECK(m_mu_SF.initialize(m_systematicsList, m_muonHandle));
      ATH_CHECK(m_Lepton1_effSF.initialize(m_systematicsList, m_eventHandle));

      // ATH_CHECK(
      //     m_ProbeJet_TruthLabel.initialize(m_systematicsList,
      //     m_eventHandle));
      // // initialize large jet truth decorator
      // ATH_CHECK(m_probeJetTruthLabel.initialize(m_systematicsList,
      // m_ttcalib_lrjetHandle));
    }
    // Intialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK(m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsttCalibAlg::execute()
  {
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {

      const xAOD::EventInfo *event = nullptr;
      ANA_CHECK(m_eventHandle.retrieve(event, sys));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK(m_ttcalib_jetHandle.retrieve(jets, sys));

      const xAOD::JetContainer *lrjets = nullptr;
      ANA_CHECK(m_ttcalib_lrjetHandle.retrieve(lrjets, sys));

      const xAOD::MuonContainer *muons = nullptr;
      ANA_CHECK(m_ttcalib_muonHandle.retrieve(muons, sys));

      const xAOD::ElectronContainer *electrons = nullptr;
      ANA_CHECK(m_ttcalib_electronHandle.retrieve(electrons, sys));

      const xAOD::MissingETContainer *metCont = nullptr;
      ANA_CHECK(m_metHandle.retrieve(metCont, sys));

      // Is this needed?
      const xAOD::MissingET *met = (*metCont)["Final"];

      if (!met)
      {
        ATH_MSG_ERROR("Could not retrieve MET");
        return StatusCode::FAILURE;
      }
      // A bunch of ints I need to save at one point
      int nLeptons = muons->size() + electrons->size();
      int nMuons = muons->size();
      int nElectrons = electrons->size();
      bool isSingleLeptonEvent = (nLeptons == 1);
      bool isSingleMuonEvent = (nMuons == 1 && nElectrons == 0);
      bool isSingleElectronEvent = (nElectrons == 1 && nMuons == 0);
      int nLargeRJets = lrjets->size();
      int hasProbeJetCandidate = (nLargeRJets >= 1) ? 1 : 0;
      float m_minMet = 20000;
      bool pass_minMet_selection = met->met() > m_minMet;

      TLorentzVector lepton;
      const xAOD::Jet *tagjet = nullptr;
      const xAOD::Jet *probejet = nullptr;
      float minDeltaR = std::numeric_limits<float>::max(); // Why max?

      m_nEl.set(*event, nElectrons, sys);
      m_nMu.set(*event, nMuons, sys);

      if (isSingleMuonEvent)
      {
        lepton = muons->at(0)->p4();
        if (m_isMC)
        {
          float SF = m_mu_SF.get(*muons->at(0), sys);

          m_Lepton1_effSF.set(*event, SF, sys);
        }
      }
      else if (isSingleElectronEvent)
      {
        lepton = electrons->at(0)->p4();
        if (m_isMC)
        {
          float SF = m_ele_SF.get(*electrons->at(0), sys);
          m_Lepton1_effSF.set(*event, SF, sys);
        }
      }

      if (isSingleLeptonEvent && pass_minMet_selection)
      {
        for (const xAOD::Jet *jet : *jets)
        {
          float deltaR = lepton.DeltaR(jet->p4());
          if (deltaR < minDeltaR)
          {
            minDeltaR = deltaR;
            tagjet = jet;
          }
        }
      }

      if (tagjet && hasProbeJetCandidate)
      {
        probejet = lrjets->at(0);

        m_ProbeJet_candidate_4vec->set(*event, *probejet, sys);

        m_ProbeJet_float_copier->set(*event, *probejet, sys);
        m_ProbeJet_int_copier->set(*event, *probejet, sys);

        m_TagJet_candidate_4vec->set(*event, *tagjet, sys);

        m_Lepton_candidate_4vec->set(*event, lepton, sys);

        float tag_lep_m = (tagjet->p4() + lepton).M();
        m_tagjet_lep_m.set(*event, tag_lep_m, sys);
        m_dRJetLep.set(*event, minDeltaR, sys); // why min but above max?
      }
      else
      {
        m_ProbeJet_candidate_4vec->setDefault(*event, sys);

        m_ProbeJet_float_copier->setDefault(*event, sys);
        m_ProbeJet_int_copier->setDefault(*event, sys);

        m_TagJet_candidate_4vec->setDefault(*event, sys);

        m_Lepton_candidate_4vec->setDefault(*event, sys);

        m_tagjet_lep_m.set(*event, -99., sys);
        m_dRJetLep.set(*event, -99., sys); // why min but above max?
      }
    }
    return StatusCode::SUCCESS;
  }
} // namespace XBBCALIB