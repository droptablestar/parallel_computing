#ifndef GRAPH_FHANDLER
#define GRAPH_FHANDLER

#include <fstream>
#include <string>

class graph_fhandler {
  public:
    const char *filename;

    graph_fhandler(const char *);
    ~graph_fhandler();

    char *get_data();
    int read_file();
    void free_file();
    std::string get_error();
    
  private:
    int open();
    void close();
    std::ifstream inputfile;
    std::string er;
    char *file;
};


#endif // GRAPH_FHANDLER
