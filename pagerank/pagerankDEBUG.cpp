#include <iostream>
#include <fstream>

#include <mpi.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cmath>

#include "debug.cpp"
#include "pagerank.hpp"

using namespace std;

double time_bcast=0.0, time_reduce=0.0, time_ex=0.0, time_lreduce=0.0, time_num=0.0, time_snd=0.0;

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: mpirun -machinefile <filename>"
               "-np <# of procs> <name of input file>\n");
        return 1;
    }

    int numprocs, rank;

    MPI::Init(argc, argv);
    numprocs = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();

    double stime0=0.0;
    struct timeval start, end;
    /* parse the input file and get the initial data to everyone */
    if (rank == 0) {
        printf("reading data...\n");
        read_data(numprocs, rank, argv);
        printf("finished reading data...\n");
        for (uint i=0; i<num_nodes; i++) {
            for (uint j=0; j<connect_to.at(i).size(); j++) {
                to_me[connect_to[i][j]].push_back(i);
            }
        }

        // give the data to the other nodes
        if (numprocs > 1) {
            // nrm = (1 - damp) / num_nodes;
            // gettimeofday(&start, NULL);
            // double rank_vector[num_nodes];
            // double initial_rank = 1 / (double)num_nodes;
            // for (uint i=0; i<num_nodes; i++)
            //     rank_vector[i] = initial_rank;
            // printf("computing in serial...\n");
            // get_num_out(connect_to);
            // compute_rank(to_me, rank_vector, num_out, 0, 1);
            // printf("computed in serial...\n");
            // gettimeofday(&end, NULL);
            // stime0 = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);

            distribute_data(connect_to, numprocs, rank);
            distribute_data(to_me, numprocs, rank);
        }
    }
    else {
        receive_data(connect_to, numprocs, rank);
        receive_data(to_me, numprocs, rank);
    }
    MPI::COMM_WORLD.Barrier();

    if (rank == 0) cout << "BARRIER\n";

    if (rank == 0) gettimeofday(&start, NULL);
    nrm = (1 - damp) / num_nodes;

    // these calls will get the local information each node needs
    // number of outgoing edges per node, and who to send to in each
    // iteration
    if (rank == 0) gettimeofday(&start, NULL);
    get_num_out(connect_to);
    if (rank == 0) {
        gettimeofday(&end, NULL);
        time_num += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
    }
    if (rank == 0) gettimeofday(&start, NULL);
    get_snd_m(connect_to, rank, numprocs);
    // cout << "p" << rank << " "; print_map_set(snd_m, "snd_m");
    // cout << "p" << rank << " "; print_map(to_me, "to_me");
    if (rank == 0) {
        gettimeofday(&end, NULL);
        time_snd += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
    }

    double rank_vector[num_nodes];
    double initial_rank = 1 / (double)num_nodes;
    for (uint i=0; i<num_nodes; i++)
        rank_vector[i] = initial_rank;

    if (rank == 0) printf("computing...\n");
    compute_rank(to_me, rank_vector, num_out, rank, numprocs);

    MPI::COMM_WORLD.Barrier();
    if (rank == 0) gettimeofday(&end, NULL);

    if (rank == 0) {
        printf("serial time: %f ", stime0);
        stime0 = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
        printf("parallel time: %f\n", stime0);
        printf("bcast: %f reduce: %f ex: %f last: %f num: %f snd: %f\n",
               time_bcast, time_reduce, time_ex, time_lreduce, time_num, time_snd);
    }
    // printf("FIN::%d\n",rank);
    MPI::Finalize();

    return 0;
}

void get_snd_m(map<uint, vector<uint> > &connect_to, int rank, int numprocs) {
    uint lower, upper;
    compute_bounds(&lower, &upper, rank, numprocs);
    // printf("p%d lower: %d upper: %d\n", rank, lower, upper);
    for (map<uint, vector<uint> >::iterator mi=connect_to.begin();
         mi!=connect_to.end(); ++mi) 
        for (vector<uint>::iterator vi=mi->second.begin();
             vi!=mi->second.end(); ++vi) {
            if (!((*vi) >= lower && (*vi) < upper))
                snd_m[*vi].insert(mi->first);
        }
}

void get_num_out(map<uint, vector<uint> > &connect_to) {
    for (map<uint, vector<uint> >::iterator mi=connect_to.begin();
         mi!=connect_to.end(); ++mi)
        num_out.push_back(1.0/mi->second.size());
}

void compute_rank(map<uint, vector<uint> > &to_me, double rank_vector[],
                  vector<double> &num_out, int rank, int numprocs) {
    struct timeval start, end;
    
    // this contains the elements each proc needs by rank
    uint lower, upper;
    compute_bounds(&lower, &upper, rank, numprocs);
    vector<double> sums_tome;
    sums_tome.resize(num_nodes);
    for (uint i=0; i<num_nodes; ++i) sums_tome[i] = 0.0;
    // printf("P%d: lower: %d upper: %d\n", rank, lower, upper);

    vector<double> tmp_rank(0.0, num_nodes);
    tmp_rank.resize(num_nodes);
    double norm2_new = 1.0, norm2_prev, diff = 1.0, tmp_sum;
    norm2_prev = two_norm(rank_vector, lower, upper);

    if (numprocs > 1)
        MPI::COMM_WORLD.Reduce(&norm2_prev, &tmp_sum, 1, MPI::DOUBLE,
                               MPI::SUM, 0);
    
    if (rank == 0) 
        norm2_prev = (numprocs>1) ? sqrt(tmp_sum) : sqrt(norm2_prev);

    vector<uint>::iterator vi;
    int i=1;
    // while (i > 0) {
    while (diff > 0.00001) {
        for (uint r=lower; r<upper; r++) {
            tmp_rank[r] = 0.0;
            int idx = idx2id(rank, r, numprocs);
            tmp_rank[r] = rank_vector[r] * num_out[idx];
        }

        // printf("p%d ", rank);
        // print_vector(tmp_rank, "tmp_rank");
        if (numprocs > 1) {
            if (rank == 0) gettimeofday(&start, NULL);
            exchange_ranks(tmp_rank, rank_vector, rank, numprocs, snd_m, sums_tome);
            if (rank == 0) {
                gettimeofday(&end, NULL);
                time_ex += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
            }
        }

        // printf("p%d ", rank); print_array(sums_tome, "sums_tome", num_nodes);
        // update based on new info
        bool outofrange = 1;
        for (uint r=lower; r<upper; r++) {
            // printf("p%d r: %d\n", rank, r);
            rank_vector[r] = 0.0;
            if (to_me.find(r) != to_me.end())
                for (vi=to_me.at(r).begin(); vi!=to_me.at(r).end(); ++vi) {
                    // printf("*vi: %d lower: %d upper: %d\n", *vi, lower, upper);
                    // if (*vi >= lower && *vi < upper) {
                        // printf("inside - r: %d\n", r);
                        rank_vector[r] += tmp_rank[*vi];
                    // }
                    // else if(outofrange) {
                    //     // printf("outside - r: %d\n", r);
                    //     rank_vector[r] += sums_tome[r];
                    //     outofrange = !outofrange;
                    // }
                }

            rank_vector[r] = nrm + damp * rank_vector[r];
            outofrange = !outofrange;
        }

        MPI::COMM_WORLD.Barrier();
        norm2_new = two_norm(rank_vector, lower, upper);

        if (numprocs > 1) {
            if (rank == 0) gettimeofday(&start, NULL);
            MPI::COMM_WORLD.Reduce(&norm2_new, &tmp_sum, 1, MPI::DOUBLE,
                                   MPI::SUM, 0);
            if (rank == 0) {
                gettimeofday(&end, NULL);
                time_reduce += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
            }
        }
        if (rank == 0) {
            norm2_new = (numprocs>1) ? sqrt(tmp_sum) : sqrt(norm2_new);
            diff = abs(norm2_new - norm2_prev);
            printf("p%d new: %f prev: %f diff: %f\n",rank,norm2_new,norm2_prev,diff);
            norm2_prev = norm2_new;
        }

        if (numprocs > 1) {
            if (rank == 0) gettimeofday(&start, NULL);
            MPI::COMM_WORLD.Bcast(&diff, 1, MPI::DOUBLE, 0);
            if (rank == 0) {
                gettimeofday(&end, NULL);
                time_bcast += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
            }
        }
        i--;
    }

    // printf("p%d ", rank);
    // print_array(rank_vector, "rank_vector", num_nodes);
    if (numprocs > 1)
        MPI::COMM_WORLD.Barrier();

    double max = 0.0; int loc =0;
    for (uint i=0; i<num_nodes; i++) {
        if (rank_vector[i] > max) {
            max = rank_vector[i];
            loc = i;
        }
    }

    if (numprocs == 1) {max_info.val = max; max_info.rank = loc;}
    struct rank_s max_rank;
    int root = 0;
    max_rank.val = max; max_rank.rank = loc;
    if (numprocs > 1) {
        if (rank == root) gettimeofday(&start, NULL);
        MPI::COMM_WORLD.Reduce(&max_rank, &max_info, 1, MPI::DOUBLE_INT,
                               MPI::MAXLOC, root);
        if (rank == root) {
            gettimeofday(&end, NULL);
            time_lreduce += (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
        }
    }
    if (rank == root)
        printf("highest rank: %f node id: %d\n", max_info.val, max_info.rank);;
}

void exchange_ranks(vector<double> &tmp_rank, double rank_vector[], int rank,
                    int numprocs, map<uint, set<uint, clcomp> > &snd_m,
                    vector<double> &sums_tome) {
    MPI::Status statuss[4];
    MPI::Status status;
    MPI::Request send_request[4], recv_request[3];
    int size_out[numprocs][2], size_in[numprocs][2];
    uint upper, lower;
    double psum = 0.0;
    
    for (uint i=0; i<num_nodes; ++i) sums_tome[i] = 0.0;
    for (int p=(rank+1)%numprocs; p!=rank; p=(p+1)%numprocs) {
        compute_bounds(&lower, &upper, p, numprocs);
        for (uint i=lower; i<upper; i++) {
            if (snd_m.find(i) != snd_m.end()) {
                size_out[p][0] = snd_m[i].size();
                size_out[p][1] = rank;
                double data[size_out[p][0]];
                uint ranks[size_out[p][0]];
                int j=0;
                for (set<uint, clcomp>::iterator si=snd_m[i].begin();
                     si!=snd_m[i].end(); ++si) {
                    // we havent sent this element to p yet
                        data[j] = tmp_rank[*si];
                        ranks[j++] = *si;
                        // psum += tmp_rank[*si];
                }
                // size_out[p][0] = psum;
                // size_out[p][1] = i;
                // size_out[p][2] = rank;
                send_request[0] = MPI::COMM_WORLD.Isend(size_out[p], 2,
                                                        MPI::INT, p, SIZE);
                // MPI::COMM_WORLD.Send(size_out[p], 3, MPI::DOUBLE, p, SIZE);
                // printf("p%d sending: [%f][%f] to %d\n",rank,size_out[p][0],size_out[p][1],p);
                // psum = 0.0;
                send_request[1] = MPI::COMM_WORLD.Isend(data,size_out[p][0],
                                                        MPI::DOUBLE, p, DATA);
                send_request[2] = MPI::COMM_WORLD.Isend(ranks,size_out[p][0],
                                                        MPI::UNSIGNED,p,RANKS);
                // MPI::COMM_WORLD.Send(size_out[p], 2, MPI::INT, p, SIZE);
                // MPI::COMM_WORLD.Send(data,size_out[p][0],MPI::DOUBLE,p,DATA);
                // MPI::COMM_WORLD.Send(ranks,size_out[p][0],MPI::UNSIGNED,p,RANKS);
                send_request[0].Waitall(3, send_request, statuss);
                // send_request[0].Wait();
            }
        }
        // printf("p%d sending: %d to %d\n",rank,EOT[0],p);
        MPI::COMM_WORLD.Send(EOT, 2, MPI::INT, p, SIZE);
        // EOD[3] = rank;
        // send_request[3] = MPI::COMM_WORLD.Isend(EOD, 3, MPI::DOUBLE, p, 0);
        // MPI::COMM_WORLD.Send(EOD, 3, MPI::DOUBLE, p, 0);
        MPI::COMM_WORLD.Recv(size_in[p], 2, MPI::INT, MPI::ANY_SOURCE,
                             SIZE, status);
        // printf("p%d received [%f][%f] from %f\n",
        //        rank, size_in[p][0], size_in[p][1], size_in[p][2]);
        while (size_in[p][0] != EOD[0] ) {
            double data[size_in[p][0]];
            uint ranks[size_in[p][0]];
            // MPI::COMM_WORLD.Recv(data, size_in[p][0], MPI::DOUBLE, size_in[p][1],
            //                      DATA, status);
            // MPI::COMM_WORLD.Recv(ranks, size_in[p][0], MPI::UNSIGNED, size_in[p][1],
            //                      RANKS, status);
            recv_request[0] = MPI::COMM_WORLD.Irecv(data, size_in[p][0],
                                                    MPI::DOUBLE, size_in[p][1],
                                                    MPI::ANY_TAG);
            recv_request[1] = MPI::COMM_WORLD.Irecv(ranks, size_in[p][0],
                                                    MPI::UNSIGNED, size_in[p][1],
                                                    MPI::ANY_TAG);
            // recv_request[2] = MPI::COMM_WORLD.Irecv(size_in[p], 2, MPI::INT,
            //                                         size_in[p][1], SIZE);
            recv_request[0].Waitall(2, recv_request, statuss);
            // sums_tome[(uint)size_in[p][1]] += size_in[p][0];
            for (uint i=0; i<(uint)size_in[p][0]; i++) {
                // printf("i%d: p%d received %f putting it in: %d\n",
                //        i,rank, data[i], ranks[i]);
                tmp_rank[ranks[i]] = data[i];
            }
            MPI::COMM_WORLD.Recv(size_in[p], 2, MPI::INT, size_in[p][1],
                                 SIZE, status);
            // printf("p%d received [%f][%f] from %f\n",
            //        rank, size_in[p][0], size_in[p][1], size_in[p][2]);
            // recv_request[2].Wait(status);
            // printf("p%d receivied %d from %d\n",rank,size_in[p][0],size_in[p][1]);
        }
    }
}

double two_norm(double rank_vector[], uint start, uint end) {
    double sum = 0.0;
    for (uint i=start; i<end; i++) 
        sum += pow((float)rank_vector[i], 2);
    return sum;
}

void distribute_data(map<uint, vector<uint> > &dist_m, int numprocs, int rank) {
    int p=1;
    // send out the number of nodes to everyone
    MPI::COMM_WORLD.Bcast(&num_nodes, 1, MPI::UNSIGNED, 0);

    int size[2];
    uint lower, upper;
    // go through all the nodes
    map<uint, vector<uint> >::iterator mi=dist_m.begin();
    while (mi!=dist_m.end()) {
        // go through each proc
        for (p=1; p<numprocs; p++) {
            compute_bounds(&lower, &upper, p, numprocs);
            // printf("0: p%d lower: %d upper: %d first: %d\n",
            //        p, lower, upper, mi->first);
            // this node belongs to this proc
            if (mi->first >= lower && mi->first < upper) {
                size[0] = mi->second.size(); size[1] = mi->first;
                MPI::COMM_WORLD.Send(size, 2, MPI::INT, p, 0);
                MPI::COMM_WORLD.Send(&(mi->second[0]), size[0], MPI::UNSIGNED,
                                     p, 0);
                // remove this element from connect_to
                dist_m.erase(mi);
                break;
            }
        }
        ++mi;
    }
    for (p=1; p<numprocs; p++)
        MPI::COMM_WORLD.Send(EOT, 2, MPI::INT, p, 0);
}

void receive_data(map<uint, vector<uint> > &recv_m, int numprocs, int rank) {
    MPI::Status status;
    MPI::COMM_WORLD.Bcast(&num_nodes, 1, MPI::UNSIGNED, 0);

    // get nodes allocated to each proc
    nodes_per_proc = round((float)num_nodes / (uint)numprocs);
    if (rank == numprocs-1 && numprocs*nodes_per_proc < num_nodes)
        nodes_per_proc += num_nodes - (numprocs * nodes_per_proc);

    // this is the size of each connect_to vector
    int size[2];
    MPI::COMM_WORLD.Recv(&size, 2, MPI::INT, 0, MPI::ANY_TAG, status);
    // loop until end of tranmission is received
    while(size[0] != EOT[0]) {
        // read in the to_me vector to an array and then copy to this proc's
        // to_me. no, i dont like this way of doing it.
        // printf("p%d receive [%d][%d]\n", rank, size[0], size[1]);
        uint data[size[0]];
        MPI::COMM_WORLD.Recv(data, size[0], MPI::UNSIGNED, 0, MPI::ANY_TAG,
                             status);
        for (int i=0; i<size[0]; i++) recv_m[size[1]].push_back(data[i]);
        // get size of to_me vector (break on EOT)
        MPI::COMM_WORLD.Recv(size, 2, MPI::INT, 0, MPI::ANY_TAG, status);
    }
}

/* read the file
 * chop it into chunnks
 * send to the rest of the processors
 */
void read_data(int numprocs, int rank, char *argv[]) {
    long begin, end;
    ifstream inputfile;
    inputfile.open(argv[1], ios::in);
    
    if (!inputfile.is_open()) {
        printf("Error in opening the file!\n");
        exit(1);
    }
    
    begin = inputfile.tellg();
    inputfile.seekg(0, ios::end);
    end = inputfile.tellg();
    inputfile.seekg(0, ios::beg);
    
    long nbytes = end-begin;
    char *file = (char *)malloc(sizeof(char) * nbytes + 1);

    if (file == NULL) {printf("Malloc() error!\n"); exit(1);}
    inputfile.read(file, nbytes);
    printf("Successful read() (%ld bytes)\n", nbytes);
    inputfile.close();

    file[nbytes-1] = '\0';
    uint last_sent = 0;

    char *line;
    double line_nums[3];

    line = strtok(file, "\n");
    while (line != NULL && *line != '\0') {
        // get the contents of this line
        get_line(line, line_nums);
        if (last_sent == (uint)line_nums[1]) {
            connect_to[last_sent].push_back((uint)line_nums[0]);
        }
        else {
            last_sent = (uint)line_nums[1];
            connect_to[last_sent].push_back((uint)line_nums[0]);
            num_nodes++;
        }
        // move to next line
        line = strtok(NULL, "\n");
    }
    
    nodes_per_proc = round((float)++num_nodes / numprocs);
    printf("nodes: %d\n", num_nodes);

    free(file);
}

inline void get_line(char *line, double line_nums[3]) {
    int start=0;
    get_line_part(line, &start, '\t', line_nums, 0);
    get_line_part(line, &start, '\t', line_nums, 1);
    get_line_part(line, &start, '\n', line_nums, 2);
}

inline void get_line_part(char *line, int *start, char sep, double line_nums[3],
                          int index) {
    char buf[100];
    int i=0;
    while (line[*start] != sep && line[*start] != '\0') 
        buf[i++] = line[(*start)++];

    (*start)++;
    buf[i] = '\0';
    line_nums[index] = atof(buf);
}


void compute_bounds(uint *lower, uint *upper, int rank, int numprocs) {
    int nodes_per;
    nodes_per = round((float)num_nodes / (uint)numprocs);
    if (rank == numprocs-1 && (uint)(numprocs*nodes_per) < num_nodes) {
        nodes_per += num_nodes - (numprocs * nodes_per);
        (*lower) = rank * round((float)num_nodes / numprocs);
        (*upper) = (*lower) + nodes_per;
    }
    else {
        (*lower) = rank * nodes_per;;
        (*upper) = (*lower) + nodes_per;
    }
    if (*upper > num_nodes) *upper = num_nodes;
}

int idx2id(int rank, uint node_id, int numprocs) {
    uint nodes_per = round((float)num_nodes / numprocs);
    return node_id - (rank * nodes_per);
}
