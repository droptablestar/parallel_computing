#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <new>
#include <queue>

#include "random_walks.hpp"

using namespace std;

// vector containing a NodeData object for each node in the graph
vector<NodeData> nodes;

// number of threads being used, and number of nodes in the graph
uint numthrds, num_nodes;

// barrier type
pthread_barrier_t barr;

// lock for the heaps
pthread_mutex_t h_lock;

// each thread will place a vector of the 100 best nodes in here
vector<vector <NodeData> *> heaps;

// number of iterations
uint iterations;

barrier_node logbarrier[MAX_TH];

int main(int argv, char **argc) {
    if (argv < 5)
        error("Usage: ./randwalk <input file> <output file> <iterations> "
              "<number of threads>");

    pthread_mutex_init(&h_lock, NULL);

    char *file = read_file(argc[1]);
    parse_input(file);

    numthrds = 1;
    // pthread_barrier_init(&barr, NULL, numthrds);    
    init_barrier(logbarrier);    
    iterations = atoi(argc[3]);

    srand(time(0));
    // sequential
    double stime=0.0, ptime=0.0;
    struct timeval start, end;
    NodeData highest[100];
    printf("Computing sequential time...\n");
    gettimeofday(&start, NULL);
    compute_seq();
    get_highest(highest);
    gettimeofday(&end, NULL);
    stime = (end.tv_sec-start.tv_sec) + 1e-6*(end.tv_usec-start.tv_usec);
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

    write_results(highest, argc[2], stime, ptime);
    cleanup();

    return 0;
} // main()

// write the results of this random walk to the file specified
void write_results(NodeData highest[], const char *filename, double stime,
                   double ptime) {
    ofstream outfile;
    outfile.open(filename, ios::out);
    outfile << "sequential: " << stime << endl;
    outfile << "parallel: " << ptime << endl;
    uint up = num_nodes < 100 ? num_nodes : 100;
    for (uint i=0; i<up; i++)
        outfile << "id: " << highest[i].id <<
            "\tvisits: " << highest[i].visits << endl;
    outfile.close();
    printf("sequential: %f\nparallel: %f\nspeedup: %f\n\n", stime, ptime, stime/ptime);
} // write_results()

// get the highest 100 nodes. each thread will produce a vector of 100 nodes
// so we take the 100 best from all these vectors
void get_highest(NodeData highest[]) {
    for (uint i=0; i<100; i++) {
        if (i>=num_nodes) break;
        uint maxv = 0, maxi = 0;
        for (uint j=0; j<heaps.size(); j++) {
            if (!heaps[j]->empty() && ((*heaps[j])[0].visits >= maxv)) {
                maxv = (*heaps[j])[0].visits; maxi = j;
            }
        }
        highest[i] = (*heaps[maxi])[0];
        (*heaps[maxi]).erase((*heaps[maxi]).begin());
    }
} // get_highest()

// run a parallel computation
void compute_par() {
    pthread_t p_threads[numthrds];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_barrier_init(&barr, NULL, numthrds);    

    struct thread_data t_data[numthrds];
    
    for (uint i=0; i<numthrds; i++) {
        uint lower, upper;
        compute_bounds(lower, upper, i);
        // contruct a thread struct for each thread and pass this as a parameter
        // this will hold things like the thread is, and its boundaries
        t_data[i].id = i;
        t_data[i].lower = lower;
        t_data[i].upper = upper;
        pthread_create(&p_threads[i], &attr, walk, &t_data[i]);
    }

    for (uint i=0; i<numthrds; i++) {
        pthread_join(p_threads[i], NULL);
    }
} // compute_par()

// run a sequential computation
void compute_seq() {
    struct thread_data s_data;
    s_data.id = 0;
    s_data.lower = 0;
    s_data.upper = num_nodes;
    walk(&s_data);
    heaps.clear();
} // compute_seq()

// do the random walk...
void *walk(void *ptr) {
    const struct thread_data *t_ptr  = (struct thread_data *)ptr;
    const uint seed = t_ptr->id;
    const uint lower = t_ptr->lower;
    const uint upper = t_ptr->upper;
    const uint local_iter = iterations;
    const uint range = upper-lower;

    uint i=0;
    uint walkers[range];
    for (uint i=0; i<range; i++)
        walkers[i] = 1;
    
    uint move_i, moveto;
    uint randar[local_iter];

    // rand() seems slow so we will 'pretend' to be random
    for (uint count=0; count<local_iter; count++) 
        randar[count] = (uint)rand();

    for (uint count=0; count<local_iter; count++) {
        // go through each node this thread owns and move the walkers
        for (uint i=0; i<range; i++) {
            uint top = walkers[i];
            for (uint j=0; j<top; j++) {
                // this is the location to which this walker is moving
                move_i = randar[(count+i)%local_iter] % nodes[i+lower].connect_to.size();
                moveto = nodes[i+lower].connect_to[move_i];

                // lock the node, update its info, unlock the node
                pthread_mutex_lock(&(nodes[moveto].node_lock));
                nodes[moveto].num_walkers++;
                nodes[moveto].visits++;
                pthread_mutex_unlock(&(nodes[moveto].node_lock));
            }
        }
        if (numthrds > 1) {
            uint nthreads = numthrds; uint under2 = 0, prev = 0;
            while (nthreads > 0) {
                under2 = low2(nthreads);
                printf("under2: %d seed: %d prev: %d\n", under2+prev, seed, prev);
                if (seed >= prev && seed < (under2+prev)) {
                    printf("t%d barrier!\n", seed);
                    log_barrier(logbarrier, under2, seed);
                }
                nthreads -= under2;
                prev = under2;
            }
        }
        
        // if (numthrds > 1) 
        //     pthread_barrier_wait(&barr);
        for (uint i=0; i<range; i++) {
            walkers[i] = nodes[i+lower].num_walkers;
            nodes[i+lower].num_walkers = 0;
        }
    }

    // here we will sort the top 100 nodes for each thread
    // 1. put them all in a priority queue
    // 2. pop off the top 100 nodes
    priority_queue<NodeData , vector<NodeData>, compare> srtd;
    for (uint i=0; i<range; i++)
        srtd.push(nodes[i+lower]);

    vector<NodeData> *nds = new vector<NodeData>;
    nds->reserve(100);
    i=100;
    while(!srtd.empty() && i-- > 0) {
        NodeData nd = srtd.top();
        srtd.pop();
        nds->push_back(nd);
    }

    nds->shrink_to_fit();
    pthread_mutex_lock(&h_lock);
    heaps.push_back(nds);
    pthread_mutex_unlock(&h_lock);

    return ptr;
} // walk()

// parses the input file and creates a NodeData object for each node
void parse_input(char *file) {
    char *line = strtok(file, "\n");
    uint line_nums[] = {0,0};
    uint last_id = 0;
    num_nodes = 0;

    NodeData nd;
    
    while (line != NULL) {
        get_line(line, line_nums);

        if (last_id == line_nums[1]) 
            nd.connect_to.push_back(line_nums[0]);

        else {
            nodes.push_back(nd);
            last_id = line_nums[1];
            nd.id = line_nums[1];
            nd.connect_to.clear();
            nd.connect_to.push_back(line_nums[0]);

            num_nodes++;
        }
        line = strtok(NULL, "\n");
    }
    nodes.push_back(nd);

    free(file);
} // parse_input()

// gets a line from the file and parses it. places the first two itmes in
// line_nums[0] and [1]
inline void get_line(char *line, uint line_nums[2]) {
    int start=0;
    line_nums[0] = get_line_part(line, &start);
    line_nums[1] = get_line_part(line, &start);
} // get_line()

// gets portion of the line
inline uint get_line_part(char *line, int *start) {
    char buf[100];
    int i=0;
    while (line[*start] != '\t') 
        buf[i++] = line[(*start)++];

    (*start)++;
    buf[i] = '\0';

    return atoi(buf);
} // get_lines_part()

// read the file fname from the disk and return a pointer to the file
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
} // read_file()

// print str and exit the program (something has gone wrong)
void error(const char *str) {
    printf("%s\n",str);
    exit(-1);
} // error()

// given a nodes rank will compute the lower and upper ids that it controls
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
} // compute_bounds()

// clean up any memory allocated with malloc() or new
void cleanup() {
    free_heaps();
} // cleanup()

// delete the memory allocated for the 100 top nodes
void free_heaps() {
    for (uint i=0; i<heaps.size(); i++)
        delete heaps[i];
} // free_heaps()

void init_barrier(barrier_node b[]) {
    uint i;
    for (i=0; i<MAX_TH; i++) {
        b[i].count = 0;
        pthread_mutex_init(&(b[i].count_lock), NULL);
        pthread_cond_init(&(b[i].ok_up), NULL);
        pthread_cond_init(&(b[i].ok_down), NULL);
    }
}

void log_barrier(barrier_node b[], uint nthreads, uint id) {
    uint i, base, index;
    i = 2;
    base = 0;

    do {
        index = base + id / i;
        if (id % i == 0) {
            pthread_mutex_lock(&(b[index].count_lock));
            b[index].count++;
            while (b[index].count < 2)
                pthread_cond_wait(&(b[index].ok_up), &(b[index].count_lock));
            pthread_mutex_unlock(&(b[index].count_lock));
        }
        else {
            pthread_mutex_lock(&(b[index].count_lock));
            b[index].count++;
            if (b[index].count == 2)
                pthread_cond_signal(&(b[index].ok_up));
            while (pthread_cond_wait(&(b[index].ok_down), &(b[index].count_lock)) != 0) ;
            pthread_mutex_unlock(&(b[index].count_lock));
            break;
        }
        base = base + nthreads/i;
        i = i*2;
    } while(i <= nthreads);
    i = i/2;
    for (; i>1; i=i/2) {
        base = base - nthreads/i;
        index = base + id / i;
        pthread_mutex_lock(&(b[index].count_lock));
        b[index].count = 0;
        pthread_cond_signal(&(b[index].ok_down));
        pthread_mutex_unlock(&(b[index].count_lock));
    }
}

void init_lin_barrier(lin_barrier_t &b) {
    pthread_mutex_init(&(b.count_lock), NULL);
    pthread_cond_init(&(b.ok), NULL);
    
}

void lin_barrier(lin_barrier_t &, uint) {

}
    
