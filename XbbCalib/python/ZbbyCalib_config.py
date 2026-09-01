from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
# from AthenaCommon.Constants import VERBOSE
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    MuonSelectorAlgCfg, ElectronSelectorAlgCfg, JetSelectorAlgCfg, PhotonSelectorAlgCfg)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)

from .vars_to_copy import lrj_floats_to_copy, lrj_gn3x_working_points
from .zdecay_label_config import ZDecayTruthLabelAlgCfg, truth_z_decay_int_vars, is_zqq_sample
from XbbCalib.hists_output_config import hists_output_cfg

copiedVariablePrefix0 = "Zcand_0_"
copiedVariablePrefix1 = "Zcand_1_"


def ZbbyCalib_cfg(flags, smalljetkey, largejetkey, muonkey, electronkey, photonkey,
                  float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    selected_large_r_jets = "XbbCalibLRJets_%SYS%"
    bjr_ordered_large_r = "XbbCalibBJROrderedLRJets_%SYS%"
    
    cfg.merge(
        JetSelectorAlgCfg(
            flags,
            name="LargeJetSelectorAlg",
            containerInKey=largejetkey,
            containerOutKey=selected_large_r_jets,
            minPt=flags.Analysis.Large_R_jet.min_pT,
            maxEta=flags.Analysis.Large_R_jet.max_eta,
            minimumAmount=1,
        )
    )

    selected_photons = "XbbCalibPhotons_%SYS%"
    cfg.merge(PhotonSelectorAlgCfg(flags,
                                   containerInKey=photonkey,
                                   containerOutKey=selected_photons,
                                   minPt=flags.Analysis.Photon.min_pT,
                                   ))

    cfg.merge(ZDecayTruthLabelAlgCfg(flags))

    # signal jets selection
    # ZcandSelectorAlg: photon pT enforced upstream; only dR cut remains
    z_candidate_jets = "ZcandLRJets_%SYS%"
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.ZcandSelectorAlg(
            "ZcandSelectorAlg",
            lrjets=selected_large_r_jets,
            photons=selected_photons,
            zCandidateJets=z_candidate_jets,
        )
    )
    
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.ZbbyCalibSelectorAlg(
            "ZbbyCalibSelectorAlg",
            lrjets=z_candidate_jets,
            bjrOrderedJets=bjr_ordered_large_r,
            photons=selected_photons,
            eventDecisionOutputDecoration="XbbCalib_pass_sr_%SYS%",
            bypass=flags.Analysis.bypass,
            remove_zcc_overlap=flags.Analysis.remove_zcc_overlap and is_zqq_sample(flags),
        )
    )
    
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.BaselineVarsZbbyCalibAlg(
            "BaselineVarsZbbyCalibAlg",
            lrjets=bjr_ordered_large_r,
            photons=selected_photons,
            floatsToCopy=lrj_floats_to_copy(flags),
            intsToCopy=lrj_gn3x_working_points(flags),
            copiedVariablePrefix0="Zcand_0_",
            copiedVariablePrefix1="Zcand_1_",
        )
    )

    out_path = (
        flags.Analysis.h5_output
        if flags.Analysis.output.h5.hists.enable else None
    )
    cfg.merge(hists_output_cfg(
        flags, flags.Analysis.output.h5.hists, out_path,
        tag_wps=lrj_gn3x_working_points(flags)))

    return cfg


def get_BaselineVarsZbbyCalibAlg_variables(flags):
    float_variable_names = []
    int_variable_names = ["photons_n", "lrjets_n"]

    for object in ["Zcand_0", "Zcand_1", "photon"]:
        for var in ["pt", "eta", "phi", "m"]:
            float_variable_names.append(f"{object}_{var}")

    float_variable_names += [
        f'{p}{x}'
        for x in lrj_floats_to_copy(flags)
        for p in (copiedVariablePrefix0, copiedVariablePrefix1)
    ]

    int_variable_names += [
        f'{p}{x}'
        for x in lrj_gn3x_working_points(flags)
        for p in (copiedVariablePrefix0, copiedVariablePrefix1)
    ]
    int_variable_names += truth_z_decay_int_vars(flags)

    return float_variable_names, int_variable_names


def ZbbyCalib_branches(flags):
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsZbbyCalibAlg algorithm
    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    # these are the variables that will always be stored by easyjet specific
    # to XbbCalib. However, at this stage we have no varibles that are
    # calculated by this algorithm
    # so we will just make a placeholder list

    # These are the variables always saved with the objects selected
    # by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarsZbbyCalibAlg_variables(flags)
    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables

    nosys_variable_names = set(truth_z_decay_int_vars(flags))
    all_baseline_variable_names += [
        v for v in [*float_variable_names, *int_variable_names]
        if v not in nosys_variable_names
    ]
    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> ZbbyCalib_{var}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"]
    for var in [v for v in int_variable_names if v in nosys_variable_names]:
        branches += [f"EventInfo.{var} -> ZbbyCalib_{var} type=int"]

    # These are the variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    object_level_branches, object_level_float_variables, object_level_int_variables = \
        get_selected_objects_branches_variables(flags, "ZbbCalib")

    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables

    branches += object_level_branches

    # branches += ["EventInfo.ZbbCalib_pass_sr_%SYS% -> ZbbCalib_pass_SR"
    #              + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    return branches, float_variable_names, int_variable_names
