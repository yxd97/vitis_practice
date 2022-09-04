echo setting up Vitis-2022.1 environment......
unset LM_LICENSE_FILE
export XILINXD_LICENSE_FILE=2100@flex.ece.cornell.edu
module load xilinx-vivado-vitis_2022.1.2
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/xilinx/2022.1/Vitis/2022.1/lib/lnx64.o
echo Vitis-2022.1 setup finished
