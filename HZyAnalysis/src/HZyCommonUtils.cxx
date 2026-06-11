/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "HZyCommonUtils.h"

using namespace HZY;

FSR::FsrPhotonTool *CommonUtils::m_FsrPhotonTool = nullptr;

void CommonUtils::setFSRTool(FSR::FsrPhotonTool *tool)
{
  m_FsrPhotonTool = tool;
}

bool CommonUtils::findFSR(const xAOD::Muon *mu,
                          const xAOD::PhotonContainer *allPhotons,
                          const xAOD::ElectronContainer *allElectrons,
                          FSR::FsrCandidate &bestFSR, TLorentzVector &fsrP4)
{
  if (!m_FsrPhotonTool || !mu || !allPhotons || !allElectrons)
  {
    return false;
  }

  auto *fsrList =
      m_FsrPhotonTool->getNearFsrCandidateList(mu, allPhotons, allElectrons);
  if (!fsrList || fsrList->empty())
  {
    return false;
  }
  bestFSR = fsrList->front();

  fsrP4.SetPtEtaPhiM(bestFSR.Et, bestFSR.eta, bestFSR.phi, 0.0);
  return true;
}
