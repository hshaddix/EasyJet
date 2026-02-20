runConfig="../easyjet/monojetAnalysis/share/RunConfig-Monojet.yaml"
executable="monojet-ntupler"
campaignName="mc23e_monojet_v5"

#Diboson
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/Diboson_mc23e_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#SingleTop
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/SingleTop_mc23e_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag

#ttbar
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/TTbar_mc23e_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#ttbarX
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/TTbarX_mc23e_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#V+jets
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23e/Vjets_mc23e_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

