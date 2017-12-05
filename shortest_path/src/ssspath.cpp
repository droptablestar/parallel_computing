#include <iostream>
#include <tr1/unordered_map>

#include "../include/graph_fhandler.hpp"
#include "../include/graph_node.hpp"
#include "../include/graph_parser.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Usage ./test <input_file>\n";
        exit(-1);
    }
    graph_fhandler fhandler(argv[1]);
    fhandler.read_file();
    cout << fhandler.get_data() << endl;

    tr1::unordered_map<uint, graph_node> graph;
    graph_parser parser;
    uint num_nodes = parser.parse(fhandler.get_data(), graph);
    cout << "Number of nodes: " << num_nodes << endl;
    fhandler.free_file();
    parser.print_graph(graph);
}
