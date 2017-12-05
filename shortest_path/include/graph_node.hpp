#ifndef GRAPH_NODE_HEADER_INCLUDED
#define GRAPH_NODE_HEADER_INCLUDED

#include <vector>

typedef unsigned int uint;

const uint INIT_SIZE = 1000;

struct graph_node {
    uint id;
    void print_connections();
    std::vector<uint> connect_to;
};

#endif // GRAPH_NODE_HEADER_INCLUDED
