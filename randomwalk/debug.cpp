#include <iostream>
#include <vector>
#include <map>
#include <set>

pthread_mutex_t p_lock;

template<typename T> void print_vector(std::vector<T> &vect, const char *name) {
    if (!vect.empty()) {
        uint i;
        std::cout << name << " = [";
        for (i=0; i<vect.size()-1; i++)
            std::cout << vect.at(i) << ", ";
        std::cout << vect.at(i) << "]" << std::endl;
    }
}

void print_nodes(std::vector<NodeData> &nodes, const char *name, uint seed) {
    pthread_mutex_lock(&p_lock);

    std::cout << "Thread " << seed << ":" << std::endl;
    for (uint i=0; i<nodes.size(); i++) {
        printf("id: %d num_walkers: %d visits: %d\n",
               nodes[i].id, nodes[i].num_walkers, nodes[i].visits);
        printf("\t");
        print_vector((nodes[i].connect_to), "connect_to");
    }
    std::cout << std::endl;

    pthread_mutex_unlock(&p_lock);
}

void print_nodes2(NodeData nodes[], const char *name, uint seed, uint size) {
    pthread_mutex_lock(&p_lock);

    std::cout << "Thread " << seed << ":" << std::endl;
    for (uint i=0; i<size; i++) {
        printf("id: %d num_walkers: %d visits: %d\n",
               nodes[i].id, nodes[i].num_walkers, nodes[i].visits);
        printf("\t");
        print_vector((nodes[i].connect_to), "connect_to");
    }
    std::cout << std::endl;

    pthread_mutex_unlock(&p_lock);
}
// void print_nodes(struct node_data **nodes, const char *name, uint seed, uint size) {
//     pthread_mutex_lock(&p_lock);

//     std::cout << "Thread " << seed << ":" << std::endl;
//     for (uint i=0; i<size; i++) {
//         printf("id: %d num_walkers: %d visits: %d owner: %d\n",
//                nodes[i]->id, nodes[i]->num_walkers, nodes[i]->visits, nodes[i]->owner);
//         printf("\t");
//         print_vector((nodes[i]->connect_to), "connect_to");
//     }
//     std::cout << std::endl;

//     pthread_mutex_unlock(&p_lock);
// }

// template<typename T> void print_set(std::set<T,bool(*)(uint,uint)> &s, const char *name) {
//     int i;
//     std::cout << name << " = [";
//     for (std::set<uint>::iterator vi=s.begin();
//          vi!=s.end(); ++vi) {
//         std::cout << (*vi) << ", ";
//     }
//     std::cout << "]" << std::endl;
// }
void print_set(std::set<uint> &s, const char *name, uint seed) {
    pthread_mutex_lock(&p_lock);
    std::cout << "thread " << seed << "\n" << name << " = [";
    for (std::set<uint>::iterator vi=s.begin();
         vi!=s.end(); ++vi) {
        std::cout << (*vi) << ", ";
    }
    std::cout << "]" << std::endl;
    pthread_mutex_unlock(&p_lock);
}

template<typename T> void print_vector2(std::vector<T> &vect, const char *name) {
    int i=0;
    char buf[50];
    std::cout << name << " =" << std::endl;
    for (i=0; i<vect.size()-1; i++) {
        sprintf(buf, "%d", i++);
        print_vector(vect.at(i), buf);
    }
}

// void print_map(std::map<uint, std::vector<uint> > &to_me, const char *name) {
//     std::cout << name << " = [" << std::endl;
//     for (std::map<uint, std::vector<uint> >::iterator mi=to_me.begin();
//          mi!=to_me.end(); ++mi) {
//         std::cout << (*mi).first << " : ";
//         for (std::vector<uint>::iterator vi=mi->second.begin();
//              vi!=mi->second.end(); ++vi) {
//             std::cout << *vi << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::cout << "]" << std::endl;
// }

// template<typename T> void print_map_set(std::map<uint, std::set<uint,T> > &to_me,
//                                    const char *name) {
//     std::cout << name << " = [" << std::endl;
//     for (typename std::map<uint, typename std::set<uint, T> >::iterator mi=to_me.begin();
//          mi!=to_me.end(); ++mi) {
//         std::cout << (*mi).first << " : ";
//         for (typename std::set<uint, T>::iterator vi=mi->second.begin();
//              vi!=mi->second.end(); ++vi) {
//             std::cout << *vi << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::cout << "]" << std::endl;
// }

template<typename T> void print_array(T arr[], const char *name, uint length) {
    uint i;
    std::cout << name << " = [";
    for (i=0; i<length-1; i++)
        std::cout << arr[i] << ", ";
    std::cout << arr[i] << "]" << std::endl;
}
