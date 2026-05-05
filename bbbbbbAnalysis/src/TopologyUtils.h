/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HHH6BANALYSIS_TOPOLOGYUTILS
#define HHH6BANALYSIS_TOPOLOGYUTILS

#include <vector>
#include <TLorentzVector.h>
#include <array>

namespace HHH6B {

  std::array<double, 3> momentumTensorEigenvalues(const std::vector<TLorentzVector>& jets);

  float calculateSphericity(const std::vector<TLorentzVector>& jets);
  float calculatePlanarity(const std::vector<TLorentzVector>& jets);
  float calculateAplanarity(const std::vector<TLorentzVector>& jets);
  float calculateThrust(const std::vector<TLorentzVector>& jets, float totalEnergy);

  std::array<double, 2> transverseMomentumTensorEigenvalues(const std::vector<TLorentzVector>& jets);

  float calculateTransverseSphericity(const std::vector<TLorentzVector>& jets);

}

#endif