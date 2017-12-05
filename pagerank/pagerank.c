#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <vector.h>

int **distribute_data(int, int, char**);
void print_arrays(int **, int, unsigned int);

unsigned int num_nodes;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: mpirun -machinefile <filename>"
               "-np <# of procs> <name of input file>\n");
        return 1;
    }

    unsigned int nodes_per_proc;
    int numprocs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int i, j;
    int **connect_to;
    /* parse the input file and get the initial data to everyone */
    if (rank == 0) {
        connect_to = distribute_data(numprocs, rank, argv);
        nodes_per_proc = num_nodes / numprocs;

        print_arrays(connect_to, rank, nodes_per_proc);
    }
    else {
        /* get number of nodes from proc0 (through bcast) */
        MPI_Bcast(&num_nodes, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

        /* receive the node information */
        MPI_Status status;
        unsigned int connections;
        nodes_per_proc = num_nodes / numprocs;
        connect_to = malloc(sizeof(int) * nodes_per_proc);

        MPI_Recv(&connections, 1, MPI_UNSIGNED, 0, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &status);

        int data[connections];
        MPI_Recv(&data, connections, MPI_INT, 0, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &status);
        
        int k;
        /* move the data array into a 2-D array */
        for (i=0,k=0; i<connections; i+=data[i]+1, k++) {
            connect_to[k] = malloc(sizeof(int) * data[i]);
            for (j=0; j<data[i]+1; j++) {
                connect_to[k][j] = data[j+i];
            }
        }
        print_arrays(connect_to, rank, nodes_per_proc);
    }
    /* get total number of connections for each node
     * and find any connections in the local data */
    for (i=0; i<nodes_per_proc; i++) {
        for (j=1; j<connect_to[i][0]+1; j++) {
            
        }
    }
    for (j=0; j<nodes_per_proc; j++)
        free(connect_to[j]);
    free(connect_to);

    MPI_Finalize();

    return 0;
}
/* rank = 0; */
/* read the file
 * chop it into chunnks
 * send to the rest of the processors
 */
int **distribute_data(int numprocs, int rank, char *argv[]) {
    int fd=0;
    if ((fd=open(argv[1], O_RDONLY)) < 0) {
        printf("Error in opening the file!\n");
        exit(1);
    }
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        printf("Error getting fstat!\n");
        exit(1);
    }
    long i;
    long nbytes = fileStat.st_size;
    char *file = malloc(sizeof(char) * nbytes);

    read(fd, file, nbytes);

    /* get the number of nodes in this graph */
    char *nodes;
    nodes = strstr(file, "Nodes: ");
    nodes+=7;

    num_nodes = nodes[0]-'0';

    /* tell everyone how many nodes their are */
    MPI_Bcast(&num_nodes, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    
    /* find start of file */
    nodes = strstr(file, "ToNodeId");
    nodes+=9;

    /* chop up and send to other processes */
    unsigned int nodes_per_proc = num_nodes / numprocs;

    /* create an array to store these nodes */
    /* int **connect_to = malloc(sizeof(int *) * nodes_per_proc); */
    /* int **send_data = malloc(sizeof(int *) * nodes_per_proc); */

    vector<int> connect_to[nodes_per_proc];
    vector<int> send_data[nodes_per_proc];
    /* printf("nodes:[%d] per_proc:[%d] [%ld]\n", */
    /*        num_nodes, nodes_per_proc, nodes - file); */

    unsigned int nodes_sent,last_sent,receiver,connections,j, total_connections;
    nodes_sent = 0; connections = 0; total_connections = 0; receiver = 0;

    last_sent = nodes[0]-'0'; 
    for (i=0; receiver<numprocs; i+=4) {
        /* check to see if this is a new node */
        if (last_sent != (nodes[i] - '0')) {
            int start = i-4*connections;
            /* printf("last_sent: %d nodes[%d]: %d nodes[%d]: %d " */
            /*        "connection: %d sent: %d\n", */
            /*        last_sent, */
            /*        start, nodes[start]-'0', */
            /*        start+2, nodes[start+2]-'0', */
            /*        connections, */
            /*        nodes_sent); */

            /* if this is meant for proc0 allocate the memory needed
             * else send the data to the appropriate proc
             */
            total_connections += connections;
            if (receiver == 0) {
                /* connect_to[nodes_sent]=malloc(sizeof(int)*(connections+1));*/
                /* connect_to[nodes_sent][0] = connections; */
                connect_to[nodes_sent].push_back(connections);
                
                for (j=0; j<connections; j++) {
                    /* printf("\tnodes_sent: %d j: %d nodes[%d]: %d\n", */
                    /*        nodes_sent, j, start+2, nodes[start+2]-'0'); */
                    /* connect_to[nodes_sent][j+1] = nodes[start+2]-'0'; */
                    connect_to[nodes_sent].push_back(nodes[start+2]-'0');
                    start+=4;
                }
            }
            /* else { */
            /*     send_data[nodes_sent] = malloc(sizeof(int) * (connections+1)); */
            /*     send_data[nodes_sent][0] = connections; */
                
            /*     for (j=0; j<connections; j++) { */
            /*         /\* printf("\tfor: %d tnodes_sent: d j:%d nodes[%d]:%d\n", *\/ */
            /*         /\*     receiver,nodes_sent,j,start+2,nodes[start+2]-'0'); *\/ */
            /*         send_data[nodes_sent][j+1] = nodes[start+2]-'0'; */
            /*         start+=4; */
            /*     } */
            /* } */
            nodes_sent++;
            connections = 0;

            if (nodes_sent == nodes_per_proc) {
                total_connections += nodes_per_proc;
                /* if (receiver != 0) { */
                /*     /\* printf("SENDING: %d to %d\n", total_connections, receiver); *\/ */
                /*     MPI_Send(&total_connections, 1, MPI_UNSIGNED, receiver, */
                /*              0, MPI_COMM_WORLD); */
                /*     int data[total_connections]; */
                /*     int k=0; int m=0; */
                    
                /*     for (j=0; j<nodes_per_proc; j++)  */
                /*         for (k=0; k<send_data[j][0]+1; k++) */
                /*             data[m++] = send_data[j][k]; */

                /*     /\* printf("SENDING: data to %d\n", receiver); *\/ */
                /*     MPI_Send(data, total_connections, MPI_INT, */
                /*              receiver, 0, MPI_COMM_WORLD); */
                /*     for (j=0; j<nodes_per_proc; j++) */
                /*         free(send_data[j]); */
                /* } */
                total_connections = 0;
                receiver++;
                nodes_sent = 0;
            }
        }
        connections++;
        last_sent = nodes[i] - '0';
    }

    free(file);

    return connect_to;
}

void print_arrays(int **connect_to, int rank, unsigned int nodes_per_proc) {
    int i,j;
    for (i=0; i<nodes_per_proc; i++) {
        printf("%d: [", i + (rank * nodes_per_proc));
        for (j=0;j<connect_to[i][0]+1; j++) {
            printf(" %d", connect_to[i][j]);
        }
        printf("]\n");
    }
}
