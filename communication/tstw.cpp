#include "mpi.h"
#include <stdlib.h>

#include <iostream>
#include <fstream>

using namespace std;

int MSGS;
int SIZE;

int main(int argc, char **argv) {
    int rank, dest, source, avgT, i, numprocs, tag;
    double sumT, Ts, Te, delta;

    if (MSGS < 0 || SIZE < 0) {
        cout << "MSGS ans SIZE must be > 0\n";
        exit(-1);
    }
    MSGS = atoi(argv[1]); SIZE = atoi(argv[2]);
    MPI::Status status;
    
    char msg[SIZE];
    for (i=0; i<SIZE; i++) msg[i] = 'a';

    MPI::Init(argc, argv);
    numprocs = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();
    
    sumT = 0;
    tag = 1;

    if (rank == 0) {
        ofstream filestr("results.txt", fstream::app);
    
        source = 1; dest = 1;
        cout << "Sending messages of size " << SIZE << " bytes" << endl;
        cout << "*********************************************" << endl;
        cout << "MSG#\t\tTs\t\tTe\tdelta" << endl;

        for (i=0; i<MSGS; i++) {
            Ts = MPI::Wtime();
            MPI::COMM_WORLD.Send(msg, SIZE, MPI_CHAR, dest, tag);
            MPI::COMM_WORLD.Recv(msg, SIZE, MPI_CHAR, source, tag, status);
            Te = MPI::Wtime();
            delta = Te - Ts;
            sumT += delta;
            
            cout << i << "\t" << Ts << "\t" << Te << "\t" << delta << endl;
        }
        avgT = (sumT*1000000)/MSGS;
        cout << "*********************************************" << endl;
        cout << "Avg roundtrip: " << avgT << endl;
        cout << "Avg one way: " << avgT/2 << endl;

        filestr << SIZE << "\t" << MSGS << "\t" << avgT/2 << endl;
        filestr.close();
    }

    else if (rank == 1) {
        source = 0; dest = 0;
        for (i=0; i<MSGS; i++) {
            MPI::COMM_WORLD.Recv(msg, SIZE, MPI_CHAR, source, tag, status);
            MPI::COMM_WORLD.Send(msg, SIZE, MPI_CHAR, dest, tag);
        }
    }
    MPI::Finalize();
    exit(0);
}
