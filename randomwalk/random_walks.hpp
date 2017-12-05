#ifndef RANDWALK_HEADER_INCLUDED
#define RANKWALK_HEADER_INCLUDED

#include <vector>
#include <cmath>

#define round(x) floor(x+0.49);
#define low2(x) pow(2,floor(log2(x)));

typedef unsigned int uint;

// initial size of the connection vector for each node
const uint INIT_SIZE = 1000;
// maximum number of threads
const uint MAX_TH = 1024;

// Each node will be an instance of this class. Contains the number of walkers
// currently present in the node, the total number of visits, the nodes unique id,
// a lock specific to the node, and a vector containing all the node ids to which
// this node connects
class NodeData {
  public:
    uint num_walkers;
    uint visits;
    uint id;
    pthread_mutex_t node_lock;    
    std::vector<uint> connect_to;

    NodeData() : num_walkers(0), visits(0), id(0) {
        connect_to.reserve(INIT_SIZE);
        pthread_mutex_init(&(this->node_lock), NULL);
    }

    NodeData(const uint id) : num_walkers(0), visits(0), id(id) {
        connect_to.reserve(INIT_SIZE);
        pthread_mutex_init(&(this->node_lock), NULL);
    }
}; // NodeData

// This class is used as a comparator for insertion into the priority heap
// used in determining the highest ranked nodes
class compare {
  public:
    bool operator() (const NodeData &lhs, const NodeData &rhs) const {
        return (lhs.visits < rhs.visits);
    }
}; //compare

// thread data, passed to the walk() function with each thread
struct thread_data {
    uint id, lower, upper;
}; // thread_data

typedef struct lin_barrier_node {
    pthread_mutex_t count_lock;
    pthread_cond_t ok;
    uint count;
} lin_barrier_t;

typedef struct barrier_node {
    pthread_mutex_t count_lock;
    pthread_cond_t ok_up;
    pthread_cond_t ok_down;
    uint count;
} barrier_t;

void init_lin_barrier(lin_barrier_t &);
void lin_barrier(lin_barrier_t &, uint);
void init_barrier(barrier_node[]);
void log_barrier(barrier_node[], uint, uint);
char *read_file(const char *fname);
void error(const char *str);
void parse_input(char *);
void get_line(char *, uint []);
uint get_line_part(char *, int *);
void compute_bounds(uint &, uint &, uint);
void *walk(void *);
void compute_seq();
void compute_par();
void get_highest(NodeData[]);
void write_results(NodeData [], const char *, double, double);
void cleanup();
void free_heaps();

#endif // RANDWALK_HEADER_INCLUDED
