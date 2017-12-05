/*
 * graph_parser: Parses the input file and creates node objects containing
 * the informatino specified in the file
 *
 * @author jreese
 */

#include <iostream>

#include "../include/graph_parser.hpp"

using namespace std;

/**
 * Parses the input file
 *
 * @param file The contents of the file
 * @param graph A reference to the graph. These are stored in unordered
 *        maps where the key is the node id and the value is the node object
 *
 * @return the number of nodes in this graph
 */
uint graph_parser::parse(char *file,
                         tr1::unordered_map<uint, graph_node> &graph) {

    uint line_nums[] = {0,0};
    uint last_id = 0;
    uint num_nodes = 0;

    char *line = strtok(file, "\n");

    graph_node nd;
    get_line(line, line_nums);
    nd.id = line_nums[1];
    
    while (line != NULL) {
        get_line(line, line_nums);
        if (last_id == line_nums[1])
            nd.connect_to.push_back(line_nums[0]);

        else {
            graph[line_nums[1]] = nd;
            last_id = line_nums[1];
            nd.id = line_nums[1];
            nd.connect_to.clear();
            nd.connect_to.push_back(line_nums[0]);

            num_nodes++;
        }
        line = strtok(NULL, "\n");
    }
    graph[last_id] = nd;

    return ++num_nodes;
}

// TODO: add the third element (WEIGHT for now)
/**
 * Parses a single line of the file
 *
 * @param line The contents of this line
 * @param line_nums[] An array the contains the individual elements in the line
 *
 */
inline void graph_parser::get_line(char *line, uint line_nums[2]) {
    int start=0;
    line_nums[0] = get_line_part(line, &start);
    line_nums[1] = get_line_part(line, &start);
} // get_line()

/**
 * Breaks the line into its unique parts
 *
 * @param line The contents of this line
 * @param start The index of where to start in this line
 */
inline uint graph_parser::get_line_part(char *line, int *start) {
    char buf[100];
    int i=0;
    while (line[*start] != '\t')
        buf[i++] = line[(*start)++];

    (*start)++;
    buf[i] = '\0';

    return atoi(buf);
} // get_lines_part()

/**
 * Prints the graph
 *
 * @param graph The graph to be printed
 */
void graph_parser::print_graph(tr1::unordered_map<uint, graph_node> &graph) {
    for (tr1::unordered_map<uint, graph_node>::iterator mi=graph.begin();
         mi!=graph.end(); ++mi)
        (mi->second).print_connections();
} // print_graph()
