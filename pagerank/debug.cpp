#include <vector>
#include <map>
#include <set>

// template<typename T> void print_vector2(std::vector<T> &, const char *);
// template<typename T> void print_vector(std::vector<T> &, const char *);
// template<typename T> void print_array(T[], const char *, uint);
// void print_map(std::map<int, std::vector<uint> > &, const char *);

#define get_node_id(rank, index, numprocs, per) \
    rank == (numprocs-1) ? per+(rank*index)-1 : \
        per + (rank * index);

template<typename T> void print_vector(std::vector<T> &vect, const char *name) {
    if (!vect.empty()) {
        uint i;
        std::cout << name << " = [";
        for (i=0; i<vect.size()-1; i++)
            std::cout << vect.at(i) << ", ";
        std::cout << vect.at(i) << "]" << std::endl;
    }
}

template<typename T> void print_set(std::set<T,bool(*)(uint,uint)> &s, const char *name) {
    int i;
    std::cout << name << " = [";
    for (std::set<uint>::iterator vi=s.begin();
         vi!=s.end(); ++vi) {
        std::cout << (*vi) << ", ";
    }
    std::cout << "]" << std::endl;
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

void print_map(std::map<uint, std::vector<uint> > &to_me, const char *name) {
    std::cout << name << " = [" << std::endl;
    for (std::map<uint, std::vector<uint> >::iterator mi=to_me.begin();
         mi!=to_me.end(); ++mi) {
        std::cout << (*mi).first << " : ";
        for (std::vector<uint>::iterator vi=mi->second.begin();
             vi!=mi->second.end(); ++vi) {
            std::cout << *vi << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

template<typename T> void print_map_set(std::map<uint, std::set<uint,T> > &to_me,
                                   const char *name) {
    std::cout << name << " = [" << std::endl;
    for (typename std::map<uint, typename std::set<uint, T> >::iterator mi=to_me.begin();
         mi!=to_me.end(); ++mi) {
        std::cout << (*mi).first << " : ";
        for (typename std::set<uint, T>::iterator vi=mi->second.begin();
             vi!=mi->second.end(); ++vi) {
            std::cout << *vi << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
}

template<typename T> void print_array(T arr[], const char *name, uint length) {
    uint i;
    std::cout << name << " = [";
    for (i=0; i<length-1; i++)
        std::cout << arr[i] << ", ";
    std::cout << arr[i] << "]" << std::endl;
}
