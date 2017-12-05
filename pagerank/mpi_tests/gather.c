#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int size, rank, i, idata;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int data[size];

    for (i=0; i<size; i++)
        data[i] = 0;

    idata = rank;

    MPI_Gather(&idata, 1, MPI_INT,
                &data, 1, MPI_INT,
                0, MPI_COMM_WORLD);

    printf("p%d: ", rank);
    if (rank == 0)
        for (i=0; i<size; i++)
            printf("%d ", data[i]);
    else
        printf("%d",idata);
    printf("\n");
    MPI_Finalize();
    return 0;
}
