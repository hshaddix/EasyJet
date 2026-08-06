#!/bin/bash
set -euo pipefail
shopt -s dotglob nullglob

executable="bbbbbb-ntupler"

bypass_config="bbbbbbAnalysis/RunConfig-HHH6b-Resolved-bypass.yaml"
bypass_campaign="EJ_HHH6b-Resolved-bypass-%y%m%d"

datasets_dir="../easyjet/bbbbbbAnalysis/datasets/HHH6b"


# check for files in the current directory and exit if it isn't empty
pwd_contents=(*)
if (( "${#pwd_contents[@]}" > 0 ))
then
    echo "ERROR: submit from an empty directory!" >&2
    exit 1
fi

# print each command as it is executed
set -x

# signal (mc20)
easyjet-gridsubmit \
    --mc-list "${datasets_dir}/mc20_DAOD_PHYS_HHH6b_SMHHH.txt" \
    --run-config "${bypass_config}" \
    --exec "${executable}" \
    --campaign "${bypass_campaign}" \
    --mergeOutput \
    --noTag
rm -- *
