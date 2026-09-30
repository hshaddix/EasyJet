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
            minMet=flags.Analysis.MET.min_met,
        )
    )

    # Trigger chains per year: the STT group is listed explicitly in
    # trigger_tau.yaml, the large-R jet (LJT) chains are the remaining
    # chains of the (concatenated) selection list
    selection_chains = flags.Analysis.Trigger.selection.chains
    stt_chains = flags.Analysis.Trigger.STT.chains
    ljt_chains = {
        year: [
            c for c in chains
            if year not in stt_chains or c not in stt_chains[year]
        ]
        for year, chains in selection_chains.items()
    }

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.BaselineVarsZtautauCalibAlg(
            "BaselineVarsZtautauCalibAlg",
            isMC=flags.Input.isMC,
            floatVariableList=float_variables,
            intVariableList=int_variables,
            trigMatchingTool=matchingtool,
            triggerlist=selection_chains,
            singleTauTriggerList=stt_chains,
            largeRJetTriggerList=ljt_chains,
            allTaus=taukey,
            minSTTTauPt=flags.Analysis.Tau.min_stt_pT,
            tau0IDSelectionDecor=f"baselineSelection_{flags.Analysis.Tau.ID}_%SYS%",
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
    float_variable_names = [
        "Zcand0_met_mass",
        "Zcand0_truth_m",
        "Zcand0_truth_pt",
        "Zcand0_truth_eta",
        "Zcand0_truth_phi",
        # highest-pT STT-matched tau (tau0)
        "tau0_pt",
        "tau0_eta",
        "tau0_phi",
        "tau0_dR_Zcand",
    ]
    # per-trigger-path copies of the Z-candidate variables (filled only when
    # the LJT / STT analysis selection passes, the STT path requiring tau0 to
    # pass the RNNLoose ID; stt1p / stt3p additionally require a 1-prong /
    # 3-prong tau0), used by the h5 histograms
    for path in ("ljt", "stt", "stt1p", "stt3p"):
        float_variable_names += [
            f"Zcand0_{path}_met_mass",
            f"Zcand0_{path}_pt",
            f"Zcand0_{path}_truth_pt",
        ]
    int_variable_names = [
        "nJets",
        "nTaus",
        "nTaus_STT_matched",
        "pass_LJT",
        "pass_STT",
        "tau0_nProng",
        "tau0_charge",
        "tau0_index",
    ]

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

    # Z-candidate four-momentum (decorated by the FourVectorOutBlock of the alg)
    for var in ("pt", "eta", "phi", "m"):
        branches += [
            f"EventInfo.Zcand0_{var}_%SYS% -> XbbCalib_Zcand0_{var}"
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

    # Per-chain tau trigger match flags, decorated on the full calibrated
    # tau container so they align with the standard tau_* output branches
    tau_chains = sorted(
        c for c in flags.Analysis.TriggerChains if c.startswith("HLT_tau")
    )
    tau_container = flags.Analysis.container_names.output.taus
    for chain in tau_chains:
        # same chain-name sanitisation as the trigPassed_* decorations
        var = "tau_trigMatch_" + chain.replace("-", "_").replace(".", "p")
        branches += [
            f"{tau_container}.{var}_%SYS% -> {var}"
            + flags.Analysis.systematics_suffix_separator
            + "%SYS%"
        ]
        int_variable_names.append(var)

    # Grouped (OR over the STT chain list) decision and match flags
    branches += [
        "EventInfo.trigPassed_STT_%SYS% -> trigPassed_STT"
        + flags.Analysis.systematics_suffix_separator
        + "%SYS%",
        f"{tau_container}.tau_trigMatch_STT_%SYS% -> tau_trigMatch_STT"
        + flags.Analysis.systematics_suffix_separator
        + "%SYS%",
    ]
    int_variable_names.append("trigPassed_STT")
    int_variable_names.append("tau_trigMatch_STT")

    return branches, float_variable_names, int_variable_names
