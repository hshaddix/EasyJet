# Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration

from AnalysisAlgorithmsConfig.ConfigSequence import ConfigSequence
from AnalysisAlgorithmsConfig.ConfigFactory import ConfigFactory
from AnalysisAlgorithmsConfig.ConfigAccumulator import ExpertModeWarning

from EasyjetHub.steering.utils.log_helper import log
from EasyjetHub.steering.utils.systematics_helper import consolidate_systematics_regex
from EasyjetHub.steering.sample_metadata import is_at_least


import pathlib
import os
import yaml
import warnings


def get_sys_weight_name(input):
    # Determine if input is flags or an integer DSID
    if isinstance(input, int):
        dsid = input
    else:
        dsid = int(input.Input.MCChannelNumber)

    def FullPath(rawpath):
        fpath = pathlib.Path(rawpath)
        for dirpath in [""] + os.environ["DATAPATH"].split(":"):
            fullpath = dirpath / fpath
            if fullpath.exists():
                return fullpath
        raise FileNotFoundError(f'{rawpath} did not match any files in DATAPATH')

    # file name hard-coded
    with open(FullPath("EasyjetHub/SpecialWeightIndices.yaml"), 'r') as file_in:
        try:
            content = yaml.safe_load(file_in)
        except yaml.YAMLError as exc:
            raise ValueError(f"Error in configuration file: {exc}")

        for entry in content:
            if entry['DSID'] == dsid:
                sys_weight_name = entry.get("sysWeightName", "")

                # Exception in case I forgot to map the DSID with
                # the sys weight name for a MC sample.
                if sys_weight_name is None:
                    raise ValueError(
                        f"DSID {dsid} must have sysWeightName"
                    )

                return sys_weight_name if sys_weight_name else ""

    return ""  # in case no matching DSID is found


def common_services_sequence(flags, configAcc):

    # Set the warning level for expert mode options
    if level := flags.Analysis.expert_mode_warning_level:
        warnings.simplefilter(level, ExpertModeWarning)

    configSeq = ConfigSequence()
    config = ConfigFactory()
    makeConfig = config.makeConfig

    configSeq += makeConfig('CommonServices')

    has_uid = is_at_least(flags, "p7017")
    configSeq.setOptionValue('.fixDAODTruthRecord', not has_uid)
    truthParContFix = list(flags.Analysis.Truth.truthParticleContainersToFix)
    if truthParContFix:
        configSeq.setOptionValue('.truthParticleContainersToFix', truthParContFix)
    truthVertContFix = list(flags.Analysis.Truth.truthVertexContainersToFix)
    if truthVertContFix:
        configSeq.setOptionValue('.truthVertexContainersToFix', truthVertContFix)

    sys_weight_name = get_sys_weight_name(flags)
    configSeq.setOptionValue('.runSystematics',
                             flags.Analysis.do_CP_systematics or bool(sys_weight_name))

    # Keeping systematics regex config unchanged for now
    # Check later how onlySystematicsCategories can be used

    if flags.Analysis.do_CP_systematics or sys_weight_name:
        # if already selecting all systematics, no need to check for special one
        if (set(flags.Analysis.systematics_regex) == {'.*'}):
            systs = consolidate_systematics_regex(flags.Analysis.systematics_regex)

        # if not already selection all systematics,
        # add special one (if there is one) to the list
        elif sys_weight_name:
            # Convert sys weight name to be consolidated
            sys_weight_name = [f".*{sys_weight_name}"]
            # Join the CP systematics with the sys weight name
            if flags.Analysis.do_CP_systematics:
                sys_weight_name += list(flags.Analysis.systematics_regex)
            systs = consolidate_systematics_regex(sys_weight_name)
        else:
            systs = consolidate_systematics_regex(flags.Analysis.systematics_regex)

        log.info("Systematics regex:")
        log.info(systs)
        configSeq.setOptionValue('.filterSystematics', systs)

        configSeq.setOptionValue('.systematicsHistogram', "systematics")

        if hasattr(flags.Analysis, "ttree_output"):
            configSeq.setOptionValue('.streamName',
                                     'CBK' if flags.Analysis.splitCBK else
                                     flags.Analysis.ttree_output.stream_name)

    return configSeq
