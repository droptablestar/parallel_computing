#ifndef PAGRANK_HEADER_INCLUDED
#define PAGRANK_HEADER_INCLUDED

#include <vector>
#include <map>
#include <set>

typedef unsigned int uint;

#define round(x) floor(x+0.49);
const double damp = 0.85;
const int SIZE = 0;
const int DATA = 1;
const int RANKS = 2;

struct clcomp {
    bool operator() (const int &lhs, const int &rhs) const
        {return lhs<rhs;}
};

struct rank_s {
    double val;
    int rank;
} max_info;

void read_data(int, int, char**);
void get_line(char *, double[]);
void compute_rank(std::map<uint, std::vector<uint> > &, double [],
                  std::vector<double> &, int, int);
double two_norm(double[], uint, uint);
void distribute_data(std::map<uint, std::vector<uint> > &, int, int);
void receive_data(std::map<uint, std::vector<uint> > &, int, int);
void compute_bounds(uint *, uint *, int, int);
int idx2id(int, uint, int);
void exchange_ranks(std::vector<double> &, double [], int, int,
                    std::map<uint, std::set<uint, clcomp> > &);
void get_line_part(char *, int *, char, double[], int);
void get_num_out(std::map<uint, std::vector<uint> > &);
void get_snd_m(std::map<uint, std::vector<uint> > &, int, int);
void get_max_in(std::map<uint, std::vector<uint> > &, uint *);

bool fncomp(uint lhs, uint rhs) {return lhs<rhs;}
uint max(uint lhs, uint rhs) {uint x = (lhs<rhs) ? rhs : lhs; return x;}

uint num_nodes, nodes_per_proc;

// kluge alert
int EOT[2] = {-1, -1};
double EOD[3] = {-1.0, -1.0, -1.0};
int CT[2] = {1, 1};

// 'adjacency' matrix
std::map<uint, std::vector <uint> > connect_to;
// the number of nodes that come out of this node
std::vector <double> num_out;
// generate the list of 'who is connected to me'
std::map<uint, std::vector<uint> > to_me;
// map of what ranks to send to each node
// this contains all the indices inside the rank_vector this proc needs
std::map<uint, std::set<uint, clcomp> > snd_m ;

double nrm;

uint max_in;

#endif // PAGRANK_HEADER_INCLUDED
