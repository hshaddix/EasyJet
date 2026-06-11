/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HZYANALYSIS_HZYCOMMONUTILS
#define HZYANALYSIS_HZYCOMMONUTILS

#include <TLorentzVector.h>
#include <TVector2.h>

// Include necessary xAOD headers for the base types used by the templates
#include <xAODBase/IParticle.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODMuon/Muon.h>

#include <FsrUtils/FsrPhotonTool.h>

namespace HZY
{

  /// Check whether one particle overlaps with any particle in a reference
  /// container.
  template <class T>
  bool overlap(const xAOD::IParticle *ptcl, const T &refPtcls,
               const double DRcut)
  {
    if (!ptcl || DRcut < 0.)
      return false;
    const double dr2Cut = DRcut * DRcut;
    const double eta = ptcl->eta();
    const double phi = ptcl->phi();
    for (const auto *ref : refPtcls)
    {
      if (!ref || ref == ptcl)
        continue;

      const double dEta = eta - ref->eta();
      const double dPhi = TVector2::Phi_mpi_pi(phi - ref->phi());
      if (dEta * dEta + dPhi * dPhi < dr2Cut)
      {
        return true;
      }
    }
    return false;
  }

  /// Remove particles from `ptcls` if they overlap with any particle in
  /// `refPtcls`.
  template <class T1, class T2>
  void safeRemoveOverlap(T1 &ptcls, const T2 &refPtcls, const double DRcut)
  {
    T1 kept(SG::VIEW_ELEMENTS);
    for (auto ptcl : ptcls)
    {
      if (!overlap(ptcl, refPtcls, DRcut))
      {
        kept.push_back(ptcl);
      }
    }
    ptcls.swap(kept);
  }

  /// Common HZY helper utilities.
  class CommonUtils
  {
public:
    // Set FSR tool
    static void setFSRTool(FSR::FsrPhotonTool *tool);

    // Query FSR for a single muon
    static bool findFSR(const xAOD::Muon *mu,
                        const xAOD::PhotonContainer *allPhotons,
                        const xAOD::ElectronContainer *allElectrons,
                        FSR::FsrCandidate &bestFSR, TLorentzVector &fsrP4);

private:
    static FSR::FsrPhotonTool *m_FsrPhotonTool;
  };

} // namespace HZY

#endif // HZYANALYSIS_HZYCOMMONUTILS
