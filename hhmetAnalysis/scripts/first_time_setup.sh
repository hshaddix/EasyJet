export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
alias setupATLAS='source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh'
setupATLAS

lsetup git
cd easyjet
git lfs install
git lfs pull
cd ../
mkdir -p build run
cd build
source ../easyjet/setup.sh
#'asetup 25.2.60,AnalysisBase
cmake ../easyjet/
make -j16
source */setup.sh
cd ../run
