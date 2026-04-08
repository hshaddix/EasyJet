ptag_mc=p6697
ptag_data=p6700
campaign=v2

dir_samples="../easyjet/yyMLAnalysis/datasets/PHYS/"
yaml="RunConfig-yyml.yaml"
#mc_campaign="mc23_13p6TeV"
mc_campaign="mc23a"
mc_list_sample_names=(
    "PHYS_Signal_Resonant_HH_$mc_campaign.txt"
    "PHYS_SingleHiggs_$mc_campaign.txt"
    "PHYS_Vyy_$mc_campaign.txt"
    "PHYS_ttyy_$mc_campaign.txt"
    "PHYS_yyjets_$mc_campaign.txt"
    "PHYS_yyjets_High_$mc_campaign.txt"
)

easyjet-gridsubmit --mc-list <(sed -e '$a\' "${mc_list_sample_names[@]/#/${dir_samples}}") \
    --run-config yyMLAnalysis/${yaml} \
    --exec yyml-ntupler \
    --nGBperJob 20 \
    --campaign ${campaign} \
    --noEmail

easyjet-gridsubmit --data-list ${dir_samples}/PHYS_data22.txt \
    --run-config yyMLAnalysis/${yaml} \
    --exec yyml-ntupler \
    --nGBperJob 50 \
    --campaign ${campaign}



