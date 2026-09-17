#!/usr/bin/env python3
# Yassine El Ghazali
# Modified for XbbClaib use by Iza Veliscek and Nikita Pond
# script helps with grid submission
# to run:
# all samples: XbbCalib-submitGrid.py --analysis Zbbj --mc mc20 mc23 \
# --data run2 run3
# A specific samples:
# python3 XbbCalib-submitGrid.py -s "Wjet" --ptag p701X \
#  --mc mc20 mc23
# By default will use:
# All samples for a specific analysis, as defined in MC_PROCESSES_BY_ANALYSIS
# Default config for a specific analysis, as defined in CONFIG_BY_ANALYSIS
# Any additional arguments not covered here will be passed to
# easyjet-gridsubmit, e.g. --nFilesPerJob, --maxFiles, --noSubmit, etc.
# A note on systematics production: The parton shower variations need to be
# submitted with the specific sample option described above and with nominal
# only config

import os
import tempfile
from argparse import (
    ArgumentParser,
    ArgumentDefaultsHelpFormatter,
    ArgumentTypeError,
)
from pathlib import Path

from EasyjetHub.steering.utils.config_files import run_config_arg

DEFAULT_PTAG_BY_ANALYSIS = {
    "Zbbj":       "p701X",
    "Zbbj-aod":   "p701X",
    "Zbbj-hists": "p701X",
    "Zbby":       "p701X",
    "Zbby-aod":   "p701X",
    "Zbby-hists": "p701X",
    "Zllj":        "p701X",
    "Zlly":        "p701X",
    "tt":         "p701X",
}

OUTPUT_FLAGS_BY_ANALYSIS = {
    "Zbbj-aod":   ["--output-xaod"],
    "Zbbj-hists": ["--h5-output"],
    "Zbby-aod":   ["--output-xaod"],
    "Zbby-hists": ["--h5-output"],
}

_ZBBY_PROCESSES = [
    "dijets",
    "Zbb_ptZ_200_ECMS",
    "Zcc_pTZ_200_ECMS",
    "Zbbgamma_pTZ100",
    "Zccgamma_pTZ_100_ECMS",
    "Zqq_ptZ_200_ECMS",
    "Zqqgamma_pTZ100",
    "SinglePhoton",
    "Vgamma_Vgammagamma",
    "Wqq_ptW_200_ECMS",
    "Wqqgamma_pTW140",
    "ttbar_allhad",
    "tty",
    "Higgs",
]

_ZBBJ_PROCESSES = [
    "dijets",
    # "dijet_bfilt",
    "Zbb_ptZ_200_ECMS",
    "Zcc_pTZ_200_ECMS",
    "Zqq_ptZ_200_ECMS",
    "Wqq_ptW_200_ECMS",
    "ttbar_allhad",
    "Higgs",
]

MC_PROCESSES_BY_ANALYSIS = {
    "Zbbj":       _ZBBJ_PROCESSES,
    "Zbbj-aod":   _ZBBJ_PROCESSES,
    "Zbbj-hists": _ZBBJ_PROCESSES,
    "Zbby":       _ZBBY_PROCESSES,
    "Zbby-aod":   _ZBBY_PROCESSES,
    "Zbby-hists": _ZBBY_PROCESSES,
    "Zllj": [
        "Diboson",
        "Zllj"
    ],
    "Zlly": [
        "Zlly"
    ],
    "tt": [
        "Diboson",
        "Wjet",
        "singletop",
        "ttbar_nonallhad",
    ],
}

GRID_PARAM_FLAGS = {
    'n_files_per_job': lambda x: ['--nFilesPerJob', str(x)],
}


def _load_grid_defaults(config_path_str: str) -> list:
    """Return CLI args derived from the 'grid' section of a config file."""
    try:
        config = run_config_arg(config_path_str)
    except ArgumentTypeError:
        return []
    grid_section = config['grid']
    result = []
    for key, value in grid_section.items():
        if key not in GRID_PARAM_FLAGS:
            raise ValueError(f"Unknown grid config key: {key!r}")
        result += GRID_PARAM_FLAGS[key](value)
    return result


# Define the default config to use for a given analysis
CONFIG_BY_ANALYSIS = {
    "Zbbj":       "XbbCalib/RunConfig_ZbbjCalib.yaml",
    "Zbbj-aod":   "XbbCalib/RunConfig_ZbbjCalib.yaml",
    "Zbbj-hists": "XbbCalib/histonly-config-Zbbj.yaml",
    "Zbby":       "XbbCalib/RunConfig-ZbbyCalib.yaml",
    "Zbby-aod":   "XbbCalib/RunConfig-ZbbyCalib.yaml",
    "Zbby-hists": "XbbCalib/histonly-config-Zbby.yaml",
    "Zllj":        "XbbCalib/RunConfig-Zllj.yaml",
    "Zlly":        "XbbCalib/RunConfig-Zlly.yaml",
    "tt":         "XbbCalib/RunConfig-ttCalib.yaml",
}


def _find_samples_base() -> Path:
    """Search DATAPATH for the XbbCalib dataset directory."""
    for entry in os.environ.get("DATAPATH", "").split(":"):
        candidate = Path(entry) / "XbbCalib"
        if candidate.is_dir():
            return candidate
    # Fallback when running directly from source tree
    return (Path(__file__).parent / "../datasets/").resolve()


SAMPLES_BASE_PATH = _find_samples_base()


def get_args():

    all_data = ["run2", "run3"]
    all_mc = ["mc20", "mc23"]

    parser = ArgumentParser(description="",
                            formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "--ptag",
        type=str,
        default=None,
        help="Production tag (e.g. p7018, p6490). "
             "Default: highest available for the chosen analysis.",
    )
    parser.add_argument(
        "--data",
        nargs="*",
        choices=all_data,
        help="Space-separated list of data periods to process"
    )
    parser.add_argument(
        "--mc",
        nargs="*",
        choices=all_mc,
        help="Space-separated list of mc campaigns to process"
    )
    parser.add_argument(
        "-c",
        "--config",
        help="Use this config instead of the default one")
    parser.add_argument(
        "-s",
        "--samples",
        nargs="+",
        required=False,
        help="Space-delimited list of samples, \"MG_Zqqgamma SinglePhoton\" "
             "If none are given, all samples for a given analysis are run",)
    parser.add_argument(
        "--analysis",
        type=str,
        required=True,
        choices=list(CONFIG_BY_ANALYSIS),
        help="Which analysis to run - defines the default config and samples"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="If set, will only print the commands that would be run, "
             "without executing"
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Limit each campaign/period to the first dataset only.",
    )
    args, remainder = parser.parse_known_args()

    if args.mc == []:
        args.mc = all_mc
    if args.data == []:
        args.data = all_data
    if args.ptag is None:
        args.ptag = DEFAULT_PTAG_BY_ANALYSIS[args.analysis]

    return args, remainder


def make_temp_file_with_containers(mc_list, limit=None):
    '''Creates a temporary file containing all of the unique containers found
    by reading all the text files in mc_list, preserving order.'''
    containers = []
    seen = set()
    for mc_file in mc_list:
        with open(mc_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    if line not in seen:
                        seen.add(line)
                        containers.append(line)
    if limit is not None:
        containers = containers[:limit]
    with tempfile.NamedTemporaryFile(mode='w+', delete=False) as temp_file:
        for container in containers:
            temp_file.write(f"{container}\n")
    return temp_file.name


def get_all_mc_samples(
    analysis,
    campaign,
    ptag,
    samples=None,
):
    '''
    Get all the relevent samples.txt files for a given analysis, mc campaign,
    and ptag

    Parameters
    ----------
    analysis : str
        Analysis name, e.g. Zbbj, Zbby
    campaign : str
        MC campaign, e.g. mc20, mc23
    ptag : str
        Production tag, e.g. p7018, p6490, etc
    '''
    assert analysis in MC_PROCESSES_BY_ANALYSIS, \
        f"Analysis {analysis} not found in MC_PROCESSES_BY_ANALYSIS"
    assert campaign in ["mc20", "mc23"], f"Campaign {campaign} not supported"
    if samples is None:
        samples = MC_PROCESSES_BY_ANALYSIS[analysis]
    mc_list = []
    missing_files = []

    for process in samples:
        sample_file = SAMPLES_BASE_PATH / process / ptag / f"{campaign}.txt"

        if sample_file.exists():
            mc_list.append(str(sample_file))
        else:
            missing_files.append(str(sample_file))

    if missing_files:
        raise ValueError(
            f"For analysis {analysis}, campaign {campaign}, ptag {ptag}, "
            f"the following sample files are missing: {missing_files}"
        )
    return mc_list


def submit(command, dry_run):
    if dry_run:
        command = command + ["--dry-run"]
    os.system(" ".join(command))


def main(args, ej_grid_submit_args):

    analysis = args.analysis
    runConfig = args.config or CONFIG_BY_ANALYSIS[analysis]
    if not args.mc and not args.data:
        raise ValueError(
            "At least one of --mc or --data must be provided"
        )
    test_limit = 1 if args.test else None

    ej_command = [
        "easyjet-gridsubmit",
        "--run-config", runConfig,
        "--exec", "xbbcalib-ntupler",
        "--mergeOutput",
    ]
    ej_command += OUTPUT_FLAGS_BY_ANALYSIS.get(analysis, [])
    ej_command += _load_grid_defaults(runConfig)
    ej_command += ej_grid_submit_args

    for campaign in args.mc or []:
        mc_list = get_all_mc_samples(
            analysis, campaign, args.ptag, args.samples
        )
        tmp = make_temp_file_with_containers(mc_list, limit=test_limit)
        submit(ej_command + ["--mc-list", tmp], args.dry_run)

    for data_period in args.data or []:
        data_file = (
            SAMPLES_BASE_PATH / "data" / args.ptag / f"{data_period}.txt"
        )
        assert data_file.exists(), f"Data file {data_file} not found"
        tmp = make_temp_file_with_containers(
            [str(data_file)], limit=test_limit
        )
        submit(ej_command + ["--data-list", tmp], args.dry_run)


if __name__ == "__main__":
    this_args, ej_grid_submit_args = get_args()
    main(this_args, ej_grid_submit_args)
