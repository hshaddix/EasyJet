runConfig="../easyjet/monojetAnalysis/share/RunConfig-Monojet.yaml"
executable="monojet-ntupler"
campaignName="v5_data_monojet"

#data 23a
easyjet-gridsubmit --data-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/Data_Run3_p7017.txt \
     --run-config ${runConfig} \
     --exec ${executable} \
     --campaign ${campaignName} \
     --noTag
#data23d
easyjet-gridsubmit --data-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/Data_Run3_p7017.txt \
     --run-config ${runConfig} \
     --exec ${executable} \
     --campaign ${campaignName} \
     --noTag
#data23e
easyjet-gridsubmit --data-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/Data_Run3_p7017.txt \
     --run-config ${runConfig} \
     --exec ${executable} \
     --campaign ${campaignName} \
     --noTag

