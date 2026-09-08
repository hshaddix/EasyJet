runConfig="vbshiggsAnalysis/RunConfig-semiLep.yaml"
executable="vbshiggs-ntupler"
campaignName="VBSHiggs_v02_skim"

r2_tag="p7018"
r2_path="../../datasets/PHYS/$r2_tag"

r3_tag="p7017"
r3_path="../../datasets/PHYS/$r3_tag"

mc_list=(
    "$r2_path/mc20_semilep_signal_DAOD_PHYS_$r2_tag.txt", # Run2 mc20
    "$r2_path/mc20_VH_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_Wjets_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_stop_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_ttW_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_EWVVjj_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_VVV_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_Zjets_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_ttH_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_ttbar_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_Vgamma_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_ttV_DAOD_PHYS_$r2_tag.txt",
    "$r2_path/mc20_tty_DAOD_PHYS_$r2_tag.txt",
    "$r3_path/mc23_SMVH_DAOD_PHYS_$r3_tag.txt", # Run3 mc23
    "$r3_path/mc23_VVV_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_stop_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttVV_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttZ_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttyy_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_EWKWjets_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_SMVV_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_Wjets_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttHH_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttWH_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttbar_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_EWKZjets_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_VGamma_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_Zjets_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttH_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_ttW_DAOD_PHYS_$r3_tag.txt",
    "$r3_path/mc23_tty_DAOD_PHYS_$r3_tag.txt",
)

data_list=(
    "$r2_path/data_Run2_$r2_tag.txt",
    "$r3_path/data_Run3_$r3_tag.txt",
)

#mc
easyjet-gridsubmit --mc-list <(sed -e '$a\' "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag \
    --mergeOutput \
    --noEmail 

#data
easyjet-gridsubmit --data-list <(sed -e '$a\' "${data_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag \
    --mergeOutput \
    --noEmail \
    --ProductionRole HMBS
