/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/
/// @author Derrick Allen
/// @author Jason Oliver - systematics, scale factors, handle aliasing.
// Always protect against multiple includes!

#ifndef TTCALIB_FINALVARSXBBCALIBALG
#define TTCALIB_FINALVARSXBBCALIBALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEgamma/ElectronContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>
#include <xAODMissingET/MissingETContainer.h>
#include <xAODMuon/MuonContainer.h>

namespace XBBCALIB
{
  // forward declare, define in the Cxx file
  class FourVectorOutBlock;
  template <typename T> class SystVariableCopier;

  /// \brief An algorithm for counting containers
  class BaselineVarsttCalibAlg final : public AthHistogramAlgorithm
  {
    /// \brief The standard constructor
public:
    using AthHistogramAlgorithm::AthHistogramAlgorithm;
    /// \brief Initialisation method, for setting up tools and other
    /// persistent configs
    StatusCode initialize() override;
    /// \brief Execute method, for actions to be taken in the event loop
    StatusCode execute(const EventContext& ctx) override;
    /// We use default finalize() -- this is for cleanup, and we don't do any

private:
    // ToolHandle<whatever> handle {this, "pythonName", "defaultValue",
    // "someInfo"};

    /// \brief Setup syst-aware input container handles
    CP::SysListHandle m_systematicsList{this};

    // Handles for ttCalib object containers
    CP::SysReadHandle<xAOD::JetContainer> m_ttcalib_jetHandle{
        this, "ttcalib_jets", "ttCalibJets_%SYS%", "Jet container to read"};
    CP::SysReadHandle<xAOD::JetContainer> m_ttcalib_lrjetHandle{
        this, "ttcalib_lrjets", "ttCalibLRJets_%SYS%",
        "Large-R  Jet container to read"};
    // Handles for electron and muon scale factor extraction tools
    CP::SysReadHandle<xAOD::ElectronContainer> m_ttcalib_electronHandle{
        this, "ttcalib_electrons", "ttCalibElectrons_%SYS%",
        "Electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_ttcalib_muonHandle{
        this, "ttcalib_muons", "ttCalibMuons_%SYS%", "Muon container to read"};

    CP::SysReadHandle<xAOD::ElectronContainer> m_electronHandle{
        this, "electrons", "", "Original electron container to read"};

    CP::SysReadHandle<xAOD::MuonContainer> m_muonHandle{
        this, "muons", "", "Original muon container to read"};

    CP::SysReadHandle<xAOD::MissingETContainer> m_metHandle{
        this, "met", "AnalysisMET_%SYS%", "MET container to read"};

    CP::SysReadHandle<xAOD::EventInfo> m_eventHandle{
        this, "event", "EventInfo", "EventInfo container to read"};

    //

    Gaudi::Property<bool> m_isMC{this, "isMC", false, "Is this simulation?"};

    // Electron and muon ID working points
    Gaudi::Property<std::string> m_electronWPName{
        this, "eleWP", "", "Electron ID + Iso working point"};
    Gaudi::Property<std::string> m_muonWPName{this, "muonWP", "",
                                              "Muon ID + Iso working point"};

    CP::SysReadDecorHandle<float> m_ele_SF{"", this};
    CP::SysReadDecorHandle<float> m_mu_SF{"", this};

    CP::SysWriteDecorHandle<float> m_Lepton1_effSF{"Lepton1_effSF_%SYS%",
                                                   this};
    CP::SysWriteDecorHandle<float> m_tagjet_lep_m{"TagJet_Lepton1_m_%SYS%",
                                                  this};
    CP::SysWriteDecorHandle<float> m_dRJetLep{"dR_TagJet_Lepton1_%SYS%", this};
    // ints
    CP::SysWriteDecorHandle<int> m_nEl{"nElectrons_%SYS%", this};
    CP::SysWriteDecorHandle<int> m_nMu{"nMuons_%SYS%", this};

    // Setting up the simplified method
    Gaudi::Property<std::string> m_copied_variable_prefix0{
        this, "copiedVariablePrefix0", "", "prefix for copied variables"};
    // floats
    Gaudi::Property<std::vector<std::string>> m_floats_to_copy{
        this, "floatsToCopy", {}, "floats to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<float>> m_ProbeJet_float_copier;
    // ints
    Gaudi::Property<std::vector<std::string>> m_ints_to_copy{
        this, "intsToCopy", {}, "intss to copy to eventinfo"};
    std::unique_ptr<SystVariableCopier<int>> m_ProbeJet_int_copier;

    /// \brief Setup sys-aware output decorations
    std::unique_ptr<FourVectorOutBlock> m_ProbeJet_candidate_4vec;
    std::unique_ptr<FourVectorOutBlock> m_TagJet_candidate_4vec;
    std::unique_ptr<FourVectorOutBlock> m_Lepton_candidate_4vec;
  };
} // namespace XBBCALIB

#endif
