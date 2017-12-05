#ifndef SPATH_HEADER_INCLUDED
#define SPATH_HEADER_INCLUDED

#include <pthread.h>
#include <vector>
#include <set>
#include <cmath>
#include <climits>
#include <queue>
#include <utility>

#define round(x) floor(x+0.49);
#define low2(x) pow(2,floor(log2(x)));

#ifndef max
#define max( x, y ) ((x)>(y))?(x):(y)
#endif

typedef unsigned int uint;

// initial size of the connection vector for each node
const uint INIT_SIZE = 1000;
// maximum number of threads
const uint MAX_TH = 1024;

// current weight of all edges (unit weight)
const uint WEIGHT = 1;

const short WHITE = 0;
const short BLACK = 1;

struct token_t {
    short color;
    short owner;
    bool isover;
};

struct QNode {
    uint id;
    uint distance;
    std::vector<uint> connect_to;
};

// This class is used as a comparator for insertion into the priority heap
// used in determining the highest ranked nodes
struct CompareNodes_q {
    bool operator() (QNode &lhs, QNode &rhs)  {
        return (lhs.distance > rhs.distance);
    }
}; // CompareNodes_q

struct setcomp {
    bool operator() (const QNode &lhs, const QNode &rhs) const {
        return (lhs.id > rhs.id);
    }
}; // setcomp

// thread data, passed to the walk() function with each thread
struct thread_data {
    uint id, lower, upper;
    short color;
}; // thread_data


char *read_file(const char *fname);
void error(const char *str);
void parse_input(char *);
void get_line(char *, uint []);
uint get_line_part(char *, int *);
void compute_bounds(uint &, uint &, uint);
void *path(void *);
void compute_seq();
void compute_par();
void write_results(const char *, const double, const double);
void set_weights(std::vector<QNode> &);
void print_heap(const uint, const uint, const uint, std::vector<QNode> &,
                const uint);
void decrease_key(const uint, const uint, std::vector<QNode> &, short *);
void perform_update(const uint, std::vector<QNode> &, short *);
void set_find(std::set<QNode,setcomp> &, const uint id,
              std::set<QNode,setcomp>::iterator &);
uint get_proc(const uint);
void print_nodes(std::vector<QNode> &, uint);
void send_remotes(std::set<QNode,setcomp> *, const uint);
#endif // SPATH_HEADER_INCLUDED
