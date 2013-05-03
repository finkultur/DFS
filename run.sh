#/bin/sh
# run.sh 
# Run this with ./run <workload-file>

make
/opt/tilepro/bin/tile-monitor --tile 2x3 --pci --here --mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/ --hv-bin-dir /scratch/src/sys/hv/ -- ./main $1
