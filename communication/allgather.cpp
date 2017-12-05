#include "mpi.h"
#include <stdlib.h>

#include <iostream>
#include <fstream>

using namespace std;

int MSGS;
int SIZE;

void printdata(char *data, int rank) {
    cout << "P" << rank << ":" << endl;
    for (int i=0; i<SIZE; i++)
        cout << data[i];
    cout << endl;
}

int main(int argc, char **argv) {
    int rank, avgT, i, numprocs, tag;
    double sumT, Ts, Te, delta;

    if (MSGS < 0 || SIZE < 0) {
        cout << "MSGS ans SIZE must be > 0\n";
        exit(-1);
    }
    MPI::Status status;
    
    MPI::Init(argc, argv);
    numprocs = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();

    MSGS = atoi(argv[1]); SIZE = atoi(argv[2]);
    char snd[SIZE], recv[SIZE];
    for (i=0; i<SIZE; i++) {snd[i] = 'a'+rank; recv[i]='0';}

    sumT = 0;
    tag = 1;

    ofstream filestr;
    if (rank ==0)
        filestr.open("res_allgather.txt", fstream::app);

    if (rank == 0) {
        cout << "Sending messages of size " << SIZE << " bytes" << endl;
        cout << "*********************************************" << endl;
        cout << "#proc\tMSG#\t\tTs\t\tTe\tdelta" << endl;
    }
    for (i=0; i<MSGS; i++) {
        if (rank == 0) Ts = MPI::Wtime();
        MPI::COMM_WORLD.Allgather(snd, SIZE/numprocs, MPI_CHAR,
                                 recv, SIZE/numprocs, MPI_CHAR);
        if (rank == 0) Te = MPI::Wtime();

        if (rank == 0) {
            delta = Te - Ts;
            sumT += delta;
            cout << numprocs << "\t" << i << "\t" << Ts << "\t" << Te <<
                "\t" << delta << endl;
        }
    }
    if (rank == 0) {
        avgT = (sumT*1000000)/MSGS;
        cout << "*********************************************" << endl;
        cout << "Avg roundtrip: " << avgT << endl;
        cout << "Avg one way: " << avgT/2 << endl;
        
        filestr << numprocs << "\t" << SIZE << "\t" << MSGS << "\t" <<
            avgT/2 << endl;
        filestr.close();
    }
    MPI::COMM_WORLD.Barrier();
    // printdata(snd, rank);
    // printdata(recv, rank);

    MPI::Finalize();
    exit(0);
}

