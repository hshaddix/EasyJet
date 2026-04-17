from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

from bbyyAnalysis.bbyy_base_config import bbyy_branches
from bbyyAnalysis.bbyy_base_config import bbyy_cfg

from itertools import chain


def bbyy_SH_cfg(flags, smalljetkey, photonkey, muonkey, electronkey,
                largeRjetkey, float_variables=None, int_variables=None):
    # Call base bbyy config
    cfg = ComponentAccumulator()
    cfg.merge(
        bbyy_cfg(flags, smalljetkey, photonkey, muonkey, electronkey,
                 largeRjetkey, float_variables, int_variables)
    )

    if flags.Analysis.do_resonant_PNN:
        cfg.addEventAlgo(
            CompFactory.SHBBYY.ResonantPNNbbyyAlg(
                "ResonantPNNbbyyAlg",
                bTagWPDecorName="ftag_select_" + flags.Analysis.Small_R_jet.btag_wp,
                mX_mS_pairs=flags.Analysis.mX_mS_pairs,
                mS_values=flags.Analysis.mS_values,
                mX_values=flags.Analysis.mX_values,
                mX_1bjet=flags.Analysis.mX_1bjet,
                floatVariableList=float_variables["PNN"],
            )
        )

    return cfg


def get_PNNVarsbbyyAlg_SH(flags):
    SH_float_variable_names = []
    SH_int_variable_names = []

    for pair in flags.Analysis.mX_mS_pairs:
        m_X = pair[0]
        m_S = pair[1]
        SH_float_variable_names += [f"SH_PNN_Score_X{m_X}_S{m_S}"]

    for m_X in flags.Analysis.mX_values:
        for m_S in flags.Analysis.mS_values:
            if m_X > 500 and m_S < 70:
                continue
            if m_X - m_S <= 125:
                continue
            SH_float_variable_names += [f"SH_PNN_Score_X{m_X}_S{m_S}"]

    for m_X in flags.Analysis.mX_1bjet:
        SH_float_variable_names += [f"SH_PNN_Score_1bjet_X{m_X}"]

    return SH_float_variable_names, SH_int_variable_names


def bbyy_SH_branches(flags):
    # Get base bbyy branches and variables
    branches, float_variable_names, int_variable_names = bbyy_branches(flags)

    SH_variable_names = []
    keys = ["PNN"]

    SH_float_variable_names = {key: [] for key in keys}
    SH_int_variable_names = {key: [] for key in keys}

    # these are the variables that will always be stored by easyjet specific to SH
    SH_float_variables, SH_int_variables \
        = get_PNNVarsbbyyAlg_SH(flags)

    if flags.Analysis.save_detailed_Input_variables:
        SH_float_variable_names['PNN'] += SH_float_variable_names
        SH_int_variable_names['PNN'] += SH_int_variable_names

    SH_variable_names += [*chain.from_iterable(SH_float_variable_names.values()),
                          *chain.from_iterable(SH_int_variable_names.values())]

    if not flags.Analysis.do_CP_systematics:
        sys_suffix = "NOSYS"
    else:
        sys_suffix = "%SYS%"

    for var in SH_variable_names:
        branches += [f"EventInfo.{var}_{sys_suffix} -> bbyy_{var}"
                     + flags.Analysis.systematics_suffix_separator + sys_suffix]

    return branches, \
        float_variable_names | SH_float_variable_names, \
        int_variable_names | SH_int_variable_names
