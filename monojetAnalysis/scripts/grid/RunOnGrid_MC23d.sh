runConfig="../easyjet/monojetAnalysis/share/RunConfig-Monojet.yaml"
executable="monojet-ntupler"
campaignName="mc23d_monojet_v5"

#Diboson
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/Diboson_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag 


#SingleTop
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/SingleTop_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
   --noTag 

#ttbar
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/TTbar_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag

#ttbarX
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/TTbarX_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag 

#V+jets(lep)
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/Vjets_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag 

#V+jets(quarks)
easyjet-gridsubmit --mc-list ../easyjet/monojetAnalysis/datasets/PHYS/MC23d/Vjets_qq_mc23d_p7017.txt \
    --run-config ${runConfig} \
    --exec ${executable} \
    --campaign ${campaignName} \
    --noTag 
