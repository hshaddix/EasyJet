# Change to easyjet directory to avoid long path issues with cpack
# cd /lustre/umt3/user/csuslu/HHH/easyjet || exit 1

runConfig="../bbbbbbAnalysis/share/RunConfig-HHH6b-SemiResolved.yaml"
executable="bbbbbb-ntupler"
campaignName="EJ_v1_0_HHH6b_NoSyst_SemiResolved" 
# my_eos="/eos/user/c/csuslu/6b_ntuples"

dir_samples="../bbbbbbAnalysis/datasets/HHH6b"
mc_list=(
    "$dir_samples/mc20_DAOD_PHYS_HHH6b_backgrounds.txt"
    "$dir_samples/mc20_DAOD_PHYS_HHH6b_SMHHH.txt"
)

# data
easyjet-gridsubmit --data-list $dir_samples/data_DAOD_PHYS_Run2.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 4 \
    --campaign ${campaignName} \
    --noTag #\
    # --dest-se y_eos 
    # --e 10

# rm -rf co*

#mc
easyjet-gridsubmit --mc-list <(cat "${mc_list[@]}") \
    --run-config ${runConfig} \
    --exec ${executable} \
    --nGBperJob 4 \
    --campaign ${campaignName} \
    --noTag #\
    # --dest-se my_eos 

rm -rf co*
