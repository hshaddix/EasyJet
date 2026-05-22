/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef SSWWANALYSIS_SSWWTRUTHVARSALG
#define SSWWANALYSIS_SSWWTRUTHVARSALG

#include <AnaAlgorithm/AnaAlgorithm.h>
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/ReadHandleKey.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <StoreGate/WriteDecorHandleKeyArray.h>

#include <xAODEgamma/ElectronContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODTruth/TruthParticleContainer.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace ssWWVBS
{

  class ssWWTruthVarsAlg final : public EL::AnaAlgorithm
  {
  public:
    ssWWTruthVarsAlg(const std::string& name, ISvcLocator* pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute() override;

  private:
    SG::ReadHandleKey<xAOD::EventInfo>
    m_eventInKey{this, "eventInfo", "EventInfo", "EventInfo container to decorate"};

    SG::ReadHandleKey<xAOD::TruthParticleContainer>
    m_truthBosonInKey{this, "truthBosonInContainer", "TruthBosonsWithDecayParticles",
                      "Truth bosons with decay particles"};

    SG::ReadHandleKey<xAOD::TruthParticleContainer>
    m_truthNeutrinoInKey{this, "truthNeutrinoInContainer", "TruthNeutrinos",
                         "Truth neutrino container"};

    SG::ReadHandleKey<xAOD::TruthParticleContainer>
    m_truthElectronInKey{this, "truthElectronInContainer", "ssWWTruthElectrons",
                         "Selected truth electron container"};

    SG::ReadHandleKey<xAOD::TruthParticleContainer>
    m_truthMuonInKey{this, "truthMuonInContainer", "ssWWTruthMuons",
                     "Selected truth muon container"};

    SG::ReadHandleKey<xAOD::ElectronContainer>
    m_recoElectronInKey{this, "recoElectronInContainer", "ssWWAnalysisElectrons_NOSYS",
                        "Nominal selected electron container"};

    SG::ReadHandleKey<xAOD::MuonContainer>
    m_recoMuonInKey{this, "recoMuonInContainer", "ssWWAnalysisMuons_NOSYS",
                    "Nominal selected muon container"};

    SG::ReadDecorHandleKey<xAOD::ElectronContainer>
    m_recoElectronSelectedKey{this, "recoElectronSelectedDecoration",
                              "ssWWAnalysisElectrons_NOSYS.ele_is_selected_NOSYS",
                              "Nominal ssWW electron selection decoration"};

    SG::ReadDecorHandleKey<xAOD::MuonContainer>
    m_recoMuonSelectedKey{this, "recoMuonSelectedDecoration",
                          "ssWWAnalysisMuons_NOSYS.mu_is_selected_NOSYS",
                          "Nominal ssWW muon selection decoration"};

    Gaudi::Property<std::vector<std::string>> m_floatVariables{
      this, "floatVariableList", {}, "Name list of floating variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables{
      this, "intVariableList", {}, "Name list of integer variables"};

    SG::WriteDecorHandleKeyArray<xAOD::EventInfo, float> m_floatDecorKeys;
    SG::WriteDecorHandleKeyArray<xAOD::EventInfo, int> m_intDecorKeys;
    std::unordered_map<std::string, std::size_t> m_floatDecorIndex;
    std::unordered_map<std::string, std::size_t> m_intDecorIndex;
  };

}

#endif
