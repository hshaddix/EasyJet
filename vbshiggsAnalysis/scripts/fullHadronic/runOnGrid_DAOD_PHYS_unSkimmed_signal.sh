runConfig="vbshiggsAnalysis/RunConfig-fullHad-bypass.yaml"
executable="vbshiggs-ntupler"
campaignName="VBSHiggs_v4_unskimmed"

mc_list=(
    "../../datasets/PHYS/mc20_FullHadronic_signal_DAOD_PHYS_p7018.txt"
)


#mc
easyjet-gridsubmit --mc-list <(sed -e '$a\' "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag \
    --mergeOutput \
    --noEmail
