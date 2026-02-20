runConfig="../easyjet/monojetAnalysis/share/RunConfig-Monojet.yaml"
executable="monojet-ntupler"
campaignName="mc23a_monojet_v5"

#Diboson
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/Diboson_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#SingleTop
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/SingleTop_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag
#RareTop
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/RareTop_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable}  \
    --campaign ${campaignName} \
    --noTag
#ttbar
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/TTbar_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#ttbarX
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/TTbarX_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#V+jets(lep)
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/Vjets_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#V+jets(quarks)
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23a/Vjets_qq_mc23a_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag
