/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef SSWWANALYSIS_SSWWTRUTHSELECTORALG
#define SSWWANALYSIS_SSWWTRUTHSELECTORALG

#include <memory>
#include <AnaAlgorithm/AnaAlgorithm.h>
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>

#include <AthenaBaseComps/AthReentrantAlgorithm.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODJet/JetAuxContainer.h>
#include <xAODMissingET/MissingETContainer.h>
#include <xAODTruth/TruthParticleContainer.h> 
#include <xAODTruth/TruthParticleAuxContainer.h>
#include <xAODTruth/TruthParticle.h>


#include <EasyjetHub/CutManager.h>
#include "ssWWEnums.h"


namespace ssWWVBS
{

  /// \brief An algorithm for counting containers
  class ssWWTruthSelectorAlg final : public EL::AnaAlgorithm {

    public:
      ssWWTruthSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(/*const EventContext& ctx*/) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private :

      const std::vector<std::string> m_BASELINE_CUTS{
        "EXACTLY_TWO_LEPTONS_TRUTH",
        "SAME_CHARGE_LEPTONS_TRUTH",
        "DILEPTON_MASS_TRUTH",
        "MET_TRUTH",
        "TWO_JETS_TRUTH",
        "DIJET_MASS_TRUTH",
        "DIJET_RAPIDITY_TRUTH",
      };

      Gaudi::Property<bool> m_bypass
      { this, "bypass", false, "Run selector algorithm in pass-through mode" };


      SG::ReadHandleKey<xAOD::JetContainer>
      m_truthjetsInKey{ this, "truthSmallJetInContainer", "", "Truth jet container to read" };

      // SG::ReadHandleKey<xAOD::MissingETContainer>
      // m_truthmetInKey{ this, "truthMetInContainer", "", "Truth met container to read" };

      SG::ReadHandleKey<xAOD::EventInfo>
      m_eventInKey{ this, "event", "EventInfo", "EventInfo container to read" };

      SG::ReadHandleKey<xAOD::TruthParticleContainer>
      m_truthelectronInKey{ this, "truthElectronInContainer", "", "Truth electron container to read" };

      SG::ReadHandleKey<xAOD::TruthParticleContainer>
      m_truthmuonInKey{ this, "truthMuonInContainer", "", "Truth muon container to read" };

      long long int m_total_events{0};

      std::unordered_map<ssWWVBS::Booleans, SG::WriteDecorHandleKey<xAOD::EventInfo>> m_BbranchKeys;
      std::unordered_map<ssWWVBS::Booleans, bool> m_bools;
      std::unordered_map<ssWWVBS::Booleans, std::string> m_boolnames{
        {ssWWVBS::IS_ee_TRUTH, "IS_ee_TRUTH"},
        {ssWWVBS::IS_mm_TRUTH, "IS_mm_TRUTH"},
        {ssWWVBS::IS_em_TRUTH, "IS_em_TRUTH"},
        {ssWWVBS::EXACTLY_TWO_LEPTONS_TRUTH, "EXACTLY_TWO_LEPTONS_TRUTH"},
        {ssWWVBS::SAME_CHARGE_LEPTONS_TRUTH, "SAME_CHARGE_LEPTONS_TRUTH"},
        {ssWWVBS::DILEPTON_MASS_TRUTH, "DILEPTON_MASS_TRUTH"},
        {ssWWVBS::MET_TRUTH, "MET_TRUTH"},
        {ssWWVBS::TWO_JETS_TRUTH, "TWO_JETS_TRUTH"},
        {ssWWVBS::DIJET_MASS_TRUTH, "DIJET_MASS_TRUTH"},
        {ssWWVBS::DIJET_RAPIDITY_TRUTH, "DIJET_RAPIDITY_TRUTH"},
      };

      CutManager m_ssWWTruthCuts;
      Gaudi::Property<std::vector<std::string>> m_inputCutList{this, "truthcutList", {}};
      std::vector<ssWWVBS::Booleans> m_inputCutKeys;
      Gaudi::Property<bool> m_saveCutFlow{this, "saveCutFlow", false};
      
      
      SG::WriteDecorHandleKey<xAOD::EventInfo> m_passTruthCutsKey{
        this, "PassTruthCutsKey", "EventInfo.PassTruthCuts", "Decorate event with pass/fail flag"
      };


      // Floating point variables for truth c-jets and Z boson
      Gaudi::Property<std::vector<std::string>> m_floatVariables
          {this, "floatVariableList", {}, "Name list of floating variables"};

      std::unordered_map<std::string, SG::WriteDecorHandleKey<xAOD::EventInfo>>
        m_FbranchesKeys;

      std::unordered_map<std::string, SG::WriteDecorHandle<xAOD::EventInfo, float>> m_Fbranches;

      void truthOverlapRemoval(const xAOD::JetContainer& truthJets, ConstDataVector<xAOD::TruthParticleContainer>& truthElectrons, ConstDataVector<xAOD::TruthParticleContainer>& truthMuons, ConstDataVector<xAOD::JetContainer>& jetsAfterOverlap);
      void evaluateTruthLeptonCuts(const xAOD::EventInfo& truthevent, const xAOD::TruthParticleContainer& truthElectrons, const xAOD::TruthParticleContainer& truthMuons, CutManager& ssWWTruthCuts, ConstDataVector<xAOD::TruthParticleContainer>& electronsAfterSelection, ConstDataVector<xAOD::TruthParticleContainer>& muonsAfterSelection);
      void evaluateTruthJetCuts(const ConstDataVector<xAOD::JetContainer>& truthjets, CutManager& ssWWTruthCuts);
      // void evaluateTruthMetCuts(const ConstDataVector<xAOD::MissingETContainer>& truthmet, CutManager& ssWWTruthCuts);
      void setThresholds(const xAOD::EventInfo* event);
      StatusCode initialiseCutflow(); 
  };

}

#endif 
