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

ptag=p7018
campaign=v4p1
dir_samples="../easyjet/bbyyAnalysis/datasets/PHYS/nominal/"
mc_campaign="mc20_13TeV"
yaml_yyjets="RunConfig-HH_H-skimming-loosest_yyjets.yaml"
yaml_HH_H="RunConfig-HH_H-skimming-loosest_HH_H.yaml"
hh_h_mc_list=(
    "$mc_campaign.HH_bbyy_SM.$ptag.txt"
    "$mc_campaign.ggFHH_bbyy_BSM.$ptag.txt"
    "$mc_campaign.VBFHH_bbyy_BSM.$ptag.txt"
    "$mc_campaign.SingleHyy.$ptag.txt"
    "$mc_campaign.yyBkg.$ptag.txt"
)
yyjets_mc_list=(
    "$mc_campaign.yyjets.$ptag.txt"
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

    easyjet-gridsubmit --data-list ${dir_samples}/data_13TeV.Run2.${ptag}.txt \
        --run-config bbyyAnalysis/${yaml_yyjets} \
        --exec bbyy-ntupler \
        --nGBperJob 50 \
        --campaign ${campaign} \
        --noEmail
fi
