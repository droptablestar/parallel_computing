#ifndef RECUDER_HPP
#define REDUCER_HPP

#include <vector>
#include <utility>
#include <unordered_map>

void hash_global(std::unordered_map<int,int> &, int, int, int);
void hash_local(int [], std::unordered_map<int, int> &, int);
void read_data(int , const char *, std::vector<std::pair<int, int> > &);
std::pair<int, int> split(char *);


#endif // REDUCER_HPP
