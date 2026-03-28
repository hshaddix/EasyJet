/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef DATAENRICHMENTALG_H
#define DATAENRICHMENTALG_H

#include <AthenaBaseComps/AthReentrantAlgorithm.h>
#include <StoreGate/ReadHandleKey.h>
#include <xAODJet/JetContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <AthContainers/AuxElement.h>
#include <atomic>

class DataEnrichmentAlg final : public AthReentrantAlgorithm {
public:
    using AthReentrantAlgorithm::AthReentrantAlgorithm;
    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) const override;

private:
    SG::ReadHandleKey<xAOD::JetContainer> m_jetKey{
        this, "JetKey",
        "AntiKt10UFOCSSKSoftDropBeta100Zcut10Jets",
        "large-R jet container"};
    SG::ReadHandleKey<xAOD::PhotonContainer> m_photonKey{
        this, "PhotonKey", "Photons", "photon container"};
    Gaudi::Property<float> m_minJetPt{
        this, "MinJetPt", 250'000.f, "minimum jet pt [MeV]"};
    Gaudi::Property<float> m_minPhotonPt{
        this, "MinPhotonPt", 150'000.f, "minimum photon pt [MeV]"};
    Gaudi::Property<bool> m_requirePhotonTight{
        this, "RequirePhotonTight", true,
        "require at least one photon to pass DFCommonPhotonsIsEMTight"};
    Gaudi::Property<bool> m_requirePhotonCleaning{
        this, "RequirePhotonCleaning", true,
        "require at least one photon to pass DFCommonPhotonsCleaning"};
    Gaudi::Property<long> m_maxWritten{
        this, "MaxWrittenEvents", -1L,
        "maximum events to write; -1 = unlimited"};
    mutable std::atomic<long> m_nWritten{0};
};

#endif // DATAENRICHMENTALG_H
