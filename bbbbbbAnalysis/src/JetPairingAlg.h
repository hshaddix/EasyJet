/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HHH6BANALYSIS_JETPAIRINGALG
#define HHH6BANALYSIS_JETPAIRINGALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <xAODEventInfo/EventInfo.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>
#include <SystematicsHandles/SysListHandle.h>

#include <xAODJet/JetContainer.h>

namespace HHH6B
{
  class JetSet {
  public:
    // Original variables
    float dr1 {0.};
    float dr2 {0.};
    float dr3 {0.};
    float deta1 {0.};
    float deta2 {0.};
    float deta3 {0.};
    float dphi1 {0.};
    float dphi2 {0.};
    float dphi3 {0.};
    float jet_system_pt_1 {0.};
    float jet_system_pt_2 {0.};
    float jet_system_pt_3 {0.};
    float jet_system_mass_1 {0.};
    float jet_system_mass_2 {0.};
    float jet_system_mass_3 {0.};
    float systems_mass_difference {0.};
    float m6j {0.};
    
    // === NEW VARIABLES FOR minDeltaR COMPUTED VARIABLES ===
    // Inter-jet pair delta-R calculations
    float dr12 {0.};
    float dr34 {0.};
    float dr56 {0.};
    float dr13 {0.};
    float dr14 {0.};
    float dr24 {0.};
    float dr35 {0.};
    
    // Delta-eta between specific jet pairs
    float deta_jet1_jet2 {0.};
    float deta_jet3_jet4 {0.};
    float deta_jet5_jet6 {0.};
    
    // Delta-phi between specific jet pairs
    float dphi_jet1_jet2 {0.};
    float dphi_jet3_jet4 {0.};
    float dphi_jet5_jet6 {0.};
    
    // Jet system pT for Higgs candidates
    float jet12_pt {0.};
    float jet34_pt {0.};
    float jet56_pt {0.};
    
    // Jet system masses for Higgs candidates  
    float jet12_mass {0.};
    float jet34_mass {0.};
    float jet56_mass {0.};
    
    // Combined system variables
    float m4j {0.};  // For H1+H2 combination
    
    // Mass differences between Higgs candidate pairs
    float mass_diff_12_34 {0.};
    float mass_diff_12_56 {0.};
    float mass_diff_34_56 {0.};
    
    // Parent mask for the 6 paired jets (shows which Higgs each jet belongs to)
    std::vector<float> parentMasks {0., 0., 0., 0., 0., 0.};
    
    // Jet pairing indices - original jet indices that were paired together
    std::vector<float> jetPairingIndices {0., 1., 2., 3., 4., 5.};  // [H1_j1, H1_j2, H2_j1, H2_j2, H3_j1, H3_j2]
    
    // Higgs mass radius variable
    float mH_radius {0.};  // Euclidean distance from (120,115,110) GeV center

    JetSet() = default;
  };

  enum PairingStrategy {
    minDeltaR = 0,
    minTargetMass,
    invalid
  };



  /// \brief An algorithm for counting containers
  class JetPairingAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    JetPairingAlg(const std::string &name, ISvcLocator *pSvcLocator);

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

    PairingStrategy stringtoPairingStrategy(const std::string& pairing_strategy_str);
    void extractParentMasks(const ConstDataVector<xAOD::JetContainer>& jets, std::vector<float>& parentMasks);
    void storeJetPairingIndices(const std::vector<size_t>& best_pairing, const xAOD::Jet* firstJet);

private:
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    CP::SysListHandle m_systematicsList {this};
    CP::SysReadHandle<xAOD::JetContainer>
      m_inJetHandle{this, "containerInKey", "", "Input jet container to read"};
    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
      m_outJetHandle{this, "containerOutKey", "", "Output jet container to write"};
    SG::ReadHandleKey<xAOD::EventInfo> m_eventInfoKey{
        this, "eventInfoKey", "EventInfo", "eventInfo container"};


    Gaudi::Property<std::string> m_pairingStrategy {
        this, "pairingStrategy", "", "Pairing strategy."};
    // Declare the enum for jet pairing strategy
    PairingStrategy m_pairing_strategy {invalid};
    
    Gaudi::Property<std::string> m_analysisRegion {
        this, "analysisRegion", "", "Analysis Region"};
  };
}

#endif
