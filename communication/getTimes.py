#! /usr/bin/env python

import sys
import subprocess

def main():
    sizes = [1,50,100,1000,4000,8000,50000,75000,100000,150000,250000,350000,\
                 500000,750000,1000000,2000000,3000000,4000000,5000000]
    if sys.argv[1] == 'tstw':
        for i in sizes:
            subprocess.call(["mpirun", "-machinefile", "machines", "-np", "2",\
                                 "tstw","50", str(i)]);
    elif sys.argv[1] == 'all2all':
        for j in [2,4,8,12,16]:
            for i in sizes[1:-1]:
                subprocess.call(["mpirun", "-machinefile", "machines", "-np",\
                                     str(j), "all2all","50", str(i)]);
            
    elif sys.argv[1] == 'allgather':
        for j in [2,4,8,12,16]:
            for i in sizes[1:-1]:
                subprocess.call(["mpirun", "-machinefile", "machines", "-np",\
                                     str(j), "allgather","50", str(i)]);
    elif sys.argv[1] == 'allreduce':
        for j in [2,4,8,12,16]:
            for i in sizes[1:-1]:
                subprocess.call(["mpirun", "-machinefile", "machines", "-np",\
                                     str(j), "allreduce","50", str(i)]);
        

if __name__ == '__main__':
    main()
