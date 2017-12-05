#ifndef GRAPH_PARSER_HEADER_INCLUDED
#define GRAPH_PARSER_HEADER_INCLUDED

#include <tr1/unordered_map>

#include "../include/graph_node.hpp"

struct graph_parser {
    uint parse(char *file, std::tr1::unordered_map<uint, graph_node> &);
    void get_line(char *, uint [2]);
    uint get_line_part(char *, int *);
    void print_graph(std::tr1::unordered_map<uint, graph_node> &);
};

#endif // GRAPH_PARSER_HEADER_INCLUDED
