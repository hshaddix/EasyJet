#!/bin/bash
set -euo pipefail
shopt -s dotglob nullglob

runConfig="../bbbbbbAnalysis/share/RunConfig-HHH6b-Resolved.yaml"
executable="bbbbbb-ntupler"
campaignName="EJ_v1_0_HHH6b_NoSyst_Resolved" 

dir_samples="../bbbbbbAnalysis/datasets/HHH6b"
mc_list=(
    "$dir_samples/mc20_DAOD_PHYS_HHH6b_SMHHH.txt"
    "$dir_samples/mc23_DAOD_PHYS_HHH6b_SMHHH.txt"
)

data_list=(
    "$dir_samples/data_DAOD_PHYS_Run2.txt"
    "$dir_samples/data_DAOD_PHYS_Run3.txt"
)

# data(Run2 and Run3)
easyjet-gridsubmit --data-list <(cat "${data_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 4 \
    --campaign ${campaignName} \
    --noTag

set -x

#mc(mc20 and mc23)
easyjet-gridsubmit --mc-list <(cat "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 4 \
    --campaign ${campaignName} \
    --noTag

set -x

rm -rf co*
