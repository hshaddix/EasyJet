from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from EasyjetHub.output.ttree.selected_objects import (
    get_selected_objects_branches_variables,
)


def fullHad_cfg(flags, float_variables=None, int_variables=None):
    if not float_variables:
        float_variables = []
    if not int_variables:
        int_variables = []

    cfg = ComponentAccumulator()

    # set jets container labels
    # Full-hadronic analyses no longer create a dedicated VBS-jet split container.
    # The resolved Higgs selection therefore runs on the already-selected small-R
    # jet pool, which is the same container used for the VBF-RNN path.
    SignalJetsLabel = "vbshiggsAnalysisJets_%SYS%"
        
    # Selection
    cfg.addEventAlgo(CompFactory.VBSHIGGS.FullHadSelectorAlg(
            "FullHadSelectorAlg",
            signaljets=SignalJetsLabel,
            UseVBFRNN=flags.Analysis.UseVBFRNN,
            bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
            eventDecisionOutputDecoration="vbshiggs_pass_sr_%SYS%",
            cutList=flags.Analysis.CutList,
            saveCutFlow=flags.Analysis.save_cutflow,
            bypass=(flags.Analysis.bypass if hasattr(flags.Analysis, 'bypass')
                    else False),
            # do_resolved=flags.Analysis.do_resolved
    ))

    # calculate final vbshiggs vars
    cfg.addEventAlgo(
        CompFactory.VBSHIGGS.BaselineVarsFullHadAlg(
            "FinalVarsFullHadAlg",
            signaljets=SignalJetsLabel,
            UseVBFRNN=flags.Analysis.UseVBFRNN,
            isMC=flags.Input.isMC,
            do_resolved=flags.Analysis.do_resolved,
            bTagWPDecorName="ftag_select_"
                            + flags.Analysis.Small_R_jet.btag_wp,
            wtag_type=flags.Analysis.Large_R_jet.wtag_type,
            wtag_wp=flags.Analysis.Large_R_jet.wtag_wp, 
            floatVariableList=float_variables,
            intVariableList=int_variables
        )
    )
    return cfg


def get_BaselineVarsFullHadAlg_variables(flags):
    float_variable_names = []
    int_variable_names = []

    int_variable_names += ["nJets", "nBJets", "nLargeJets"]
    
    # obj = "LargeJet1"
    wtag_type = flags.Analysis.Large_R_jet.wtag_type
    wtag_wp = flags.Analysis.Large_R_jet.wtag_wp

    if flags.Analysis.do_resolved:
        for i in [1, 2]:
            for var in ["m", "pt", "eta", "phi", "E"]:
                float_variable_names.append(f"Jet_Higgs_candidate{i}_{var}")
    
        float_variable_names += [
            "Hdijet_m", "Hdijet_pt", "Hdijet_eta", "Hdijet_phi",
            "dRHjj", "dPhiHjj", "dEtaHjj"
        ]

  #  if not flags.Analysis.UseVBFRNN:
  #      for jet in ["VBSJ1", "VBSJ2", "VBSdijet"]:
  #          for var in ["m", "pt", "eta", "phi"]:
  #              float_variable_names.append(f"{jet}_{var}")
  #      float_variable_names += ["dRVBSjj", "dEtaVBSjj", "dPhiVBSjj"]
  #  else:
  #      regions = ["boostedCategory"]
  #      if flags.Analysis.do_resolved:
  #          regions.append("resolvedCategory")
  #      for reg in regions:
  #          for jet in ["Jet1", "Jet2"]:
  #              for var in ["m", "pt", "eta", "phi"]:
  #                  float_variable_names.append(f"RNNJets_{reg}_{jet}_{var}")

    return float_variable_names, int_variable_names


def get_BaselineVarsFullHadAlg_highlevelvariables(flags):
    high_level_float_variables = []
    high_level_int_variables = []

    return high_level_float_variables, high_level_int_variables


def fullHad_branches(flags):
    branches = []
    # this will be all the variables that are calculated by the
    # BaselineVarsFullHadAlg algorithm
    all_baseline_variable_names = []
    float_variable_names = []
    int_variable_names = []

    # these are the variables that will always be stored by easyjet
    # further below there are more high level variables which can be
    # stored using the flag
    # flags.Analysis.store_high_level_variables
    baseline_float_variables, baseline_int_variables \
        = get_BaselineVarsFullHadAlg_variables(flags)
    float_variable_names += baseline_float_variables
    int_variable_names += baseline_int_variables
    all_baseline_variable_names += [*float_variable_names, *int_variable_names]

    for var in all_baseline_variable_names:
        branches += [f"EventInfo.{var}_%SYS% -> {var}_%SYS%"]
        
    # Variables always saved with the objects selected by the analysis
    # This is tunable with the flags amount and variables
    # in the object configs.
    object_level_branches, object_level_float_variables, \
        object_level_int_variables \
        = get_selected_objects_branches_variables(flags, "FullHad")
    float_variable_names += object_level_float_variables
    int_variable_names += object_level_int_variables

    branches += object_level_branches
    
    cutList = flags.Analysis.CutList
    for cut in cutList:
        branches += [f"EventInfo.{cut}_%SYS% -> {cut}_%SYS%"]

    # truth info
    if flags.Input.isMC and flags.Analysis.AddTruthVBSQuarks:
        branches += ['EventInfo.nVBSQuarks -> nVBSQuarks']
        for var in ['pT', 'eta', 'phi', 'E']:
            branches += [f'EventInfo.VBSQuark1_{var} -> VBSQuark1_{var}']
            branches += [f'EventInfo.VBSQuark2_{var} -> VBSQuark2_{var}']

    # VBF tagger
    if flags.Analysis.UseVBFRNN:
        vars = ['RNNScore', 'nRNNJets']
        regs = ['boostedCategory']
        if flags.Analysis.do_resolved:
            regs += ['resolvedCategory']
        for var in vars:
            for reg in regs:
                branches += [f'EventInfo.{var}_{reg}_%SYS% -> {var}_{reg}_%SYS%']

    return branches, float_variable_names, int_variable_names
