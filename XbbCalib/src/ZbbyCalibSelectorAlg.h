/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!

#ifndef SELECTIONFLAGSZBBYCALIBALG_H
#define SELECTIONFLAGSZBBYCALIBALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <AthenaBaseComps/AthAlgorithm.h>

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysFilterReporterParams.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODEgamma/PhotonContainer.h>

#include <AthContainers/AuxElement.h>
#include <AthContainers/ConstDataVector.h>
#include <optional>

namespace XBBCALIB
{

  /// \brief An algorithm for counting containers
  class ZbbyCalibSelectorAlg final : public AthHistogramAlgorithm {

    public:
      ZbbyCalibSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override;

    private :
      Gaudi::Property<bool> m_bypass
        { this, "bypass", false, "Run selector algorithm in pass-through mode" };

      Gaudi::Property<bool> m_remove_zcc_overlap
        { this, "remove_zcc_overlap", false,
          "Veto Z->cc events when running on Zqq samples (OR with Zcc-dedicated sample)" };
      std::optional<SG::AuxElement::ConstAccessor<int>> m_isCC_acc;
      
      // BJR decoration names
      Gaudi::Property<std::string> m_bjrPtName
        { this, "bjrPtDecoration", "bJR10v01_pt", "BJR pT decoration name" };
      Gaudi::Property<std::string> m_bjrMassName
        { this, "bjrMassDecoration", "bJR10v01_mass", "BJR mass decoration name" };
        
    
      // Dijet selection cuts
      Gaudi::Property<unsigned int> m_minJets
        { this, "minJets", 1, "Minimum number of jets" };
      Gaudi::Property<float> m_bjrMassCut
        { this, "bjrMassCut", 40e3, "Minimum BJR mass [MeV]" };
      Gaudi::Property<float> m_bjrMassUpperCut
        { this, "bjrMassUpperCut", 160e3, "Maximum BJR mass [MeV]" };
      Gaudi::Property<float> m_minJetBjrPtCut
        { this, "minJetBjrPtCut", 200e3, "Minimum BJR pT [MeV]" };
      Gaudi::Property<float> m_minPhotonPtCut
        { this, "minPhotonPtCut", 175e3, "Minimum photon pT [MeV]" };
      


      // Accessors (initialised from property strings)
      std::unique_ptr<SG::AuxElement::ConstAccessor<float>> m_accBjrPt;
      std::unique_ptr<SG::AuxElement::ConstAccessor<float>> m_accBjrMass;

      /// \brief Setup syst-aware input container handles
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::JetContainer>
      m_lrjetHandle{ this, "lrjets", "",   "Large-R jet container to read" };

      CP::SysReadHandle<xAOD::PhotonContainer>
      m_photonHandle{ this, "photons", "", "Photon container to read" };

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

      CP::SysFilterReporterParams m_filterParams {this, "Zbb+y Calib selection"};
      
      CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
      m_outHandle{ this, "bjrOrderedJets", "", "Output container reordered by BJR pT" };


  };

}

#endif // SELECTIONFLAGSZBBYCALIBALG_H
