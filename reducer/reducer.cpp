#include <iostream>
#include <fstream>
#include <unordered_map>

#include <sstream>
#include <string>
#include <boost/foreach.hpp>

#include <mpi.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cmath>

#include "reducer.hpp"

using namespace std;

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: mpirun -machinefile <filename>"
               "-np <# of procs> <name of input file>\n");
        return 1;
    }

    int numprocs, rank, perproc;

    MPI::Init(argc, argv);
    MPI::Status status;
    numprocs = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();
    
    vector<pair<int, int> > data;
    unordered_map<int, int> hmap;

    double time=0.0;
    struct timeval start, end;
    // rank is 0, read the file and distribute to the other procs
    if (rank == 0) {
        // cout << "reading data...\n";
        read_data(numprocs, argv[1], data);
        perproc = data.size() / numprocs;

        // cout << perproc << endl;
        vector<int> to_send;
        for (int j=0; j<perproc; j++) {
            to_send.push_back(data[j].first);
            to_send.push_back(data[j].second);
        }
        int size = to_send.size();
        int recv[size];

        for (int i=0; i<size; i++)
            recv[i] = to_send[i];
        to_send.clear();
        
        for (int i=0; i<numprocs; i++) {
            for (int j=perproc*i; j<(perproc*i)+perproc; j++) {
                to_send.push_back(data[j].first);
                to_send.push_back(data[j].second);
            }
            int snd_size = to_send.size();
            // cout << "sending size of " << snd_size << " to " <<i<< "...\n";
            MPI::COMM_WORLD.Send(&snd_size, 1, MPI::INT, i, 0);
            // cout << "sending data to " << i << "...\n";
            MPI::COMM_WORLD.Send(&(to_send[0]), snd_size, MPI::INT, i, 0);
            to_send.clear();
        }
        gettimeofday(&start, NULL);
        hash_local(recv, hmap, size);
    }

    // rank isn't 0, receive the partitioned file
    else {
        int size;
        MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, 0, MPI::ANY_TAG, status);
        int recv[size];
        MPI::COMM_WORLD.Recv(recv, size, MPI::INT, 0, MPI::ANY_TAG, status);
        hash_local(recv, hmap, size);
    }
    MPI::COMM_WORLD.Bcast(&perproc, 1, MPI::INT, 0);
    
    // cout << rank << ": ";
    // for (auto mi=hmap.begin(); mi != hmap.end(); ++mi) {
    //     cout << mi->first << ":" << mi->second << ' ' ;
    // }
    // cout << endl;
    // cout << "hashing global\n";
    hash_global(hmap, perproc, numprocs, rank);
    if (rank == 0) {
        gettimeofday(&end, NULL);
        time = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
        cout << time << endl;
    }
    
    // cout << rank << ": ";
    // for (auto mi=hmap.begin(); mi != hmap.end(); ++mi) {
    //     cout << mi->first << ":" << mi->second << ' ' ;
    // }
    // cout << endl;

    MPI::Finalize();
} // main()

/* hash_global:
 * takes the locally created hash map, the number of items per processor,
 * the number of processors, and the individual rank.
 * Creates an array of vectors that will be sent to each processor. This array
 * will contain the key value pairs which will be owned by that processor
 * (key % p). Will then distribute the arrays to each processor via MPI::Gather.
 * Finally, we will go through the list received from each processor and add
 * these values to the local hash map.
 */
void hash_global(unordered_map<int,int> &hmap, int perproc, int numprocs,
                 int rank) {
    vector<int> dist[numprocs];

    for (auto mi=hmap.begin(); mi!=hmap.end(); ) {
        int key = mi->first % numprocs;
        if (key != rank) {
            dist[key].push_back(mi->first);
            dist[key].push_back(mi->second);

            mi = hmap.erase(mi);
        }
        else
            ++mi;
    }
    // make sure all the arrays are of size 2*perproc
    for (int p=0; p<numprocs; p++) dist[p].resize(2*perproc);

    int *rbuf = new int[numprocs*(2*perproc)];

    // gather the data local to each proc
    for (int p=0; p<numprocs; p++) 
        MPI::COMM_WORLD.Gather(&(dist[p][0]), 2*perproc, MPI::INT,
                               rbuf, 2*perproc, MPI::INT, p);

    // handle the data received from the other procs. this data contains the
    // [key,value] pairs local to this proc.
    for (int i=0; i<numprocs*2*perproc; i+=2) {
        if (rbuf[i] == 0 && rank != 0) continue;
        hmap[rbuf[i]] = (hmap.find(rbuf[i]) == hmap.end()) ?
            rbuf[i+1] : hmap[rbuf[i]] + rbuf[i+1];
    }

    delete[] rbuf;
} // hash_global()

/* hash_local:
 * Takes an array of integers [key,value,key,value,...], an unordered map
 * and the size of the array. Inserts all the elements of the array into the
 * map, summing values if there are multiple keys.
 */
void hash_local(int recv[], unordered_map<int, int> &hmap, int size) {
    for (int i=0; i<size; i+=2) {
        hmap[recv[i]] = (hmap.find(recv[i]) == hmap.end()) ?
            recv[i+1] : hmap[recv[i]] + recv[i+1];
    }
} // hash_local()

string repr(const string& s) {
    ostringstream ss;
    BOOST_FOREACH(const char c, s) {
        if ((signed char)c < 32) {
            ss << "\\x" << hex << right;
            ss.width(2); ss.fill('0');
            ss << (int)(unsigned char)c;
        } else {
            ss << c;
        }
    }
    return ss.str();
}

/* read_data:
 * Takes the number of processors, a pointer to the name of the file, a vector
 * of pairs that represent the [key,value].
 * Reads the file pointed to by filename and splits each line into a pair of
 * ints.
 */
void read_data(int numprocs, const char *filename,
               vector<pair<int, int> > &data) {
    long begin, end;
    ifstream inputfile(filename, ios::in);

    if (!inputfile.is_open()) {printf("Error in opening the file!\n");exit(1);}

    begin = inputfile.tellg();
    inputfile.seekg(0, ios::end);
    end = inputfile.tellg();
    inputfile.seekg(0, ios::beg);

    long nbytes = end-begin;
    char *file = (char *)malloc(sizeof(char) * nbytes);

    if (file==NULL){cout<<"Malloc() error!\n";exit(1);}
    inputfile.read(file, nbytes);
    inputfile.close();

    // cout << file;

    char *pch = strtok(file, "\n");
    while (pch != NULL && strcmp(pch, "EOF") != 0) {
        data.push_back(split(pch));
        pch = strtok(NULL, "\n");
    }

    free(file);
} // read_data()

/* split:
 * splits a line (pch) on the tab and returns an int pair
 */
pair<int, int> split(char *pch) {
    int i=0;
    while (pch[i++] != '\t') ;

    pch[i-1] = '\0';

    pair<int, int> rval (atoi(pch),atoi(pch+i));
    return rval;
} // split()

