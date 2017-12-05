/*
 * A very simple MPI program which sends messages from node 0 to all
 * the other nodes.
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int size, rank, data;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* p0 will send something to everyone */
    if (rank == 0) {
        int i;
        for (i=1; i<size; i++) {
            printf("p%d sending %d to p%d\n",
                   rank, i, i);
            MPI_Send(&i, 1, MPI_INT, i,
                     0, MPI_COMM_WORLD);
        }
    }
    else {
        MPI_Status status;
        MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE,
                 0, MPI_COMM_WORLD, &status);
        printf("p%d received %d\n", rank, data);
    }

    printf("Hello from p%d of %d\n", rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
