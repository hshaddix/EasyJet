/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Can Suslu 

#include "BaselineVarsResolvedAlg.h"
#include "TopologyUtils.h"
#include "AthContainers/AuxElement.h"
#include "FourMomUtils/xAODP4Helpers.h"
#include <AthenaKernel/Units.h>
#include <TLorentzVector.h>
#include <TMatrixD.h>
#include <TVectorD.h>
#include <TMath.h>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <memory>
#include <cmath>
#include <array>

namespace HHH6B
{
  StatusCode BaselineVarsResolvedAlg ::initialize()
  {
    ATH_CHECK (m_jetHandle.initialize(m_systematicsList));
    // if(m_UseVBFRNN){
    //   ATH_CHECK (m_RNNjetResolvedHandle.initialize(m_systematicsList));
    // }
    ATH_CHECK (m_eventHandle.initialize(m_systematicsList));

    // make decorators
    for (const std::string& var : m_vars)
    {
      std::string deco_var = var + m_bTagWP;
      
      // Special handling for vector variables
      if (var == "resolved_jet_pairing_indices_") {
        CP::SysWriteDecorHandle<std::vector<int>> deco{deco_var + "_%SYS%", this};
        m_decos_vector.emplace(deco_var, deco);
        ATH_CHECK (m_decos_vector.at(deco_var).initialize(m_systematicsList, m_eventHandle));
      } else {
        CP::SysWriteDecorHandle<float> deco{deco_var + "_%SYS%", this};
        // CP::SysWriteDecorHandle<float> deco{deco_var + "_NOSYS", this};
        m_decos.emplace(deco_var, deco);
        ATH_CHECK (m_decos.at(deco_var).initialize(m_systematicsList, m_eventHandle));
      }
    }

    // make decorators (RNN Jets)
    // for (const std::string &string_var: m_Fvars_RNN) {
    //   CP::SysWriteDecorHandle<float> var {string_var+"_%SYS%", this};
    //   m_decos.emplace(string_var, var);
    //   ATH_CHECK (m_decos.at(string_var).initialize(m_systematicsList, m_eventHandle));
    // }

    // Initialise syst list (must come after all syst-aware inputs and outputs)
    ATH_CHECK (m_systematicsList.initialize());

    return StatusCode::SUCCESS;
  }

  StatusCode BaselineVarsResolvedAlg ::execute(const EventContext& ctx)
  {
    ATH_MSG_DEBUG("BaselineVarsResolvedAlg::execute() called");
    for (const auto& sys : m_systematicsList.systematicsVector()) {
      const xAOD::EventInfo *eventInfo = nullptr;
      ANA_CHECK (m_eventHandle.retrieve (eventInfo, sys, ctx));

      const xAOD::JetContainer *jets = nullptr;
      ANA_CHECK (m_jetHandle.retrieve (jets, sys, ctx));

      // const xAOD::JetContainer *RNNJets= nullptr;
      // if(m_UseVBFRNN) {
      //   ANA_CHECK (m_RNNjetResolvedHandle.retrieve (RNNJets, sys, ctx));
      // }
      // set defaults
      for (const std::string& var : m_vars)
      {
        std::string deco_var = var + m_bTagWP;
        
        // Special handling for vector variables
        if (var == "resolved_jet_pairing_indices_") {
          std::vector<int> defaultVector(6, -1);
          m_decos_vector.at(deco_var).set(*eventInfo, defaultVector, sys);
        } else {
          m_decos.at(deco_var).set(*eventInfo, -1, sys);
        }
      }

      // check if we have 6 small R jets
      ATH_MSG_DEBUG("Processing jets: size = " << jets->size());
      if (jets->size() >= 6)
      {
        // construct Higgs Candidates
        xAOD::JetFourMom_t h1 = jets->at(0)->jetP4() + jets->at(1)->jetP4();
        xAOD::JetFourMom_t h2 = jets->at(2)->jetP4() + jets->at(3)->jetP4();
        xAOD::JetFourMom_t h3 = jets->at(4)->jetP4() + jets->at(5)->jetP4();
        
        // === COMPUTE AND STORE minDeltaR COMPUTED VARIABLES ===
        // Store jet four-momenta for calculations  
        xAOD::JetFourMom_t tlv1 = jets->at(0)->jetP4();
        xAOD::JetFourMom_t tlv2 = jets->at(1)->jetP4();
        xAOD::JetFourMom_t tlv3 = jets->at(2)->jetP4();
        xAOD::JetFourMom_t tlv4 = jets->at(3)->jetP4();
        xAOD::JetFourMom_t tlv5 = jets->at(4)->jetP4();
        xAOD::JetFourMom_t tlv6 = jets->at(5)->jetP4();
        
        // Inter-jet ΔR, Δη, Δφ from jetP4() (tlv*), same convention as masses below
        const auto deltaRjj = [](const xAOD::JetFourMom_t& a,
                                 const xAOD::JetFourMom_t& b) -> float {
          return static_cast<float>(
            xAOD::P4Helpers::deltaR(a.Eta(), a.Phi(), b.Eta(), b.Phi()));
        };
        const auto dphiAbs = [](const xAOD::JetFourMom_t& a,
                                const xAOD::JetFourMom_t& b) -> float {
          return std::abs(static_cast<float>(
            xAOD::P4Helpers::deltaPhi(a.Phi(), b.Phi())));
        };

        float deltaR12 = deltaRjj(tlv1, tlv2);
        float deltaR13 = deltaRjj(tlv1, tlv3);
        float deltaR14 = deltaRjj(tlv1, tlv4);
        float deltaR23 = deltaRjj(tlv2, tlv3);
        float deltaR24 = deltaRjj(tlv2, tlv4);
        float deltaR34 = deltaRjj(tlv3, tlv4);
        float deltaR56 = deltaRjj(tlv5, tlv6);
        float deltaR35 = deltaRjj(tlv3, tlv5);

        float deta_jet1_jet2 = std::abs(tlv1.Eta() - tlv2.Eta());
        float deta_jet3_jet4 = std::abs(tlv3.Eta() - tlv4.Eta());
        float deta_jet5_jet6 = std::abs(tlv5.Eta() - tlv6.Eta());

        float dphi_jet1_jet2 = dphiAbs(tlv1, tlv2);
        float dphi_jet3_jet4 = dphiAbs(tlv3, tlv4);
        float dphi_jet5_jet6 = dphiAbs(tlv5, tlv6);
        
        // Calculate jet system kinematic variables for Higgs candidates
        float jet12_pt = (tlv1 + tlv2).Pt();
        float jet34_pt = (tlv3 + tlv4).Pt();
        float jet56_pt = (tlv5 + tlv6).Pt();
        
        float jet12_mass = (tlv1 + tlv2).M();
        float jet34_mass = (tlv3 + tlv4).M();
        float jet56_mass = (tlv5 + tlv6).M();
        
        // Calculate combined system variables
        float m4j = (tlv1 + tlv2 + tlv3 + tlv4).M();  // Invariant mass of 4-leading jets
        float m6j = (tlv1 + tlv2 + tlv3 + tlv4 + tlv5 + tlv6).M();  // Invariant mass of 6 leading jets
        
        // Calculate mass differences between Higgs candidate pairs
        float mass_diff_12_34 = std::abs(jet12_mass - jet34_mass);
        float mass_diff_12_56 = std::abs(jet12_mass - jet56_mass);
        float mass_diff_34_56 = std::abs(jet34_mass - jet56_mass);
        
        // Calculate jet event shape variables
        std::vector<TLorentzVector> jetMomenta;
        float totalEnergy = 0.0;
        for (int i = 0; i < 6; ++i) {
          TLorentzVector jetMom = TLorentzVector(jets->at(i)->px(), jets->at(i)->py(), 
                                                 jets->at(i)->pz(), jets->at(i)->e());
          jetMomenta.push_back(jetMom);
          totalEnergy += jetMom.E();
        }
      
        float thrust_value = HHH6B::calculateThrust(jetMomenta, totalEnergy);
        float sphericity_value = HHH6B::calculateSphericity(jetMomenta);
        float transversesphericity_value = HHH6B::calculateTransverseSphericity(jetMomenta);
        float planarity_value = HHH6B::calculatePlanarity(jetMomenta);
        float aplanarity_value = HHH6B::calculateAplanarity(jetMomenta);
        
        // Store computed variables as decorations
        m_decos.at("resolved_DeltaR12_" + m_bTagWP).set(*eventInfo, deltaR12, sys);
        m_decos.at("resolved_DeltaR13_" + m_bTagWP).set(*eventInfo, deltaR13, sys);
        m_decos.at("resolved_DeltaR14_" + m_bTagWP).set(*eventInfo, deltaR14, sys);
        m_decos.at("resolved_DeltaR23_" + m_bTagWP).set(*eventInfo, deltaR23, sys);
        m_decos.at("resolved_DeltaR24_" + m_bTagWP).set(*eventInfo, deltaR24, sys);
        m_decos.at("resolved_DeltaR34_" + m_bTagWP).set(*eventInfo, deltaR34, sys);
        m_decos.at("resolved_DeltaR56_" + m_bTagWP).set(*eventInfo, deltaR56, sys);
        m_decos.at("resolved_DeltaR35_" + m_bTagWP).set(*eventInfo, deltaR35, sys);
        
        // Delta-eta/phi decorations
        m_decos.at("resolved_DeltaEtaJet1Jet2_" + m_bTagWP).set(*eventInfo, deta_jet1_jet2, sys);
        m_decos.at("resolved_DeltaEtaJet3Jet4_" + m_bTagWP).set(*eventInfo, deta_jet3_jet4, sys);
        m_decos.at("resolved_DeltaEtaJet5Jet6_" + m_bTagWP).set(*eventInfo, deta_jet5_jet6, sys);
        m_decos.at("resolved_DeltaPhiJet1Jet2_" + m_bTagWP).set(*eventInfo, dphi_jet1_jet2, sys);
        m_decos.at("resolved_DeltaPhiJet3Jet4_" + m_bTagWP).set(*eventInfo, dphi_jet3_jet4, sys);
        m_decos.at("resolved_DeltaPhiJet5Jet6_" + m_bTagWP).set(*eventInfo, dphi_jet5_jet6, sys);
        
        // Jet system kinematic decorations
        m_decos.at("resolved_Jet12Pt_" + m_bTagWP).set(*eventInfo, jet12_pt, sys);
        m_decos.at("resolved_Jet34Pt_" + m_bTagWP).set(*eventInfo, jet34_pt, sys);
        m_decos.at("resolved_Jet56Pt_" + m_bTagWP).set(*eventInfo, jet56_pt, sys);
        m_decos.at("resolved_Jet12Mass_" + m_bTagWP).set(*eventInfo, jet12_mass, sys);
        m_decos.at("resolved_Jet34Mass_" + m_bTagWP).set(*eventInfo, jet34_mass, sys);
        m_decos.at("resolved_Jet56Mass_" + m_bTagWP).set(*eventInfo, jet56_mass, sys);
        
        // Combined system decorations
        m_decos.at("resolved_M4j_" + m_bTagWP).set(*eventInfo, m4j, sys);
        m_decos.at("resolved_M6j_" + m_bTagWP).set(*eventInfo, m6j, sys);
        
        // Mass difference decorations
        m_decos.at("resolved_MassDiff12vs34_" + m_bTagWP).set(*eventInfo, mass_diff_12_34, sys);
        m_decos.at("resolved_MassDiff12vs56_" + m_bTagWP).set(*eventInfo, mass_diff_12_56, sys);
        m_decos.at("resolved_MassDiff34vs56_" + m_bTagWP).set(*eventInfo, mass_diff_34_56, sys);
        
        // Euclidean distance in (m12, m34, m56) from (120, 115, 110) GeV (same jetP4() masses as above)
        const float mH1_GeV = jet12_mass / Athena::Units::GeV;
        const float mH2_GeV = jet34_mass / Athena::Units::GeV;
        const float mH3_GeV = jet56_mass / Athena::Units::GeV;
        constexpr float target_mH1 = 120.0f;
        constexpr float target_mH2 = 115.0f;
        constexpr float target_mH3 = 110.0f;
        const float mH_radius = std::sqrt(
          std::pow(mH1_GeV - target_mH1, 2.0f) +
          std::pow(mH2_GeV - target_mH2, 2.0f) +
          std::pow(mH3_GeV - target_mH3, 2.0f));
        m_decos.at("resolved_mH_radius_" + m_bTagWP).set(*eventInfo, mH_radius, sys);
        
        // Jet event shape decorations
        m_decos.at("resolved_Thrust_" + m_bTagWP).set(*eventInfo, thrust_value, sys);
        m_decos.at("resolved_Sphericity_" + m_bTagWP).set(*eventInfo, sphericity_value, sys);
        m_decos.at("resolved_TransverseSphericity_" + m_bTagWP).set(*eventInfo, transversesphericity_value, sys);
        m_decos.at("resolved_Planarity_" + m_bTagWP).set(*eventInfo, planarity_value, sys);
        m_decos.at("resolved_Aplanarity_" + m_bTagWP).set(*eventInfo, aplanarity_value, sys);
        
        m_decos.at("resolved_h1_m_" + m_bTagWP).set(*eventInfo, h1.M(), sys);
        m_decos.at("resolved_h1_pt_" + m_bTagWP).set(*eventInfo, h1.Pt(), sys);
        m_decos.at("resolved_h1_eta_" + m_bTagWP).set(*eventInfo, h1.Eta(), sys);
        m_decos.at("resolved_h1_phi_" + m_bTagWP).set(*eventInfo, h1.Phi(), sys);
        m_decos.at("resolved_h2_m_" + m_bTagWP).set(*eventInfo, h2.M(), sys);
        m_decos.at("resolved_h2_pt_" + m_bTagWP).set(*eventInfo, h2.Pt(), sys);
        m_decos.at("resolved_h2_eta_" + m_bTagWP).set(*eventInfo, h2.Eta(), sys);
        m_decos.at("resolved_h2_phi_" + m_bTagWP).set(*eventInfo, h2.Phi(), sys);
        m_decos.at("resolved_h3_m_" + m_bTagWP).set(*eventInfo, h3.M(), sys);
        m_decos.at("resolved_h3_pt_" + m_bTagWP).set(*eventInfo, h3.Pt(), sys);
        m_decos.at("resolved_h3_eta_" + m_bTagWP).set(*eventInfo, h3.Eta(), sys);
        m_decos.at("resolved_h3_phi_" + m_bTagWP).set(*eventInfo, h3.Phi(), sys);
        m_decos.at("resolved_hhh_m_" + m_bTagWP).set(*eventInfo, (h1 + h2 + h3).M(), sys);
        m_decos.at("resolved_hhh_pt_" + m_bTagWP).set(*eventInfo, (h1 + h2 + h3).Pt(), sys);
        // m_decos.at("resolved_DeltaEtaHHH_" + m_bTagWP).set(*eventInfo, std::abs(h1.Eta() - h2.Eta() - h3.Eta()), sys);
        
        // === EXTRACT PARENT MASK FROM PAIRED JETS ===
        ATH_MSG_DEBUG("Starting parent mask extraction from paired Higgs jets...");
        // Read the parent masks from the 6 jets that are paired to form the 3 Higgs candidates
        std::vector<float> parentMasks(6, -1.0f);
        
        // Try to extract parent masks from each of the 6 jets
        for (size_t i = 0; i < 6; ++i) {
          const xAOD::Jet* jet = jets->at(i);
          
          // Try multiple possible variable names for parent masks
          std::vector<std::string> possible_names = {
            "recojet_antiktPFlow_parentHiggsParentsMask",
            "recojet_antiktPFlow_parentHiggsParentsMask_NOSYS", 
            "recojet_antikt4PFlow_parentHiggsParentsMask",
            "recojet_antikt4PFlow_parentHiggsParentsMask_NOSYS",
            "parentHiggsParentsMask_NOSYS",
            "parentHiggsParentsMask",
            "recojet_antikt10UFO_parentHiggsParentsMask_NOSYS",
            "HiggsParentsMask_NOSYS",
            "parentHiggsMask_NOSYS",
            "parentHiggsParentsMask_NOMINAL",
            "parentHiggsParentsMask_NOSYS_NOMINAL",
            "truthParentHiggsParentsMask",
            "truthHiggsParentsMask"
          };
          
          bool mask_found = false;
          uint64_t mask = 0;
          
          for (const auto& var_name : possible_names) {
            try {
              const SG::ConstAccessor<uint64_t> acc(var_name);
              if (acc.isAvailable(*jet)) {
                mask = acc(*jet);
                mask_found = true;
                ATH_MSG_DEBUG("Found parent mask variable: " << var_name << " with value: " << mask << " for jet " << i);
                break;
              }
            } catch (const std::exception& e) {
              // Try next variable name if this one fails
              continue;
            }
          }
          
          if (mask_found) {
            // Convert uint64_t to float for storage
            parentMasks[i] = static_cast<float>(mask);
            ATH_MSG_DEBUG("Set parentMasks[" << i << "] = " << parentMasks[i]);
          } else {
            ATH_MSG_DEBUG("No parent mask found for jet " << i);
          }
        }
        
        // Create combined mask encoding
        float combinedMask = 0.0f;
        for (size_t i = 0; i < 6; ++i) {
          if (parentMasks[i] >= 0) {
            combinedMask += parentMasks[i] * std::pow(10.0f, static_cast<float>(i));
          }
        }
        m_decos.at("resolved_bjets_parentMasks_" + m_bTagWP).set(*eventInfo, combinedMask, sys);
        
        // === EXTRACT JET PAIRING INDICES ===
        ATH_MSG_DEBUG("Reading jet pairing indices from JetPairingAlg decoration...");
        std::vector<int> pairingIndices(6, -1);  // Default values
        
        // Read the best pairing indices from the jet decoration (set by JetPairingAlg)
        const SG::AuxElement::ConstAccessor<std::vector<int>> jet_pairing_indices_acc("jet_pairing_indices");
        if (jet_pairing_indices_acc.isAvailable(*jets->at(0))) {
          pairingIndices = jet_pairing_indices_acc(*jets->at(0));
          ATH_MSG_DEBUG("Successfully read jet pairing indices from decoration: [" 
                       << pairingIndices[0] << ", " << pairingIndices[1] << ", " 
                       << pairingIndices[2] << ", " << pairingIndices[3] << ", " 
                       << pairingIndices[4] << ", " << pairingIndices[5] << "]");
        } else {
          ATH_MSG_DEBUG("Jet pairing indices decoration not found, using default values");
          // Fallback: Since JetPairingAlg reorders the jets, the first 6 jets are already in paired order
          for (size_t i = 0; i < 6; ++i) {
            pairingIndices[i] = static_cast<int>(i);
          }
        }
        
        // Store pairing indices as a single vector variable
        // Format: [H1_j1_index, H1_j2_index, H2_j1_index, H2_j2_index, H3_j1_index, H3_j2_index]
        m_decos_vector.at("resolved_jet_pairing_indices_" + m_bTagWP).set(*eventInfo, pairingIndices, sys);
      
        // if (m_UseVBFRNN){
        //   std::string RNNJets_names = "resolved_RNNJets";

        //   std::vector<float> rnn_jet_m(2, -99.0f);
        //   std::vector<float> rnn_jet_pt(2, -99.0f);
        //   std::vector<float> rnn_jet_eta(2, -99.0f);
        //   std::vector<float> rnn_jet_phi(2, -99.0f);
        //   for(unsigned int j=0; j<std::min(size_t(2),RNNJets->size()); j++) {
        //     const xAOD::Jet* RNNJet = RNNJets->at(j);
        //     rnn_jet_m[j] = RNNJet->m();
        //     rnn_jet_pt[j] = RNNJet->pt();
        //     rnn_jet_eta[j] = RNNJet->eta();
        //     rnn_jet_phi[j] = RNNJet->phi();
        //   }   

        //   for(unsigned int j=0; j<2; j++) {
        //     std::string prefix = "Jet"+std::to_string(j+1);

        //     m_decos.at(RNNJets_names+"_"+prefix+"_m").set(*eventInfo, rnn_jet_m[j], sys);
        //     m_decos.at(RNNJets_names+"_"+prefix+"_pt").set(*eventInfo, rnn_jet_pt[j], sys);
        //     m_decos.at(RNNJets_names+"_"+prefix+"_eta").set(*eventInfo, rnn_jet_eta[j], sys);
        //     m_decos.at(RNNJets_names+"_"+prefix+"_phi").set(*eventInfo, rnn_jet_phi[j], sys);
        //   }   
        // } // end of RNN jets
      }
    }
    return StatusCode::SUCCESS;
  }

}
