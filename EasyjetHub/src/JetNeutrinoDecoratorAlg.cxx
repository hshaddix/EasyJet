#include "JetNeutrinoDecoratorAlg.h"

#include "StoreGate/WriteDecorHandle.h" 

#include <cmath> 

namespace Easyjet {

  JetNeutrinoDecoratorAlg::JetNeutrinoDecoratorAlg(const std::string &name, ISvcLocator *pSvcLocator)
      : AthAlgorithm(name, pSvcLocator) {}

  StatusCode JetNeutrinoDecoratorAlg::initialize() {
    ATH_CHECK(m_jetsInKey.initialize());
    ATH_CHECK(m_truthParticlesInKey.initialize());
    
    // Initialize the decoration key
    m_neutrinoPtDecorKey = m_jetsInKey.key() + ".neutrino_pt";
    ATH_CHECK(m_neutrinoPtDecorKey.initialize());
    
    return StatusCode::SUCCESS;
  }

  StatusCode JetNeutrinoDecoratorAlg::execute(const EventContext& ctx) {
    SG::ReadHandle<xAOD::JetContainer> jets(m_jetsInKey, ctx);    SG::ReadHandle<xAOD::TruthParticleContainer> truthParticles(m_truthParticlesInKey, ctx);
    // Check validity
    if (!jets.isValid() || !truthParticles.isValid()) {
      ATH_MSG_ERROR("Could not retrieve Jets or TruthParticles");
      return StatusCode::FAILURE;
    }

    // Initialization
    SG::WriteDecorHandle<xAOD::JetContainer, float> neutrinoPtDecor(m_neutrinoPtDecorKey, ctx);
    for (const xAOD::Jet* jet : *jets) { // Loop over jets
      
      float sum_nu_pt = 0.0;

      // Loop over Truth Particles to find neutrinos
      for (const xAOD::TruthParticle* tp : *truthParticles) {
        
        // Filtering neutrinos (PDG IDs: 12, 14, 16)
        int absId = std::abs(tp->pdgId());
        if (absId != 12 && absId != 14 && absId != 16) continue;

        // Check if particle is stable (status 1)
        if (tp->status() != 1) continue;

        // Check DeltaR
        float dR = jet->p4().DeltaR(tp->p4());
        
        if (dR < m_dR_cone) {
           sum_nu_pt += tp->pt();
        }
      }

      // Save the value to the jet
      neutrinoPtDecor(*jet) = sum_nu_pt;
    }

    return StatusCode::SUCCESS;
  }
}


