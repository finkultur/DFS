#!/bin/bash
# Runs some CPU2006 tests with the train input data.

start429=$(date +%s)
echo $start429
echo "Starting test 429.milc"
cd /opt/benchmarks/SPEC2006/benchspec/CPU2006/429.mcf/
/opt/tilepro/bin/tile-monitor --pci --tile 1x1 --here -- run/build_base_compsys.0000/mcf data/train/input/inp.in
end429=$(date +%s)
time429=$(($end429-$start429))
echo "test 429 took $time429 seconds\n"

start433=$(date +%s)
echo $start433
echo "Starting test 433.milc"
cd /opt/benchmarks/SPEC2006/benchspec/CPU2006/433.milc/
/opt/tilepro/bin/tile-monitor --pci --tile 1x1 --here -- run/build_base_compsys.0000/milc data/train/input/su3imp.in
end433=$(date +%s)
time433=$(($end433-$start433))
echo "test 433 took $time433 seconds\n"

start450=$(date +%s)
echo $start450
echo "Starting test 450.soplex"
cd /opt/benchmarks/SPEC2006/benchspec/CPU2006/450.soplex/
/opt/tilepro/bin/tile-monitor --pci --tile 1x1 --here -- run/build_base_compsys.0000/soplex data/train/input/train.mps
end450=$(date +%s)
time450=$(($end450-$start450))
echo "test 450 took $time450 seconds\n"

start470=$(date +%s)
echo $start470
echo "Starting test 470.lbm"
cd /opt/benchmarks/SPEC2006/benchspec/CPU2006/470.lbm/
/opt/tilepro/bin/tile-monitor --pci --tile 1x1 --here -- run/build_base_compsys.0000/lbm 300 myreference.dat 0 1 data/train/input/100_100_130_cf_b.of 
end470=$(date +%s)
time470=$(($end470-$start470))
echo "test 470 took $time470 seconds\n"


echo "Time elapsed per test:"
echo "test 429 took $time429 seconds\n"
echo "test 433 took $time433 seconds\n"
echo "test 450 took $time450 seconds\n"
echo "test 470 took $time470 seconds\n"

