/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

// Always protect against multiple includes!
#ifndef SELECTIONFLAGSZTAUTAUCALIBALG_H
#define SELECTIONFLAGSZTAUTAUCALIBALG_H

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
#include <xAODMissingET/MissingETContainer.h>

#include <AthContainers/AuxElement.h>
#include <AthContainers/ConstDataVector.h>
#include <optional>

namespace XBBCALIB
{
  /// \brief An algorithm for counting containers, applying the Ztautau-calib
  /// baseline selection, and selecting the Z-candidate large-R jet.
  ///
  /// The Z candidate is defined (as in the original python getZcandLR()) as the
  /// large-R jet with the smallest |Delta Phi| with respect to the missing
  /// transverse momentum; the other jet is taken as the recoiling jet. The two
  /// jets are written out, in that order, into a dedicated "ordered" jet
  /// container so that downstream algorithms (e.g. BaselineVarsZtautauCalibAlg)
  /// can retrieve index 0 as the Z candidate and index 1 as the recoiling jet
  /// without having to repeat the MET-matching logic.
  class ZtautauCalibSelectorAlg final : public AthHistogramAlgorithm {

    public:
      using AthHistogramAlgorithm::AthHistogramAlgorithm;

      /// \brief Initialisation method, for setting up tools and other persistent
      /// configs
      StatusCode initialize() override;
      /// \brief Execute method, for actions to be taken in the event loop
      StatusCode execute(const EventContext& ctx) override;
      /// \brief This is the mirror of initialize() and is called after all events are processed.
      StatusCode finalize() override;

    private :
      // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
      // "someInfo"};
      Gaudi::Property<bool> m_bypass
        { this, "bypass", false, "Run selector algorithm in pass-through mode" };

      Gaudi::Property<bool> m_isMC
        { this, "isMC", false, "Is this simulation?" };

      /// \brief Baseline missing transverse momentum requirement [MeV]
      Gaudi::Property<float> m_minMet
        { this, "minMet", 30.e3, "Minimum MET of the baseline selection [MeV]" };

      /// \brief Upper cut on tau21 = Tau2_wta/Tau1_wta of the Z-candidate jet
      Gaudi::Property<float> m_maxZcandT21
        { this, "maxZcandT21", 0.2, "Maximum tau21 of the Z-candidate jet" };

      /// \brief Setup syst-aware input container handles
      CP::SysListHandle m_systematicsList {this};

      CP::SysReadHandle<xAOD::JetContainer>
      m_lRjetHandle{ this, "lrjets", "XbbCalibLRJets_%SYS%",   "Large-R jet container to read" };

      CP::SysReadHandle<xAOD::ElectronContainer>
      m_electronHandle{ this, "electrons", "XbbCalibElectrons_%SYS%", "Electron container to read" };

      CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{
        this, "met", "AnalysisMET_%SYS%", "MET container to read"};

      CP::SysReadHandle<xAOD::JetContainer>
      m_truthJetHandle{ this, "truthjets", "AntiKt10TruthSoftDropBeta100Zcut10Jets", "Truth large-R jet container to read (MC only)" };

      CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle{ this, "event", "EventInfo",   "EventInfo container to read" };

      /// \brief Output container holding just the two selected large-R jets,
      /// re-ordered so that index 0 is the Z candidate (smallest |Delta Phi|
      /// w.r.t. MET) and index 1 is the recoiling jet.
      CP::SysWriteHandle<ConstDataVector<xAOD::JetContainer>>
      m_outHandle{ this, "OrderedJets", "XbbCalibLRJetsOrdered_%SYS%", "Output container: [0]=Z candidate, [1]=recoiling jet" };

      /// \brief Zcand mass corrected with MET in the collinear approximation 
      /// decorated directly onto the Z-candidate jet so downstream algorithms/
      /// fhistogram illing can read it back (as met_mass) without needing to 
      /// retrieve MET themselves.
      CP::SysWriteDecorHandle<float> m_zcandMetMassHandle{this,"metMass","met_mass_%SYS%","MET corrected Z candidate mass"};

      /// \brief Full 4-momentum of the truth large-R jet closest in DeltaR to
      /// the Z-candidate jet (MC only), decorated as a single JetFourMom_t
      /// block directly onto the Zcand jet -- the same type xAOD::Jet itself
      /// uses internally for its various momentum states/scales -- so
      /// downstream algorithms can read it back without re-doing the match.
      /// Set to (-99,-99,-99,-99) for data, or for MC events with no truth
      /// jet within truthMatchMaxDeltaR.
      CP::SysWriteDecorHandle<xAOD::JetFourMom_t>
      m_zcandTruthJetHandle{this,"TruthJet","TruthLRJet_%SYS%","Truth matched Z candidate jet"};
    
      CP::SysFilterReporterParams m_filterParams {this, "XbbCalib selection"};

      /// \brief Select the Z candidate large-R jet as the one closest in |Delta Phi|
      /// to the missing transverse momentum (mirrors python getZcandLR()).
      /// \param jets     input large-R jet container (assumed size == 2)
      /// \param met_phi  phi of the missing transverse momentum
      /// \param zcandIdx [out] index of the selected Z-candidate jet
      /// \param recoilIdx [out] index of the recoiling jet
      static void findZcandIndex(
        const xAOD::JetContainer& jets,
        float met_phi,
        size_t& zcandIdx,
        size_t& recoilIdx
      );

      /// \brief Correct the Z-candidate mass with MET in the collinear approximation:
      /// alpha = (met.pt/jet.pt) * cos(DeltaPhi(jet,met)); returns jet.mass * (1 + alpha)
      /// if alpha >= 0, otherwise defaultValue (collinear approx only holds for
      /// |DeltaPhi(jet,met)| < pi/2).
      static float computeZcandMetMass(
        const xAOD::Jet& zcandJet,
        const xAOD::MissingET& met,
        float defaultValue
      );

      /// \brief Compute t21 = Tau2_wta/Tau1_wta substructure ratio for a jet.
      /// \param jet     input large-R jet container
      static float computeT21(const xAOD::Jet& jet);

      /// \brief Find the truth jet closest in DeltaR to recoJet.
      /// \return index into truthJets, or -1 if truthJets is empty or the
      /// closest truth jet is farther than maxDeltaR.
      static int findTruthMatch(
        const xAOD::Jet& recoJet,
        const xAOD::JetContainer& truthJets,
        float maxDeltaR
      );

  };

}

#endif //SELECTIONFLAGSZTAUTAUCALIBALG_H
