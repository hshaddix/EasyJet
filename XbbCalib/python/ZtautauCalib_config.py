from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    ElectronSelectorAlgCfg,
    JetSelectorAlgCfg,
    MuonSelectorAlgCfg,
    PhotonSelectorAlgCfg,
    TauSelectorAlgCfg,
)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)

from EasyjetHub.algs.postprocessing.trigger_matching import TriggerMatchingToolCfg
from XbbCalib.hists_output_config import hists_output_cfg
from .vars_to_copy import lrj_floats_to_copy, lrj_ints_to_copy, lrj_gn3x_working_points

def ZtautauCalib_cfg(
    flags,
    largejetkey,
    taukey,
    photonkey,
    electronkey,
    muonkey,
    float_variables=None,
    int_variables=None,
):
    """Callects the objects to reconstruct."""

    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    # Trigger matching tools
    matchingtool = cfg.popToolsAndMerge(TriggerMatchingToolCfg(flags))

    # resolved Taus
    cfg.merge(
        TauSelectorAlgCfg(
            flags,
            containerInKey=taukey,
            containerOutKey="XbbCalibTaus_%SYS%",
            minPt=flags.Analysis.Tau.min_pT,
        )
    )

    # Large R jets
    cfg.merge(
        JetSelectorAlgCfg(
            flags,
            name="LargeJetSelectorAlg",
            containerInKey=largejetkey,
            containerOutKey="XbbCalibLRJets_%SYS%",
            minPt=flags.Analysis.Large_R_jet.min_pT,
            maxEta=flags.Analysis.Large_R_jet.max_eta,
            minimumAmount=1,
        )
    )

    # photons
    cfg.merge(
        PhotonSelectorAlgCfg(
            flags,
            containerInKey=photonkey,
            containerOutKey="XbbCalibPhotons_%SYS%",
        )
    )

    # light leptons
    cfg.merge(
        ElectronSelectorAlgCfg(
            flags,
            containerInKey=electronkey,
            containerOutKey="XbbCalibElectrons_%SYS%",
            minPt=flags.Analysis.Electron.min_pT,
            maxEta=flags.Analysis.Electron.max_eta,
        )
    )
    cfg.merge(
        MuonSelectorAlgCfg(
            flags,
            containerInKey=muonkey,
            containerOutKey="XbbCalibMuons_%SYS%",
            minPt=flags.Analysis.Muon.min_pT,
            maxEta=flags.Analysis.Muon.max_eta,
        )
    )

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.ZtautauCalibSelectorAlg(
            "ZtautauCalibSelectorAlg",
            eventDecisionOutputDecoration="XbbCalib_pass_sr_%SYS%",
            bypass=flags.Analysis.bypass,
            isMC=flags.Input.isMC,
        )
    )

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.BaselineVarsZtautauCalibAlg(
            "BaselineVarsZtautauCalibAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables,
            intVariableList=int_variables,
            trigMatchingTool=matchingtool,
            triggerlist=flags.Analysis.Trigger["selection"]["chains"].as_mutable(),
            copiedVariablePrefix0="Zcand0_",
            intsToCopy=lrj_ints_to_copy(flags),
            floatsToCopy=lrj_floats_to_copy(flags)
        )
    )
    
    out_path = (
        flags.Analysis.h5_output
        if flags.Analysis.output.h5.hists.enable else None
    )
    cfg.merge(
        hists_output_cfg(
            flags, flags.Analysis.output.h5.hists, out_path,
            tag_wps=lrj_gn3x_working_points(flags)
        )
    )

    return cfg


def ZtautauCalib_branches(flags):
    """Variables to compute."""
    branches = []
    float_variable_names = ["Zcand0_met_mass", "Zcand0_truth_m", "Zcand0_truth_pt", "Zcand0_truth_eta", "Zcand0_truth_phi"]
    int_variable_names = ["nJets", "nTaus"]

    object_level_branches, object_level_float_variables, object_level_int_variables = (
        get_selected_objects_branches_variables(flags, "XbbCalib")
    )
    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables

    for var in object_level_branches:
        branches += [
            f"EventInfo.{var}_%SYS% -> XbbCalib_{var}"
            + flags.Analysis.systematics_suffix_separator
            + "%SYS%"
        ]

    for var in int_variable_names:
        branches += [
            f"EventInfo.{var}_%SYS% -> XbbCalib_{var}"
            + flags.Analysis.systematics_suffix_separator
            + "%SYS%"
        ]
    for var in float_variable_names:
        branches += [
            f"EventInfo.{var}_%SYS% -> XbbCalib_{var}"
            + flags.Analysis.systematics_suffix_separator
            + "%SYS%"
        ]

    # Trigger match
    branches += [
        "XbbCalibLRJets_%SYS%.largeR_trigMatch_%SYS% -> recojet_antikt10UFO_trigMatch"
        + flags.Analysis.systematics_suffix_separator
        + "%SYS%",
        "XbbCalibPhotons_%SYS%.photon_trigMatch_%SYS% -> ph_trigMatch"
        + flags.Analysis.systematics_suffix_separator
        + "%SYS%",
    ]
    int_variable_names.extend(
        [
            "largeR_trigMatch",
            "photon_trigMatch",
        ]
    )

    return branches, float_variable_names, int_variable_names
