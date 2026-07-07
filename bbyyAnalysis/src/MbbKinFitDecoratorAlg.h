/*
  Copyright (C) 2002-2024 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef MBBKINFIT_DECORATORALG
#define MBBKINFIT_DECORATORALG

#include <memory>

#include "AnaAlgorithm/AnaAlgorithm.h"

#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysWriteHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODEgamma/PhotonContainer.h>
#include <AthContainers/ConstDataVector.h>

#include "KinematicFitTool/KinematicFitTool.h"

namespace HHBBYY {
  /// \brief An algorithm for counting containers
  class MbbKinFitDecoratorAlg final : public EL::AnaAlgorithm
  {
    /// \brief The standard constructor
  public:
    using EL::AnaAlgorithm::AnaAlgorithm;

    /// \brief Initialisation method, for setting up tools and other persistent
    /// configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

  private:

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::PhotonContainer>
    m_photonHandle{ this, "photons", "bbyyAnalysisPhotons_%SYS%", "Photon container to read"};

    CP::SysReadHandle<xAOD::JetContainer>
    m_jetHandle{ this, "jets", "bbyyAnalysisJets_%SYS%", "Jet container to read" };
      
    CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{ this, "event", "EventInfo", "EventInfo container to read" };

    CP::SysReadHandle<xAOD::MissingETContainer>
    m_metHandle{ this, "met", "AnalysisMET_%SYS%", "MET container to read" };

    /// \brief Setup sys-aware output decorations

    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
    m_jetOutHandle_KF1{ this, "jetContainerOutKey_KF1", "bbyyAnalysisKF1Jets_%SYS%", "Jet container to write" };

    CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
    m_jetOutHandle_KF2{ this, "jetContainerOutKey_KF2", "bbyyAnalysisKF2Jets_%SYS%", "Jet container to write" };

    CP::SysWriteDecorHandle<float> m_KF_MBB{"KF_mbb_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_LH_KF1{"LH_KF1_%SYS%", this};
    CP::SysWriteDecorHandle<float> m_LH_KF2{"LH_KF2_%SYS%", this};
    CP::SysWriteDecorHandle<int> m_Conv_KF1{"Conv_KF1_%SYS%", this};
    CP::SysWriteDecorHandle<int> m_Conv_KF2{"Conv_KF2_%SYS%", this};

    /// \brief Steerable properties
    Gaudi::Property<bool> m_doSystematics
      { this, "doSystematics", false, "Run on all systematics" };

    ToolHandle<KinematicFitTool> m_KFTool{this, "KinFitTool", ""};

    //Needed to calculate mbb
    Gaudi::Property<float> m_Jet_Min_Pt{this, "JetMinPt", 20.*Athena::Units::GeV, "Minimum pT of jets"};

    CP::SysReadDecorHandle<char>  m_isBtag {this, "bTagWPDecorName", "ftag_select_GN2v00LegacyWP_FixedCutBEff_77", "Name of input decorator for b-tagging"};

  };
}
  #endif
