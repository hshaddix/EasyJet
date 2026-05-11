from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from EasyjetHub.algs.postprocessing.SelectorAlgConfig import (
    MuonSelectorAlgCfg, ElectronSelectorAlgCfg, JetSelectorAlgCfg)
from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)
from .vars_to_copy import lrj_floats_to_copy, lrj_ints_to_copy

copiedVariablePrefix0 = "ProbeJet_"
copiedVariablePrefix1 = "TagJet_"


def ttCalib_cfg(flags, smalljetkey, largejetkey, muonkey, electronkey,
                float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    cfg.merge(JetSelectorAlgCfg(flags,
                                name="LargeJetSelectorAlg",
                                containerInKey=largejetkey,
                                containerOutKey="ttCalibLRJets_%SYS%",
                                minPt=flags.Analysis.Large_R_jet.min_pT,
                                maxEta=flags.Analysis.Large_R_jet.max_eta,
                                minimumAmount=1,
                                ))

    cfg.merge(JetSelectorAlgCfg(flags,
                                name="SmallRJet_SelectorAlg",
                                containerInKey=smalljetkey,
                                containerOutKey="ttCalibJets_%SYS%",
                                minPt=flags.Analysis.Small_R_jet.min_pT,
                                maxEta=2.5,
                                minimumAmount=1,
                                bTagWPDecorName="ftag_select_"
                                + flags.Analysis.Small_R_jet.btag_wp,
                                selectBjet=True,
                                ))

    cfg.merge(ElectronSelectorAlgCfg(flags,
                                     containerInKey=electronkey,
                                     containerOutKey="ttCalibElectrons_%SYS%",
                                     minPt=flags.Analysis.Electron.min_pT,
                                     maxEta=2.47
                                     ))

    cfg.merge(MuonSelectorAlgCfg(flags,
                                 containerInKey=muonkey,
                                 containerOutKey="ttCalibMuons_%SYS%",
                                 minPt=flags.Analysis.Muon.min_pT,
                                 maxEta=2.5,
                                 ))

    MuonWPLabel = f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}'
    ElectronWPLabel = f'{flags.Analysis.Electron.ID}_{flags.Analysis.Electron.Iso}'
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.ttCalibSelectorAlg(
            "ttCalibSelectorAlg",
            eventDecisionOutputDecoration="ttCalib_pass_sr_%SYS%",
            bypass=flags.Analysis.bypass,
            minMet=flags.Analysis.MET.min_met,
            muonWP=MuonWPLabel,
            eleWP=ElectronWPLabel
        )
    )

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.BaselineVarsttCalibAlg(
            "BaselineVarsttCalibAlg",
            isMC=flags.Input.isMC,
            ttcalib_jets="ttCalibJets_%SYS%",
            ttcalib_lrjets="ttCalibLRJets_%SYS%",
            ttcalib_electrons="ttCalibElectrons_%SYS%",
            ttcalib_muons="ttCalibMuons_%SYS%",
            electrons="AnalysisElectrons_%SYS%",
            muons="AnalysisMuons_%SYS%",
            muonWP=MuonWPLabel,
            eleWP=ElectronWPLabel,
            floatsToCopy=lrj_floats_to_copy(flags),
            intsToCopy=lrj_ints_to_copy(flags),
            copiedVariablePrefix0=copiedVariablePrefix0,
        )
    )

    return cfg


def get_BaselineVarsttCalibAlg_variables(flags):

    float_variable_names = ["TagJet_Lepton1_m", "dR_TagJet_Lepton1"]
    int_variable_names = ["nElectrons", "nMuons"]

    for object in ["ProbeJet", "TagJet", "Lepton"]:
        for var in ["pt", "eta", "phi", "m"]:
            float_variable_names.append(f"{object}_{var}")

    float_variable_names += [
        f'{copiedVariablePrefix0}{x}'
        for x in lrj_floats_to_copy(flags)
    ]

    int_variable_names += [
        f'{copiedVariablePrefix0}{x}'
        for x in lrj_ints_to_copy(flags)
    ]
    if flags.Input.isMC:
        float_variable_names += ["Lepton1_effSF"]
        int_variable_names += ["ProbeJet_R10TruthLabel_R22v1"]
    return float_variable_names, int_variable_names


def ttCalib_branches(flags):
    branches = []

    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarsttCalibAlg_variables(flags)

    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables

    all_baseline_variable_names += [*float_variable_names, *int_variable_names]

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> ttCalib_{var}"
                     + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    object_level_branches, object_level_float_variables, object_level_int_variables \
        = get_selected_objects_branches_variables(flags, "ttCalib")

    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables

    branches += object_level_branches
    # branches += ["EventInfo.ttCalib_pass_sr_%SYS% -> ttCalib_pass_SR"
    #              + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    # branches += [flags.Analysis.container_names.output.reco10UFOJet
    #              + ".isAnalysisJet_%SYS% -> recojet_antikt10UFO_isAnalysisLRJet"
    #              + flags.Analysis.systematics_suffix_separator + "%SYS%"]

    return branches, float_variable_names, int_variable_names
