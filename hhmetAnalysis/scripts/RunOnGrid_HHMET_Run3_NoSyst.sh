runConfig="hhmetAnalysis/RunConfig-HHMET-all.yaml"
executable="hhmet-ntupler"
campaignName="HHMET_v0p2_NoSyst"

dir_samples="../easyjet/hhmetAnalysis/datasets/"
ptag_dir_data="p7267_Run3"
ptag_dir_mc="p7266_mc23"

mc_list=(
    "$dir_samples/$ptag_dir_mc/diboson_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/singletop_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/ttbar_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/ttbb_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/ttV_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/VH_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/Wjets_PHYS.txt"
    "$dir_samples/$ptag_dir_mc/Zjets_PHYS.txt"
)
data_list=(
    "$dir_samples/$ptag_dir_data/data_PHYS.txt"
)

#data
easyjet-gridsubmit --data-list <(cat "${data_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 5 \
    --campaign ${campaignName} \
    --noTag \
    --dest-se CERN-PROD_LOCALGROUPDISK

rm -rf co*

#mc
easyjet-gridsubmit --mc-list <(cat "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 5 \
    --campaign ${campaignName} \
    --noTag \
    --dest-se CERN-PROD_LOCALGROUPDISK 

rm -rf co*
