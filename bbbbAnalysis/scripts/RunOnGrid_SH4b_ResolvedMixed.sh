#!/bin/bash
set -euo pipefail
shopt -s dotglob nullglob

executable="bbbb-ntupler"

sys_config="bbbbAnalysis/RunConfig-SH4b-ResolvedMixed-Sys.yaml"
nosys_config="bbbbAnalysis/RunConfig-SH4b-ResolvedMixed.yaml"

sys_campaign="EJ_SH4b-ResolvedMixed-Sys_%y%m%d"
nosys_campaign="EJ_SH4b-ResolvedMixed_%y%m%d"

datasets_dir="../easyjet/bbbbAnalysis/datasets/SH4b"

destination="MWT2_UC_LOCALGROUPDISK"


# check for files in the current directory and exit if it isn't empty
pwd_contents=(*)
if (( "${#pwd_contents[@]}" > 0 ))
then
    echo "ERROR: submit from an empty directory!" >&2
    exit 1
fi

# print each command as it is executed
set -x

# signal
easyjet-gridsubmit \
    --mc-list "${datasets_dir}/mc20_DAOD_PHYS_ResolvedMixed_SH4b.txt" \
    --run-config "${sys_config}" \
    --exec "${executable}" \
    --campaign "${sys_campaign}" \
    --dest-se "${destination}" \
    --mergeOutput \
    --noTag
rm -- *

# data
easyjet-gridsubmit \
    --data-list "${datasets_dir}/data_DAOD_PHYS_ResolvedMixed.txt" \
    --run-config "${nosys_config}" \
    --exec "${executable}" \
    --campaign "${nosys_campaign}" \
    --dest-se "${destination}" \
    --mergeOutput \
    --noTag
rm -- *
