# Change to easyjet directory to avoid long path issues with cpack
# cd /lustre/umt3/user/csuslu/HHH/easyjet || exit 1

runConfig="/lustre/umt3/user/csuslu/HHH/easyjet/bbbbbbAnalysis/share/RunConfig-HHH6b-Resolved.yaml"
executable="bbbbbb-ntupler"
campaignName="EJ_v0_94_HHH6b_NoSyst" 
my_eos="/eos/user/c/csuslu/6b_ntuples"

dir_samples="/lustre/umt3/user/csuslu/HHH/easyjet/bbbbbbAnalysis/datasets/HHH6b"
mc_list=(
    "$dir_samples/mc20_DAOD_PHYS_HHH6b_backgrounds2.txt"
    # "$dir_samples/mc20_DAOD_PHYS_HHH6b_SMHHH.txt"
)

#data
# easyjet-gridsubmit --data-list $dir_samples/data_DAOD_PHYS_Run2.txt \
#     --run-config ${runConfig} \
#     --exec ${executable} \
#     --nGBperJob 4 \
#     --campaign ${campaignName} \
#     --noTag #\
#     # --dest-se y_eos 
#     # --e 10

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
