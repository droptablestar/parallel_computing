#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int size, rank, i;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int data[size];

    for (i=0; i<size; i++)
        data[i] = 0;

    if (rank == 0)
        for (i=0; i<size; i++)
            data[i] = i;

    MPI_Scatter(&data, 1, MPI_INT,
                &data, 1, MPI_INT,
                0, MPI_COMM_WORLD);

    printf("p%d: ", rank);
    for (i=0; i<size; i++)
        printf("%d ", data[i]);
    printf("\n");
    MPI_Finalize();
    return 0;
}
