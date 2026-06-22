/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

/// @author Can Suslu

#include "JetPairingAlg.h"
#include "AthContainers/AuxElement.h"
#include <AthContainers/ConstDataVector.h>
#include "FourMomUtils/xAODP4Helpers.h"
#include <AthenaKernel/Units.h>
#include "PathResolver/PathResolver.h"
#include "TFile.h"
#include <vector>
#include <cmath>
#include <limits>
#include <array>
#include <algorithm>
#include <TLorentzVector.h>

namespace {
  /// The 15 unique ways to split six labeled jets into three pairs.  Fixes the pair that
  /// contains index 0 (5 choices), then the three matchings of the other four indices.
  template <typename F>
  void forEachUniqueSixJetPairing(F &&f) {
    for (size_t p = 1; p < 6; ++p) {
      size_t r[4]{};
      size_t n = 0;
      for (size_t k = 0; k < 6; ++k) {
        if (k != 0 && k != p) {
          r[n++] = k;
        }
      }
      const size_t a = r[0], b = r[1], c = r[2], d = r[3];
      f(0, p, a, b, c, d);
      f(0, p, a, c, b, d);
      f(0, p, a, d, b, c);
    }
  }
}  // namespace

// For the semi-resolved regime
namespace {

  template <typename F>
  void forEachUniqueFourJetPairing(F&& f) {

    // (0,1)(2,3)
    f(0,1,2,3);

    // (0,2)(1,3)
    f(0,2,1,3);

    // (0,3)(1,2)
    f(0,3,1,2);
  }

}

namespace HHH6B
{
  JetPairingAlg ::JetPairingAlg(const std::string &name,
                                ISvcLocator *pSvcLocator)
      : AthHistogramAlgorithm(name, pSvcLocator)
  {
  }

  StatusCode JetPairingAlg ::initialize()
  {
    ATH_CHECK(m_inJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_outJetHandle.initialize(m_systematicsList));
    ATH_CHECK(m_eventInfoKey.initialize());
    
    // Convert string to enum
    // Pairing strategy
    m_pairing_strategy = stringtoPairingStrategy(m_pairingStrategy);


    ATH_CHECK (m_systematicsList.initialize());
    return StatusCode::SUCCESS;
  }

  StatusCode JetPairingAlg ::execute(const EventContext& /*ctx*/)
  {
    // SG::ReadHandle<xAOD::EventInfo> evtInfoContainer(m_eventInfoKey, ctx);    // ATH_CHECK(evtInfoContainer.isValid()); 
    // [[maybe_unused]] uint64_t event_number = evtInfoContainer->eventNumber();
    // Loop over all systs
    for (const auto &sys : m_systematicsList.systematicsVector())
    {
      // Retrieve inputs
      const xAOD::JetContainer *inContainer = nullptr;
      ATH_CHECK(m_inJetHandle.retrieve(inContainer, sys));

      // fill workContainer with "views" of the inContainer
      // see TJ's tutorial for this
      auto workContainer = std::make_unique<ConstDataVector<xAOD::JetContainer>>(
          inContainer->begin(), inContainer->end(), SG::VIEW_ELEMENTS);

      // this assumes that container is pt sorted (use the JetSelectorAlg for
      // this) and checks if we have at least 6 jets otherwise exit this alg
      if (m_pairing_strategy == HHH6B::PairingStrategy::minDeltaR)
      {
        if (m_analysisRegion == "resolved" && workContainer->size() >= 6) {
          // Find the optimal pairing that minimizes Delta R within Higgs pairs
          float best_deltaR_sum = std::numeric_limits<float>::max();
          std::vector<size_t> best_pairing;
          
          // 15 unique partitions of 6 jets into 3 pairs (old 6-level loops counted each partition several times)
          forEachUniqueSixJetPairing([&](size_t i1, size_t j1, size_t i2, size_t j2, size_t i3, size_t j3) {
                      float dr_pair1 = xAOD::P4Helpers::deltaR((*workContainer)[i1], (*workContainer)[j1]);
                      float dr_pair2 = xAOD::P4Helpers::deltaR((*workContainer)[i2], (*workContainer)[j2]);
                      float dr_pair3 = xAOD::P4Helpers::deltaR((*workContainer)[i3], (*workContainer)[j3]);
                      float total_deltaR = dr_pair1 + dr_pair2 + dr_pair3;
                      if (total_deltaR < best_deltaR_sum) {
                        best_deltaR_sum = total_deltaR;
                        best_pairing = {i1, j1, i2, j2, i3, j3};
                      }
          });
          
          // Apply the best pairing: reorder the workContainer according to best_pairing
          if (!best_pairing.empty()) {
            // Get the three jet pairs from the best pairing  
            std::vector<std::array<const xAOD::Jet*, 2>> all_candidates(3);
            all_candidates[0] = {(*workContainer)[best_pairing[0]], (*workContainer)[best_pairing[1]]};
            all_candidates[1] = {(*workContainer)[best_pairing[2]], (*workContainer)[best_pairing[3]]};
            all_candidates[2] = {(*workContainer)[best_pairing[4]], (*workContainer)[best_pairing[5]]};
            
            // Order pairs by their combined pT (highest pT first)
            std::vector<float> pair_pts(3);
            for (int i = 0; i < 3; ++i) {
              pair_pts[i] = (all_candidates[i][0]->p4() + all_candidates[i][1]->p4()).Pt();
            }
            
            // Sort pairs by pT (descending)
            std::vector<int> pair_order = {0, 1, 2};
            std::sort(pair_order.begin(), pair_order.end(), 
                    [&pair_pts](int a, int b) { return pair_pts[a] > pair_pts[b]; });
            
            // Apply the ordered pairing
            (*workContainer)[0] = all_candidates[pair_order[0]][0]; // cand1 jet 1 (highest dijet pT)
            (*workContainer)[1] = all_candidates[pair_order[0]][1]; // cand1 jet 2 (highest dijet pT)
            (*workContainer)[2] = all_candidates[pair_order[1]][0]; // cand2 jet 1
            (*workContainer)[3] = all_candidates[pair_order[1]][1]; // cand2 jet 2
            (*workContainer)[4] = all_candidates[pair_order[2]][0]; // cand3 jet 1
            (*workContainer)[5] = all_candidates[pair_order[2]][1]; // cand3 jet 2
          }

          // Store the best pairing indices as a decoration on the first jet
          storeJetPairingIndices(best_pairing, (*workContainer)[0]);

          // Keep 3 dijet candidates (6 jets total) in the container.
          // Candidates are ordered by dijet pT (cand1 highest), with cand2/cand3
          // optionally swapped below based on single-jet pT.
          if ((*workContainer)[2]->pt() < (*workContainer)[4]->pt())
          {
            // no swap method on ConstDataVector, so by hand
            const xAOD::Jet *temp2 = (*workContainer)[2];
            const xAOD::Jet *temp3 = (*workContainer)[3];
            (*workContainer)[2] = (*workContainer)[4];
            (*workContainer)[3] = (*workContainer)[5];
            (*workContainer)[4] = temp2;
            (*workContainer)[5] = temp3;
          }
          
          // Parent masks will be extracted directly in BaselineVarsResolvedAlg
          
          // keep only the higgs candidate ones to avoid confusion
          workContainer->erase(workContainer->begin() + 6, workContainer->end());

        } else if (m_analysisRegion == "semiresolved"  && workContainer->size() >= 4) {
          float best_deltaR_sum = std::numeric_limits<float>::max();
          std::vector<size_t> best_pairing;

          forEachUniqueFourJetPairing(
              [&](size_t i1, size_t j1,
                  size_t i2, size_t j2)
          {
              float dr1 =
                  xAOD::P4Helpers::deltaR(
                      (*workContainer)[i1],
                      (*workContainer)[j1]);

              float dr2 =
                  xAOD::P4Helpers::deltaR(
                      (*workContainer)[i2],
                      (*workContainer)[j2]);

              float total_dr = dr1 + dr2;

              if (total_dr < best_deltaR_sum) {
                  best_deltaR_sum = total_dr;
                  best_pairing = {i1, j1, i2, j2};
              }
          });

          if (!best_pairing.empty()) {

              std::vector<std::array<const xAOD::Jet*,2>> all_candidates(2);

              all_candidates[0] = {
                  (*workContainer)[best_pairing[0]],
                  (*workContainer)[best_pairing[1]]
              };

              all_candidates[1] = {
                  (*workContainer)[best_pairing[2]],
                  (*workContainer)[best_pairing[3]]
              };

              // order by dijet pT
              std::vector<float> pair_pts(2);

              for (int i = 0; i < 2; ++i) {
                  pair_pts[i] =
                      (all_candidates[i][0]->p4()
                    + all_candidates[i][1]->p4()).Pt();
              }

              std::vector<int> order = {0,1};

              std::sort(order.begin(), order.end(),
                  [&](int a, int b) {
                      return pair_pts[a] > pair_pts[b];
                  });

              (*workContainer)[0] =
                  all_candidates[order[0]][0];

              (*workContainer)[1] =
                  all_candidates[order[0]][1];

              (*workContainer)[2] =
                  all_candidates[order[1]][0];

              (*workContainer)[3] =
                  all_candidates[order[1]][1];
          }

          storeJetPairingIndices(best_pairing,
                                (*workContainer)[0]);

          // keep only the paired jets
          workContainer->erase(workContainer->begin() + 4,
                              workContainer->end());
        
        }
      } else if(m_pairing_strategy == HHH6B::PairingStrategy::minTargetMass)
      {
        if (m_analysisRegion == "resolved" && workContainer->size() >= 6) {
          // Target masses for each Higgs candidate
          const float target_m1 = 120.0; // GeV
          const float target_m2 = 115.0; // GeV  
          const float target_m3 = 110.0; // GeV
          
          float best_score = std::numeric_limits<float>::max();
          std::vector<size_t> best_pairing;
          
          forEachUniqueSixJetPairing([&](size_t i1, size_t j1, size_t i2, size_t j2, size_t i3, size_t j3) {
                      xAOD::JetFourMom_t tlv_pair1 = (*workContainer)[i1]->jetP4() + (*workContainer)[j1]->jetP4();
                      xAOD::JetFourMom_t tlv_pair2 = (*workContainer)[i2]->jetP4() + (*workContainer)[j2]->jetP4();
                      xAOD::JetFourMom_t tlv_pair3 = (*workContainer)[i3]->jetP4() + (*workContainer)[j3]->jetP4();
                      float mass_pair1 = tlv_pair1.M() / Athena::Units::GeV;  // Convert MeV to GeV 
                      
                      float mass_pair2 = tlv_pair2.M() / Athena::Units::GeV;
                      float mass_pair3 = tlv_pair3.M() / Athena::Units::GeV;
                      std::vector<float> masses = {mass_pair1, mass_pair2, mass_pair3};
                      std::vector<float> targets = {target_m1, target_m2, target_m3};
                      float local_best_score = 999999;
                      std::vector<int> perm = {0, 1, 2};
                      do {
                        float perm_score = std::abs(masses[perm[0]] - targets[0]) +
                                        std::abs(masses[perm[1]] - targets[1]) +
                                        std::abs(masses[perm[2]] - targets[2]);
                        if (perm_score < local_best_score) {
                          local_best_score = perm_score;
                        }
                      } while (std::next_permutation(perm.begin(), perm.end()));
                      if (local_best_score < best_score) {
                        best_score = local_best_score;
                        best_pairing = {i1, j1, i2, j2, i3, j3};
                      }
          });
          
          // Apply the best pairing: reorder the workContainer according to best_pairing
          if (!best_pairing.empty()) {
            // Get the three jet pairs from the best pairing  
            std::vector<std::array<const xAOD::Jet*, 2>> all_candidates(3);
            all_candidates[0] = {(*workContainer)[best_pairing[0]], (*workContainer)[best_pairing[1]]};
            all_candidates[1] = {(*workContainer)[best_pairing[2]], (*workContainer)[best_pairing[3]]};
            all_candidates[2] = {(*workContainer)[best_pairing[4]], (*workContainer)[best_pairing[5]]};
            
            // Calculate masses for each Higgs candidate
            std::vector<float> masses(3);
            for (int i = 0; i < 3; ++i) {
              TLorentzVector sum4mom = all_candidates[i][0]->p4() + all_candidates[i][1]->p4();
              masses[i] = sum4mom.M() / Athena::Units::GeV;  // GeV
            }
            
            // Assign targets: H1->120 GeV, H2->115 GeV, H3->110 GeV
            std::vector<float> targets = {120.0f, 115.0f, 110.0f};
            
            // Find the best assignment by minimizing total distance
            std::vector<int> best_assignment = {0, 1, 2};  // default assignment
            float best_total_distance = 999999;
            
            // Try all permutations of assignments
            std::vector<int> assignment = {0, 1, 2};
            do {
              float total_distance = 0;
              for (int i = 0; i < 3; ++i) {
                total_distance += std::abs(masses[assignment[i]] - targets[i]);
              }
              if (total_distance < best_total_distance) {
                best_total_distance = total_distance;
                best_assignment = assignment;
              }
            } while (std::next_permutation(assignment.begin(), assignment.end()));
            
            // Apply the best assignment
            (*workContainer)[0] = all_candidates[best_assignment[0]][0]; // H1 jet 1
            (*workContainer)[1] = all_candidates[best_assignment[0]][1]; // H1 jet 2
            (*workContainer)[2] = all_candidates[best_assignment[1]][0]; // H2 jet 1
            (*workContainer)[3] = all_candidates[best_assignment[1]][1]; // H2 jet 2
            (*workContainer)[4] = all_candidates[best_assignment[2]][0]; // H3 jet 1
            (*workContainer)[5] = all_candidates[best_assignment[2]][1]; // H3 jet 2
            
            
          }

          // Store the best pairing indices as a decoration on the first jet
          storeJetPairingIndices(best_pairing, (*workContainer)[0]);

          // lets return the pairing of the leading (h1) and subleading (h2) Higgs
          // candidates as four jets in the order:
          // h1_leading_pt_jet
          // h1_subleading_pt_jet
          // h2_leading_pt_jet
          // h2_subleading_pt_jet
          if ((*workContainer)[2]->pt() < (*workContainer)[4]->pt())
          {
            // no swap method on ConstDataVector, so by hand
            const xAOD::Jet *temp = (*workContainer)[2];
            (*workContainer)[2] = (*workContainer)[4];
            (*workContainer)[3] = temp;
          }
          
          // Parent masks will be extracted directly in BaselineVarsResolvedAlg
          
          // Keep only the higgs candidate ones to avoid confusion
          workContainer->erase(workContainer->begin() + 6, workContainer->end());

        } else if (m_analysisRegion == "semiresolved" && workContainer->size() >= 4) {

            // Target masses for the two Higgs candidates
            const float target_m1 = 120.0; // GeV
            const float target_m2 = 115.0; // GeV

            float best_score = std::numeric_limits<float>::max();
            std::vector<size_t> best_pairing;

            forEachUniqueFourJetPairing(
                [&](size_t i1, size_t j1,
                    size_t i2, size_t j2)
            {
                xAOD::JetFourMom_t tlv_pair1 =
                    (*workContainer)[i1]->jetP4()
                  + (*workContainer)[j1]->jetP4();

                xAOD::JetFourMom_t tlv_pair2 =
                    (*workContainer)[i2]->jetP4()
                  + (*workContainer)[j2]->jetP4();

                float mass1 =
                    tlv_pair1.M() / Athena::Units::GeV;

                float mass2 =
                    tlv_pair2.M() / Athena::Units::GeV;

                // Two possible assignments:
                //
                // pair1 -> H1, pair2 -> H2
                // OR
                // pair1 -> H2, pair2 -> H1

                float score_a =
                    std::abs(mass1 - target_m1)
                  + std::abs(mass2 - target_m2);

                float score_b =
                    std::abs(mass1 - target_m2)
                  + std::abs(mass2 - target_m1);

                float local_best =
                    std::min(score_a, score_b);

                if (local_best < best_score) {
                    best_score = local_best;
                    best_pairing = {i1, j1, i2, j2};
                }
            });

            // Apply best pairing
            if (!best_pairing.empty())
            {
                std::vector<std::array<const xAOD::Jet*,2>>
                    all_candidates(2);

                all_candidates[0] = {
                    (*workContainer)[best_pairing[0]],
                    (*workContainer)[best_pairing[1]]
                };

                all_candidates[1] = {
                    (*workContainer)[best_pairing[2]],
                    (*workContainer)[best_pairing[3]]
                };

                // Compute masses
                std::vector<float> masses(2);

                for (int i = 0; i < 2; ++i)
                {
                    TLorentzVector sum4mom =
                        all_candidates[i][0]->p4()
                      + all_candidates[i][1]->p4();

                    masses[i] =
                        sum4mom.M() / Athena::Units::GeV;
                }

                // Determine assignment
                float score_a =
                    std::abs(masses[0] - target_m1)
                  + std::abs(masses[1] - target_m2);

                float score_b =
                    std::abs(masses[0] - target_m2)
                  + std::abs(masses[1] - target_m1);

                std::vector<int> assignment;

                if (score_a <= score_b) {
                    assignment = {0,1};
                }
                else {
                    assignment = {1,0};
                }

                // Apply ordering
                (*workContainer)[0] =
                    all_candidates[assignment[0]][0];

                (*workContainer)[1] =
                    all_candidates[assignment[0]][1];

                (*workContainer)[2] =
                    all_candidates[assignment[1]][0];

                (*workContainer)[3] =
                    all_candidates[assignment[1]][1];
            }

            // Store pairing indices
            storeJetPairingIndices(best_pairing,
                                  (*workContainer)[0]);

            // Optional:
            // Ensure H1 has higher dijet pT than H2

            xAOD::JetFourMom_t h1 =
                (*workContainer)[0]->jetP4()
              + (*workContainer)[1]->jetP4();

            xAOD::JetFourMom_t h2 =
                (*workContainer)[2]->jetP4()
              + (*workContainer)[3]->jetP4();

            if (h2.Pt() > h1.Pt())
            {
                const xAOD::Jet* temp0 = (*workContainer)[0];
                const xAOD::Jet* temp1 = (*workContainer)[1];

                (*workContainer)[0] = (*workContainer)[2];
                (*workContainer)[1] = (*workContainer)[3];

                (*workContainer)[2] = temp0;
                (*workContainer)[3] = temp1;
            }

            // Keep only 4 jets
            workContainer->erase(
              workContainer->begin() + 4,
              workContainer->end());
            
          }
        }
        ATH_CHECK(m_outJetHandle.record(std::move(workContainer), sys));
      }
    return StatusCode::SUCCESS;
  }

  PairingStrategy JetPairingAlg::stringtoPairingStrategy(const std::string& pairing_strategy_str) {
    if(pairing_strategy_str=="minDeltaR"){
      return HHH6B::PairingStrategy::minDeltaR;
    }
    else if(pairing_strategy_str=="minTargetMass"){
      return HHH6B::PairingStrategy::minTargetMass;
    }
    else {
      ATH_MSG_ERROR("Invalid vbfjets method string");
      return HHH6B::PairingStrategy::invalid;
    }
  }

  void JetPairingAlg::extractParentMasks(const ConstDataVector<xAOD::JetContainer>& jets, std::vector<float>& parentMasks) {
    ATH_MSG_INFO("extractParentMasks called with " << jets.size() << " jets");
    parentMasks.resize(6, -1.0f);  // Initialize with -1 for missing masks
    
    for (size_t i = 0; i < std::min(size_t(6), jets.size()); ++i) {
      const xAOD::Jet* jet = jets[i];
      
      // Try to get the parent mask - try multiple possible variable names
      std::vector<std::string> possible_names = {
        "recojet_antiktPFlow_parentHiggsParentsMask",  // The actual variable name from ROOT file
        "recojet_antiktPFlow_parentHiggsParentsMask_NOSYS",
        "recojet_antikt4PFlow_parentHiggsParentsMask",
        "recojet_antikt4PFlow_parentHiggsParentsMask_NOSYS",
        "parentHiggsParentsMask_NOSYS",
        "parentHiggsParentsMask",
        "recojet_antikt10UFO_parentHiggsParentsMask_NOSYS",
        "HiggsParentsMask_NOSYS",
        "parentHiggsMask_NOSYS",
        "parentHiggsParentsMask_NOSYS"
      };
      
      bool mask_found = false;
      uint64_t mask = 0;
      
      ATH_MSG_INFO("Checking jet " << i << " for parent mask variables...");
      
      // List all available auxiliary data on this jet
      ATH_MSG_INFO("Available auxiliary data on jet " << i << ":");
      for (const auto& aux : jet->getAuxIDs()) {
        ATH_MSG_INFO("  " << aux);
      }
      
      for (const auto& var_name : possible_names) {
        try {
          const SG::ConstAccessor<uint64_t> acc(var_name);
          if (acc.isAvailable(*jet)) {
            mask = acc(*jet);
            mask_found = true;
            ATH_MSG_INFO("Found parent mask variable: " << var_name << " with value: " << mask);
            break;
          } else {
            ATH_MSG_INFO("Variable " << var_name << " not available for jet " << i);
          }
        } catch (const std::exception& e) {
          ATH_MSG_INFO("Exception when checking " << var_name << ": " << e.what());
          continue;
        }
      }
      
      if (mask_found) {
        // Convert uint64_t to float for storage
        parentMasks[i] = static_cast<float>(mask);
        ATH_MSG_INFO("Set parentMasks[" << i << "] = " << parentMasks[i]);
      } else {
        ATH_MSG_INFO("No parent mask found for jet " << i);
      }
    }
  }

  void JetPairingAlg::storeJetPairingIndices(const std::vector<size_t>& best_pairing, const xAOD::Jet* firstJet) {
    // Store the best pairing indices as a decoration on the first jet
    const SG::AuxElement::Decorator<std::vector<int>> jet_pairing_indices_dec("jet_pairing_indices");
    std::vector<int> pairing_indices;
    pairing_indices.reserve(best_pairing.size());
    for (size_t i = 0; i < best_pairing.size(); ++i) {
      pairing_indices.push_back(static_cast<int>(best_pairing[i]));
    }
    jet_pairing_indices_dec(*firstJet) = pairing_indices;
    
    if (m_analysisRegion == "resolved") {
      ATH_MSG_DEBUG("Stored jet pairing indices: ["
                  << pairing_indices[0] << ", " << pairing_indices[1] << ", "
                  << pairing_indices[2] << ", " << pairing_indices[3] << ", "
                  << pairing_indices[4] << ", " << pairing_indices[5] << "]");
    } else if (m_analysisRegion == "semiresolved") {
      ATH_MSG_DEBUG("Stored jet pairing indices: ["
                  << pairing_indices[0] << ", " << pairing_indices[1] << ", "
                  << pairing_indices[2] << ", " << pairing_indices[3] << "]");
    }
  }
} // namespace HHH6B
