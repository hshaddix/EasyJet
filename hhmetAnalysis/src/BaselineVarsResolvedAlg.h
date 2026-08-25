/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef HHMETANALYSIS_FINALVARSRESOLVEDALG
#define HHMETANALYSIS_FINALVARSRESOLVEDALG

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHMET
{

  /// \brief An algorithm for counting containers
  class BaselineVarsResolvedAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
    public:

      using AthHistogramAlgorithm::AthHistogramAlgorithm;
      //BaselineVarsResolvedAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// We use default finalize() -- this is for cleanup, and we don't do any

    private:
    
      struct PairingChoice
      {
        std::array<unsigned int, 4> indices{{0, 1, 2, 3}};
        float metric{0.0};
        float dR1{0.0};
        float dR2{0.0};

        void orderByPt(const std::vector<const xAOD::Jet*> &jets);
      };

      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::JetContainer>
        m_jetHandle {this, "smallRContainerInKey", "", "Selected jet container to read"};

      CP::SysReadHandle<xAOD::JetContainer>
        m_recoJetHandle {this, "recoJetContainerInKey", "", "Reco-jet container written to the output tree"};
    
      CP::SysReadHandle<xAOD::EventInfo>
        m_eventHandle {this, "event", "EventInfo", "EventInfo container to read"};

      CP::SysReadDecorHandle<char>
    	  m_isBtag {this, "bTagSelDecor", "", "Name of input dectorator for b-tagging"};
    
      Gaudi::Property<std::string>
        m_bTagWP {this, "bTagWP", "", "B-tagging working point."};
    
      Gaudi::Property<std::vector<std::string>>
        m_pairingAlgorithms {this, "pairingAlgorithms", {"mass_125", "min_deltaR_max"}, "Resolved b-jet pairing algorithms to run."};

      Gaudi::Property<float>
        m_higgsMass {this, "higgsMass", 125000.0, "Higgs mass hypothesis in MeV for mass-based pairing."};

      Gaudi::Property<bool>
        m_isMC {this, "isMC", false, "Whether to try truth-labelling diagnostics."};

      Gaudi::Property<std::string>
        m_truthBLabelDecorName {this, "truthBLabelDecorName", "HadronConeExclTruthLabelID", "Jet int decoration used to identify truth-b jets. A value of 5 is treated as b."};

      Gaudi::Property<std::string>
        m_truthHiggsParentsMaskDecorName {this, "truthHiggsParentsMaskDecorName", "parentHiggsParentsMask", "Jet unsigned long long decoration encoding the truth Higgs parent mask from EasyJetHub."};

      // clang-format off
      std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_floatDecos;
      std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_intDecos;
      std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_commonFloatDecos;
      std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_commonIntDecos;
      
      const std::vector<std::string> m_commonFloatVars{
        "hh_m", "hh_pt", "hh_eta", "hh_phi",
      };

      const std::vector<std::string> m_commonIntVars{
        "reco_success", "n_truth_pairing_bjets", "n_truth_higgs_pairing_bjets", "truth_status",
      };

      std::vector<std::string> m_floatVars{
        "h1_m", "h1_pt", "h1_eta", "h1_phi", "h1_deltaR",
        "h2_m", "h2_pt", "h2_eta", "h2_phi", "h2_deltaR",
        "hh_deltaR", "hh_deltaEta", "metric"
      };

      std::vector<std::string> m_intVars{
        "j1_index", "j2_index", "j3_index", "j4_index",
        "j1_is_b", "j2_is_b", "j3_is_b", "j4_is_b", "truth_correct",
        "truth_parent_j1", "truth_parent_j2", "truth_parent_j3", "truth_parent_j4",
      };

      std::string decorName(const std::string &algorithm, const std::string &var) const;
      std::string baseDecorName(const std::string &var) const;
      PairingChoice choosePairing(const std::string &algorithm,
                                const std::vector<const xAOD::Jet*> &jets) const;
      bool isTruthBJet(const xAOD::Jet &jet) const;
      unsigned long long truthHiggsParentsMask(const xAOD::Jet &jet) const;
      int truthParentIndex(std::vector<const xAOD::Jet*> &jets,
                           std::size_t jetIndex) const;
      int nTruthBJets(const std::vector<const xAOD::Jet*> &jets) const;
      int nTruthHiggsBJets(const std::vector<const xAOD::Jet*> &jets) const;
      int truthStatus(const std::vector<const xAOD::Jet*> &jets) const;
      int truthCorrect(const std::vector<const xAOD::Jet*> &jets, const PairingChoice &choice, int jet_truthStatus) const;
      void setTruthParentDecorations(const xAOD::EventInfo &eventInfo,
                                     const std::string &algorithm,
                                     std::vector<const xAOD::Jet*> &jets,
                                     const PairingChoice &choice,
                                     const CP::SystematicSet &sys) const;
      void setDefaults(const xAOD::EventInfo &eventInfo,
                       const CP::SystematicSet &sys) const;

      // clang-format on
  
  };
}

#endif
