ptag_mc=p6697
#ptag_data=p6700
campaign=v3

dir_samples="../easyjet/yyMLAnalysis/datasets/PHYS/"
yaml="RunConfig-yyml-resonant.yaml"
mc_campaign_signal="mc23e"
mc_campaign="${mc_campaign_signal}_p6"
mc_list_sample_names=(
    "PHYS_Signal_Resonant_HH_$mc_campaign_signal.txt"
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

easyjet-gridsubmit --data-list ${dir_samples}/PHYS_data24_p6.txt \
    --run-config yyMLAnalysis/${yaml} \
    --exec yyml-ntupler \
    --nGBperJob 50 \
    --campaign ${campaign}



