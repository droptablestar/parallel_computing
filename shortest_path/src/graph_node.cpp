/*
 * graph_node: A struct for storing data related to each node in the graph
 *
 * @author jreese
 */
#include <iostream>

#include "../include/graph_node.hpp"

using namespace std;

/**
 * Prints the vector of nodes this node object is connected to
 */
void graph_node::print_connections() {
    cout << id << " connect_to:" << endl << "[";
    for (vector<uint>::iterator vi=connect_to.begin();
         vi!=connect_to.end(); ++vi) {
        if (vi != connect_to.end()-1) cout << (*vi) << ", ";
        else cout << (*vi) << "]" << endl;
    }
} // print_connections()
