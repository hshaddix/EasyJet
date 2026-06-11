/*
Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HZYANALYSIS_HZYLEPTONSORALG
#define HZYANALYSIS_HZYLEPTONSORALG

#include <AthContainers/ConstDataVector.h>
#include <AthenaBaseComps/AthHistogramAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>

#include <xAODEgamma/ElectronContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODMuon/MuonContainer.h>

namespace HZY
{
  /// \brief An algorithm for Overlap Removal (OR1)
  class HZyLeptonsORAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    HZyLeptonsORAlg(const std::string &name, ISvcLocator *pSvcLocator);
    /// \brief Initialisation method
    StatusCode initialize() override;
    /// \brief Execute method
    StatusCode execute() override;

private:
    CP::SysListHandle m_systematicsList{this};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_HZyElectronHandle{
        this, "HZyElectrons", "HZyAnalysisElectrons_%SYS%", "HZy Electrons container to read"};
    
    CP::SysReadHandle<xAOD::MuonContainer> m_HZyMuonHandle{
        this, "HZyMuons", "HZyAnalysisMuons_%SYS%", "HZy Muons container to read"};

    CP::SysWriteHandle<ConstDataVector<xAOD::ElectronContainer>> m_HZyElectronORHandle{
        this, "HZyElectronsOR", "HZyElectronsOR_%SYS%", "Electrons container after Overlap Removal"};

    CP::SysWriteHandle<ConstDataVector<xAOD::MuonContainer>> m_HZyMuonORHandle{
        this, "HZyMuonsOR", "HZyMuonsOR_%SYS%", "Muons container after Overlap Removal"};

    CP::SysReadDecorHandle<char> m_electronLIdLIsoDecorHandle{
        this, "ElectronLIdLIsoDecor", "baselineSelection_LooseBLayerLH_Loose_VarRad_%SYS%",
          "Name of input decorator for selected electrons"};

    CP::SysReadDecorHandle<char> m_muonMIdLIsoDecorHandle{
        this, "MuonMIdLIsoDecor", "baselineSelection_Medium_Loose_VarRad_%SYS%",
        "Name of input decorator for selected muons"};

    Gaudi::Property<double> m_dEta_ElEl {this, "dEta_ElEl", 0.075, "dEta cut for Electron/Electron Calo Direction OR (OR1)"};
    Gaudi::Property<double> m_dPhi_ElEl {this, "dPhi_ElEl", 0.125, "dPhi cut for Electron/Electron Calo Direction OR (OR1)"};
    Gaudi::Property<double> m_dR_El_StacoMu {this, "dR_El_StacoMu", 0.02, "dR cut for Electron/Staco Muon OR (OR1)"};
    Gaudi::Property<double> m_dR_StacoMu_StacoMu {this, "dR_StacoMu_StacoMu", 0.0, "dR cut for Staco Muon/Staco Muon OR (OR1)"};
    Gaudi::Property<double> m_dR_CaloMu_StacoMu {this, "dR_CaloMu_StacoMu", 0.1, "dR cut for Calo Muon/Staco Muon OR (OR1)"};
    Gaudi::Property<double> m_dR_CaloMu_Electron {this, "dR_CaloMu_Electron", 0.2, "dR cut for Calo Muon/Electron OR (OR1)"};

    Gaudi::Property<bool> m_doSystematics
    {this, "doSystematics", false, "Run on all systematics"};
    
  };
} // namespace HZY

#endif // HZYANALYSIS_HZYLEPTONSORALG
