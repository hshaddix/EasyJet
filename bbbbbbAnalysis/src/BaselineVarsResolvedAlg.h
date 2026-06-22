/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HHH6BANALYSIS_FINALVARSRESOLVEDALG
#define HHH6BANALYSIS_FINALVARSRESOLVEDALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <TLorentzVector.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHH6B
{

  /// \brief An algorithm for counting containers
  class BaselineVarsResolvedAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    BaselineVarsResolvedAlg(const std::string &name, ISvcLocator *pSvcLocator);

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    // Jet event shape calculation methods
    float calculateThrust(const std::vector<TLorentzVector>& jetMomentum, float totalEnergy);
    float calculateSphericity(const std::vector<TLorentzVector>& jetMomentum);
    float calculatePlanarity(const std::vector<TLorentzVector>& jetMomentum);
    float calculateAplanarity(const std::vector<TLorentzVector>& jetMomentum);
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    CP::SysListHandle m_systematicsList {this};
    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle{ this, "smallRContainerInKey", "", "Jet container to read" };
    CP::SysReadHandle<xAOD::JetContainer> 
      m_RNNjetResolvedHandle{ this, "RNNJets_resolved", "RNNJets_resolved_%SYS%", "input RNN jets container for resolved 20 GeV" };
    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    Gaudi::Property<std::string> m_bTagWP {this, "bTagWP", "", "B-tagging working point."};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_decos;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<std::vector<int>>> m_decos_vector;
    // clang-format off
    std::vector<std::string> m_vars{
      // Original Higgs variables (always used)
      "resolved_h1_m_",
      "resolved_h1_pt_",
      "resolved_h1_eta_",
      "resolved_h1_phi_",
      "resolved_h2_m_",
      "resolved_h2_pt_",
      "resolved_h2_eta_",
      "resolved_h2_phi_",
      "resolved_h3_m_",
      "resolved_h3_pt_",
      "resolved_h3_eta_",
      "resolved_h3_phi_",
      "resolved_hhh_m_",
      "resolved_hhh_pt_",
      // === ADDED: minDeltaR computed variables matching Python config ===
      // Original Delta-R variables
      "resolved_DeltaR12_",
      "resolved_DeltaR13_",
      "resolved_DeltaR14_", 
      "resolved_DeltaR23_",
      "resolved_DeltaR24_",
      "resolved_DeltaR34_",
      // NEW computed variables for minDeltaR
      "resolved_DeltaR56_",    // dr56
      "resolved_DeltaR35_",    // dr35
      // Delta-eta/phi calculations
      "resolved_DeltaEtaJet1Jet2_",    // deta_jet1_jet2
      "resolved_DeltaEtaJet3Jet4_",    // deta_jet3_jet4  
      "resolved_DeltaEtaJet5Jet6_",    // deta_jet5_jet6
      "resolved_DeltaPhiJet1Jet2_",    // dphi_jet1_jet2
      "resolved_DeltaPhiJet3Jet4_",    // dphi_jet3_jet4
      "resolved_DeltaPhiJet5Jet6_",    // dphi_jet5_jet6
      // Jet system kinematic variables
      "resolved_Jet12Pt_",             // jet12_pt
      "resolved_Jet34Pt_",             // jet34_pt  
      "resolved_Jet56Pt_",             // jet56_pt
      "resolved_Jet12Mass_",           // jet12_mass  
      "resolved_Jet34Mass_",           // jet34_mass
      "resolved_Jet56Mass_",           // jet56_mass
      // Combined system variables  
      "resolved_M4j_",                  // m4j - invariant mass of 4 leading jets
      "resolved_M6j_",                  // m6j - invariant mass of 6 leading jets
      // Mass differences between Higgs candidate pairs
      "resolved_MassDiff12vs34_",      // mass_diff_12_34
      "resolved_MassDiff12vs56_",      // mass_diff_12_56
      "resolved_MassDiff34vs56_",      // mass_diff_34_56
      // Jet event shape variables
      "resolved_Thrust_",              // thrust
      "resolved_Sphericity_",          // sphericity  
      "resolved_TransverseSphericity_",// transverse sphericity  
      "resolved_Planarity_",           // planarity
      "resolved_Aplanarity_",          // aplanarity
      // Parent mask variables for b-jets
      "resolved_bjets_parentMasks_",   // parent masks for 6 b-jets
      // Jet pairing indices - store original jet indices that were paired together as a vector
      "resolved_jet_pairing_indices_", // vector of original jet indices [H1_j1, H1_j2, H2_j1, H2_j2, H3_j1, H3_j2]
      // Higgs mass radius variable
      "resolved_mH_radius_",           // Euclidean distance from (120,115,110) GeV center
    };
    // clang-format on
  
    Gaudi::Property<std::vector<std::string>> m_Fvars_RNN
      {this, "floatVariableList", {}, "Name list of floating variables"};
    
//   Gaudi::Property<bool> m_UseVBFRNN { this, "UseVBFRNN", false, "Add VBF-RNN jets in ntuples or not" };
  };
}

#endif