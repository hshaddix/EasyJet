#!/bin/bash

submit_yyjets_data=false
submit_HH_H=false
for arg in "$@"; do
    case $arg in
        --yyjets-data) submit_yyjets_data=true ;;
        --HH-H) submit_HH_H=true ;;
        -h|--help) echo "Usage: $0 [--yyjets-data] [--HH-H]"; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; exit 1 ;;
    esac
done
$submit_yyjets_data || $submit_HH_H || { submit_yyjets_data=true; submit_HH_H=true; }

ptag_mc=p7266
ptag_data=p7267
data25_nfiles_first=190000
campaign=v4p1
dir_samples="../easyjet/bbyyAnalysis/datasets/PHYS/nominal/"
mc_campaign="mc23_13p6TeV"
yaml_yyjets="RunConfig-HH_H-skimming-loosest_yyjets.yaml"
yaml_HH_H="RunConfig-HH_H-skimming-loosest_HH_H.yaml"
data_list="${dir_samples}/data_13p6TeV.Run3.${ptag_data}.txt"
hh_h_mc_list=(
    "$mc_campaign.HH_bbyy_SM.$ptag_mc.txt"
    "$mc_campaign.ggFHH_bbyy_BSM.$ptag_mc.txt"
    "$mc_campaign.VBFHH_bbyy_BSM.$ptag_mc.txt"
    "$mc_campaign.SingleHyy.$ptag_mc.txt"
)
yyjets_mc_list=(
    "$mc_campaign.yyBkg.$ptag_mc.txt"
)

if $submit_HH_H; then
    easyjet-gridsubmit --mc-list <(sed -e '$a\' "${hh_h_mc_list[@]/#/${dir_samples}}") \
        --run-config bbyyAnalysis/${yaml_HH_H} \
        --exec bbyy-ntupler \
        --nGBperJob 20 \
        --campaign ${campaign} \
        --noEmail
fi

if $submit_yyjets_data; then
    easyjet-gridsubmit --mc-list <(sed -e '$a\' "${yyjets_mc_list[@]/#/${dir_samples}}") \
        --run-config bbyyAnalysis/${yaml_yyjets} \
        --exec bbyy-ntupler \
        --nGBperJob 20 \
        --campaign ${campaign} \
        --noEmail

    easyjet-gridsubmit --data-list <(head -n 3 "${data_list}") \
        --run-config bbyyAnalysis/${yaml_yyjets} \
        --exec bbyy-ntupler \
        --nGBperJob 50 \
        --campaign ${campaign} \
        --noEmail

    easyjet-gridsubmit --data-list <(tail -n 1 "${data_list}") \
        --run-config bbyyAnalysis/${yaml_yyjets} \
        --exec bbyy-ntupler \
        --nGBperJob 50 \
        --nFiles ${data25_nfiles_first} \
        --campaign ${campaign}_data25_0 \
        --noTag \
        --noEmail

    easyjet-gridsubmit --data-list <(tail -n 1 "${data_list}") \
        --run-config bbyyAnalysis/${yaml_yyjets} \
        --exec bbyy-ntupler \
        --nGBperJob 50 \
        --nSkipFiles ${data25_nfiles_first} \
        --campaign ${campaign}_data25_1 \
        --noTag \
        --noEmail
fi
