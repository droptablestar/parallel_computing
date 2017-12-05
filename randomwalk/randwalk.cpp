#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <unordered_map>
#include <new>
#include <queue>
#include <map>
#include <set>

#include "randwalk.hpp"
#include "debug.cpp"

using namespace std;

const uint MAX_TH = 512;

vector<NodeData> nodes;
// vector<NodeData *> nodes;
uint numthrds, num_nodes, nodes_per_th;
pthread_barrier_t barr;
pthread_mutex_t h_lock;
// vector<vector <NodeData *> *> heaps;
vector<vector <NodeData> *> heaps;
uint iterations;

int main(int argv, char **argc) {
    if (argv < 5)
        error("Usage: ./randwalk <input file> <output file> <iterations> "
              "<number of threads>");

    pthread_mutex_init(&p_lock, NULL);
    pthread_mutex_init(&h_lock, NULL);

    char *file = read_file(argc[1]);

    parse_input(file);

    // print_nodes(nodes, "nodes", 0);
    // exit(0);

    numthrds = 1;
    pthread_barrier_init(&barr, NULL, numthrds);    
    iterations = atoi(argc[3]);

    // sequential
    double stime=0.0, ptime=0.0;
    struct timeval start, end;
    // NodeData *highest[100];
    NodeData highest[100];
    printf("Computing sequential time...\n");
    // gettimeofday(&start, NULL);
    // compute_seq();
    // get_highest(highest);
    // gettimeofday(&end, NULL);
    // stime = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
    printf("Finished computing sequential time...\n");

    numthrds = atoi(argc[4]);
    if (numthrds > MAX_TH) error("Too many threads. Can be changed by modifying"
                                 " const uint MAX_TH");

    // parallel
    printf("Computing parallel time...\n");
    gettimeofday(&start, NULL);
    compute_par();
    get_highest(highest);
    gettimeofday(&end, NULL);
    ptime = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
    printf("Finished computing parallel time...\n");

    // print_nodes(&(nodes[0]), "nodes", 0, num_nodes);
    write_results(highest, argc[2], stime, ptime);
    cleanup();

    return 0;
}

void write_results(NodeData highest[], const char *filename, double stime,
                   double ptime) {
// void write_results(NodeData **highest, const char *filename, double stime,
//                    double ptime) {
    ofstream outfile;
    outfile.open(filename, ios::out);
    outfile << "sequential: " << stime << endl;
    outfile << "parallel: " << ptime << endl;
    uint up = num_nodes < 100 ? num_nodes : 100;
    for (uint i=0; i<up; i++)
        outfile << "id: " << highest[i].id <<
            "\tvisits: " << highest[i].visits << endl;
        // outfile << "id: " << highest[i]->id <<
        //     "\tvisits: " << highest[i]->visits << endl;
    outfile.close();
    printf("sequential: %f\nparallel: %f\nspeedup: %f\n\n", stime, ptime, stime/ptime);
}

void get_highest(NodeData highest[]) {
// void get_highest(NodeData **highest) {
    for (uint i=0; i<100; i++) {
        if (i>=num_nodes) break;
        uint maxv = 0, maxi = 0;
        for (uint j=0; j<heaps.size(); j++) {
            if (!heaps[j]->empty() && ((*heaps[j])[0].visits >= maxv)) {
                maxv = (*heaps[j])[0].visits; maxi = j;
            // if (!heaps[j]->empty() && ((*heaps[j])[0]->visits >= maxv)) {
            //     maxv = (*heaps[j])[0]->visits; maxi = j;
            }
        }
        highest[i] = (*heaps[maxi])[0];
        // highest[i] = (*heaps[maxi])[0];
        (*heaps[maxi]).erase((*heaps[maxi]).begin());
        // heaps[maxi]->erase(heaps[maxi]->begin());
    }
}

void compute_par() {
    pthread_t p_threads[numthrds];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_barrier_init(&barr, NULL, numthrds);    

    struct thread_data t_data[numthrds];
    
    for (uint i=0; i<numthrds; i++) {
        // divide up the vector for each thread
        uint lower, upper;
        compute_bounds(lower, upper, i);
        t_data[i].id = i;
        t_data[i].lower = lower;
        t_data[i].upper = upper;
        pthread_create(&p_threads[i], &attr, walk, &t_data[i]);
    }

    for (uint i=0; i<numthrds; i++) {
        pthread_join(p_threads[i], NULL);
    }
}

void compute_seq() {
    struct thread_data s_data;
    s_data.id = 0;
    s_data.lower = 0;
    s_data.upper = num_nodes;
    walk(&s_data);
    heaps.clear();
}

void *walk(void *ptr) {
//     // struct timeval start, end, start_i, end_i;
//     // double cross_t = 0.0, walk_t = 0.0, walk_b = 0.0, bar1_t = 0.0, bar2_t = 0.0;
//     // double local_t = 0.0, global_t = 0.0, sort_t = 0.0, update_t = 0.0, walk_a = 0.0;

    uint seed, lower, upper;
    struct thread_data *t_ptr;
    t_ptr = (struct thread_data *)ptr;
    seed = t_ptr->id;
    lower = t_ptr->lower;
    upper = t_ptr->upper;
        
    uint local_iter = iterations;
    int lock_status;
    uint range = upper-lower;

    unordered_map<uint, uint>cross_walk;
    // NodeData **local_nodes = new NodeData*[range];

    uint i=0;
    uint sizes[range];
    for (uint i=0; i<range; i++)
        sizes[i] = nodes[i+lower].connect_to.size();
    // for (vector<NodeData *>::iterator vi=nodes.begin()+lower;
    //      vi!=nodes.begin()+upper; ++vi, ++i) {
    //     (*vi)->owner = seed;
    //     local_nodes[i] = *vi;
    //     local_nodes[i]->num_walkers = 1;
    //     sizes[i] = local_nodes[i]->connect_to.size();
    // }

    uint move_i, moveto;
    srand(time(0));
    uint randar[local_iter];
    for (uint count=0; count<local_iter; count++) 
        randar[count] = (uint)rand();

    // go through each node this thread owns and move the walkers
    for (uint count=0; count<local_iter; count++) {
        for (uint i=0; i<range; i++) {
            uint top = nodes[i+lower].num_walkers;
            nodes[i+lower].num_walkers = 0;
            for (uint j=0; j<top; j++) {
            // for (uint j=0; j<nodes[i+lower]->num_walkers; j++) {
            // for (uint j=0; j<local_nodes[i]->num_walkers; j++) {
                move_i = randar[(count+i)%local_iter] % nodes[i+lower].connect_to.size();
                // move_i = randar[(count+i)%local_iter] % sizes[i];
                // move_i = randar[(count+i)%local_iter] % sizes[i];
                moveto = nodes[i+lower].connect_to[move_i];
                // moveto = local_nodes[i]->connect_to[move_i];
                // local_nodes[i]->num_walkers--;
                if (moveto >= lower && moveto < upper) {
                    // printf("t%d local  - node: %d moving to: %d move_i: %d\n",
                    //        seed, nodes[moveto]->id, moveto+lower, move_i);
                    // moveto -= lower;
                    // move the walker to the new node (also local)
                    nodes[moveto].num_walkers++; 
                    // nodes[moveto]->tmp_walkers++; 
                    nodes[moveto].visits++;
                    // local_nodes[moveto]->tmp_walkers++; 
                    // local_nodes[moveto]->visits++;
                }
                else {
                    // printf("t%d global  - node: %d moving to: %d move_i: %d\n",
                    //        seed, nodes[moveto]->id, moveto, move_i);
                    lock_status = pthread_mutex_trylock(&(nodes[moveto].node_lock));
                    if (lock_status != EBUSY) {
                        nodes[moveto].num_walkers++;
                        nodes[moveto].visits++;
                        // nodes[moveto]->tmp_walkers++;
                        pthread_mutex_unlock(&(nodes[moveto].node_lock));
                    }
                    else {
                        if (cross_walk.find(moveto) != cross_walk.end())
                            cross_walk[moveto]++;
                        else
                            cross_walk[moveto] = 0;
                    }
                }
            }
        }

        // print_nodes(&(nodes[0]), "nodes", seed, num_nodes);
        
        while (!cross_walk.empty()) {
            unordered_map<uint,uint>::iterator si=cross_walk.begin();
            while(si!=cross_walk.end()) {
                lock_status = pthread_mutex_trylock(&(nodes[si->first].node_lock));
                if (lock_status != EBUSY) {
                    nodes[si->first].num_walkers += si->second;
                    nodes[si->first].visits++;
                    // nodes[si->first]->tmp_walkers += si->second;
                    // nodes[si->first]->tmp_walkers += si->second;
                    pthread_mutex_unlock(&(nodes[si->first].node_lock));
                    cross_walk.erase(si);
                }
                ++si;
            }
        }

        // print_nodes(&(nodes[0]), "nodes", seed, num_nodes);
        // if (numthrds > 1)
        //     pthread_barrier_wait(&barr);
        
        // update local nodes with global info
        // for (uint i=0; i<range; i++) {
        //     // uint num = local_nodes[i]->tmp_walkers;
        //     uint num = nodes[i+lower]->tmp_walkers;
        //     // local_nodes[i]->num_walkers = nums[i];
        //     // local_nodes[i]->visits += nums[i];
        //     // local_nodes[i]->num_walkers = num;
        //     // local_nodes[i]->visits += num;
        //     // local_nodes[i]->tmp_walkers = 0;
        //     nodes[i+lower]->num_walkers = num;
        //     nodes[i+lower]->visits += num;
        //     nodes[i+lower]->tmp_walkers = 0;
        // }
        
        if (numthrds > 1)
            pthread_barrier_wait(&barr);
    }

    // priority_queue<NodeData *, vector<NodeData *>, compare> srtd;
    priority_queue<NodeData , vector<NodeData>, compare> srtd;
    for (uint i=0; i<range; i++)
        srtd.push(nodes[i+lower]);

    vector<NodeData> *nds = new vector<NodeData>;
    // vector<NodeData *> *nds = new vector<NodeData *>;
    nds->reserve(100);
    i=100;
    while(!srtd.empty() && i-- > 0) {
        NodeData nd = srtd.top();
        // NodeData *nd = srtd.top();
        srtd.pop();
        nds->push_back(nd);
    }

    nds->shrink_to_fit();
    pthread_mutex_lock(&p_lock);
    heaps.push_back(nds);
    pthread_mutex_unlock(&p_lock);

//     // delete[] local_nodes;

    return ptr;
}

void parse_input(char *file) {
    char *line = strtok(file, "\n");
    uint line_nums[] = {0,0};
    uint last_id = 0;
    num_nodes = 0;

    NodeData nd;
    // NodeData *nd = new NodeData(0);
    
    while (line != NULL) {
        get_line(line, line_nums);

        if (last_id == line_nums[1]) 
            nd.connect_to.push_back(line_nums[0]);
            // nd->connect_to.push_back(line_nums[0]);

        else {
            nodes.push_back(nd);
            last_id = line_nums[1];
            nd.id = line_nums[1];
            // nd = new NodeData(line_nums[1]);
            // nd->connect_to.push_back(line_nums[0]);
            nd.connect_to.clear();
            nd.connect_to.push_back(line_nums[0]);

            num_nodes++;
        }
        line = strtok(NULL, "\n");
    }
    nodes.push_back(nd);
    nodes_per_th = round((float)++num_nodes / numthrds);

    free(file);
}

inline void get_line(char *line, uint line_nums[2]) {
    int start=0;
    line_nums[0] = get_line_part(line, &start);
    line_nums[1] = get_line_part(line, &start);
}

inline uint get_line_part(char *line, int *start) {
    char buf[100];
    int i=0;
    while (line[*start] != '\t') 
        buf[i++] = line[(*start)++];

    (*start)++;
    buf[i] = '\0';

    return atoi(buf);
}

char *read_file(const char *fname) {
    char *file;
    int fd = open(fname, O_RDONLY);
    if (fd <= 0) error("Error opening file.");

    long end = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    long nbytes = end;

    file = (char *)malloc(sizeof(char) * nbytes);
    read(fd, file, nbytes);

    return file;
}

void error(const char *str) {
    printf("%s\n",str);
    exit(-1);
}

void compute_bounds(uint &lower, uint &upper, const uint rank) {
    int nodes_per;
    nodes_per = round((float)num_nodes / (uint)numthrds);
    if (rank == numthrds-1 && (uint)(numthrds*nodes_per) < num_nodes) {
        nodes_per += num_nodes - (numthrds * nodes_per);
        lower = rank * round((float)num_nodes / numthrds);
        upper = lower + nodes_per;
    }
    else {
        lower = rank * nodes_per;;
        upper = lower + nodes_per;
    }
    if (upper > num_nodes) upper = num_nodes;
}

void cleanup() {
    // free_nodes();
}

void free_heaps() {
    for (uint i=0; i<heaps.size(); i++)
        delete heaps[i];
}

void free_nodes() {
    // for (uint i=0; i<nodes.size(); i++) 
    //     delete nodes[i];
}
