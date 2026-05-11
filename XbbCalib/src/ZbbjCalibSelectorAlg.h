/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

#ifndef SELECTIONFLAGSZBBJCALIBALG_H
#define SELECTIONFLAGSZBBJCALIBALG_H

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

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

#include <AthContainers/AuxElement.h>
#include <AthContainers/ConstDataVector.h>
#include <optional>

namespace XBBCALIB
{

  /// \brief Dijet event selection for Zbbj calibration, with BJR pT reordering
  class ZbbjCalibSelectorAlg final : public AthHistogramAlgorithm {

    public:
      ZbbjCalibSelectorAlg(const std::string &name, ISvcLocator *pSvcLocator);

      StatusCode initialize() override;
      StatusCode execute() override;
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
        { this, "minJets", 2, "Minimum number of jets" };
      Gaudi::Property<float> m_bjrMassCut
        { this, "bjrMassCut", 40e3, "Minimum BJR mass for both jets [MeV]" };
      Gaudi::Property<float> m_leadJetUfoPtCut
        { this, "leadJetUfoPtCut", 500e3, "Minimum leading jet UFO/calo pT [MeV] (trigger plateau)" };
      Gaudi::Property<float> m_minJetBjrPtCut
        { this, "minJetBjrPtCut", 200e3, "Minimum BJR pT for both jets [MeV]" };
      Gaudi::Property<float> m_leadJetPtCut
        { this, "leadJetPtCut", -1, "Minimum leading jet BJR pT [MeV] (-1 = disabled)" };
      Gaudi::Property<float> m_asymmetryCut
        { this, "asymmetryCut", 0.15, "Maximum pT asymmetry (pt0-pt1)/(pt0+pt1)" };
      Gaudi::Property<float> m_rapidityCut
        { this, "rapidityCut", 1.2, "Maximum rapidity difference |y0-y1|" };

      // Accessors (initialised from property strings)
      std::unique_ptr<SG::AuxElement::ConstAccessor<float>> m_accBjrPt;
      std::unique_ptr<SG::AuxElement::ConstAccessor<float>> m_accBjrMass;

      /// \brief Syst-aware handles
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::JetContainer>
      m_lrjetHandle{ this, "largeRJets", "", "Large-R jet container to read" };

      CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
      m_outHandle{ this, "bjrOrderedJets", "", "Output container reordered by BJR pT" };

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

      CP::SysFilterReporterParams m_filterParams {this, "XbbCalib selection"};

  };

}

#endif // SELECTIONFLAGSZBBJCALIBALG_H
