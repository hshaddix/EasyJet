/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "TopologyUtils.h"

#include <TMatrixD.h>
#include <TVectorD.h>
#include <algorithm>
#include <cmath>

namespace HHH6B
{

  // 3D momentum tensor eigenvalues
  std::array<double, 3>
  momentumTensorEigenvalues(const std::vector<TLorentzVector>& jets)
  {
    std::array<double, 3> out{{0,0,0}};
    if (jets.empty()) return out;

    TMatrixD tensor(3,3);
    double sum = 0.0;

    for (const auto& j : jets) {
      const TVector3 p = j.Vect();
      sum += p.Mag2();

      for (int i=0;i<3;i++)
        for (int k=0;k<3;k++)
          tensor[i][k] += p[i]*p[k];
    }

    if (sum <= 0) return out;

    tensor *= 1.0/sum;

    TVectorD eig;
    tensor.EigenVectors(eig);

    std::array<double,3> v{{eig[0],eig[1],eig[2]}};
    std::sort(v.rbegin(), v.rend());

    return v;
  }

  // =========================================================
  float calculateSphericity(const std::vector<TLorentzVector>& jets)
  {
    auto e = momentumTensorEigenvalues(jets);
    return 1.5 * (e[1] + e[2]);
  }

  float calculatePlanarity(const std::vector<TLorentzVector>& jets)
  {
    auto e = momentumTensorEigenvalues(jets);
    return 1.5 * e[1];
  }

  float calculateAplanarity(const std::vector<TLorentzVector>& jets)
  {
    auto e = momentumTensorEigenvalues(jets);
    return 1.5 * e[2];
  }

  float calculateThrust(const std::vector<TLorentzVector>& jetMomentum, float totalEnergy) 
  {
    if (jetMomentum.empty()) return 0.0;
    (void)totalEnergy;  // Suppress unused parameter warning

    // Transform jets to 3-momentum for thrust calculation
    float sumP = 0.0;
    std::vector<TVector3> jetMomenta;
    for (const auto& jet : jetMomentum) {
      TVector3 mom_vec = jet.Vect();
      jetMomenta.push_back(mom_vec);
      sumP += mom_vec.Mag();
    }
    
    if (sumP <= 0.0) return 0.0;

    // Find thrust direction by maximizing momentum projections
    // This is a simplified approach - for truly accurate thrust, use iterative optimization
    float maxThrust = 0.0;
    
    // Check multiple directions on unit sphere
    for (int phi = 0; phi < 360; phi += 30) {
      for (int theta = 0; theta < 180; theta += 30) {
        float phi_rad = phi * TMath::Pi() / 180.0;
        float theta_rad = theta * TMath::Pi() / 180.0;
        
        TVector3 thrust_dir(std::sin(theta_rad) * std::cos(phi_rad),
                           std::sin(theta_rad) * std::sin(phi_rad),
                           std::cos(theta_rad));
        
        float thrust_sum = 0.0;
        for (const auto& momentum : jetMomenta) {
          thrust_sum += std::abs(momentum.Dot(thrust_dir));
        }
        
        maxThrust = std::max(maxThrust, thrust_sum);
      }
    }
    
    return maxThrust / sumP;
  }


  // 2D transverse tensor
  std::array<double, 2>
  transverseMomentumTensorEigenvalues(const std::vector<TLorentzVector>& jets)
  {
    std::array<double,2> out{{0,0}};
    if (jets.empty()) return out;


    TMatrixD t(2,2);
    double sum = 0.0;

    for (const auto& j : jets) {
      double px = j.Px();
      double py = j.Py();
      double pt2 = px*px + py*py;

      sum += pt2;

      t[0][0] += px*px;
      t[0][1] += px*py;
      t[1][0] += py*px;
      t[1][1] += py*py;
    }

    if (sum <= 0) return out;

    t *= 1.0/sum;

    TVectorD eig;
    t.EigenVectors(eig);

    std::array<double,2> v{{eig[0],eig[1]}};
    if (v[1] > v[0]) std::swap(v[0], v[1]);

    return v;
  }

  float calculateTransverseSphericity(const std::vector<TLorentzVector>& jets)
  {
    auto e = transverseMomentumTensorEigenvalues(jets);

    double l1 = e[0];
    double l2 = e[1];

    if (l1 + l2 <= 0) return 0.0;

    return 2.0 * l2 / (l1 + l2);
  }

}