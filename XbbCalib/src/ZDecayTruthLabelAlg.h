/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/

#ifndef XBBCALIB_ZDECAYTRUTHLABELALG_H
#define XBBCALIB_ZDECAYTRUTHLABELALG_H

#include <AthenaBaseComps/AthAlgorithm.h>
#include <StoreGate/ReadHandleKey.h>
#include <StoreGate/WriteDecorHandleKey.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODTruth/TruthParticleContainer.h>

namespace XBBCALIB {

/// Classifies the Z boson decay mode from generator-level truth and writes
/// z_is_cc (int, 1 if Z->cc, 0 otherwise) as a decoration on EventInfo.
///
/// Uses TruthBosonsWithDecayParticles: any charm quark (|pdgId|==4) in this
/// container indicates a Z->cc decay. Only schedule on Zqq DSIDs.
class ZDecayTruthLabelAlg final : public AthAlgorithm {
public:
  using AthAlgorithm::AthAlgorithm;

  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) override;

private:
  SG::ReadHandleKey<xAOD::EventInfo> m_eventKey{
    this, "EventInfo", "EventInfo", "EventInfo key"};

  SG::WriteDecorHandleKey<xAOD::EventInfo> m_isCC_key{
    this, "ZDecayDecoration", "EventInfo.z_is_cc",
    "Truth Z->cc label written on EventInfo"};

  SG::ReadHandleKey<xAOD::TruthParticleContainer> m_truthKey{
    this, "TruthBosons", "TruthBosonsWithDecayParticles",
    "Container with the Z boson and its direct decay products"};
};

} // namespace XBBCALIB
#endif
