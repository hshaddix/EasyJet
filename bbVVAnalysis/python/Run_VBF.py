#!/usr/bin/python3

import subprocess
import os
import sys
import argparse

dbg_level = "INFO"

easyjet_build_dir = os.environ['easyjet_DIR']
easyjet_run_dir = easyjet_build_dir + "/../../run/"

# 1lep SplitBoosted under development
sample_DSID_Dict = {
    "cvv0": "546872", "cvv0p5": "546873", "cvv1": "546874",
    "cvv1p5": "546875", "cvv2": "546876", "cvv3": "546877"}


def get_sample_dir(path, dsid_str):
    tmp = "999999"
    with os.scandir(path) as entries:  # Get a list of file/subdir in path as entries

        for entry in entries:
            # Assume the subdir's name has substr
            if (entry.is_dir()) and (dsid_str in entry.name):
                tmp = os.path.realpath(entry) + '/'

    if tmp == "999999":
        sys.exit("!!!!!SAMPLE NOT FOUND IN SAMPLEDIR!!!!!")

    return tmp


def io_manager(
        process="Signal",
        Sample_dir="", kfactor="", systematics=False):

    outFile = "ejOutput_PHYS_bbVV_"
    sys = "_sys" if systematics else ""
    configFile = "bbVVAnalysis/RunConfig-PHYS-bbVV-1lep-boosted-VBF" + sys + ".yaml"
    outFile += "1lep_boosted_"

    if process == "Signal":

        sample = get_sample_dir(
            Sample_dir, sample_DSID_Dict[kfactor])
        outFile += kfactor + "_VBF.root"

    return sample, outFile, configFile


def run_local(
        process="Signal",
        sample_dir="", campaign="", systematics=False):
    kfactor = ["cvv0", "cvv0p5", "cvv1", "cvv1p5", "cvv2", "cvv3"]
    for entry in kfactor:
        inputFile, outputFile, configFile = io_manager(
            process, sample_dir, entry, systematics)
        input = '"' + inputFile + '*"'
        command = ("bbVV-ntupler " + input + " --run-config "
                   + configFile + " --out-file " + outputFile)

        os.chdir(easyjet_run_dir)  # Move to the easyjet/run directory

        print(command)
        subprocess.run(command, shell=True)
        print(
            "End of Run_VBF.py, check your output\nOutFile:",
            os.path.realpath(outputFile))


def run_grid(process="vbfHHbbWW1lep", campaign="", notag=False, systematics=False):
    executable = "bbVV-ntupler"
    ListDir = easyjet_build_dir + "/data/bbVVAnalysis/PHYS/nominal/"

    sys = "_sys" if systematics else ""
    configFile = "bbVVAnalysis/RunConfig-PHYS-bbVV-1lep-boosted-VBF" + sys + ".yaml"

    if notag:
        notag = " --noTag"
    else:
        notag = " "
    if process == "data":
        if campaign == "mc20a":
            year = "20152016"
        if campaign == "mc20d":
            year = "2017"
        if campaign == "mc20e":
            year = "2018"
        if campaign == "mc23a":
            year = "2022"
        if campaign == "mc23d":
            year = "2023"
        List = ListDir + campaign + "/" + process + "_" + year + "_p*.txt"
        command = ("easyjet-gridsubmit --data-list "
                   + List + " --run-config " + configFile + " --exec " + executable
                   + " --nGBperJob 5 " + " --campaign " + campaign + notag)
    else:
        List = ListDir + campaign + "/" + campaign + "_" + process + "_p*.txt"
        command = ("easyjet-gridsubmit --mc-list "
                   + List + " --run-config " + configFile + " --exec " + executable
                   + " --nGBperJob 5 " + " --campaign " + campaign + notag)
    print(command)
    subprocess.run(command, shell=True)
    print("End of Run_VBF.py, check your BigPanda!")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--Process",
        help="""Define which process: vbfHHbbWW1lep, vbfHHbbWW1lepFASTSIM, dijet,
        singleTop, singleHiggs, ttV, ttbar, Wjets, Zjets, diboson, data_{year}""",
        default="Signal")
    parser.add_argument("--GridRun", help="Run on Grid", action='store_true')
    parser.add_argument(
        "--SampleDir", help="Type your PHYS directory",
        default="/eos/user/c/cstauch/PostProcessing_HH/run/Sample_Lists/mc20a/Signal/")
    parser.add_argument(
        "--campaign", help="mc20a, mc20d, mc20e, mc23a, mc23d", default="mc20a")
    parser.add_argument(
        "--noTag", help="Skip git tag creation", action="store_true")
    parser.add_argument(
        "--sys", help="Run systematics", action='store_true')
    args = parser.parse_args()
    process = args.Process
    grid = args.GridRun
    sample_dir = args.SampleDir
    campaign = args.campaign
    notag = args.noTag
    systematics = args.sys
    print(
        "Process:", process, "\tGrid:", grid,
        "\tSampleDir:", sample_dir, "\tCampaign", campaign, "\tSys:", systematics)

    if notag:
        print("noTag = true : no git tag will be created")

    # Need to synchronize with io_manager / run_grid
    if grid:
        print("Running on Grid.")
        run_grid(process, campaign, notag, systematics)
    else:
        print("Running locally.")
        run_local(process, sample_dir, systematics)
