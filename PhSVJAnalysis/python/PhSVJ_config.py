from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
import AthenaCommon.SystemOfUnits as Units

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
        MuonSelectorAlgCfg, ElectronSelectorAlgCfg, JetSelectorAlgCfg, PhotonSelectorAlgCfg)
from EasyjetHub.output.ttree.selected_objects import (
        get_selected_objects_branches_variables,
        )


def PhSVJ_cfg(flags, smalljetkey, largejetkey, muonkey, electronkey, photonkey,
        float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    if flags.Analysis.do_large_R_UFO_jets:
        cfg.merge(JetSelectorAlgCfg(flags, name="LargeJetSelectorAlg",
            containerInKey=largejetkey,
            containerOutKey="PhSVJ_LRJets_%SYS%",
            minPt=flags.Analysis.Large_R_jet.min_pT,
            maxEta=flags.Analysis.Large_R_jet.maxEta,
            minimumAmount=flags.Analysis.Large_R_jet.minAmount,
            ))

    if flags.Analysis.do_small_R_jets:
        cfg.merge(JetSelectorAlgCfg(flags, name="SmallRJet_SelectorAlg",
            containerInKey=smalljetkey,
            containerOutKey="PhSVJ_Jets_%SYS%",
            minPt=flags.Analysis.Small_R_jet.min_pT,
            maxEta=flags.Analysis.Small_R_jet.maxEta,
            minimumAmount=flags.Analysis.Small_R_jet.minAmount,
            bTagWPDecorName="ftag_select_"
            + flags.Analysis.Small_R_jet.btag_wp,
            selectBjet=flags.Analysis.Small_R_jet.selectBjet,
            ))
    
    if flags.Analysis.do_electrons:
        cfg.merge(ElectronSelectorAlgCfg(flags,
            containerInKey=electronkey,
            containerOutKey="PhSVJ_Electrons_%SYS%",
            minPt=flags.Analysis.Electron.min_pT,
            maxEta = flags.Analysis.Electron.maxEta,
            minimumAmount=flags.Analysis.Electron.minAmount
            ))

    if flags.Analysis.do_muons:
        cfg.merge(MuonSelectorAlgCfg(flags,
            containerInKey=muonkey,
            containerOutKey="PhSVJ_Muons_%SYS%",
            minPt=flags.Analysis.Muon.min_pT,
            maxEta=flags.Analysis.Muon.maxEta,
            minimumAmount=flags.Analysis.Muon.minAmount
            ))

    if flags.Analysis.do_photons:
        cfg.merge(PhotonSelectorAlgCfg(flags,
            containerInKey=photonkey,
            containerOutKey="PhSVJ_Photons_%SYS%",
            minPt=flags.Analysis.Photon.min_pT,
            maxEta=flags.Analysis.Photon.maxEta,
            minimumAmount=-1
            ))

    cfg.addEventAlgo(
            CompFactory.PhSVJ.PhSVJSelectorAlg(
                "PhSVJSelectorAlg",   
                eventDecisionOutputDecoration="PhSVJ_pass_sr_%SYS%",
                bypass=flags.Analysis.bypass,
                doLargeRJets=flags.Analysis.do_large_R_UFO_jets,
                doSmallRJets=flags.Analysis.do_small_R_jets,
                doMuons=flags.Analysis.do_muons,
                doElectrons=flags.Analysis.do_electrons,
                doPhotons=flags.Analysis.do_photons,
                minLargeRJets=flags.Analysis.Large_R_jet.minAmount if flags.Analysis.do_large_R_UFO_jets else -1,
                minPhotons=-1,
                triggerChains=list(flags.Analysis.TriggerChains),
                do_trigger_filtering=flags.Analysis.do_trigger_filtering
                )
            )

    cfg.addEventAlgo(
            CompFactory.PhSVJ.BaselineVarsPhSVJAlg( 
                "BaselineVarsPhSVJAlg",    
                floatVariableList=float_variables,
                intVariableList=int_variables,
                doLargeRJets=flags.Analysis.do_large_R_UFO_jets,
                doElectrons=flags.Analysis.do_electrons,
                doMuons=flags.Analysis.do_muons,
                doPhotons=flags.Analysis.do_photons,
                )
            )

    return cfg


def get_BaselineVarsPhSVJAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    for object in ["tag_jet", "probe_jet"]:
        for var in ["pt", "eta", "phi"]:
            float_variable_names.append(f"{object}_{var}")

    return float_variable_names, int_variable_names

def PhSVJ_branches(flags):
    branches = []

    # this will be all the variables that are calculated by the
    # BaselineVarsPhSVJAlg algorithm
    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    # these are the variables that will always be stored by easyjet specific to PhSVJ
    baseline_float_variables, baseline_int_variables \
            = get_BaselineVarsPhSVJAlg_variables(flags)
    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables


    if flags.Analysis.do_large_R_UFO_jets:
        prefix = flags.Analysis.Large_R_jet.output_prefix
        for var in flags.Analysis.Large_R_jet.additional_variables:
            continue #fixme: decoration is not ready.
            all_baseline_variable_names.append(f"{prefix}_{var}")

    if flags.Analysis.do_small_R_jets:
        prefix = flags.Analysis.Small_R_jet.output_prefix
        for var in flags.Analysis.Small_R_jet.additional_variables:
            continue #fixme: decoration is not ready.
            all_baseline_variable_names.append(f"{prefix}_{var}")
    #End of custom add
    all_baseline_variable_names += [*float_variable_names, *int_variable_names]

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> {var}"
                + flags.Analysis.systematics_suffix_separator + "%SYS%"]
    
    branches_to_remove = [] #fixme: decoration is not ready.
    for item in ['recojet_antikt10UFO_em_frac', 'recojet_antikt4PFlow_em_frac']:
        for br in branches:
            if item in br:
                branches_to_remove.append(br)
                break
    for br in branches_to_remove:
        branches.remove(br)
    
    # These are the variables always saved with the objects selected by the analysis
    object_level_branches, object_level_float_variables, object_level_int_variables \
            = get_selected_objects_branches_variables(flags, "PhSVJ")

    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables


    branches += object_level_branches

    branches += ["EventInfo.PhSVJ_pass_sr_%SYS% -> pass_SR"
            + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    return branches, float_variable_names, int_variable_names
