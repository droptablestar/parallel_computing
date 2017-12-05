#ifndef RANDWALK_HEADER_INCLUDED
#define RANKWALK_HEADER_INCLUDED

#include <vector>
#include <cmath>
#include <iostream>

#define round(x) floor(x+0.49);
typedef unsigned int uint;

const uint INIT_SIZE = 1000;

class NodeData {
  public:
    uint id;
    std::vector<uint> connect_to;
    uint num_walkers;
    uint visits;
    uint owner;
    uint tmp_walkers;
    pthread_mutex_t node_lock;    

    NodeData(const uint id) : id(id), num_walkers(1), visits(0),
                              tmp_walkers(0) {
        connect_to.reserve(INIT_SIZE);
        pthread_mutex_init(&(this->node_lock), NULL);
    }
};

class compare {
  public:
    bool operator() (const NodeData *lhs, const NodeData *rhs) const {
        return (lhs->visits > rhs->visits);
    }
};
struct thread_data {
    uint id, lower, upper;
};
    
char *read_file(const char *fname);
void error(const char *str);
void parse_input(char *, std::vector<NodeData *> &);
void get_line(char *, uint []);
uint get_line_part(char *, int *);
void compute_bounds(uint &, uint &, uint);
void *walk(void *);
void compute_seq();
void compute_par();
void get_highest(NodeData **);
void write_results(NodeData **, const char *, double, double);
void cleanup();
void free_nodes();
void free_heaps();

#endif // RANDWALK_HEADER_INCLUDED
