/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef ZCHARMANALYSIS_ZCHARMTRUTHSELECTORALG
#define ZCHARMANALYSIS_ZCHARMTRUTHSELECTORALG

#include <memory>
#include "EasyjetHub/BaseSelectorAlg.h"
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/WriteDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>

#include <AthenaBaseComps/AthReentrantAlgorithm.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODJet/JetAuxContainer.h>
#include <xAODTruth/TruthParticleContainer.h> 
#include <xAODTruth/TruthParticleAuxContainer.h>
#include <xAODTruth/TruthParticle.h>


#include "ZCharmEnums.h"


namespace ZCC
{

  /// \brief An algorithm for counting containers
  class ZCharmTruthSelectorAlg final : public Easyjet::BaseSelectorAlg {

    public:
    using Easyjet::BaseSelectorAlg::BaseSelectorAlg;

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute() override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override; ///I added this to write the cutflow histogram.

    private :

      const std::vector<std::string> m_BASELINE_CUTS{
        "EXACTLY_TWO_LEPTONS_TRUTH",
        "OPPOSITE_CHARGE_LEPTONS_TRUTH",
        "DILEPTON_MASS_WINDOW_TRUTH",
      };

      Gaudi::Property<bool> m_bypass
      { this, "bypass", false, "Run selector algorithm in pass-through mode" };


      SG::ReadHandleKey<xAOD::JetContainer>
      m_truthjetsInKey{ this, "truthSmallJetInContainer", "", "Truth jet container to read" };

      SG::ReadHandleKey<xAOD::EventInfo>
      m_eventInKey{ this, "event", "EventInfo", "EventInfo container to read" };

      SG::ReadHandleKey<xAOD::TruthParticleContainer>
      m_truthelectronInKey{ this, "truthElectronInContainer", "", "Truth electron container to read" };

      SG::ReadHandleKey<xAOD::TruthParticleContainer>
      m_truthmuonInKey{ this, "truthMuonInContainer", "", "Truth muon container to read" };

      SG::ReadDecorHandleKey<xAOD::JetContainer> m_jetTruthFlavourKey
	{this, "jetTruthFlavourDecoration", m_truthjetsInKey,
	    "HadronConeExclTruthLabelID", "Name of jet truth flavour decoration"};

      std::unordered_map<ZCC::Booleans, SG::WriteDecorHandleKey<xAOD::EventInfo>> m_BbranchKeys;
      std::unordered_map<ZCC::Booleans, bool> m_bools;
      std::unordered_map<ZCC::Booleans, std::string> m_boolnames{
        {ZCC::IS_ee_TRUTH, "IS_ee_TRUTH"},
        {ZCC::IS_mm_TRUTH, "IS_mm_TRUTH"},
        {ZCC::IS_em_TRUTH, "IS_em_TRUTH"},
        {ZCC::EXACTLY_TWO_LEPTONS_TRUTH, "EXACTLY_TWO_LEPTONS_TRUTH"},
        {ZCC::OPPOSITE_CHARGE_LEPTONS_TRUTH, "OPPOSITE_CHARGE_LEPTONS_TRUTH"},
        {ZCC::DILEPTON_MASS_WINDOW_TRUTH, "DILEPTON_MASS_WINDOW_TRUTH"},
        {ZCC::ONE_C_JETS_TRUTH, "ONE_C_JETS_TRUTH"},
        {ZCC::TWO_C_JETS_TRUTH, "TWO_C_JETS_TRUTH"},
      };
      std::unordered_map<std::string, ZCC::Booleans> m_nameToBool;
      
      SG::WriteDecorHandleKey<xAOD::EventInfo> m_passTruthCutsKey{
        this, "PassTruthCutsKey", m_eventInKey, "PassTruthCuts", "Decorate event with pass/fail flag"
      };


      // Floating point variables for truth c-jets and Z boson
      Gaudi::Property<std::vector<std::string>> m_floatVariables
          {this, "floatVariableList", {}, "Name list of floating variables"};

      std::unordered_map<std::string, SG::WriteDecorHandleKey<xAOD::EventInfo>>
        m_FbranchesKeys;

      std::unordered_map<std::string, SG::WriteDecorHandle<xAOD::EventInfo, float>> m_Fbranches;

      void truthOverlapRemoval
	(const xAOD::JetContainer& truthJets,
	 ConstDataVector<xAOD::TruthParticleContainer>& truthElectrons,
	 ConstDataVector<xAOD::TruthParticleContainer>& truthMuons,
	 ConstDataVector<xAOD::JetContainer>& jetsAfterOverlap);
      void evaluateTruthLeptonCuts
	(const xAOD::EventInfo& truthevent,
	 const xAOD::TruthParticleContainer& truthElectrons,
	 const xAOD::TruthParticleContainer& truthMuons,
	 ConstDataVector<xAOD::TruthParticleContainer>& electronsAfterSelection,
	 ConstDataVector<xAOD::TruthParticleContainer>& muonsAfterSelection);
      void evaluateTruthCJetCuts(const ConstDataVector<xAOD::JetContainer>& truthCjets);
      void setThresholds(const xAOD::EventInfo* event);
  };

}

#endif 
