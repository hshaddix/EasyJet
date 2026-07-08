///////////////////////// -*- C++ -*- /////////////////////////////

/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// JetNeutrinoDecoratorAlg.h
// Author: ORLANDINI Romano
// Email : romano.orlandini@cern.ch
///////////////////////////////////////////////////////////////////

#ifndef EASYJET_JETNEUTRINODECORATORALG_H
#define EASYJET_JETNEUTRINODECORATORALG_H

#include "AthenaBaseComps/AthAlgorithm.h"
#include "StoreGate/ReadHandleKey.h"
#include "StoreGate/WriteDecorHandleKey.h"
#include "xAODJet/JetContainer.h"
#include "xAODTruth/TruthParticleContainer.h"

namespace Easyjet {

  class JetNeutrinoDecoratorAlg : public AthAlgorithm {
  public:
    using AthAlgorithm::AthAlgorithm;

    virtual StatusCode initialize() override;
    virtual StatusCode execute(const EventContext& ctx) override;

  private:
    // Input Key for Jets
    SG::ReadHandleKey<xAOD::JetContainer> m_jetsInKey {
      this, "jetsIn", "AntiKt4TruthJets", "Input jet container to decorate"
    };

    // Input Key for Truth Particles
    SG::ReadHandleKey<xAOD::TruthParticleContainer> m_truthParticlesInKey {
      this, "truthParticlesIn", "TruthParticles", "Input truth particle container"
    };

    // The Output Decoration Key
    SG::WriteDecorHandleKey<xAOD::JetContainer> m_neutrinoPtDecorKey {
      this, "neutrinoPtDecor", "AntiKt4TruthJets.neutrino_pt", "Decoration for neutrino pT"
    };
    
    // Configurable DeltaR cone
    Gaudi::Property<float> m_dR_cone {this, "DeltaRCone", 0.4, "DeltaR cone size"};
  };
}
#endif
