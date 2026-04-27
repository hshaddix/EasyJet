runConfig="vbshiggsAnalysis/RunConfig-fullHad-bypass.yaml"
executable="vbshiggs-ntupler"
campaignName="VBSHiggs_v4_unskimmed"

mc_list=(
    "../easyjet/vbshiggsAnalysis/datasets/PHYS/mc20_FullHadronic_signal_DAOD_PHYS_p7018.txt"
    "../easyjet/vbshiggsAnalysis/datasets/allHad/PHYS/mc20_ttbar_allHad_DAOD_PHYS_p6697.txt"
    "../easyjet/vbshiggsAnalysis/datasets/allHad/PHYS/mc20_ttbar_DAOD_PHYS_allHad_p7018.txt"
    "../easyjet/vbshiggsAnalysis/datasets/allHad/PHYS/mc23_ttbar_allHad_DAOD_PHYS_p7017.txt"
)


#mc
easyjet-gridsubmit --mc-list <(sed -e '$a\' "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag \
    --mergeOutput \
    --noEmail
